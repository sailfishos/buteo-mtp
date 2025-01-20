/*
* This file is part of libmeegomtp package
*
* Copyright (C) 2010 Nokia Corporation. All rights reserved.
*
* Contact: Deepak Kodihalli <deepak.kodihalli@nokia.com>
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

#include "mtptransporterdummy.h"
#include "mtpcontainerwrapper.h"

using namespace meegomtp1dot0;

MTPTransporterDummy::MTPTransporterDummy()
    : m_currentTransactionPhase(eMTP_CONTAINER_TYPE_UNDEFINED)
    , m_isNextChunkData(false)
    , m_noOfDataChunksExpected(0)
    , m_noOfDataChunksReceived(0)
    , m_noOfDataChunksToFollow(0)
    , m_transactionId(0xFFFFFFFF)
{}

MTPTransporterDummy::~MTPTransporterDummy()
{
}

bool MTPTransporterDummy::sendData(const quint8 *data, quint32 len, bool /*sendZeroPacket*/)
{
    MTPContainerWrapper mtpHeader(const_cast<quint8 *>(data));
    // Determine the phase
    if (!m_isNextChunkData) {
        m_currentTransactionPhase = static_cast<transactionPhase>(mtpHeader.containerType());
    }

    // If this is a response, we emit a signal dataReceived, this will be used by protocol's UT class to validate response.
    if (eMTP_CONTAINER_TYPE_RESPONSE == m_currentTransactionPhase) {
        emit dataReceived(const_cast<quint8 *>(data), len, true, true);
    }

    if (eMTP_CONTAINER_TYPE_DATA == m_currentTransactionPhase || m_isNextChunkData) {
        return checkData(data, len);
    } else {
        return checkHeader(&mtpHeader, len);
    }
}

bool MTPTransporterDummy::sendEvent(const quint8 * /*data*/, quint32 /*len*/, bool /*sendZeroPacket*/)
{
    return true;
}

bool MTPTransporterDummy::checkHeader(MTPContainerWrapper *mtpHeader, quint32 len)
{
    //Check length
    quint32 packetLength = mtpHeader->containerLength();
    if (packetLength != len) {
        return false;
    }

    //Check transaction id
    quint32 transactionId = mtpHeader->transactionId();
    if (0xFFFFFFFF != m_transactionId && transactionId < m_transactionId) {
        return false;
    }
    m_transactionId = transactionId;

    return true;
}

bool MTPTransporterDummy::checkData(const quint8 *data, quint32 len)
{
    // The first packet for data which also has header
    if (eMTP_CONTAINER_TYPE_DATA == m_currentTransactionPhase && !m_isNextChunkData) {
        MTPContainerWrapper mtpHeader(const_cast<quint8 *>(data));
        // Determine total data length
        quint32 dataLength = mtpHeader.containerLength() - MTP_HEADER_SIZE;
        // Check how many bytes of data are present in the current packet
        quint32 currLength = len - MTP_HEADER_SIZE;
        // Determine how many chunks will follow; data may be segmented
        m_noOfDataChunksToFollow = (dataLength / currLength + dataLength % currLength ? 1 : 0) - 1;
        m_noOfDataChunksExpected = m_noOfDataChunksToFollow;
        m_isNextChunkData = m_noOfDataChunksExpected ? true : false;
        return true;
    } else if (m_isNextChunkData) {
        if (m_noOfDataChunksToFollow) {
            --m_noOfDataChunksToFollow;
            ++m_noOfDataChunksReceived;
        }
        if (!m_noOfDataChunksToFollow) {
            m_isNextChunkData = false;
            if (m_noOfDataChunksReceived != m_noOfDataChunksExpected) {
                return false;
            }
        }
        return true;
    }
    return false;
}
