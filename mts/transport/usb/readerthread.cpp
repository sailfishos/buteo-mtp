#include "readerthread.h"
#include <linux/usb/functionfs.h>

#include <QDebug>
#include <QMutex>
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
    : QThread(parent), m_fd(fd), m_state(0)
{
}

ControlReaderThread::~ControlReaderThread()
{
}

void ControlReaderThread::run()
{
    struct usb_functionfs_event event;
    int readSize;

    while(read(m_fd, &event, sizeof(event)) == sizeof(event)) {
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
            if(!m_state)
                emit startIO();
            m_state = 1;
            break;
        case FUNCTIONFS_DISABLE:
        case FUNCTIONFS_SUSPEND:
            if(m_state)
                emit stopIO();
            m_state = 0;
            break;
        case FUNCTIONFS_SETUP:
            emit setupRequest((void*)event);
            break;
        default:
            qDebug() << "FIXME: Event" << event_names[event->type] << "not implemented";
            break;
    }
}

OutReaderThread::OutReaderThread(int fd, QObject *parent, QMutex *mutex)
    : QThread(parent), m_fd(fd), m_lock(mutex)
{
}

OutReaderThread::~OutReaderThread()
{
}

#define INITIAL_SIZE MAX_DATA_IN_SIZE

void OutReaderThread::run()
{
    int readSize;

    // FIXME: This is a bit hacky
    char* inbuf = new char[MAX_DATA_IN_SIZE];
    m_lock->lock();

    do {
        readSize = read(m_fd, inbuf, MAX_DATA_IN_SIZE); // Read Header
        while(readSize != -1) {
            emit dataRead(inbuf, readSize);
            // This will wait until it's released in the main thread
            m_lock->lock();
            inbuf = new char[MAX_DATA_IN_SIZE];
            readSize = read(m_fd, inbuf, MAX_DATA_IN_SIZE); // Read Header
        }
    } while(errno == EINTR || errno == ESHUTDOWN);
    // TODO: Handle the exceptions above properly.

    perror("OutReaderThread");
    qDebug() << "Exiting data thread";
}

InWriterThread::InWriterThread(QObject *parent) : QThread(parent)
{
}

void InWriterThread::setData(int fd, const quint8 *buffer, quint32 dataLen, bool isLastPacket, QMutex *sendLock)
{
    m_buffer = buffer;
    m_dataLen = dataLen;
    m_lock = sendLock;
    m_isLastPacket = isLastPacket;
    m_fd = fd;
    m_result = false;
}

void InWriterThread::run()
{
    int bytesWritten = 0;
    char *dataptr = (char*)m_buffer;

    do {
        bytesWritten = write(m_fd, dataptr, m_dataLen);
        if(bytesWritten == -1)
        {
            m_result = false;
            return;
        }
        dataptr += bytesWritten;
        m_dataLen -= bytesWritten;
    } while(m_dataLen);
    m_result = true;

    m_lock->unlock();
}

bool InWriterThread::getResult()
{
    return m_result;
}
