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

#include <QtGlobal>
// work around breakage in statefs-contextkit
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include "contextproperty.h"
#else
#include "contextsubscriber/contextproperty.h"
#endif
#include "contextsubscriber.h"

using namespace meegomtp1dot0;

/***************************************
 * ContextSubscriber::ContextSubscriber
 **************************************/
ContextSubscriber::ContextSubscriber(QObject* parent) : QObject(parent)
{
    bool ok;
    m_propBatteryLevel = new ContextProperty("Battery.ChargePercentage", this);
    connect(m_propBatteryLevel, SIGNAL(valueChanged()), this, SLOT(onBatteryLevelChanged()));
    m_batteryLevel = static_cast<quint8>( m_propBatteryLevel->value().toString().toUShort(&ok) );
    if( !ok )
    {
        m_batteryLevel = 0;
    }
}

/***************************************
 * ContextSubscriber::~ContextSubscriber
 **************************************/
ContextSubscriber::~ContextSubscriber()
{
    delete m_propBatteryLevel;
}

/***************************************
 * quint8 ContextSubscriber::batteryLevel
 **************************************/
quint8 ContextSubscriber::batteryLevel()
{
    return m_batteryLevel;
}

/***************************************
 * void ContextSubscriber::onBatteryLevelChanged
 **************************************/
void ContextSubscriber::onBatteryLevelChanged()
{
    bool ok;
    quint16 currBatteryLevel = m_propBatteryLevel->value().toString().toUShort(&ok);

    // If conversion failed we keep the battery as it is.
    if( !ok )
    {
        return;
    }

    // We emit the signal only if the battery level changed by 10% or more.
    qint8 diff = static_cast<qint8>(currBatteryLevel) - static_cast<qint8>(m_batteryLevel);
    if( diff < 0 )
    {
        diff = diff * -1;
    }
    if( diff >= 10 )
    {
        emit( batteryLevelChanged( static_cast<quint8>(currBatteryLevel) ) );
    }
    m_batteryLevel = static_cast<quint8>(currBatteryLevel);
}
