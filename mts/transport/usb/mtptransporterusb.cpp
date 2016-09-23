/*
* This file is part of libmeegomtp package
*
* Copyright (C) 2010 Nokia Corporation. All rights reserved.
*
* Contact: Santosh Puranik <santosh.puranik@nokia.com>
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this list
* of conditions and the following disclaimer. Redistributions in binary form must
* reproduce the above copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of Nokia Corporation nor the names of its contributors may be
* used to endorse or promote products derived from this software without specific
* prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "mtptransporterusb.h"
#include "mtpresponder.h"
#include "mtpcontainer.h"
#include <linux/usb/functionfs.h>
#include "trace.h"
#include "threadio.h"
#include "mtp1descriptors.h"
#include <QMutex>
#include <QCoreApplication>

using namespace meegomtp1dot0;

static void signalHandler(int signum)
{
    Q_UNUSED(signum);

    return; // This handler just exists to make blocking I/O return with EINTR
}

static void catchUserSignal()
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGUSR1, &action, NULL) < 0)
        MTP_LOG_WARNING("Could not establish SIGUSR1 signal handler");
}

MTPTransporterUSB::MTPTransporterUSB() : m_ioState(SUSPENDED), m_containerReadLen(0),
    m_ctrlFd(-1), m_intrFd(-1), m_inFd(-1), m_outFd(-1),
    m_reader_busy(READER_FREE), m_writer_busy(false), m_events_busy(INTERRUPT_WRITER_IDLE),
    m_events_failed(0), m_inSession(false),
    m_storageReady(false),
    m_readerEnabled(false),
    m_responderBusy(true)
{
    // event write cancelation
    m_event_cancel = new QTimer(this);
    m_event_cancel->setInterval(1000);
    m_event_cancel->setSingleShot(true);
    QObject::connect(m_event_cancel, SIGNAL(timeout()),
                     this, SLOT(eventTimeout()));

    // event write completion
    QObject::connect(&m_intrWrite, &InterruptWriterThread::senderIdle,
                     this, &MTPTransporterUSB::eventCompleted,
                     Qt::QueuedConnection);

    // bulk read data
    QObject::connect(&m_bulkRead, SIGNAL(dataReady()),
        this, SLOT(handleDataReady()), Qt::QueuedConnection);

    // event write control
    MTPResponder* responder = MTPResponder::instance();
    QObject::connect(responder, &MTPResponder::commandPending,
                     this, &MTPTransporterUSB::onCommandPending);
    QObject::connect(responder, &MTPResponder::commandFinished,
                     this, &MTPTransporterUSB::onCommandFinished);
}

void MTPTransporterUSB::onCommandFinished()
{
    if( m_responderBusy ) {
        m_responderBusy = false;
        MTP_LOG_TRACE("m_responderBusy:" << m_responderBusy);
        sendQueuedEvent();
    }
}

void MTPTransporterUSB::onCommandPending()
{
    if( !m_responderBusy ) {
        m_responderBusy = true;
        MTP_LOG_TRACE("m_responderBusy:" << m_responderBusy);
    }
}

bool MTPTransporterUSB::writeMtpDescriptors()
{
    if (write(m_ctrlFd, &mtp1descriptors, sizeof mtp1descriptors) >= 0)
        return true;

    if (errno == EINVAL) {
        MTP_LOG_WARNING("Kernel did not accept endpoint descriptors;"
            " trying 'ss_count' workaround");
        // Some android kernels changed the usb_functionfs_descs_head size
        // by adding an ss_count member. Try it that way.
        mtp1_descriptors_s_incompatible descs;
        descs.header = mtp1descriptors_header_incompatible;
        descs.fs_descs = mtp1descriptors.fs_descs;
        descs.hs_descs = mtp1descriptors.hs_descs;
        if (write(m_ctrlFd, &descs, sizeof descs) >= 0)
            return true;
    }

    MTP_LOG_CRITICAL("Couldn't write descriptors to control endpoint file"
        << MTP_EP_PATH_CONTROL);
    return false;
}

bool MTPTransporterUSB::writeMtpStrings()
{
    if (write(m_ctrlFd, &mtp1strings, sizeof(mtp1strings)) >= 0)
        return true;

    MTP_LOG_CRITICAL("Couldn't write strings to control endpoint file"
        << MTP_EP_PATH_CONTROL);
    return false;
}

bool MTPTransporterUSB::activate()
{
    MTP_LOG_CRITICAL("MTPTransporterUSB::activate");
    int success = false;

    m_ctrlFd = open(MTP_EP_PATH_CONTROL, O_RDWR);
    if(-1 == m_ctrlFd)
    {
        MTP_LOG_CRITICAL("Couldn't open control endpoint file " << MTP_EP_PATH_CONTROL);
    }
    else
    {
        if (writeMtpDescriptors() && writeMtpStrings()) {
            success = true;
            MTP_LOG_INFO("mtp function set up");
        }
    }

    if(success) {
        catchUserSignal();
        m_ctrl.setFd(m_ctrlFd);
        QObject::connect(&m_ctrl, SIGNAL(startIO()),
            this, SLOT(startRead()), Qt::QueuedConnection);
        QObject::connect(&m_ctrl, SIGNAL(stopIO()),
            this, SLOT(stopRead()), Qt::QueuedConnection);
        QObject::connect(&m_ctrl, SIGNAL(bindUSB()),
            this, SLOT(openDevices()), Qt::QueuedConnection);
        QObject::connect(&m_ctrl, SIGNAL(unbindUSB()),
            this, SLOT(closeDevices()), Qt::QueuedConnection);
        QObject::connect(&m_ctrl, SIGNAL(deviceReset()),
            this, SIGNAL(deviceReset()), Qt::QueuedConnection);
        QObject::connect(&m_ctrl, SIGNAL(cancelTransaction()),
            this, SIGNAL(cancelTransaction()), Qt::QueuedConnection);
        m_ctrl.start();
    }

    return success;
}

bool MTPTransporterUSB::deactivate()
{
    MTP_LOG_INFO("MTPTransporterUSB deactivating");
    closeDevices();

    m_ctrl.exitThread();
    close(m_ctrlFd);

    return true;
}

bool MTPTransporterUSB::flushData()
{
    bool result = true;

    MTP_LOG_CRITICAL("flushData");

    return result;
}

void MTPTransporterUSB::disableRW()
{
    m_bulkRead.exitThread();
}

void MTPTransporterUSB::enableRW()
{
    startRead();
}

void MTPTransporterUSB::reset()
{
    MTP_LOG_CRITICAL("reset ...");

    m_bulkRead.exitThread();
    m_bulkWrite.exitThread();
    m_intrWrite.exitThread();

    m_ioState = ACTIVE;
    m_containerReadLen = 0;
    m_bulkRead.resetData();
    m_resetCount++;

    m_intrWrite.start();
    startRead();

    MTP_LOG_CRITICAL("reset done");
}

MTPTransporterUSB::~MTPTransporterUSB()
{
    deactivate();
}

#if MTP_LOG_LEVEL >= MTP_LOG_LEVEL_TRACE
static const char *InterruptWriterStateRepr(int state)
{
    const char *repr = "INTERRUPT_WRITER_<UNKNOWN>";
    switch( state ) {
    case MTPTransporterUSB::INTERRUPT_WRITER_IDLE:     repr = "INTERRUPT_WRITER_IDLE";     break;
    case MTPTransporterUSB::INTERRUPT_WRITER_BUSY:     repr = "INTERRUPT_WRITER_BUSY";     break;
    case MTPTransporterUSB::INTERRUPT_WRITER_RETRY:    repr = "INTERRUPT_WRITER_RETRY";    break;
    case MTPTransporterUSB::INTERRUPT_WRITER_DISABLED: repr = "INTERRUPT_WRITER_DISABLED"; break;
    default: break;
    }
    return repr;
}
#endif

void MTPTransporterUSB::setEventsBusy(int state)
{
    if( m_events_busy != state )
    {
        MTP_LOG_TRACE("m_events_busy:"
                      << InterruptWriterStateRepr(m_events_busy)
                      << "->"
                      << InterruptWriterStateRepr(state));
        m_events_busy = InterruptWriterState(state);
    }
}


bool MTPTransporterUSB::sendData(const quint8* data, quint32 dataLen, bool isLastPacket)
{
    // TODO: can't handle re-entrant calls with the current design.

    if(m_writer_busy)
    {
        // If we get here, packets are be lost and protocol broken
        MTP_LOG_CRITICAL("Refusing recursive bulk write request");
        return false;
    }
    m_writer_busy = true;
    MTP_LOG_TRACE("m_writer_busy:" << m_writer_busy);

    // wait until interrupt writer is done with event
    if (m_events_busy == INTERRUPT_WRITER_BUSY) {
        /* Serializing bulk and intr writes causes unwanted delays,
         * especially if the host is not processing intr transfers.
         * Make delays visible in verbose mode to help future tuning. */
        MTP_LOG_INFO("intr writer is busy - wait");
        while (m_events_busy == INTERRUPT_WRITER_BUSY) {
            QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
        }
        MTP_LOG_INFO("intr writer is idle - continue");
    }

    // Unlike the other IO threads, the bulk writer is only alive
    // while processing one buffer and then finishes. It all happens
    // during this call. The reason for the extra thread is to
    // remain responsive to events while the data is being written.
    // That's done by calling processEvents while waiting.

    m_bulkWrite.setData(data, dataLen, isLastPacket);
    m_bulkWrite.start();

    // The bulk writer will make sure that processEvents is woken up
    // when the result is ready.
    while(!m_bulkWrite.resultReady()) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }

    bool r = m_bulkWrite.getResult();

    m_bulkWrite.wait();

    m_writer_busy = false;
    MTP_LOG_TRACE("m_writer_busy:" << m_writer_busy);

    // check if there are events to send
    sendQueuedEvent();

    return r;
}

