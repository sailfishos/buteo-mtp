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

#define MAX_DATA_IN_SIZE (16 * 1024)
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
    : QThread(parent), m_handle(0), m_fd(0), m_shouldExit(false)
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

// TODO: In Qt 5.2, QThread::requestInterruption() can make this nicer.
void IOThread::exitThread()
{
    // Tell the thread to exit; the run() method should check this variable
    m_shouldExit = true;
    // Wake it up if it's blocked on something
    interrupt();
    // Wait for the thread to exit, interrupting again every millisecond.
    // Looping is necessary to avoid some races (such as interrupting
    // just before the thread starts blocking I/O).
    while (!wait(1))
        interrupt();
    // Clean up for the next run
    m_shouldExit = false;
}

bool IOThread::stall(bool dirIn)
{
    /* Indicate a protocol stall by requesting I/O in the "wrong" direction.
     * So after a USB_DIR_IN request (where we'd normally write a response)
     * we read instead, and after a USB_DIR_OUT request (where we'd normally
     * read a response) we write instead.
     */
    int err;
    if(dirIn) // &err is just a dummy value here, since length is 0 bytes
        err = read(m_fd, &err, 0);
    else
        err = write(m_fd, &err, 0);
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

    m_handle = pthread_self();
    while(!m_shouldExit) {
        readSize = read(m_fd, readBuffer, MAX_CONTROL_IN_SIZE);
        if (readSize <= 0) {
            if (errno != EINTR)
                perror("ControlReaderThread");
            continue;
        }
        count = readSize/(sizeof(struct usb_functionfs_event));
        event = (struct usb_functionfs_event*)readBuffer;
        for(int i = 0; i < count; i++ )
            handleEvent(event + i);
    }

    m_handle = 0;
    MTP_LOG_CRITICAL("ControlReaderThread exited");
}

void ControlReaderThread::sendStatus()
{
    QMutexLocker locker(&m_statusLock);

    int bytesWritten = 0;
    int dataLen = 4; /* TODO: If status size is ever above 0x4 */
    char *dataptr = (char*)&status_data[m_status];

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
    MTP_LOG_INFO("Event: " << event_names[event->type]);
    switch(event->type) {
        case FUNCTIONFS_ENABLE:
        case FUNCTIONFS_RESUME:
            emit startIO();
            break;
        case FUNCTIONFS_DISABLE:
        case FUNCTIONFS_SUSPEND:
            emit stopIO();
            break;
        case FUNCTIONFS_BIND:
            emit bindUSB();
            break;
        case FUNCTIONFS_UNBIND:
            emit unbindUSB();
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
                sendStatus();
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
    bool bufferSent = false;

    m_handle = pthread_self();

    char* inbuf = new char[MAX_DATA_IN_SIZE];
    // m_wait controls message flow between bulkreader and main thread.
    // This thread fills inbuf, then hands the buffer over to the main
    // thread by emitting dataRead.
    // When the main thread is done with the buffer, it will wake up
    // this thread again by calling releaseBuffer() which uses m_wait
    // to notify this thread.
    while (!m_shouldExit) {
        readSize = read(m_fd, inbuf, MAX_DATA_IN_SIZE); // Read Header
        if (m_shouldExit)
            break;
        if (readSize == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == ESHUTDOWN) {
                MTP_LOG_WARNING("BulkReaderThread delaying: errno " << errno);
                msleep(1);
                continue;
            }
            MTP_LOG_CRITICAL("BulkReaderThread exiting: errno " << errno);
            break;
        }

        bufferSent = true;
        m_lock.lock();
        emit dataRead(inbuf, readSize);
        m_wait.wait(&m_lock);
        m_lock.unlock();
        bufferSent = false;
    }

    // FIXME: We can't free the buffer if there may still be a
    // dataRead event on the main thread's queue that references it. 
    // Avoid a segfault there by leaking the buffer. Not an ideal solution.
    if (!bufferSent)
        delete[] inbuf;
    m_handle = 0;
}

// Called by the main thread when it's done with the buffer it got
// from the dataRead signal.
void BulkReaderThread::releaseBuffer()
{
    // The reader thread will take the lock before emitting dataRead,
    // and will release it inside m_wait, so taking the lock here ensures
    // that the reader is already waiting and will notice our wakeAll.
    QMutexLocker locker(&m_lock);
    m_wait.wakeAll();
}

void BulkReaderThread::interrupt() // Executed in main thread
{
    IOThread::interrupt();  // wake up the thread if it's in read()
    m_wait.wakeAll(); // wake up the thread if it's in m_wait.wait()
}

