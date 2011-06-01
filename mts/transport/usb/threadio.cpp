#include "threadio.h"
#include <linux/usb/functionfs.h>

#include <QDebug>
#include <QMutex>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usb/functionfs.h>

//#define MAX_DATA_IN_SIZE (64 * 1024)
#define MAX_DATA_IN_SIZE (64 * 256)

const struct ptp_device_status_data status_data[] = {
    /* OK     */ { 0x0004, PTP_RC_OK, 0, 0 },
    /* BUSY   */ { 0x0004, PTP_RC_DEVICE_BUSY, 0, 0 },
    /* CANCEL */ { 0x0004, PTP_RC_TRANSACTION_CANCELLED, 0, 0 }
};

static const char *const event_names[] = {
"BIND",
"UNBIND",
"ENABLE",
"DISABLE",
"SETUP",
"SUSPEND",
"RESUME"
};

ControlReaderThread::ControlReaderThread(QObject *parent)
    : QThread(parent), m_state(0), m_handle(0)
{
}

ControlReaderThread::~ControlReaderThread()
{
}

void ControlReaderThread::setFd(int fd)
{
    m_fd = fd;
}

void ControlReaderThread::run()
{
    qDebug() << "Entering control thread";
    struct usb_functionfs_event event;
    int readSize;

    // This is a nasty hack for pthread kill use
    // Qt documentation says not to use it
    m_handle = QThread::currentThreadId();

    while(read(m_fd, &event, sizeof(event)) == sizeof(event)) {
        handleEvent(&event);
    }

    m_handle = 0;
    if(errno != EINTR)
        perror("ControlReaderThread::run");

    qDebug() << "Exiting from control thread";
}

void ControlReaderThread::sendStatus(enum mtpfs_status status)
{
    int bytesWritten = 0;
    int dataLen = 0x0004; /* TODO: If status size is ever above 0x4 */
    char *dataptr = (char*)&status_data[status];

    do {
        bytesWritten = write(m_fd, dataptr, dataLen);
        if(bytesWritten == -1)
        {
            return;
        }
        dataptr += bytesWritten;
        dataLen -= bytesWritten;
    } while(dataLen);
}

void ControlReaderThread::setStatus(enum mtpfs_status status)
{
    m_status = status;
}

void ControlReaderThread::handleEvent(struct usb_functionfs_event *event)
{
    qDebug() << "Event: " << event_names[event->type];
    switch(event->type) {
        case FUNCTIONFS_ENABLE:
        case FUNCTIONFS_RESUME:
            qDebug() << "emit startio";
            emit startIO();
            break;
        case FUNCTIONFS_DISABLE:
        case FUNCTIONFS_SUSPEND:
            qDebug() << "emit stopio";
            emit stopIO();
            break;
        case FUNCTIONFS_SETUP:
            setupRequest((void*)event);
            break;
        default:
            qDebug() << "FIXME: Event" << event_names[event->type] << "not implemented";
            break;
    }
}

void ControlReaderThread::setupRequest(void *data)
{
    struct usb_functionfs_event *e = (struct usb_functionfs_event *)data;
    int ret;
    switch(e->u.setup.bRequest) {
        case PTP_REQ_GET_DEVICE_STATUS:
            sendStatus(m_status);
            break;
        case PTP_REQ_CANCEL:
            emit transferCancel();
            break;
        case PTP_REQ_DEVICE_RESET:
            emit deviceReset();
            break;
        default:

            qDebug() << "SETUP has no handling yet";
            break;
    }
}


BulkReaderThread::BulkReaderThread(QObject *parent)
    : QThread(parent), m_handle(0)
{
}

BulkReaderThread::~BulkReaderThread()
{
}

#define INITIAL_SIZE MAX_DATA_IN_SIZE
void BulkReaderThread::setFd(int fd)
{
    m_fd = fd;
}

void BulkReaderThread::run()
{
    int readSize;
    qDebug() << "Entering data reader thread";

    // This is a nasty hack for pthread kill use
    // Qt documentation says not to use it
    m_handle = QThread::currentThreadId();

    char* inbuf = new char[MAX_DATA_IN_SIZE];

    do {
        readSize = read(m_fd, inbuf, MAX_DATA_IN_SIZE); // Read Header
        while(readSize != -1) {
            emit dataRead(inbuf, readSize);
            qDebug() << "***************** Read data: " << readSize;
            // This will wait until it's released in the main thread
            m_lock.tryLock();
            m_lock.lock();
            inbuf = new char[MAX_DATA_IN_SIZE];
            readSize = read(m_fd, inbuf, MAX_DATA_IN_SIZE); // Read Header
        }
    } while(errno == ESHUTDOWN);
    //} while(errno == EINTR || errno == ESHUTDOWN);
    // TODO: Handle the exceptions above properly.

    m_handle = 0;

    perror("BulkReaderThread");
    qDebug() << "******************** Exiting data reader thread";
}

BulkWriterThread::BulkWriterThread(QObject *parent)
    : QThread(parent), m_handle(0)
{
}

void BulkWriterThread::setData(int fd, const quint8 *buffer, quint32 dataLen, bool isLastPacket)
{
    m_buffer = buffer;
    m_dataLen = dataLen;
    m_isLastPacket = isLastPacket;
    m_fd = fd;
    m_result = false;
}

void BulkWriterThread::run()
{
    int bytesWritten = 0;
    char *dataptr = (char*)m_buffer;

    // This is a nasty hack for pthread kill use
    // Qt documentation says not to use it
    m_handle = QThread::currentThreadId();

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

    m_handle = 0;

    m_lock.unlock();
}

bool BulkWriterThread::getResult()
{
    return m_result;
}
