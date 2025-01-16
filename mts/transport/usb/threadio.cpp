#include <linux/usb/functionfs.h>

#include <QMutex>
#include <QMutexLocker>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <endian.h>
#include <unistd.h>

#include "threadio.h"
#include "trace.h"

#define MTP_READ(fd,buf,len,log_success) ({\
    if( (log_success) ) {\
        MTP_LOG_WARNING("read(" << fd << (void*)(buf) << len << ") -> ...");\
    }\
    ssize_t rc = read((fd),(buf),(len));\
    int saved = errno;\
    if( rc == -1 ) {\
        MTP_LOG_CRITICAL("read(" << fd << (void*)(buf) << len << ") -> err:"\
                         << strerror(errno));\
    }\
    else if( rc == 0 ) {\
        MTP_LOG_CRITICAL("read(" << fd << (void*)(buf) << len << ") -> eof");\
    }\
    else if( (log_success) ) {\
        MTP_LOG_WARNING("read(" << fd << (void*)(buf) << len << ") -> rc:" << rc);\
    }\
    errno = saved, rc;\
})

#define MTP_WRITE(fd,buf,len,log_success) ({\
    if( (log_success) ) {\
        MTP_LOG_WARNING("write(" << fd << (void*)(buf) << len << ") -> ...");\
    }\
    ssize_t rc = write((fd),(buf),(len));\
    int saved = errno;\
    if( rc == -1 ) {\
        MTP_LOG_CRITICAL("write(" << fd << (void*)(buf) << len << ") -> err:"\
                         << strerror(errno));\
    }\
    else if( (log_success) ) {\
        MTP_LOG_WARNING("write(" << fd << (void*)(buf) << len << ") -> rc:" << rc);\
    }\
    errno = saved, rc;\
})

const int MAX_DATA_IN_SIZE = 16 * 1024;  // Matches USB transfer size
const int MAX_CONTROL_IN_SIZE = 64;

/* Maximum number of events to queue for sending via the interrupt
 * endpoint. Balance between: Too small value can cause the host
 * side to get out of sync with the reality on the device side vs.
 * too large value can hamper bulk io throughput if the host side
 * does not purge the queued events fast enough -> assume being able
 * to hold and transfer events resulting from things like deleting
 * few hundred files is enough. */
const int MAX_EVENTS_STORED = 512;

// Give BulkReaderThread some space to acquire chunks while the main
// thread is working, but still small enough for the main thread to
// process as one event.
const int READER_BUFFER_SIZE = MAX_DATA_IN_SIZE * 16;

const struct ptp_device_status_data status_data[] = {
    /* OK     */ {
        htole16(0x0004),
        htole16(PTP_RC_OK), 0, 0
    },
    /* BUSY   */ {
        htole16(0x0004),
        htole16(PTP_RC_DEVICE_BUSY), 0, 0
    },
    /* CANCEL */ {
        htole16(0x0004),
        htole16(PTP_RC_TRANSACTION_CANCELLED), 0, 0
    }
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

static void handleUSR1(int signum)
{
    Q_UNUSED(signum);

    static const char m[] = "***USR1***\n";
    if ( write(2, m, sizeof m - 1) == -1 ) {
        // dontcare
    }

    return; // This handler just exists to make blocking I/O return with EINTR
}

static void catchUSR1()
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = handleUSR1;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGUSR1, &action, NULL) < 0)
        MTP_LOG_WARNING("Could not establish SIGUSR1 signal handler");
}

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

    if (m_handle) {
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
    if (dirIn) // &err is just a dummy value here, since length is 0 bytes
        err = MTP_READ(m_fd, &err, 0, false);
    else
        err = MTP_WRITE(m_fd, &err, 0, false);
    if (err == -1 && errno == EL2HLT) {
        return true;
    } else {
        MTP_LOG_CRITICAL("Unable to halt endpoint");
        return false;
    }
}

void IOThread::run()
{
    /* set up signal handler before assigning the m_handle
     * member variable used from interrupt() method. */
    catchUSR1();

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

    while (!m_shouldExit) {
        readSize = MTP_READ(m_fd, readBuffer, MAX_CONTROL_IN_SIZE, false);
        if (readSize <= 0) {
            if (errno != EINTR)
                perror("ControlReaderThread");
            continue;
        }
        count = readSize / (sizeof(struct usb_functionfs_event));
        event = (struct usb_functionfs_event *)readBuffer;
        for (int i = 0; i < count; i++ )
            handleEvent(event + i);
    }

    MTP_LOG_CRITICAL("ControlReaderThread exited");
}