void MTPTransporterUSB::sessionOpenChanged(bool isOpen)
{
    if (m_inSession != isOpen) {
        m_inSession = isOpen;
        // reset failure counter
        if (m_inSession)
            m_events_failed = 0;

        // re-enable event sending
        if( m_events_busy == INTERRUPT_WRITER_DISABLED )
            setEventsBusy(INTERRUPT_WRITER_IDLE);

        /* If host does not close session due to errors or by design, it
         * can create problems. Log changes to make it easier to spot. */
        MTP_LOG_WARNING("mtp session" << (m_inSession ? "opened" : "closed"));
    }
}

/* How many successive event send failures to tolerate */
#define MAX_EVENT_SEND_FAILURES 3

void MTPTransporterUSB::sendQueuedEvent()
{
    if( m_writer_busy )
        return;

    if( m_responderBusy )
        return;

    if (!m_inSession)
        return;

    if (m_writer_busy)
        return;

    if( m_events_busy == INTERRUPT_WRITER_DISABLED )
        return;

    if (m_events_busy == INTERRUPT_WRITER_BUSY )
        return;

    if( m_events_busy == INTERRUPT_WRITER_IDLE ) {
        if (!m_intrWrite.hasData())
            return;
    }

    if (m_events_failed >= MAX_EVENT_SEND_FAILURES) {
        setEventsBusy(INTERRUPT_WRITER_DISABLED);
        return;
    }

    MTP_LOG_INFO("activate intr writer");
    setEventsBusy(INTERRUPT_WRITER_BUSY);
    m_event_cancel->start();
    m_intrWrite.sendOne();
}

