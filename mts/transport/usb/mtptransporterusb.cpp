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
#include "readerthread.h"
#include "mtp1descriptors.h"

using namespace meegomtp1dot0;

#define MAX_DATA_IN_SIZE 64 * 256
#define DEFAULT_MAX_PACKET_SIZE 64

MTPTransporterUSB::MTPTransporterUSB() : m_ioState(ACTIVE), m_containerReadLen(0),
    m_inFd(-1), m_outFd(-1), m_intrFd(-1), m_ctrlFd(-1)
{
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

    m_ctrlThread = new ControlReaderThread(m_ctrlFd, this);

    QObject::connect(m_ctrlThread, SIGNAL(startIO()), this, SLOT(startIO()));
    QObject::connect(m_ctrlThread, SIGNAL(stopIO()), this, SLOT(stopIO()));

    m_ctrlThread->start();
    return success;
}

bool MTPTransporterUSB::deactivate()
{
    close(m_intrFd);
    close(m_outFd);
    close(m_inFd);
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
    MTP_LOG_CRITICAL("disableRW");
}

void MTPTransporterUSB::enableRW()
{
    MTP_LOG_CRITICAL("enableRW");
}

void MTPTransporterUSB::reset()
{
    m_ioState = ACTIVE;
    m_containerReadLen = 0;

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
    if(size > 0) {
        processReceivedData((quint8 *)buffer, size);
    }
    delete buffer;
}

void MTPTransporterUSB::handleRead()
{
    if( EXCEPTION != m_ioState )
    {
        char* inbuf = new char[MAX_DATA_IN_SIZE];
        int bytesRead = -1;

        LOG_DEBUG("read req");
        bytesRead = read(m_outFd, inbuf, MAX_DATA_IN_SIZE);
        LOG_DEBUG("read done");
        if(0 < bytesRead)
        {
            processReceivedData((quint8*)inbuf, bytesRead);
        }
        if(bytesRead == -1)
        {
            perror("MTPTransporterUSB::handleRead");
            MTP_LOG_CRITICAL("Read failed !!");
        }
        delete[] inbuf;
    }
    else
    {
        //MTP_LOG_CRITICAL("trying to read while in an exception");
    }
}

void MTPTransporterUSB::handleHighPriorityData()
{
}

void MTPTransporterUSB::suspend()
{
    sendDeviceBusy();
    emit suspendSignal();
}

void MTPTransporterUSB::resume()
{
    emit resumeSignal();
}

void MTPTransporterUSB::sendDeviceOK()
{
}

void MTPTransporterUSB::sendDeviceBusy()
{
}

void MTPTransporterUSB::sendDeviceTxCancelled()
{
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

void MTPTransporterUSB::handleHangup()
{
}

bool MTPTransporterUSB::sendDataOrEvent(const quint8* data, quint32 dataLen, bool isEvent, bool isLastPacket)
{
    typedef bool (MTPTransporterUSB::*pIntDataSender)(const quint8*, quint32);
    bool r = false;
    pIntDataSender senderFunc = (isEvent ? &MTPTransporterUSB::sendEventInternal : &MTPTransporterUSB::sendDataInternal)

    MTP_HEX_TRACE(data, dataLen);

    r = (this->*senderFunc)(data, dataLen);

    if(!r)
    {
        MTP_LOG_CRITICAL("Failure in sending data");
        perror("MTPTransporterUSB::sendDataOrEvent");
        return r;
    }

    return true;
}

bool MTPTransporterUSB::sendDataInternal(const quint8* data, quint32 len)
{
    int bytesWritten = 0;
    char *dataptr = (char*)data;

    do
    {
        bytesWritten = write(m_inFd, dataptr, len);
        qDebug() << "Wrote bytes: " << bytesWritten;
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

void MTPTransporterUSB::startIO()
{
    m_inFd = open(in_file, O_WRONLY);
    if(-1 == m_inFd)
    {
        MTP_LOG_CRITICAL("Couldn't open IN endpoint file " << in_file);
    }

    if(-1 != m_outFd) {
        delete m_readSocket;
        delete m_excpSocket;
    }

    m_outFd = open(out_file, O_RDONLY | O_NONBLOCK);
    if(-1 == m_outFd)
    {
        MTP_LOG_CRITICAL("Couldn't open IN endpoint file " << out_file);
    } else {
        m_readSocket = new QSocketNotifier(m_outFd, QSocketNotifier::Read);
        m_excpSocket = new QSocketNotifier(m_outFd, QSocketNotifier::Exception);

        // Connect our slots to listen to data and exception events on the USB FD
        QObject::connect(m_readSocket, SIGNAL(activated(int)), this, SLOT(handleRead()));
        QObject::connect(m_excpSocket, SIGNAL(activated(int)), this, SLOT(handleHangup()));
    }
#if 0
    if(outThread != NULL)
        delete outThread;
    outThread = new OutReaderThread(m_outFd, this);
    QObject::connect(outThread, SIGNAL(dataRead(char*,int)),
        this, SIGNAL(dataRead(char*,int)));
    outThread->start();
#endif

    m_intrFd = open(interrupt_file, O_WRONLY | O_NONBLOCK);
    if(-1 == m_intrFd)
    {
        MTP_LOG_CRITICAL("Couldn't open INTR endpoint file " << interrupt_file);
    }
}

void MTPTransporterUSB::stopIO()
{
    // FIXME: this probably won't exit properly?
#if 0
    delete outThread;
    outThread = NULL;
#endif
    if(m_outFd != -1) {
        delete m_readSocket;
        delete m_excpSocket;
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

void MTPTransporterUSB::setupRequest(void *data)
{
    struct usb_functionfs_event *e = (struct usb_functionfs_event *)data;
    switch(e->u.setup.bRequest) {
        case PTP_REQ_CANCEL:
        case PTP_REQ_DEVICE_RESET:
        case PTP_REQ_GET_DEVICE_STATUS:
        default:
            qDebug() << "Hello World";
            break;
    }
}
