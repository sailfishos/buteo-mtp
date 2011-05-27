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
#include "mtpfsdriver.h"

using namespace meegomtp1dot0;

#define MAX_DATA_IN_SIZE 64 * 1024
#define DEFAULT_MAX_PACKET_SIZE 64

static int inFd = -1, outFd = -1, intrFd = -1, ctrlFd = -1;

MTPTransporterUSB::MTPTransporterUSB() : /*MTP_DEVICE_PORT("/dev/mtp0"), */m_ioState(ACTIVE)/*, m_usbFd(-1)*/, m_containerReadLen(0), m_driver(MTP1, VERBOSE_ON)
{
    QObject::connect(&m_driver, SIGNAL(inFdChanged(int)), this, SLOT(handleInFd(int)));
    QObject::connect(&m_driver, SIGNAL(outFdChanged(int)), this, SLOT(handleOutFd(int)));
    QObject::connect(&m_driver, SIGNAL(intrFdChanged(int)), this, SLOT(handleIntrFd(int)));

    QObject::connect(&m_driver, SIGNAL(dataRead(char*,int)),
        this, SLOT(handleDataRead(char*,int)), Qt::UniqueConnection);
}

bool MTPTransporterUSB::activate()
{
    MTP_LOG_CRITICAL("MTPTransporterUSB::activate");
    MTP_LOG_CRITICAL("activate");

    return true;
}

bool MTPTransporterUSB::deactivate()
{
    //TODO
    MTP_LOG_CRITICAL("deactivate");
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
    m_driver.mtpfs_close();
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
    qDebug() << "**** processedData";
}

void MTPTransporterUSB::handleRead()
{
    if( EXCEPTION != m_ioState )
    {
        //char* inbuf = new char[MAX_DATA_IN_SIZE];
        char* inbuf = new char[12];
        int bytesRead = -1;

        LOG_DEBUG("read req");
        //bytesRead = read(outFd, inbuf, MAX_DATA_IN_SIZE);
        bytesRead = read(outFd, inbuf, 12);
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

void MTPTransporterUSB::handleInFd(int fd)
{
    inFd = fd;
}

void MTPTransporterUSB::handleOutFd(int fd)
{
#if 0
    if(outFd == -1 && fd != -1) {
        // Create socket notifiers over the USB FD
        m_readSocket = new QSocketNotifier(fd, QSocketNotifier::Read);
        m_excpSocket = new QSocketNotifier(fd, QSocketNotifier::Exception);

        // Connect our slots to listen to data and exception events on the USB FD
        QObject::connect(m_readSocket, SIGNAL(activated(int)), this, SLOT(handleRead()));
        QObject::connect(m_excpSocket, SIGNAL(activated(int)), this, SLOT(handleHangup()));
    }
#endif
    outFd = fd;
}

void MTPTransporterUSB::handleIntrFd(int fd)
{
    intrFd = fd;
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
        qDebug() << "Processed: " << chunkLen << " leftover: " <<  dataLen;
    }
}

void MTPTransporterUSB::handleHangup()
{
}

int MTPTransporterUSB::openUsbFd(const char* name)
{
    int fd = -1;

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
        perror("MTPTransporterUSB::sendDataOrEvent");
        return r;
    }

    return true;
}

bool MTPTransporterUSB::sendDataInternal(const quint8* data, quint32 len)
{
    int bytesWritten = 0;
    char *dataptr = (char*)data;

    qDebug() << "Writing: " << len;

    for( int i = 0; i < len; i++ ) {
        printf("%.2x ", data[i]);
    }
    printf("\n");

    do
    {
        bytesWritten = write(inFd, dataptr, len);
        qDebug() << "Wrote bytes: " << bytesWritten;
        if(bytesWritten == -1)
        {
            return false;
        }
        dataptr += bytesWritten;
        len -= bytesWritten;
    } while(len);
    qDebug() << "Wrote: " << (quint32)(dataptr - (char*)data);

    return true;
}

bool MTPTransporterUSB::sendEventInternal(const quint8* data, quint32 len)
{
    int bytesWritten = 0;
    char *dataptr = (char*)data;

    do
    {
        bytesWritten = write(intrFd, dataptr, len);
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

quint32 MTPTransporterUSB::getMaxDataPacketSize()
{
    int maxPacketSize = -1;

    return maxPacketSize;
}

quint32 MTPTransporterUSB::getMaxEventPacketSize()
{
    int maxPacketSize = -1;

    return maxPacketSize;
}

