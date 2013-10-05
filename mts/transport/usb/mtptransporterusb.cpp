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
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "mtptransporterusb.h"
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
    m_ctrlFd(-1), m_intrFd(-1), m_inFd(-1), m_outFd(-1), m_writer_busy(false)
{
    QObject::connect(&m_bulkRead, SIGNAL(dataRead(char*,int)),
        this, SLOT(handleDataRead(char*,int)), Qt::QueuedConnection);
}

bool MTPTransporterUSB::activate()
{
    MTP_LOG_CRITICAL("MTPTransporterUSB::activate");
    int success = false;

    m_ctrlFd = open(control_file, O_RDWR);
    if(-1 == m_ctrlFd)
    {
        MTP_LOG_CRITICAL("Couldn't open control endpoint file " << control_file);
    }
    else
    {
        if(-1 == write(m_ctrlFd, &mtp1descriptors, sizeof mtp1descriptors))
        {
            MTP_LOG_CRITICAL("Couldn't write descriptors to control endpoint file "
                << control_file);
        }
        else
        {
            if(-1 == write(m_ctrlFd, &mtp1strings, sizeof(mtp1strings)))
            {
                MTP_LOG_CRITICAL("Couldn't write strings to control endpoint file "
                    << control_file);
            }
            else
            {
                success = true;
                MTP_LOG_INFO("mtp function set up");
            }
        }
    }

    if(success) {
        catchUserSignal();
        openDevices(); // TODO: trigger with Bind?
        m_ctrl.setFd(m_ctrlFd);
        QObject::connect(&m_ctrl, SIGNAL(startIO()),
            this, SLOT(startRead()), Qt::QueuedConnection);
        QObject::connect(&m_ctrl, SIGNAL(stopIO()),
            this, SLOT(stopRead()), Qt::QueuedConnection);
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
    closeDevices();

    m_ctrl.interrupt();
    m_ctrl.wait();
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
    m_bulkRead.start();
}

void MTPTransporterUSB::reset()
{
    m_ioState = ACTIVE;
    m_containerReadLen = 0;

    m_bulkRead.exitThread();
    m_bulkWrite.exitThread();
    m_intrWrite.exitThread();

    m_bulkRead.wait();
    m_bulkWrite.wait();
    m_intrWrite.wait();

    m_intrWrite.start();
    m_bulkRead.start();

    MTP_LOG_CRITICAL("reset");
}

MTPTransporterUSB::~MTPTransporterUSB()
{
    deactivate();
}

bool MTPTransporterUSB::sendData(const quint8* data, quint32 dataLen, bool isLastPacket)
{
    // TODO: can't handle re-entrant calls with the current design.
    if(m_writer_busy)
    {
        MTP_LOG_WARNING("Refusing bulk write request during bulk write");
        return false;
    }
    m_writer_busy = true;

    // Unlike the other IO threads, the bulk writer is only alive
    // while processing one buffer and then finishes. It all happens
    // during this call. The reason for the extra thread is to
    // remain responsive to events while the data is being written.
    // That's done by calling processEvents while waiting.

    m_bulkWrite.setData(data, dataLen);
    m_bulkWrite.start();

    // The bulk writer will make sure that processEvents is woken up
    // when the result is ready.
    while(!m_bulkWrite.resultReady()) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    bool r = m_bulkWrite.getResult();

    m_writer_busy = false;
    return r;
}

bool MTPTransporterUSB::sendEvent(const quint8* data, quint32 dataLen, bool isLastPacket)
{
    m_intrWrite.addData(data, dataLen);

    return true;
}

void MTPTransporterUSB::handleDataRead(char* buffer, int size)
{
    // TODO: Merge this with processReceivedData
    if(size > 0)
        processReceivedData((quint8 *)buffer, size);
    m_bulkRead.releaseBuffer();
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

void MTPTransporterUSB::processReceivedData(quint8* data, quint32 dataLen)
{
    bool isFirstPacket = false;
    //MTP_LOG_INFO("data=" << (void*)data << "dataLen=" << dataLen);

    // In case of operations having data phases, the intitiator sends us 2 seperate mtp usb containers
    // for the operation and then the data successively. We need the while loop and the container/chunk length
    // logic below in order to send these one after the other to mtpresponder even though we read both as one data chunk.
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
                emit fetchObjectSize(data, &objectSize);
                if(objectSize > (0xFFFFFFFF - MTP_HEADER_SIZE))
                {
                    m_containerReadLen = objectSize + MTP_HEADER_SIZE;
                }
            }
            isFirstPacket = true;
            //MTP_LOG_INFO("start container m_containerReadLen=" << m_containerReadLen << "dataLen=" << dataLen);
        }

        chunkLen = (dataLen < m_containerReadLen) ? dataLen : m_containerReadLen;
        m_containerReadLen -= chunkLen;

        emit dataReceived((quint8*)data, chunkLen, isFirstPacket, (m_containerReadLen == 0));

        data += chunkLen;
        dataLen  -= chunkLen;
    }
}

void MTPTransporterUSB::openDevices()
{
    m_ioState = ACTIVE;

    if(m_intrWrite.isRunning())
        m_intrWrite.exitThread();

    m_inFd = open(in_file, O_RDWR);
    if(-1 == m_inFd)
    {
        MTP_LOG_CRITICAL("Couldn't open IN endpoint file " << in_file);
    } else {
        m_bulkWrite.setFd(m_inFd);
    }

    // TODO: Read state might lock due to the manner of operation,
    // but we should ensure that it doesn't stick around, it wouldn't
    // really be nessesary to stop the thread, but make sure it's in the
    // correct operation state. (The read() will terminate once it's closed)

    if(-1 != m_outFd) {
        close(m_outFd);
        //m_bulkRead.wait();
    }

    m_outFd = open(out_file, O_RDWR);
    if(-1 == m_outFd)
    {
        MTP_LOG_CRITICAL("Couldn't open OUT endpoint file " << out_file);
    } else {
        m_bulkRead.setFd(m_outFd);
        m_bulkRead.start();
    }

    m_intrFd = open(interrupt_file, O_RDWR);
    if(-1 == m_intrFd)
    {
        MTP_LOG_CRITICAL("Couldn't open INTR endpoint file " << interrupt_file);
    } else {
        m_intrWrite.setFd(m_intrFd);
        m_intrWrite.start();
    }
}

void MTPTransporterUSB::closeDevices()
{
    m_ioState = SUSPENDED;

    m_bulkRead.exitThread();
    m_bulkWrite.exitThread();
    m_intrWrite.exitThread();

    // FIXME: this probably won't exit properly?
    // -- It doesn't, fixit
    if(m_outFd != -1) {
        close(m_outFd);
        m_outFd = -1;
    }
    if(m_inFd != -1) {
        close(m_inFd);
        m_inFd = -1;
    }
    if(m_intrFd != -1) {
        close(m_intrFd);
        m_intrFd = -1;
    }
}

void MTPTransporterUSB::startRead()
{
    m_bulkRead.start();
}

void MTPTransporterUSB::stopRead()
{
    emit cleanup();
    m_containerReadLen = 0;
}

void MTPTransporterUSB::handleHighPriorityData()
{
}
