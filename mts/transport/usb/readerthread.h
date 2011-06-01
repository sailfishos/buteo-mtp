#ifndef READERTHREAD_H
#define READERTHREAD_H

#include "ptp.h"

#include <QThread>
#include <QMutex>

enum mtpfs_status {
    MTPFS_STATUS_OK,
    MTPFS_STATUS_BUSY,
    MTPFS_STATUS_TXCANCEL
};

class QMutex;

class ControlReaderThread : public QThread {
    Q_OBJECT
public:
    ControlReaderThread(QObject *parent = 0);
    ~ControlReaderThread();

    void setFd(int fd);
    void run();

    pthread_t m_handle;
public slots:
    void setStatus(enum mtpfs_status status);

private:
    void handleEvent(struct usb_functionfs_event *event);
    void setupRequest(void *data);
    void sendStatus(enum mtpfs_status status);

    int m_fd;
    int m_state;
    enum mtpfs_status m_status;
signals:
    void startIO();
    void stopIO();
    void clearHalt();
    void deviceReset();
    void transferCancel();
};

class BulkReaderThread : public QThread {
    Q_OBJECT
public:
    BulkReaderThread(QObject *parent = 0);
    ~BulkReaderThread();

    void setFd(int fd);
    void run();

    pthread_t m_handle;
    QMutex m_lock;
signals:
    void dataRead(char *buffer, int size);

private:
    int m_fd;
};

class BulkWriterThread : public QThread {
    Q_OBJECT
public:
    BulkWriterThread(QObject *parent = 0);

    void setData(int fd, const quint8 *buffer, quint32 dataLen, bool isLastPacket);
    void run();
    bool getResult();
    bool isWriting;

    QMutex m_lock;
    pthread_t m_handle;
private:
    bool m_isTaken;
    const quint8 *m_buffer;
    quint32 m_dataLen;
    int m_fd;
    bool m_isLastPacket;
    bool m_result;
};

#endif
