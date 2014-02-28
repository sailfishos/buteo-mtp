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

#include "mtpevent.h"
#include "mtptypes.h"
#include "mtpresponder.h"
#include "mtptxcontainer.h"
#include "trace.h"

using namespace meegomtp1dot0;

//Constructors
MTPEvent::MTPEvent():m_eventCode(MTP_EV_Undefined), m_sessionID(MTP_NO_SESSION_ID), m_transactionID(MTP_NO_TRANSACTION_ID)
{
}

MTPEvent::MTPEvent(quint16 evCode, quint32 sessID, quint32 transID, QVector<quint32> params):
  m_eventCode(evCode), m_sessionID(sessID), m_transactionID(transID),
  m_eventParams(params)
{}

//Destructor
MTPEvent::~MTPEvent()
{}

void MTPEvent::dispatchEvent()
{
    //Make a container and send the event.
    // FIXME: How do we determine the transport type here? Although at this point an instance of MTPResponder must already be available!
    MTPTxContainer eventContainer(MTP_CONTAINER_TYPE_EVENT, m_eventCode, m_transactionID, m_eventParams.size() * sizeof(quint32));
    for(int i = 0; i < m_eventParams.size(); i++)
    {
        eventContainer << m_eventParams[i];
    }
    bool sent = MTPResponder::instance()->sendContainer(eventContainer);
    if( !sent )
    {
        MTP_LOG_CRITICAL("Couldn't dispatch event " << m_eventCode);
    }
}
