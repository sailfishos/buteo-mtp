#include <QDebug>

#include "readerthread.h"
#include "functionfs.h"

#include <errno.h>

#define MAX_DATA_IN_SIZE 64 * 1024

static const char *const event_names[] = {
"BIND",
"UNBIND",
"ENABLE",
"DISABLE",
"SETUP",
"SUSPEND",
"RESUME"
};

ControlReaderThread::ControlReaderThread(int fd, QObject *parent)
    : QThread(parent), fd(fd)
{
}

ControlReaderThread::~ControlReaderThread()
{
}

void ControlReaderThread::run() {
    struct usb_functionfs_event event[1];
    int readSize;

    while(read(fd, event, sizeof(event)) == sizeof(*event)) {
        handleEvent(event);
    }

    perror("ControlReaderThread::run");
    qDebug() << "FIXME: Breaking away, not event size: " << readSize;
}

void ControlReaderThread::handleEvent(struct usb_functionfs_event *event) {
    switch(event->type) {
        case FUNCTIONFS_ENABLE:
        case FUNCTIONFS_RESUME:
            qDebug() << "Event: ENABLE";
            emit startIO();
            break;
        case FUNCTIONFS_DISABLE:
        case FUNCTIONFS_SUSPEND:
            qDebug() << "Event: DISABLE";
            emit stopIO();
            break;
        default:
            qDebug() << "FIXME: Event: " << event_names[event->type];
            break;
    }
}

OutReaderThread::OutReaderThread(int fd, QObject *parent)
    : QThread(parent), fd(fd)
{
}

OutReaderThread::~OutReaderThread()
{
}

void OutReaderThread::run() {
    struct usb_functionfs_event event[1];
    int readSize, cntSize;

    qDebug() << "Read thread START";

    // FIXME: This is a bit hacky
    char* inbuf = new char[MAX_DATA_IN_SIZE];
    readSize = read(fd, inbuf, 12); // Read Header
    qDebug() << "Readsize: " << readSize << errno;
    while(readSize != -1) {
#if 0
        quint32 size = *(quint32 *)inbuf;
        qDebug() << "Read: " << readSize << "Attempting to read: " << size;
        cntSize = read(fd, inbuf+12, (size<MAX_DATA_IN_SIZE?size:MAX_DATA_IN_SIZE)-12);
        if(cntSize == -1) {
            qDebug() << "cntSize -1";
            break;
        }
        readSize += cntSize;
        qDebug() << "******* Read: " << readSize;
#endif
        emit dataRead(inbuf, readSize);
        inbuf = new char[MAX_DATA_IN_SIZE];
        readSize = read(fd, inbuf, 12); // Read Header
    }

    perror("OutReaderThread");
    qDebug() << "FIXME: Breaking away, error set: " << readSize;
}
