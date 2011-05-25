#include "readerthread.h"
#include <linux/usb/functionfs.h>

#include <QDebug>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usb/functionfs.h>

//#define MAX_DATA_IN_SIZE (64 * 1024)
#define MAX_DATA_IN_SIZE (64 * 256)

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
    : QThread(parent), fd(fd), state(0)
{
}

ControlReaderThread::~ControlReaderThread()
{
}

void ControlReaderThread::run()
{
    struct usb_functionfs_event event;
    int readSize;

    while(read(fd, &event, sizeof(event)) == sizeof(event)) {
        handleEvent(&event);
    }

    perror("ControlReaderThread::run");
    qDebug() << "FIXME: Breaking away, not event size: " << readSize;
}

void ControlReaderThread::handleEvent(struct usb_functionfs_event *event)
{
    qDebug() << "Event: " << event_names[event->type];
    switch(event->type) {
        case FUNCTIONFS_ENABLE:
        case FUNCTIONFS_RESUME:
            if(!state)
                emit startIO();
            state = 1;
            break;
        case FUNCTIONFS_DISABLE:
        case FUNCTIONFS_SUSPEND:
            if(state)
                emit stopIO();
            state = 0;
            break;
        case FUNCTIONFS_SETUP:
            emit setupRequest((void*)event);
            break;
        default:
            qDebug() << "FIXME: Event" << event_names[event->type] << "not implemented";
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

#define INITIAL_SIZE MAX_DATA_IN_SIZE

void OutReaderThread::run()
{
    struct usb_functionfs_event event[1];
    int readSize, cntSize;

    // FIXME: This is a bit hacky
    char* inbuf = new char[MAX_DATA_IN_SIZE];

    do {
        readSize = read(fd, inbuf, MAX_DATA_IN_SIZE); // Read Header
        while(readSize != -1) {
            emit dataRead(inbuf, readSize);
            inbuf = new char[MAX_DATA_IN_SIZE];
            readSize = read(fd, inbuf, MAX_DATA_IN_SIZE); // Read Header
        }
    } while(errno == EINTR || errno == ESHUTDOWN);
    // TODO: Handle the exceptions above properly.

    perror("OutReaderThread");
    qDebug() << "Exiting data thread";
}
