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

#ifndef MTPTRANSPORTER_DUMMY_H
#define MTPTRANSPORTER_DUMMY_H

#include "mtptransporter.h"
#include "mtptypes.h"

namespace meegomtp1dot0
{
class MTPContainerWrapper;
}

/// This class implements interfacing with a dummy inititator : it does checks against
/// malformed mtp packets, and reports failure if the packet is malformed.
namespace meegomtp1dot0
{
class MTPTransporterDummy : public MTPTransporter
{
    Q_OBJECT
public:
    /// Constructor.
    MTPTransporterDummy();

    /// Destructor.
    ~MTPTransporterDummy();

    /// Checks data, if data is good, returns true
    bool sendData( const quint8* data, quint32 len, bool sendZeroPacket = true );

    /// Checks if event data is good, if so returns true
    bool sendEvent( const quint8* data, quint32 len, bool sendZeroPacket = true ) ;

    bool activate(){ return true; }
    bool deactivate(){ return true; }
    bool flushData(){ return true; }
    void reset(){}
    void disableRW(){}
    void enableRW(){}

private:
    /// Checks if the mtp header received in sendData/Event is ok.
    bool checkHeader( MTPContainerWrapper *mtpHeader, quint32 len );

    /// Checks if data phases are ok.
    bool checkData( const quint8* data, quint32 len );

    enum transactionPhase
    {
        eMTP_CONTAINER_TYPE_UNDEFINED,
        eMTP_CONTAINER_TYPE_COMMAND,
        eMTP_CONTAINER_TYPE_DATA,
        eMTP_CONTAINER_TYPE_RESPONSE,
        eMTP_CONTAINER_TYPE_EVENT
    };
    transactionPhase m_currentTransactionPhase; ///< The MTP phase we are currently in ( when sendData/Event is called ).
    bool m_isNextChunkData; ///< When set to true, we expect raw data in sendEvent.
    quint32 m_noOfDataChunksExpected; ///< The no. of data chunks that we expect to receive when a data phase begins. 
    quint32 m_noOfDataChunksReceived; ///< The no. of data chunks that are finally received.
    quint32 m_noOfDataChunksToFollow; ///< The no. of data chunks that are yet to be received.
    quint32 m_transactionId; ///< The transaction id of the current MTP transaction ( read from the mtp packet revecied in sendData ).

Q_SIGNALS:
    void dummyDataReceived( quint8* data, quint32 len );

public Q_SLOTS:
    void sendDeviceOK(){}
    void sendDeviceBusy(){}
    void handleHighPriorityData(){}
};
}

#endif
