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
#include "ptp.h"
#include "trace.h"

using namespace meegomtp1dot0;

#define MAX_DATA_IN_SIZE 64 * 1024
#define DEFAULT_MAX_PACKET_SIZE 64

MTPTransporterUSB::MTPTransporterUSB() : MTP_DEVICE_PORT("/dev/mtp0"), m_ioState(ACTIVE), m_containerReadLen(0) 
{
}

bool MTPTransporterUSB::activate()
{
    m_usbFd = openUsbFd(MTP_DEVICE_PORT);
    if (m_usbFd < 0)
    {
        MTP_LOG_CRITICAL("Error in opening " << MTP_DEVICE_PORT);
        return false;
    }

    MTP_LOG_CRITICAL("Opened " << MTP_DEVICE_PORT );

    // Retrieve the max data and event packet sizes
    m_maxDataSize = getMaxDataPacketSize();
    m_maxEventSize = getMaxEventPacketSize();

    // Create socket notifiers over the USB FD
    m_readSocket = new QSocketNotifier(m_usbFd, QSocketNotifier::Read);
    m_excpSocket = new QSocketNotifier(m_usbFd, QSocketNotifier::Exception);

    // Connect our slots to listen to data and exception events on the USB FD
    QObject::connect(m_readSocket, SIGNAL(activated(int)), this, SLOT(handleRead()));
    QObject::connect(m_excpSocket, SIGNAL(activated(int)), this, SLOT(handleHangup()));

    return true;
}

bool MTPTransporterUSB::deactivate()
{
    //TODO
    return true;
}

bool MTPTransporterUSB::flushData()
{
    bool result = true;
    if( -1 == fsync(m_usbFd) )
    {
        result = false;
    }
    return result;
}

void MTPTransporterUSB::disableRW()
{
    m_readSocket->setEnabled(false);
}

void MTPTransporterUSB::enableRW()
{
    m_readSocket->setEnabled(true);
}

void MTPTransporterUSB::reset()
{
    m_ioState = ACTIVE;
    m_containerReadLen = 0;
}

MTPTransporterUSB::~MTPTransporterUSB()
{
    if(m_usbFd > 0)
    {
        close(m_usbFd);
        delete m_readSocket;
        m_readSocket = 0;
        delete m_excpSocket;
        m_excpSocket = 0;
    }
}

bool MTPTransporterUSB::sendData(const quint8* data, quint32 dataLen, bool isLastPacket)
{
    return sendDataOrEvent(data, dataLen, false, isLastPacket);
}

bool MTPTransporterUSB::sendEvent(const quint8* data, quint32 dataLen, bool isLastPacket)
{
    return sendDataOrEvent(data, dataLen, true, isLastPacket);
}

