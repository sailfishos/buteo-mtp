#ifndef READERTHREAD_H
#define READERTHREAD_H

#include <QThread>

class ControlReaderThread : public QThread {
    Q_OBJECT
public:
    ControlReaderThread(int fd, QObject *parent);
    ~ControlReaderThread();
    void run();

private:
    void handleEvent(struct usb_functionfs_event *event);
    int fd;
    int state;

signals:
    void startIO();
    void stopIO();
    void setupRequest(void *data); // This needs to be connected with BlockingQueuedConnection
};

class OutReaderThread : public QThread {
    Q_OBJECT
public:
    OutReaderThread(int fd, QObject *parent);
    ~OutReaderThread();

    void run();

signals:
    void dataRead(char *buffer, int size);

private:
    int fd;
};

#endif
