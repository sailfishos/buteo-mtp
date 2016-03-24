#include "threadio.h"
#include <linux/usb/functionfs.h>
#include <QMutex>
#include <QMutexLocker>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <endian.h>
#include <unistd.h>

#include "trace.h"

const int MAX_DATA_IN_SIZE = 16 * 1024;  // Matches USB transfer size
const int MAX_CONTROL_IN_SIZE = 64;
const int MAX_EVENTS_STORED = 16;
// Give BulkReaderThread some space to acquire chunks while the main
// thread is working, but still small enough for the main thread to
// process as one event.
const int READER_BUFFER_SIZE = MAX_DATA_IN_SIZE * 16;

const struct ptp_device_status_data status_data[] = {
/* OK     */ { htole16(0x0004),
               htole16(PTP_RC_OK), 0, 0 },
/* BUSY   */ { htole16(0x0004),
               htole16(PTP_RC_DEVICE_BUSY), 0, 0 },
/* CANCEL */ { htole16(0x0004),
               htole16(PTP_RC_TRANSACTION_CANCELLED), 0, 0 }
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
    : QThread(parent), m_fd(0), m_shouldExit(false), m_handle(0)
{}

void IOThread::setFd(int fd)
{
    m_fd = fd;
}

void IOThread::interrupt()
{
    QMutexLocker locker(&m_handleLock);

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

void IOThread::run()
{
    m_handle = pthread_self();

    execute();

    m_handleLock.lock();
    m_handle = 0;
    m_handleLock.unlock();
}

ControlReaderThread::ControlReaderThread(QObject *parent)
    : IOThread(parent),  m_state(0)
{

}

ControlReaderThread::~ControlReaderThread()
{
}

void ControlReaderThread::execute()
{
    char readBuffer[MAX_CONTROL_IN_SIZE];
    struct usb_functionfs_event *event;
    int readSize, count;

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
    m_buffer = new char[READER_BUFFER_SIZE];
    resetData();
}

BulkReaderThread::~BulkReaderThread()
{
    delete[] m_buffer;
}

// Should be called by the receiver thread while the reader is not running
void BulkReaderThread::resetData()
{
    m_dataStart = 0;
    m_dataSize1 = 0;
    m_dataSize2 = 0;
}

// Find a usable place in m_buffer to read MAX_DATA_IN_SIZE bytes.
// Return -1 if there's no space.
int BulkReaderThread::_getOffset_locked()
{
    // See the class definition for the story of how m_buffer is handled.
    if (READER_BUFFER_SIZE - (m_dataStart + m_dataSize1) >= MAX_DATA_IN_SIZE)
        return m_dataStart + m_dataSize1;
    if (m_dataStart - m_dataSize2 >= MAX_DATA_IN_SIZE)
        return m_dataSize2;
    return -1;
}

bool BulkReaderThread::_markNewData(int offset, int size)
{
    QMutexLocker locker(&m_bufferLock);

    // See the class definition for the story of how m_buffer is handled.
    // If nobody messed up then 'offset' should be pointing to the
    // end of one of the buffer pieces.

    if (offset == m_dataStart + m_dataSize1)
        m_dataSize1 += size;
    else if (offset == m_dataSize2)
        m_dataSize2 += size;
    else
        return false;
    return true;
}

void BulkReaderThread::execute()
{
    int readSize;

    while (!m_shouldExit) {
        int offset;
        m_bufferLock.lock();
        offset = _getOffset_locked();
        while (!m_shouldExit && offset < 0) {
            m_wait.wait(&m_bufferLock);
            offset = _getOffset_locked();
        }
        m_bufferLock.unlock();
        if (m_shouldExit)
            break;

        readSize = read(m_fd, m_buffer + offset, MAX_DATA_IN_SIZE);
        if (m_shouldExit)
            break;
        if (readSize == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == ESHUTDOWN) {
                MTP_LOG_WARNING("BulkReaderThread delaying: errno" << errno);
                msleep(1);
                continue;
            }
            MTP_LOG_CRITICAL("BulkReaderThread exiting: errno" << errno);
            break;
        }

        if (!_markNewData(offset, readSize)) {
            MTP_LOG_CRITICAL("BulkReaderThread bad offset" << offset << m_dataStart << m_dataSize1 << m_dataSize2);
            break;
        }

        emit dataReady();
    }
}

// Called by the main thread to request more data to process.
// getData() should not be called again until all of it has been
// released with releaseData().
void BulkReaderThread::getData(char **bufferp, int *size)
{
    QMutexLocker locker(&m_bufferLock);

    if (m_dataSize1 == 0 && m_dataSize2 > 0) {
        // The primary data area has been consumed and the reader has
        // already started filling the secondary data area, which means
        // it's safe to switch over.
        m_dataStart = 0;
        m_dataSize1 = m_dataSize2;
        m_dataSize2 = 0;
    }

    if (m_dataSize1 > 0) {
        *bufferp = m_buffer + m_dataStart;
        *size = m_dataSize1;
    } else {
        *bufferp = 0;
        *size = 0;
    }
}

// Called by the main thread to say that it has processed some of the
// data in m_buffer.
void BulkReaderThread::releaseData(int size)
{
    QMutexLocker locker(&m_bufferLock);
    m_dataStart += size;
    m_dataSize1 -= size;
    if (m_dataSize1 == 0 && m_dataSize2 > 0) {
        // The primary data area has been consumed and the reader has
        // already started filling the secondary data area, which means
        // it's safe to switch over.
        m_dataStart = 0;
        m_dataSize1 = m_dataSize2;
        m_dataSize2 = 0;
    }
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

void BulkWriterThread::setData(const quint8 *buffer, quint32 dataLen, bool terminateTransfer)
{
    // This runs in the main thread. The caller makes sure that the
    // writer thread is not running yet.

    m_buffer = buffer;
    m_dataLen = dataLen;
    m_terminateTransfer = terminateTransfer;
    m_result = false;
    m_result_ready.store(0);
}

void BulkWriterThread::execute()
{
    // Maximum length of individual writes
    static quint32 writeMax = 64 << 10;

    // Call setData before starting the thread.

    int bytesWritten = 0;
    char *dataptr = (char*)m_buffer;
    // PTP compatibility requires that a transfer is terminated by a
    // "short packet" (a packet of less than maximum length). This
    // happens naturally for most transfers, but if the transfer size
    // is a multiple of the packet size then a zero-length packet
    // should be added. Note: this logic requires that all previous
    // buffers for the current transfer were also multiples of the
    // packet size, which is generally not a problem because powers
    // of two are used.
    // TODO: Get the real packet size from the kernel
    bool zeropacket = m_terminateTransfer && m_dataLen % PTP_HS_DATA_PKT_SIZE == 0;

    while ((m_dataLen || zeropacket) && !m_shouldExit) {
        quint32 writeNow = (m_dataLen < writeMax) ? m_dataLen : writeMax;
        bytesWritten = write(m_fd, dataptr, writeNow);
        if(bytesWritten == -1)
        {
            if(errno == EIO && writeMax > PTP_HS_DATA_PKT_SIZE )
            {
                writeMax >>= 1;
                MTP_LOG_WARNING("BulkWriterThread limit writes to: " << writeMax);
                continue;
            }
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
        if (m_dataLen == 0)
            zeropacket = false;
        dataptr += bytesWritten;
        m_dataLen -= bytesWritten;
    }

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

bool InterruptWriterThread::hasData()
{
    QMutexLocker locker(&m_lock);
    bool has_data = !m_buffers.empty();
    return has_data;
}

void InterruptWriterThread::sendOne()
{
    QMutexLocker locker(&m_lock);
    m_wait.wakeAll();
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

    /* Note that we just buffer the event data here, the
     * actual transfer is interleaved with bulk writes. */
    m_buffers.append(QPair<quint8*,int>(copy, dataLen));
}

void InterruptWriterThread::execute()
{
    while(!m_shouldExit) {
        m_lock.lock();
        m_wait.wait(&m_lock); // will release the lock while waiting

        if (m_shouldExit) { // may have been woken up by exitThread()
            m_lock.unlock();
            break;
        }

        if (m_buffers.isEmpty()) {
            /* We should really not get here. Log it and emit
             * failure in order not to block the upper layers. */
            MTP_LOG_WARNING("stray wakeup; this should not happen");
            m_lock.unlock();
            emit senderIdle(false);
            continue;
        }

        QPair<quint8*,int> pair = m_buffers.takeFirst();
        m_lock.unlock();

        quint8 *dataptr = pair.first;
        int dataLen = pair.second;

        while(dataLen && !m_shouldExit) {
            int bytesWritten = write(m_fd, dataptr, dataLen);
            if(bytesWritten == -1)
            {
                if (errno == EINTR) {
                    /* Assume this is caused by timeout at upper layers
                     * and abandon the event send. */
                    MTP_LOG_WARNING("InterruptWriterThread - interrupted");
                    break;
                }
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

        bool success = (dataLen == 0);
        emit senderIdle(success);
    }
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
