#include "threadio.h"
#include <linux/usb/functionfs.h>

#include <QMutex>
#include <QMutexLocker>
#include <errno.h>
#include <sys/ioctl.h>

#include <pthread.h>
#include <signal.h>

#include "trace.h"

#include <assert.h>
#include <endian.h>

#define cpu_to_le16(x)  htole16(x)
#define cpu_to_le32(x)  htole32(x)
#define le32_to_cpu(x)  le32toh(x)
#define le16_to_cpu(x)  le16toh(x)

//#define MAX_DATA_IN_SIZE (64 * 1024)
#define MAX_DATA_IN_SIZE (64 * 256)
#define MAX_CONTROL_IN_SIZE 64

const struct ptp_device_status_data status_data[] = {
/* OK     */ { cpu_to_le16(0x0004),
               cpu_to_le16(PTP_RC_OK), 0, 0 },
/* BUSY   */ { cpu_to_le16(0x0004),
               cpu_to_le16(PTP_RC_DEVICE_BUSY), 0, 0 },
/* CANCEL */ { cpu_to_le16(0x0004),
               cpu_to_le16(PTP_RC_TRANSACTION_CANCELLED), 0, 0 }
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

IOThread::IOThread(QObject *parent)
    : QThread(parent), m_handle(0), m_fd(0)
{}

void IOThread::setFd(int fd)
{
    m_fd = fd;
}

void IOThread::interrupt()
{
    if(m_handle)
        pthread_kill(m_handle, SIGUSR1);
}

bool IOThread::stallWrite()
{
    int err;
    err = write(m_fd, &err, 0);
    if(err == -1 && errno == EL2HLT) {
        return true;
    } else {
        MTP_LOG_CRITICAL("Unable to halt endpoint");
        return false;
    }
}

bool IOThread::stallRead()
{
    int err;
    err = read(m_fd, &err, 0);
    if(err == -1 && errno == EL2HLT) {
        return true;
    } else {
        MTP_LOG_CRITICAL("Unable to halt endpoint");
        return false;
    }
}

ControlReaderThread::ControlReaderThread(QObject *parent)
    : IOThread(parent),  m_state(0)
{

}

ControlReaderThread::~ControlReaderThread()
{
}

void ControlReaderThread::run()
{
    char readBuffer[MAX_CONTROL_IN_SIZE];
    struct usb_functionfs_event *event;
    int readSize, count;

    // This is a nasty hack for pthread kill use
    // Qt documentation says not to use it
    m_handle = QThread::currentThreadId();
    do {
        while((readSize = read(m_fd, readBuffer, MAX_CONTROL_IN_SIZE)) > 0) {
            count = readSize/(sizeof(struct usb_functionfs_event));
            event = (struct usb_functionfs_event*)readBuffer;
            for(int i = 0; i < count; i++ )
                handleEvent(event + i);
        }
        perror("ControlReaderThread");
    } while(errno != EINTR);


    m_handle = 0;
    if(errno != EINTR) {
        perror("run");
        MTP_LOG_CRITICAL("ControlReaderThread exited: " << errno);
    }
}

void ControlReaderThread::sendStatus(enum mtpfs_status status)
{
    QMutexLocker locker(&m_statusLock);

    int bytesWritten = 0;
    int dataLen = 4; /* TODO: If status size is ever above 0x4 */
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
    m_statusLock.lock();
    m_status = status;
    m_statusLock.unlock();
}

void ControlReaderThread::handleEvent(struct usb_functionfs_event *event)
{
    qDebug() << "Event: " << event_names[event->type];
    switch(event->type) {
        case FUNCTIONFS_ENABLE:
        case FUNCTIONFS_RESUME:
            emit startIO();
            break;
        case FUNCTIONFS_DISABLE:
        case FUNCTIONFS_SUSPEND:
            emit stopIO();
            break;
        case FUNCTIONFS_SETUP:
            setupRequest((void*)event);
            break;
        default:
            break;
    }
}

void ControlReaderThread::setupRequest(void *data)
{
    struct usb_functionfs_event *e = (struct usb_functionfs_event *)data;

    qDebug() << "bRequestType:" << e->u.setup.bRequestType;
    qDebug() << "bRequest:" << e->u.setup.bRequest;
    qDebug() << "wValue:" << e->u.setup.wValue;
    qDebug() << "wIndex:" << e->u.setup.wIndex;
    qDebug() << "wLength:" << e->u.setup.wLength;

    switch(e->u.setup.bRequest) {
        case PTP_REQ_GET_DEVICE_STATUS:
            qDebug() << "Get Device Status";
            if(e->u.setup.bRequestType & USB_DIR_IN) {
                sendStatus(m_status);
            }
            break;
        case PTP_REQ_CANCEL:
            qDebug() << "Cancel Transaction";
            emit cancelTransaction();
            break;
        case PTP_REQ_DEVICE_RESET:
            qDebug() << "Device Reset";
            emit deviceReset();
            break;
        default:
            break;
    }
}


BulkReaderThread::BulkReaderThread(QObject *parent)
    : IOThread(parent)
{
}

BulkReaderThread::~BulkReaderThread()
{
}

void BulkReaderThread::run()
{
    int readSize;

    // This is a nasty hack for pthread kill use
    // Qt documentation says not to use it
    m_handle = QThread::currentThreadId();

    char* inbuf = new char[MAX_DATA_IN_SIZE];

    do {
        readSize = read(m_fd, inbuf, MAX_DATA_IN_SIZE); // Read Header
        while(readSize != -1) {
            emit dataRead(inbuf, readSize);
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

    if(errno != EINTR) {
        MTP_LOG_CRITICAL("BulkReaderThread exited: " << errno);
    }
}

BulkWriterThread::BulkWriterThread(QObject *parent)
    : IOThread(parent)
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
