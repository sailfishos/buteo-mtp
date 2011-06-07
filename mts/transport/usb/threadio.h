#ifndef THREADIO_H
#define THREADIO_H

#include "ptp.h"

#include <QThread>
#include <QMutex>

enum mtpfs_status {
    MTPFS_STATUS_OK,
    MTPFS_STATUS_BUSY,
    MTPFS_STATUS_TXCANCEL
};

class QMutex;

class IOThread : public QThread {
public:
    explicit IOThread(QObject *parent = 0);
    void setFd(int fd);
    void interrupt();

    bool stallWrite();
    bool stallRead();

    QMutex m_lock;
protected:
    pthread_t m_handle;
    int m_fd;
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
    void sendStatus(enum mtpfs_status status);

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

    QMutex m_lock;
signals:
    void dataRead(char *buffer, int size);
};

class BulkWriterThread : public IOThread {
    Q_OBJECT
public:
    explicit BulkWriterThread(QObject *parent = 0);

    void setData(int fd, const quint8 *buffer, quint32 dataLen, bool isLastPacket);
    void run();
    bool getResult();

    private:
    const quint8 *m_buffer;
    quint32 m_dataLen;
    bool m_isLastPacket;
    bool m_result;
};

#endif
