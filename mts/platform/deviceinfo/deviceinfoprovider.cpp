/*
* This file is part of libmeegomtp package
*
* Copyright (c) 2010 Nokia Corporation. All rights reserved.
* Copyright (c) 2013 - 2022 Jolla Ltd.
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
#include <QTimer>
#include <ssudeviceinfo.h>
#include <batterystatus.h>
#include <deviceinfo.h>
#include "trace.h"

using namespace meegomtp1dot0;

/**********************************************
 * DeviceInfoProvider::DeviceInfoProvider
 *********************************************/
DeviceInfoProvider::DeviceInfoProvider()
    : m_batteryStatus(new BatteryStatus(this))
{
    DeviceInfo deviceInfo;
    SsuDeviceInfo ssuDeviceInfo;

    m_serialNo = ssuDeviceInfo.deviceUid();
    m_deviceVersion = deviceInfo.osVersion();
    m_manufacturer = deviceInfo.manufacturer();
    m_model = deviceInfo.prettyName();

    connect(m_batteryStatus, &BatteryStatus::chargePercentageChanged,
            this, &DeviceInfoProvider::onBatteryPercentageChanged);

    if (m_newConfigFileWasCreated) {
        /* Override the value of the friendly name -property defined in the XML configuration */
        MTP_LOG_INFO("Setting MTP friendly name to:" << m_model);
        setDeviceFriendlyName(m_model);
    }
}

/**********************************************
 * DeviceInfoProvider::~DeviceInfoProvider
 *********************************************/
DeviceInfoProvider::~DeviceInfoProvider()
{
}

void DeviceInfoProvider::onBatteryPercentageChanged(int percentage)
{
    if (percentage >= 0) {
        setBatteryLevel(quint8(qMin(percentage, 255)));
    }
}
