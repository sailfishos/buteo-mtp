#ifndef THREADIO_H
#define THREADIO_H

#include "ptp.h"

#include <QThread>
#include <QMutex>
#include <QPair>
#include <QList>
#include <QWaitCondition>

enum mtpfs_status {
    MTPFS_STATUS_OK,
    MTPFS_STATUS_BUSY,
    MTPFS_STATUS_TXCANCEL
};

class IOThread : public QThread {
public:
    explicit IOThread(QObject *parent = 0);
    void setFd(int fd);
    virtual void interrupt();
    void exitThread();
    bool stall(bool dirIn);

protected:
    void run();
    // Implement this method in subclass.
    virtual void execute() = 0;

    int m_fd;
    bool m_shouldExit;

private:
    QMutex m_handleLock;
    pthread_t m_handle;
};

class ControlReaderThread : public IOThread {
    Q_OBJECT
public:
    explicit ControlReaderThread(QObject *parent = 0);
    ~ControlReaderThread();

    void setStatus(enum mtpfs_status status);

protected:
    void execute();

private:
    void handleEvent(struct usb_functionfs_event *event);
    void setupRequest(void *data);
    void sendStatus();

    QMutex m_statusLock;
    int m_state;
    enum mtpfs_status m_status;

signals:
    void startIO();
    void stopIO();
    void bindUSB();
    void unbindUSB();
    void deviceReset();
    void cancelTransaction();
};

class BulkReaderThread : public IOThread {
    Q_OBJECT
public:
    explicit BulkReaderThread(QObject *parent = 0);
    ~BulkReaderThread();

    // Sets *bufferp and *size to received data
    // Sets to NULL and 0 if no data available
    void getData(char **bufferp, int *size);
    void releaseData(int size); // receiver of data has processed it
    void resetData(); // discard all data in the buffer
    virtual void interrupt();

protected:
    virtual void execute();

private:
    QWaitCondition m_wait;
    QMutex m_bufferLock;
    // The buffer logic:
    //
    // It's similar to a ring buffer
    // Received data starts at m_buffer[m_dataStart] for m_dataSize1 bytes.
    // If m_dataStart > 0, then additional data starts at m_buffer[0]
    // for m_dataSize2 bytes.
    // These data ranges must never overlap.
    //
    // The main thread will consume data by advancing m_dataStart and
    // reducing m_dataSize1, until m_dataSize1 is 0. Then, if m_dataSize2 > 0,
    // it will set m_dataStart to 0, move m_dataSize2 to m_dataSize1,
    // and reset m_dataSize2 to 0.
    // (Otherwise, it will wait for the next dataReady signal)
    //
    // The reader thread will add data after m_dataStart+m_dataSize1
    // as long as there is enough space there. If it gets to the end
    // of the buffer, it will try to add data after m_dataSize2.
    // If there is not enough space there either, it will wait on m_wait
    // until releaseData() wakes it.
    //
    // Both threads will take m_bufferLock while manipulating the m_data*
    // members, and will then release the lock while working on their
    // respective parts of the buffer. Since their activity will always
    // only grow the buffer space available to the other thread, never
    // shrink it, they can work in m_buffer at the same time.
    //
    // m_buffer is allocated for the lifetime of this object.
    //
    // Some diagrams:
    // The buffer some time after reader and main thread have been active:
    //  |--------+++++++++++++++++-----------------|
    //  |        |               |
    //  0        dataStart       +dataSize1
    // Valid data marked by +
    //
    // The buffer after the reader has gotten to the end of the buffer
    // and wrapped around:
    //  |++++++--------------+++++++++++++++++-----|
    //  |     |              |               |
    //  0     dataSize2      dataStart       +dataSize1
    // Note how the reader left some space at the end unused, because
    // it was smaller than the minimum read size.
    //
    char *m_buffer;
    int m_dataStart; // protected by m_bufferLock
    int m_dataSize1; // protected by m_bufferLock
    int m_dataSize2; // protected by m_bufferLock

    int _getOffset_locked();
    bool _markNewData(int offset, int size);
signals:
    void dataReady();
};

class BulkWriterThread : public IOThread {
    Q_OBJECT
public:
    explicit BulkWriterThread(QObject *parent = 0);

    void setData(const quint8 *buffer, quint32 dataLen, bool terminateTransfer = false);
    bool resultReady();
    bool getResult();

protected:
    virtual void execute();

private:
    const quint8 *m_buffer;
    quint32 m_dataLen;
    QAtomicInt m_result_ready;
    bool m_result;
    bool m_terminateTransfer;
};

class InterruptWriterThread : public IOThread {
    Q_OBJECT
public:
    explicit InterruptWriterThread(QObject *parent = 0);
    ~InterruptWriterThread();

    bool hasData();
    void sendOne();
    void addData(const quint8 *buffer, quint32 dataLen);
    void flushData();
    void reset();
    virtual void interrupt();

signals:
    void senderIdle(bool success);

protected:
    virtual void execute();

private:
    QMutex m_lock; // protects m_buffers and used with m_wait
    QWaitCondition m_wait;

    QList<QPair<quint8 *,int> > m_buffers;
};

#endif