void ControlReaderThread::sendStatus()
{
    QMutexLocker locker(&m_statusLock);

    int bytesWritten = 0;
    int dataLen = 4; /* TODO: If status size is ever above 0x4 */
    char *dataptr = (char *)&status_data[m_status];

    do {
        bytesWritten = MTP_WRITE(m_fd, dataptr, dataLen, false);
        if (bytesWritten == -1) {
            return;
        }
        dataptr += bytesWritten;
        dataLen -= bytesWritten;
    } while (dataLen);
}

void ControlReaderThread::setStatus(enum mtpfs_status status)
{
    m_statusLock.lock();
    m_status = status;
    m_statusLock.unlock();
}

void ControlReaderThread::handleEvent(struct usb_functionfs_event *event)
{
    /* If there are problems with mtp startup, knowing
     * whether usb control events are being sent or not
     * is crucial -> use warning priority for loggin even
     * though receiving these is fine and fully expected. */
    MTP_LOG_WARNING("Event: " << event_names[event->type]);

    switch (event->type) {
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
        setupRequest((void *)event);
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

    switch (e->u.setup.bRequest) {
    case PTP_REQ_GET_DEVICE_STATUS:
        if (e->u.setup.bRequestType == 0xa1)
            sendStatus();
        else
            stall((e->u.setup.bRequestType & USB_DIR_IN) > 0);
        break;
    case PTP_REQ_CANCEL:
        emit cancelTransaction();
        break;
    case PTP_REQ_DEVICE_RESET:
        emit deviceReset();
        break;
    //case PTP_REQ_GET_EXTENDED_EVENT_DATA:
    default:
        stall((e->u.setup.bRequestType & USB_DIR_IN) > 0);
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

        /* Wait for read offset in locked state */
        m_bufferLock.lock();
        offset = _getOffset_locked();
        while (!m_shouldExit && offset < 0) {
            /* Expectation: Waiting should not be required except when
             * transferring large files and file system writes can't
             * keep up with usb transfer speed -> log in verbose mode. */
            MTP_LOG_INFO("waiting ...");
            m_wait.wait(&m_bufferLock);
            MTP_LOG_INFO("woke up");
            offset = _getOffset_locked();
        }
        m_bufferLock.unlock();

        /* Check if thread exit has been requested */
        if (m_shouldExit)
            break;

        /* Do a blocking read */
        readSize = MTP_READ(m_fd, m_buffer + offset, MAX_DATA_IN_SIZE, false);

        /* Check if thread exit has been requested */
        if (m_shouldExit)
            break;

        /* Handle I/O errors */
        if (readSize == -1) {
            /* Note: The error has already been logged by MTP_READ() */

            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN || errno == ESHUTDOWN) {
                msleep(1);
                continue;
            }

            /* Abandon thread - this should not happen */
            MTP_LOG_CRITICAL("exit thread due to unhandled error");
            break;
        }

        /* Update data availability in locked state */
        if (!_markNewData(offset, readSize)) {
            MTP_LOG_CRITICAL("exit thread due to bad offset:" << offset
                             << m_dataStart << m_dataSize1 << m_dataSize2);
            break;
        }

        /* Notify responder about available data */
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
    static quint32 writeMax = 16 << 10;

    // Call setData before starting the thread.

    int bytesWritten = 0;
    char *dataptr = (char *)m_buffer;
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
        bytesWritten = MTP_WRITE(m_fd, dataptr, writeNow, false);
        if (bytesWritten == -1) {
            if (errno == EIO && writeMax > PTP_HS_DATA_PKT_SIZE ) {
                writeMax >>= 1;
                MTP_LOG_WARNING("BulkWriterThread limit writes to: " << writeMax);
                continue;
            }
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN) {
                MTP_LOG_WARNING("BulkWriterThread delaying: errno " << errno);
                msleep(1);
                continue;
            }
            if (errno == ESHUTDOWN) {
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
    : IOThread(parent), m_eventBufferFull(false)
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


void InterruptWriterThread::flushData()
{
    QMutexLocker locker(&m_lock);

    while (m_buffers.count() ) {
        QPair<quint8 *, int> pair = m_buffers.takeFirst();
        free(pair.first);
    }
}

void InterruptWriterThread::addData(const quint8 *buffer, quint32 dataLen)
{
    QMutexLocker locker(&m_lock);

    quint8 *copy = (quint8 *)malloc(dataLen);
    if (copy == NULL) {
        MTP_LOG_CRITICAL("Couldn't allocate memory for events");
        return;
    }
    memcpy(copy, buffer, dataLen);

    // This is here in case the interrupt writing thread cannot keep up
    // with the events. It removes the oldest events.
    if (m_buffers.count() >= MAX_EVENTS_STORED) {
        if ( !m_eventBufferFull ) {
            m_eventBufferFull = true;
            MTP_LOG_CRITICAL("event buffer full - events will be lost");
        }

        do {
            QPair<quint8 *, int> pair = m_buffers.takeFirst();
            free(pair.first);
        } while (m_buffers.count() >= MAX_EVENTS_STORED);
    } else {
        if ( m_eventBufferFull ) {
            m_eventBufferFull = false;
            MTP_LOG_CRITICAL("event buffer no longer full");
        }
    }

    /* Note that we just buffer the event data here, the
     * actual transfer is interleaved with bulk writes. */
    m_buffers.append(QPair<quint8 *, int>(copy, dataLen));
}

void InterruptWriterThread::execute()
{
    quint8 *dataptr = 0;
    int     dataLen = 0;

    /* Lock on entry */
    m_lock.lock();

    while (!m_shouldExit) {

        /* Waiting happens in locked state, but the lock is
         * released for the duration of the wait itself. */
        MTP_LOG_TRACE("intr writer - waiting");
        m_wait.wait(&m_lock); // will release the lock while waiting
        MTP_LOG_TRACE("intr writer - executing");

        if (m_shouldExit) { // may have been woken up by exitThread()
            goto EXIT;
        }

        /* Make sure we have data to write */
        if ( !dataptr ) {
            if (m_buffers.isEmpty()) {
                /* We should really not get here. Log it and emit
                 * failure in order not to block the upper layers. */
                MTP_LOG_WARNING("stray wakeup; this should not happen");
                emit senderIdle(INTERRUPT_WRITE_SUCCESS);
                continue;
            }

            QPair<quint8 *, int> pair = m_buffers.takeFirst();
            dataptr = pair.first;
            dataLen = pair.second;
        }

        if ( !dataptr || !dataLen ) {
            MTP_LOG_WARNING("empty event data packet; ignored");
            continue;
        }

        /* Do IO in unlocked state */
        m_lock.unlock();
        int rc = MTP_WRITE(m_fd, dataptr, dataLen, false);
        m_lock.lock();

        /* Assume failure & retry later on */
        InterruptWriterResult result = INTERRUPT_WRITE_RETRY;

        /* Handle IO results in locked state */
        if (rc == -1) {
            /* Note: The error has already been logged by MTP_WRITE(). */

            if (errno == EINTR) {
                /* Assume this is caused by timeout at upper layers */
            } else if (errno == EAGAIN || errno == ESHUTDOWN) {
                msleep(1);
            } else {
                MTP_LOG_CRITICAL("thread exit due to unhandled error");
                goto EXIT;
            }
        } else {
            if ( rc != dataLen ) {
                /* Assumption is that for interrupt endpoints the kernel
                 * should accept the whole packet or nothing at all. */
                MTP_LOG_CRITICAL("partial write" << rc << "/" << dataLen << "bytes");
            } else {
                MTP_LOG_TRACE("intr writer - event sent");
            }

            free(dataptr);
            dataptr = 0;
            dataLen = 0;
            result = INTERRUPT_WRITE_SUCCESS;
        }

        emit senderIdle(result);
    }
EXIT:

    /* Unlock before leaving */
    m_lock.unlock();

    free(dataptr);
}

void InterruptWriterThread::reset()
{
    QMutexLocker locker(&m_lock);

    QPair<quint8 *, int> item;
    foreach (item, m_buffers) {
        delete item.first;
    }
    m_buffers.clear();
}

void InterruptWriterThread::interrupt()
{
    IOThread::interrupt();  // wake up the thread if it's in write()
    m_wait.wakeAll(); // wake up the thread if it's in m_wait.wait()
}