BulkWriterThread::BulkWriterThread(QObject *parent)
    : IOThread(parent)
{
    // Connect the finished signal (emitted from the writer thread) to
    // a do-nothing slot (received in the transporter's thread) in order to
    // make sure that sendData() in the transporter is woken up after the
    // result is ready. This way sendData doesn't have to poll.
    connect(this, SIGNAL(finished()), SLOT(quit()), Qt::QueuedConnection);
}

void BulkWriterThread::setData(const quint8 *buffer, quint32 dataLen)
{
    // This runs in the main thread. The caller makes sure that the
    // writer thread is not running yet.

    m_buffer = buffer;
    m_dataLen = dataLen;
    m_result = false;
    m_result_ready.store(0);
}

void BulkWriterThread::run()
{
    // Call setData before starting the thread.

    int bytesWritten = 0;
    char *dataptr = (char*)m_buffer;

    m_handle = pthread_self();

    while (m_dataLen && !m_shouldExit) {
        bytesWritten = write(m_fd, dataptr, m_dataLen);
        if(bytesWritten == -1)
        {
            if(errno == EINTR)
                continue;
            if(errno == EAGAIN)
            {
                MTP_LOG_WARNING("BulkWriterThread delaying: errno " << errno);
                msleep(1);
                continue;
            }
            if(errno == ESHUTDOWN)
            {
                // After a shutdown, the host won't expect this data anymore,
                // so drop it and report failure.
                MTP_LOG_WARNING("BulkWriterThread exiting (endpoint shutdown)");
                break;
            }
            MTP_LOG_CRITICAL("BulkWriterThread exiting: errno " << errno);
            break;
        }
        dataptr += bytesWritten;
        m_dataLen -= bytesWritten;
    }

    m_handle = 0;
    m_result = m_dataLen == 0;
    m_result_ready.storeRelease(1);
}

bool BulkWriterThread::resultReady()
{
    return m_result_ready.load() != 0;
}

bool BulkWriterThread::getResult()
{
    // Check resultReady() before calling getResult()
    return m_result;
}

InterruptWriterThread::InterruptWriterThread(QObject *parent)
    : IOThread(parent)
{
}

InterruptWriterThread::~InterruptWriterThread()
{
    reset();
}

void InterruptWriterThread::addData(const quint8 *buffer, quint32 dataLen)
{
    QMutexLocker locker(&m_lock);

    quint8 *copy = (quint8*)malloc(dataLen);
    if(copy == NULL) {
        MTP_LOG_CRITICAL("Couldn't allocate memory for events");
        return;
    }
    memcpy(copy, buffer, dataLen);

    // This is here in case the interrupt writing thread cannot keep up
    // with the events. It removes the oldest events.
    while(m_buffers.count() >= MAX_EVENTS_STORED) {
        QPair<quint8*,int> pair = m_buffers.takeFirst();
        delete pair.first;
    }

    if(m_buffers.empty())
        m_wait.wakeAll(); // restart processing after m_lock is released
    m_buffers.append(QPair<quint8*,int>(copy, dataLen));
}

void InterruptWriterThread::run()
{
    m_handle = pthread_self();

    while(!m_shouldExit) {
        m_lock.lock();

        while(!m_shouldExit && m_buffers.isEmpty())
            m_wait.wait(&m_lock); // will release the lock while waiting

        if (m_shouldExit) { // may have been woken up by exitThread()
            m_lock.unlock();
            break;
        }

        QPair<quint8*,int> pair = m_buffers.takeFirst();
        m_lock.unlock();

        quint8 *dataptr = pair.first;
        int dataLen = pair.second;

        while(dataLen && !m_shouldExit) {
            int bytesWritten = write(m_fd, dataptr, dataLen);
            if(bytesWritten == -1)
            {
                if (errno == EINTR)
                    continue;
                if (errno == EAGAIN || errno == ESHUTDOWN) {
                    MTP_LOG_WARNING("InterruptWriterThread delaying:" << errno);
                    msleep(1);
                    continue;
                }
                MTP_LOG_CRITICAL("InterruptWriterThread exiting:" << errno);
                break;
            }
            dataptr += bytesWritten;
            dataLen -= bytesWritten;
        }

        free(pair.first);
    }

    m_handle = 0;
}

void InterruptWriterThread::reset()
{
    QMutexLocker locker(&m_lock);

    QPair<quint8*,int> item;
    foreach(item, m_buffers) {
        delete item.first;
    }
    m_buffers.clear();
}

void InterruptWriterThread::interrupt()
{
    IOThread::interrupt();  // wake up the thread if it's in write()
    m_wait.wakeAll(); // wake up the thread if it's in m_wait.wait()
}