bool MTPTransporterUSB::sendEvent(const quint8* data, quint32 dataLen, bool isLastPacket)
{
    Q_UNUSED(isLastPacket);

    if (!m_inSession) {
        /* In theory at least there Some events could/should be sent even if the
         * host has not opened a session. But in order not to cause delays in the
         * startup phase, ignore all events until session has been opened. */
        MTP_LOG_WARNING("event ignored - no active session");
        return false;
    }

    if (m_events_failed >= MAX_EVENT_SEND_FAILURES) {
        /* For example gphoto2 based gvfs-gphoto2-volume-monitor hardly ever
         * reads intr data - at least in a timely manner. Since this creates
         * repeated delays via timer based cancelation, after few repeated
         * attempts we just ignore all events until session reopen. */
        return false;
    }

    // adds event to queue, but does not start sending
    m_intrWrite.addData(data, dataLen);

    // if possible, start sending
    sendQueuedEvent();

    return true;
}
void MTPTransporterUSB::eventTimeout()
{
    if( m_writer_busy ) {
        MTP_LOG_WARNING("event write timeout during send data - retry later");
    }
    else {
        ++m_events_failed;

        /* The intr write did not complete in expected time.
         * Log the incident and interrupt the writer thread. */
        MTP_LOG_WARNING("event write timeout" << m_events_failed
                        << "/" << MAX_EVENT_SEND_FAILURES);

        if( m_events_failed == MAX_EVENT_SEND_FAILURES ) {
            MTP_LOG_WARNING("event sending disabled - too many send failures");

            /* Clear the queue so that it won't spoil the next session */
            m_intrWrite.flushData();
        }
    }
    m_intrWrite.interrupt();
}
void MTPTransporterUSB::eventCompleted(int result)
{
    MTP_LOG_TRACE("result:" << InterruptWriterResultRepr(result));

    // cancel timeout
    m_event_cancel->stop();

    if( m_events_busy != INTERRUPT_WRITER_BUSY ) {
        // We have thread synchronization issue
        MTP_LOG_CRITICAL("unhandled intr writer result");
        return;
    }

    switch( result ) {
    case INTERRUPT_WRITE_SUCCESS:
        // reset failure counter
        m_events_failed = 0;
        // check if more events can be sent
        setEventsBusy(INTERRUPT_WRITER_IDLE);
        sendQueuedEvent();
        break;

    case INTERRUPT_WRITE_FAILURE:
        // event lost - try the next event later
        setEventsBusy(INTERRUPT_WRITER_IDLE);
        break;

    case INTERRUPT_WRITE_RETRY:
        // try again if/when possible
        setEventsBusy(INTERRUPT_WRITER_RETRY);
        sendQueuedEvent();
        break;

    default:
        MTP_LOG_CRITICAL("unhandled intr writer result");
        abort();
    }
}

