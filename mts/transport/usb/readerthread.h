#ifndef READERTHREAD_H
#define READERTHREAD_H

#include <QThread>

class QMutex;

class ControlReaderThread : public QThread {
    Q_OBJECT
public:
    ControlReaderThread(int fd, QObject *parent);
    ~ControlReaderThread();
    void run();

private:
    void handleEvent(struct usb_functionfs_event *event);
    int m_fd;
    int m_state;

signals:
    void startIO();
    void stopIO();
    void setupRequest(void *data); // This needs to be connected with BlockingQueuedConnection
};

class OutReaderThread : public QThread {
    Q_OBJECT
public:
    OutReaderThread(int fd, QObject *parent, QMutex *mutex);
    ~OutReaderThread();

    void run();

signals:
    void dataRead(char *buffer, int size);

private:
    QMutex *m_lock;
    int m_fd;
};

class InWriterThread : public QThread {
    Q_OBJECT
public:
    InWriterThread(QObject *parent);

    void setData(int fd, const quint8 *buffer, quint32 dataLen, bool isLastPacket, QMutex *sendLock);
    void run();
    bool getResult();

private:
    const quint8 *m_buffer;
    quint32 m_dataLen;
    QMutex *m_lock;
    int m_fd;
    bool m_isLastPacket;
    bool m_result;
};

#endif
