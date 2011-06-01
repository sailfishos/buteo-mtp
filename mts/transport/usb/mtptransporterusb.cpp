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

#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <QSocketNotifier>
// FIXME: Change the ioctl header below to a system header inclusion
#include "mtptransporterusb.h"
#include "mtpcontainer.h"
// FIXME include ptp.h when the new driver gets integrated
#include <linux/usb/functionfs.h>
#include "ptp.h"
#include "trace.h"
#include "threadio.h"
#include "mtp1descriptors.h"

#include <pthread.h>
#include <signal.h>

#include <QMutex>
#include <QCoreApplication>

using namespace meegomtp1dot0;

#define MAX_DATA_IN_SIZE 64 * 256
#define DEFAULT_MAX_PACKET_SIZE 64

MTPTransporterUSB::MTPTransporterUSB() : m_ioState(SUSPENDED), m_containerReadLen(0),
    m_ctrlFd(-1), m_intrFd(-1), m_inFd(-1), m_outFd(-1)
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

    return success;
}

bool MTPTransporterUSB::deactivate()
{
    closeDevices();

    interruptCtrl();
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
    interruptOut();
    MTP_LOG_CRITICAL("disableRW");
}

void MTPTransporterUSB::enableRW()
{
    m_bulkRead.start();
    MTP_LOG_CRITICAL("enableRW");
}

void MTPTransporterUSB::reset()
{
    m_ioState = ACTIVE;
    m_containerReadLen = 0;

    interruptOut();
    interruptIn();
    m_bulkRead.wait();
    m_bulkWrite.wait();

    m_bulkWrite.m_lock.unlock();
    m_bulkRead.m_lock.unlock();

    m_bulkRead.start();
    m_bulkWrite.start();

    MTP_LOG_CRITICAL("reset");
}

MTPTransporterUSB::~MTPTransporterUSB()
{
    deactivate();
}

bool MTPTransporterUSB::sendData(const quint8* data, quint32 dataLen, bool isLastPacket)
{
    return sendDataOrEvent(data, dataLen, false, isLastPacket);
}

bool MTPTransporterUSB::sendEvent(const quint8* data, quint32 dataLen, bool isLastPacket)
{
    return sendDataOrEvent(data, dataLen, true, isLastPacket);
}

void MTPTransporterUSB::handleDataRead(char* buffer, int size)
{
    // TODO: Merge this with processReceivedData
    if(size > 0) {
        processReceivedData((quint8 *)buffer, size);
    }
    delete buffer;

    m_bulkRead.m_lock.unlock();
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

void MTPTransporterUSB::interruptCtrl()
{
    // TODO: This is a hack.
    if(m_ctrl.m_handle)
        pthread_kill(m_ctrl.m_handle, SIGUSR1);
}

void MTPTransporterUSB::interruptOut()
{
    // TODO: This is a hack.
    if(m_bulkRead.m_handle)
        pthread_kill(m_bulkRead.m_handle, SIGUSR1);
}

void MTPTransporterUSB::interruptIn()
{
    // TODO: This is a hack.
    if(m_bulkWrite.m_handle)
        pthread_kill(m_bulkWrite.m_handle, SIGUSR1);
}

bool MTPTransporterUSB::sendDataOrEvent(const quint8* data, quint32 dataLen, bool isEvent, bool isLastPacket)
{
    bool r = false;
    // TODO: Re-entrancy, probably needs to be done earlier in the chain
    // ++ Should we be able to interrupt while sending normal data...
    // Aka, do we want a separate thread for Interrupts

    m_bulkWrite.m_lock.lock();

    if(isEvent) {
        m_bulkWrite.setData(m_intrFd, data, dataLen, isLastPacket);
    } else {
        m_bulkWrite.setData(m_inFd, data, dataLen, isLastPacket);
    }

    m_bulkWrite.start();

    // TODO: Check if this causes excessive load-->add a wait into it
    while(!m_bulkWrite.m_lock.tryLock()) {
        QCoreApplication::processEvents();
    }
    r = m_bulkWrite.getResult();

    m_bulkWrite.m_lock.unlock();
#if 0
    if(!isEvent && isLastPacket)
    {
        if( 0 > fsync(m_usbFd) )
        {
            r = false;
        }
    }
#endif
    return true;
}

bool MTPTransporterUSB::sendDataInternal(const quint8* data, quint32 len)
{
    int bytesWritten = 0;
    char *dataptr = (char*)data;

    do
    {
        bytesWritten = write(m_inFd, dataptr, len);
        if(bytesWritten == -1)
        {
            return false;
        }
        dataptr += bytesWritten;
        len -= bytesWritten;
    } while(len);

    return true;
}

bool MTPTransporterUSB::sendEventInternal(const quint8* data, quint32 len)
{
    int bytesWritten = 0;
    char *dataptr = (char*)data;

    do
    {
        bytesWritten = write(m_intrFd, dataptr, len);
        if(bytesWritten == -1)
        {
            MTP_LOG_CRITICAL("Write failed::" << errno << strerror(errno));
            return false;
        }
        dataptr += bytesWritten;
        len -= bytesWritten;
    }while(len);

    return true;
}

void MTPTransporterUSB::openDevices()
{
    m_ioState = ACTIVE;
    m_bulkWrite.m_lock.unlock();

    m_inFd = open(in_file, O_WRONLY);
    if(-1 == m_inFd)
    {
        MTP_LOG_CRITICAL("Couldn't open IN endpoint file " << in_file);
    }

    if(-1 != m_outFd) {
        close(m_outFd);
        //m_bulkRead.wait();
    }

    m_outFd = open(out_file, O_RDONLY | O_NONBLOCK);
    if(-1 == m_outFd)
    {
        MTP_LOG_CRITICAL("Couldn't open IN endpoint file " << out_file);
    } else {
        m_bulkRead.setFd(m_outFd);
        m_bulkRead.start();
    }


    m_intrFd = open(interrupt_file, O_WRONLY | O_NONBLOCK);
    if(-1 == m_intrFd)
    {
        MTP_LOG_CRITICAL("Couldn't open INTR endpoint file " << interrupt_file);
    }

}

void MTPTransporterUSB::closeDevices()
{
    m_ioState = SUSPENDED;

    interruptIn();
    interruptOut();

    // FIXME: this probably won't exit properly?
    if(m_outFd != -1) {
        m_bulkRead.m_lock.unlock();
        close(m_outFd);
        //m_bulkRead.wait();
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

    m_bulkRead.m_lock.unlock();
    m_bulkWrite.m_lock.unlock();
}