void MTPTransporterUSB::handleDataReady()
{
    // The buffer protocol shared with the reader does not allow nested
    // getData requests; all received data must be released before the
    // next getData. This makes recursive calls a problem. They can happen
    // if the handler for dataReceived calls processEvents and there is
    // another dataReady queued. Deal with it by not getting any new
    // data in a recursive call; the MTP responder doesn't want such data
    // anyway while it's still busy.
    if (m_reader_busy != READER_FREE) {
        m_reader_busy = READER_POSTPONED;
        return;
    }

    do {
        m_reader_busy = READER_BUSY;
        processReceivedData();
        // Loop because a READER_POSTPONED state means we skipped a dataReady
        // signal, and we have to handle it now.
    } while (m_reader_busy == READER_POSTPONED);

    m_reader_busy = READER_FREE;
}

void MTPTransporterUSB::suspend()
{
    sendDeviceBusy();
    emit suspendSignal();
}

void MTPTransporterUSB::resume()
{
    emit resumeSignal();
    sendDeviceOK();
}

void MTPTransporterUSB::sendDeviceOK()
{
    m_ctrl.setStatus(MTPFS_STATUS_OK);
}

void MTPTransporterUSB::sendDeviceBusy()
{
    m_ctrl.setStatus(MTPFS_STATUS_BUSY);
}

void MTPTransporterUSB::sendDeviceTxCancelled()
{
    m_ctrl.setStatus(MTPFS_STATUS_TXCANCEL);
}

