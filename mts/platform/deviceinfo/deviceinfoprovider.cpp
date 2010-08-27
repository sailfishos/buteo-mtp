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

#include <SyncDBusConnection.h>

using namespace meegomtp1dot0;

#define BLUEZ_DEST "org.bluez"
#define BLUEZ_MANAGER_INTERFACE "org.bluez.Manager"
#define BLUEZ_ADAPTER_INTERFACE "org.bluez.Adapter"
#define GET_DEFAULT_ADAPTER "DefaultAdapter"
#define GET_PROPERTIES "GetProperties"
#define SYSINFOD_DEST "com.nokia.SystemInfo"
#define SYSINFOD_INTF "com.nokia.SystemInfo"
#define SYSINFOD_PATH "/com/nokia/SystemInfo"
#define SYSINFOD_GET_KEYS "GetConfigKeys"
#define SYSINFOD_GET_VALUE "GetConfigValue"
#define SYSINFOD_KEY_SWVERSION "/device/sw-release-ver"
#define SYSINFOD_KEY_SERIALNO "/device/production-sn"
#define CSD_DEST "com.nokia.csd.Info"
#define CSD_INTF "com.nokia.csd.Info"
#define CSD_PATH "/com/nokia/csd/info"
#define CSD_GET_IMEI "GetIMEINumber"

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

    // Use sysinfod to get device firmware version and serial number.
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
    // Get the DBUS interface for sysinfod.
    QDBusInterface sysinfoInterface( SYSINFOD_DEST, SYSINFOD_PATH, SYSINFOD_INTF, Buteo::SyncDBusConnection::systemBus() );
    QDBusInterface csdInterface( CSD_DEST, CSD_PATH, CSD_INTF, Buteo::SyncDBusConnection::systemBus() );
    QDBusReply<QByteArray> value;
    QDBusReply<QString> valueIMEI;
    QByteArray propVal;
    if( sysinfoInterface.isValid() )
    {
        // Set the software version obtained from sysinfod, obtained by calling the appropriate method.
        value  = sysinfoInterface.call( QLatin1String(SYSINFOD_GET_VALUE), QLatin1String(SYSINFOD_KEY_SWVERSION) );
        if( value.isValid() )
        {
            propVal = value;
            m_deviceVersion = propVal.constData();
        }
    }


    if( csdInterface.isValid() )
    {
        // Set the serial number obtained from sysinfod, obtained by calling the appropriate method.
        valueIMEI  = csdInterface.call( QLatin1String(CSD_GET_IMEI) );
        if( valueIMEI.isValid() )
        {
            QString imei = valueIMEI;
#if 0
            //Ovi Suite expects the IMEI to match exactly with the device IMEI.
            //Hence, we do not append leading zeroes anymore
            //The serial number MUST be a 32 characters long string, with leading 0's,
            //if required, to make the string 32 characters long.
            for( int i = 0; i < 32 - tmp.size(); i++)
            {
                serialNumber += "0";
            }
            serialNumber += tmp;
#endif
            MTP_LOG_WARNING("*************IMEI Number::**********" << imei);
            m_serialNo = imei;
        }
    }
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

