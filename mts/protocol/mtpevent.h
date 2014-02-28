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

#ifndef MTPEVENT_H
#define MTPEVENT_H

#include <QHash>
#include <QString>
#include <QVector>



//MTP Event class - defines the structure of an MTPEvent and provides
//a method for dispatching events from responder.
namespace meegomtp1dot0
{
class MTPEvent
{
public:
    //Constructors.
    MTPEvent();
    MTPEvent(quint16 evCode, quint32 sessID,
             quint32 transID, QVector<quint32> params);
    //Destructor.
    ~MTPEvent();

    //Method used to send the event to protocol layer.
    void dispatchEvent();

    //Getters and setters
    quint16 getEventCode();
    quint32 getTransactionID();
    quint32 getSessionID();
    QVector<quint32> getEventParams();

    void setEventCode(quint16 evCode);
    void setTransactionID(quint32 transID);
    void setSessionID(quint32 sessID);
    void setEventParams(QVector<quint32> params);

private:
    //This event's code.
    quint16 m_eventCode; 
    //The current session's id.
    quint32 m_sessionID;
    //Transaction id, if this event has one.
    quint32 m_transactionID;
    //Event parameters, if this event has any.
    QVector<quint32> m_eventParams;
};
}

using namespace meegomtp1dot0;

//Inline getters and setters
inline quint16 MTPEvent::getEventCode()
{
    return m_eventCode;
}
inline quint32 MTPEvent::getTransactionID()
{
    return m_transactionID;
}
inline quint32 MTPEvent::getSessionID()
{
    return m_sessionID;
}
inline QVector<quint32> MTPEvent::getEventParams()
{
    return m_eventParams;
}

inline void MTPEvent::setEventCode(quint16 evcode)
{
    m_eventCode = evcode;
}
inline void MTPEvent::setTransactionID(quint32 transID)
{
    m_transactionID = transID;
}
inline void MTPEvent::setSessionID(quint32 sessID)
{
    m_sessionID = sessID;
}
inline void MTPEvent::setEventParams(QVector<quint32> params)
{
    m_eventParams = params;
}

#endif