void MTPTransporterUSB::processReceivedData()
{
    char *data;
    int dataLen;
    bool isFirstPacket = false;
    int resetCount = m_resetCount;

    m_bulkRead.getData(&data, &dataLen);
    //MTP_LOG_INFO("data=" << (void*)data << "dataLen=" << dataLen);

    while (dataLen > 0)
    {
        quint32 chunkLen;

        if (m_containerReadLen == 0)
        {
            // TODO: Change for big-endian machines
            m_containerReadLen = (*(const quint32 *)data);
            if(0xFFFFFFFF == m_containerReadLen)
            {
                // For object transfers > 4GB
                quint64 objectSize = 0;
                emit fetchObjectSize((quint8 *)data, &objectSize);
                if(objectSize > (0xFFFFFFFF - MTP_HEADER_SIZE))
                {
                    m_containerReadLen = objectSize + MTP_HEADER_SIZE;
                }
            }
            isFirstPacket = true;
            //MTP_LOG_INFO("start container m_containerReadLen=" << m_containerReadLen << "dataLen=" << dataLen);
        }

        chunkLen = ((quint32) dataLen < m_containerReadLen) ? dataLen : m_containerReadLen;
        m_containerReadLen -= chunkLen;

        emit dataReceived((quint8*)data, chunkLen, isFirstPacket, (m_containerReadLen == 0));

        // The connection might have been reset during data handling,
        // which makes the current buffer invalid.
        if (resetCount != m_resetCount)
            break;

        data += chunkLen;
        dataLen -= chunkLen;
        // The dataReceived signal was handled synchronously,
        // so it's safe to release the data now.
        m_bulkRead.releaseData(chunkLen);
    }
}

void MTPTransporterUSB::openDevices()
{
    m_ioState = ACTIVE;
    MTP_LOG_INFO("MTP opening endpoint devices");

    m_inFd = open(MTP_EP_PATH_IN, O_RDWR);
    if(-1 == m_inFd)
    {
        MTP_LOG_CRITICAL("Couldn't open IN endpoint file " << MTP_EP_PATH_IN);
    } else {
        m_bulkWrite.setFd(m_inFd);
    }

    m_outFd = open(MTP_EP_PATH_OUT, O_RDWR);
    if(-1 == m_outFd)
    {
        MTP_LOG_CRITICAL("Couldn't open OUT endpoint file " << MTP_EP_PATH_OUT);
    } else {
        m_bulkRead.setFd(m_outFd);
        startRead();
    }

    m_intrFd = open(MTP_EP_PATH_INTERRUPT, O_RDWR);
    if(-1 == m_intrFd)
    {
        MTP_LOG_CRITICAL("Couldn't open INTR endpoint file " << MTP_EP_PATH_INTERRUPT);
    } else {
        m_intrWrite.setFd(m_intrFd);
        m_intrWrite.start();
    }
}

void MTPTransporterUSB::closeDevices()
{
    MTP_LOG_INFO("MTP closing endpoint devices");
    m_ioState = SUSPENDED;

    m_bulkRead.exitThread();
    m_bulkWrite.exitThread();
    m_intrWrite.exitThread();

    stopRead();
    m_intrWrite.reset();

    if(m_outFd != -1) {
        close(m_outFd);
        m_bulkWrite.setFd(-1);
        m_outFd = -1;
    }
    if(m_inFd != -1) {
        close(m_inFd);
        m_bulkRead.setFd(-1);
        m_inFd = -1;
    }
    if(m_intrFd != -1) {
        close(m_intrFd);
        m_intrWrite.setFd(-1);
        m_intrFd = -1;
    }
}

void MTPTransporterUSB::rethinkRead()
{
    if (m_readerEnabled) {
        if (m_storageReady) {
            MTP_LOG_TRACE("start bulk reader");
            m_bulkRead.start();
        } else {
            MTP_LOG_TRACE("delay bulk reader");
        }
    }
}

void MTPTransporterUSB::startRead()
{
    MTP_LOG_TRACE("reader enabled");
    m_readerEnabled = true;
    rethinkRead();
}

void MTPTransporterUSB::stopRead()
{
    MTP_LOG_TRACE("reader disabled");
    m_readerEnabled = false;
    emit cleanup();
    m_containerReadLen = 0;
    m_bulkRead.resetData();
    m_resetCount++;
}

void MTPTransporterUSB::onStorageReady(void)
{
    MTP_LOG_TRACE("Storage ready");
    m_storageReady = true;
    rethinkRead();
}

void MTPTransporterUSB::handleHighPriorityData()
{
}