void MTPTransporterUSB::handleRead()
{
    if( EXCEPTION != m_ioState )
    {
        char* inbuf = new char[MAX_DATA_IN_SIZE];
        int bytesRead = -1;

        bytesRead = read(m_usbFd, inbuf, MAX_DATA_IN_SIZE);
        if(0 < bytesRead)
        {
            processReceivedData((quint8*)inbuf, bytesRead);
        }
        if(bytesRead == -1)
        {
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
    quint8 usbReqHeader[8];
    quint8 code = 0x00;
    int status = -1;

    status = ioctl(m_usbFd, MTP_IOCTL_GET_CONTROL_REQ, &usbReqHeader);
    if(status < 0)
    {
        MTP_LOG_CRITICAL("IOCTL for USB REQ FAILURE");
    }

    code = usbReqHeader[1];

    switch(code)
    {
        // Cancel Transaction.
        case PTP_REQ_CANCEL:
            MTP_LOG_CRITICAL("Received a request for cancel tx");
	    // Not sure what data will be used for.
	    emit cancelTransaction();
	    break;
     
        // Device Status
	case PTP_REQ_GET_DEVICE_STATUS:
            MTP_LOG_CRITICAL("Received a request for getting device status");
            // This shouldn't come here, as we update the driver about the changed state via ioctl
            break;

        // Device Reset Request
	case PTP_REQ_DEVICE_RESET:
            MTP_LOG_CRITICAL("Received a request for device reset");
	    // Not sure what to do here yet.
	    emit deviceReset();
	    break;

	default:
            break;
    }
}

void MTPTransporterUSB::suspend()
{
    emit suspend();
    sendDeviceBusy();
}

void MTPTransporterUSB::resume()
{
    emit resume();
    int status = -1;
    struct ptp_device_status_data data;
    MTPContainer::putl16( &data.wLength,0x0004 );
    MTPContainer::putl16( &data.Code,PTP_RC_OK );

    status = ioctl(m_usbFd, MTP_IOCTL_SET_DEVICE_STATUS, &data);
    if(status < 0)
    {
        MTP_LOG_CRITICAL(" Failed ioctl for sending device status OK \n");
    }
}

void MTPTransporterUSB::sendDeviceOK()
{
    int status = -1;
    m_containerReadLen = 0;

    struct ptp_device_status_data data;
    MTPContainer::putl16( &data.wLength,0x0004 );
    MTPContainer::putl16( &data.Code,PTP_RC_OK );

    if( EXCEPTION == m_ioState )
    {
        MTP_LOG_CRITICAL("IOCTL for BUFFER CLEAR");
        status = ioctl( m_usbFd, MTP_IOCTL_RESET_BUFFERS );
        if(status < 0)
        {
            MTP_LOG_CRITICAL("IOCTL for BUFFER CLEAR FAILURE");
        }
    }

    status = ioctl(m_usbFd, MTP_IOCTL_SET_DEVICE_STATUS, &data);
    if(status < 0)
    {
        MTP_LOG_CRITICAL(" Failed ioctl for sending device status OK \n");
    }

    m_ioState = ACTIVE;
}

void MTPTransporterUSB::sendDeviceBusy()
{
    struct ptp_device_status_data data;
    MTPContainer::putl16( &data.wLength,0x0004 );
    MTPContainer::putl16( &data.Code,PTP_RC_DEVICE_BUSY );

    int status = -1;
    status = ioctl(m_usbFd, MTP_IOCTL_SET_DEVICE_STATUS, &data);
    if(status < 0)
    {
        MTP_LOG_CRITICAL(" Failed ioctl for sending device status BUSY \n");
    }
}

void MTPTransporterUSB::sendDeviceTxCancelled()
{
    struct ptp_device_status_data data;
    MTPContainer::putl16( &data.wLength,0x0004 );
    MTPContainer::putl16( &data.Code,0x201F );

    int status = -1;
    status = ioctl(m_usbFd, MTP_IOCTL_SET_DEVICE_STATUS, &data);
    if(status < 0)
    {
        MTP_LOG_CRITICAL(" Failed ioctl for sending device status TX CANCEL \n");
    }
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
    if((ACTIVE == m_ioState) && (-1 != m_usbFd))
    {
        m_ioState = EXCEPTION;
        handleHighPriorityData();
    }
}

int MTPTransporterUSB::openUsbFd(const char* name)
{
    int fd;

    fd = open( name, O_RDWR );
    if(fd < 0)
    {
        MTP_LOG_CRITICAL("Error while opening USB FD::" << strerror(errno));
        return -1;
    }
    return fd;
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
        return r;
    }

    if(!isEvent && isLastPacket)
    {
        if( 0 > fsync(m_usbFd) )
        {
            r = false;
        }
    }
    return r;
}

bool MTPTransporterUSB::sendDataInternal(const quint8* data, quint32 len)
{
    int bytesWritten = 0;
    char *dataptr = (char*)data;

    do
    {
        bytesWritten = write(m_usbFd, dataptr, len);
        if(bytesWritten == -1)
        {
            return false;
        }
        dataptr += bytesWritten;
        len -= bytesWritten;
    }while(len);

    return true;
}

bool MTPTransporterUSB::sendEventInternal(const quint8* data, quint32 /*len*/)
{
    int status = -1;
    // FIXME: len is unused.
    status = ioctl(m_usbFd, MTP_IOCTL_WRITE_ON_INTERRUPT_EP, data);
    if(status < 0)
    {
        MTP_LOG_CRITICAL(" Failed ioctl for sending MTP event \n");
        return false;
    }
    return true;
}

quint32 MTPTransporterUSB::getMaxDataPacketSize()
{
    int maxPacketSize = -1;
    if(ioctl(m_usbFd, MTP_IOCTL_GET_MAX_DATAPKT_SIZE, &maxPacketSize) < 0)
    {
        MTP_LOG_WARNING("Failed packet size ioctl");
    }
    
    if(maxPacketSize < 0)
    {
        maxPacketSize = DEFAULT_MAX_PACKET_SIZE;
    }

    return maxPacketSize;
}

quint32 MTPTransporterUSB::getMaxEventPacketSize()
{
    int maxPacketSize = -1;
    if(ioctl(m_usbFd, MTP_IOCTL_GET_MAX_EVENTPKT_SIZE, &maxPacketSize) < 0)
    {
        MTP_LOG_WARNING("Failed packet size ioctl");
    }
    
    if(maxPacketSize < 0)
    {
        maxPacketSize = DEFAULT_MAX_PACKET_SIZE;
    }

    return maxPacketSize;
}

