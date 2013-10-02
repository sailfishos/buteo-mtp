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
    pthread_t m_handle;
    int m_fd;
    bool m_shouldExit;
};

class ControlReaderThread : public IOThread {
    Q_OBJECT
public:
    explicit ControlReaderThread(QObject *parent = 0);
    ~ControlReaderThread();

    void run();
    void setStatus(enum mtpfs_status status);

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
    void deviceReset();
    void cancelTransaction();
};

class BulkReaderThread : public IOThread {
    Q_OBJECT
public:
    explicit BulkReaderThread(QObject *parent = 0);
    ~BulkReaderThread();

    void run();
    void releaseBuffer(); // receiver of dataRead is done processing data
    virtual void interrupt();

private:
    QMutex m_lock; // used with m_wait
    QWaitCondition m_wait;
signals:
    void dataRead(char *buffer, int size);
};

class BulkWriterThread : public IOThread {
    Q_OBJECT
public:
    explicit BulkWriterThread(QObject *parent = 0);

    void setData(const quint8 *buffer, quint32 dataLen);
    void run();
    bool resultReady();
    bool getResult();

private:
    const quint8 *m_buffer;
    quint32 m_dataLen;
    QAtomicInt m_result_ready;
    bool m_result;
};

class InterruptWriterThread : public IOThread {
    Q_OBJECT
public:
    explicit InterruptWriterThread(QObject *parent = 0);
    ~InterruptWriterThread();

    void addData(const quint8 *buffer, quint32 dataLen);
    void run();
    void reset();
    virtual void interrupt();

private:
    QMutex m_lock; // protects m_buffers and used with m_wait
    QWaitCondition m_wait;

    QList<QPair<quint8 *,int> > m_buffers;
};

#endif
