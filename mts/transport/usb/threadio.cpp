#include "threadio.h"
#include <linux/usb/functionfs.h>
#include <QMutex>
#include <QMutexLocker>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <endian.h>

#include "trace.h"

#define cpu_to_le16(x)  htole16(x)
#define cpu_to_le32(x)  htole32(x)
#define le32_to_cpu(x)  le32toh(x)
#define le16_to_cpu(x)  le16toh(x)

//#define MAX_DATA_IN_SIZE (64 * 1024)
#define MAX_DATA_IN_SIZE (64 * 256)
#define MAX_CONTROL_IN_SIZE 64
#define MAX_EVENTS_STORED 16

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
    if(m_handle) {
        MTP_LOG_INFO("Sending interrupt signal");
        pthread_kill(m_handle, SIGUSR1);
    }
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

bool IOThread::stall(bool dirIn)
{
    if(dirIn)
        return stallRead();
    else
        return stallWrite();
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
    //qDebug() << "Event: " << event_names[event->type];
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

    /* USB Still Image Capture Device Definition, Section 5 */
    /* www.usb.org/developers/devclass_docs/usb_still_img10.pdf */

    //qDebug() << "bRequestType:" << e->u.setup.bRequestType;
    //qDebug() << "bRequest:" << e->u.setup.bRequest;
    //qDebug() << "wValue:" << e->u.setup.wValue;
    //qDebug() << "wIndex:" << e->u.setup.wIndex;
    //qDebug() << "wLength:" << e->u.setup.wLength;

    switch(e->u.setup.bRequest) {
        case PTP_REQ_GET_DEVICE_STATUS:
            if(e->u.setup.bRequestType == 0xa1)
                sendStatus(m_status);
            else
                stall((e->u.setup.bRequestType & USB_DIR_IN)>0);
            break;
        case PTP_REQ_CANCEL:
            emit cancelTransaction();
            break;
        case PTP_REQ_DEVICE_RESET:
            emit deviceReset();
            break;
        //case PTP_REQ_GET_EXTENDED_EVENT_DATA:
        default:
            stall((e->u.setup.bRequestType & USB_DIR_IN)>0);
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
    m_threadRunning = true;

    char* inbuf = new char[MAX_DATA_IN_SIZE];

    do {
        readSize = read(m_fd, inbuf, MAX_DATA_IN_SIZE); // Read Header
        while(readSize != -1) {
            emit dataRead(inbuf, readSize);
            // This will wait until it's released in the main thread
            m_lock.tryLock();
            if(!m_threadRunning) break;
            m_lock.lock();
            if(!m_threadRunning) break;
            inbuf = new char[MAX_DATA_IN_SIZE];
            readSize = read(m_fd, inbuf, MAX_DATA_IN_SIZE); // Read Header
        }
    } while(errno == ESHUTDOWN && m_threadRunning);
    //} while(errno == EINTR || errno == ESHUTDOWN);
    // TODO: Handle the exceptions above properly.

    m_handle = 0;

    // Known errors to exit here:
    //   EAGAIN 11 Try Again ** Seems to be triggered from DISABLE/ENABLE
    //   EINTR   4 Interrupt ** Triggered from SIGUSR1

    if(errno != EINTR) {
        MTP_LOG_CRITICAL("BulkReaderThread exited: " << errno);
    }
}

void BulkReaderThread::exitThread()
{
    // Executed in main thread
    m_threadRunning = false;
    // TODO: Not 100% reliable operation
    usleep(10);
    interrupt();
    m_lock.unlock();
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

void BulkWriterThread::exitThread()
{
    interrupt();
}

InterruptWriterThread::InterruptWriterThread(QObject *parent)
    : IOThread(parent), m_running(false)
{
}

InterruptWriterThread::~InterruptWriterThread()
{
    reset();
}

void InterruptWriterThread::setFd(int fd)
{
    m_fd = fd;
}

void InterruptWriterThread::addData(const quint8 *buffer, quint32 dataLen)
{
    QMutexLocker locker(&m_bufferLock);
    int overflow;

    quint8 *copy = (quint8*)malloc(dataLen);
    if(copy == NULL) {
        MTP_LOG_CRITICAL("Couldn't allocate memory for events");
        return;
    }
    memcpy(copy, buffer, dataLen);

    if(m_buffers.count() >= MAX_EVENTS_STORED) {
        // It is possible that that sometimes the interrupt will miss
        // consuming evevents, this is here to keep it from going out
        // of hand slowly over time
        overflow = m_buffers.count() - MAX_EVENTS_STORED;
        if(overflow > 0) {
            while(overflow--) {
                QPair<quint8*,int> pair = m_buffers.first();
                m_buffers.removeFirst();
                delete pair.first;
            }
        }
        // This will discard the oldest event
        interrupt();
    }

    m_buffers.append(QPair<quint8*,int>(copy, dataLen));

    // Incase the event system is waiting empty
    m_lock.unlock();
}

void InterruptWriterThread::run()
{
    // This is a nasty hack for pthread kill use
    // Qt documentation says not to use it
    m_handle = QThread::currentThreadId();

    m_running = true;

    while(m_running) {
        if(m_buffers.isEmpty()) {
            m_lock.tryLock();
            m_lock.lock();
        } else {
            m_bufferLock.lock();

            QPair<quint8*,int> pair = m_buffers.first();
            m_buffers.removeFirst();

            m_bufferLock.unlock();

            quint8 *dataptr = pair.first;
            int dataLen = pair.second;

            do {
                int bytesWritten = write(m_fd, dataptr, dataLen);
                if(bytesWritten == -1)
                {
                    if(errno != EINTR)
                        m_running = false;
                }
                dataptr += bytesWritten;
                dataLen -= bytesWritten;
            } while(dataLen);

            free(pair.first);
        }
    }

    m_handle = 0;
}

void InterruptWriterThread::reset()
{
    QMutexLocker locker(&m_bufferLock);

    QPair<quint8*,int> item;
    foreach(item, m_buffers) {
        delete item.first;
    }
    m_buffers.clear();
}

void InterruptWriterThread::exitThread()
{
    m_running = false;
    interrupt();
    m_lock.unlock();
}
