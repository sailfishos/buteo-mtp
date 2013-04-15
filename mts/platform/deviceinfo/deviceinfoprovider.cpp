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

#include "deviceinfoprovider.h"
#include "contextsubscriber.h"
#include "trace.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QVariant>
#include <QMap>
#include <QSystemInfo>
#include <QSystemDeviceInfo>

#include <buteosyncfw/SyncDBusConnection.h>

using namespace meegomtp1dot0;
QTM_USE_NAMESPACE

#define BLUEZ_DEST "org.bluez"
#define BLUEZ_MANAGER_INTERFACE "org.bluez.Manager"
#define BLUEZ_ADAPTER_INTERFACE "org.bluez.Adapter"
#define GET_DEFAULT_ADAPTER "DefaultAdapter"
#define GET_PROPERTIES "GetProperties"


/**********************************************
 * DeviceInfoProvider::DeviceInfoProvider
 *********************************************/
DeviceInfoProvider::DeviceInfoProvider()
{
#ifdef UT_ON
    // Temporary: The context subscriber constructor crashes while doing UT.
    m_contextSubscriber = 0;
#else
    // Listen to context subscriber for battery level change notifications.
    m_contextSubscriber = new ContextSubscriber(this);
    QObject::connect(m_contextSubscriber, SIGNAL(batteryLevelChanged(const quint8&)), this, SLOT(batteryLevelChanged(const quint8&)));
#endif

    getSystemInfo();

    // Get the BT adapter interface, this interface can later be used to get the BT name
    // which will serve as the device friendly name.
    getBTAdapterInterface();
}

/**********************************************
 * DeviceInfoProvider::~DeviceInfoProvider
 *********************************************/
DeviceInfoProvider::~DeviceInfoProvider()
{
#ifndef UT_ON
    delete m_contextSubscriber;
#endif
}

/**********************************************
 * void DeviceInfoProvider::getSystemInfo
 *********************************************/
void DeviceInfoProvider::getSystemInfo()
{
    QSystemInfo *si = new QSystemInfo(this);
    QSystemDeviceInfo *di = new QSystemDeviceInfo(this);

    m_deviceVersion = si->version(QSystemInfo::Firmware).isEmpty() ? m_deviceVersion : si->version(QSystemInfo::Firmware);
    m_serialNo = di->imei().isEmpty() ? m_serialNo : di->imei();
    m_manufacturer = di->manufacturer().isEmpty() ? m_manufacturer : di->manufacturer();
    m_model = di->model().isEmpty() ? m_model : di->model();

    delete si;
    delete di;
}

/**********************************************
 * void DeviceInfoProvider::getBTAdapterInterface
 *********************************************/
void DeviceInfoProvider::getBTAdapterInterface()
{
    // Get the DBUS interface for BT manager.
    QDBusInterface managerInterface( BLUEZ_DEST, "/", BLUEZ_MANAGER_INTERFACE, Buteo::SyncDBusConnection::systemBus() );
    if( !managerInterface.isValid() )
    {
        m_defaultAdapterPath = "";
        return;
    }

    QDBusReply<QDBusObjectPath> reply = managerInterface.call( QLatin1String( GET_DEFAULT_ADAPTER ) );
    if( reply.isValid() ) {
        m_defaultAdapterPath = reply.value().path();
    }
    else {
        m_defaultAdapterPath = "";
    }
}

// TODO Emit a devicePropChanged event when the BT name changes while the MTP session is active

/**********************************************
 * QString DeviceInfoProvider::getBTFriendlyName
 *********************************************/
QString DeviceInfoProvider::getBTFriendlyName()
{
    if( "" == m_defaultAdapterPath )
    {
        return "";
    }

    // Get the DBUS interface for BT adapter.
    QDBusInterface adapterInterface( BLUEZ_DEST, m_defaultAdapterPath, BLUEZ_ADAPTER_INTERFACE, Buteo::SyncDBusConnection::systemBus() );

    // Call appropriate method on BT adapter interface to get the BT friendly name.
    QDBusReply<QMap<QString,QVariant> > propReply = adapterInterface.call( QLatin1String( GET_PROPERTIES ) );
    if( propReply.isValid() )
    {
        QMap<QString,QVariant> props = propReply;
        QVariant name = props.value("Name");
        return name.toString();
    }
    return "";
}

/**********************************************
 * quint8 DeviceInfoProvider::batteryLevel
 *********************************************/
quint8 DeviceInfoProvider::batteryLevel( bool /*current*/ ) const
{
    return m_contextSubscriber->batteryLevel();
}

/**********************************************
 * const QString& DeviceInfoProvider::deviceFriendlyName
 *********************************************/
#if 0
const QString& DeviceInfoProvider::deviceFriendlyName( bool /*current*/ )
{
    QString name = getBTFriendlyName();
    if( "" != name )
    {
        m_deviceFriendlyName = name;
    }
    return m_deviceFriendlyName;
}
#endif

/**********************************************
 * void DeviceInfoProvider::batteryLevelChanged
 *********************************************/
void DeviceInfoProvider::batteryLevelChanged( const quint8& /*batteryLevel*/ )
{
    //TODO Send an event
}
