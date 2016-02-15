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
#include <QDeviceInfo>
#include <ssudeviceinfo.h>
#include <contextproperty.h>

using namespace meegomtp1dot0;

/**********************************************
 * DeviceInfoProvider::DeviceInfoProvider
 *********************************************/
DeviceInfoProvider::DeviceInfoProvider():
  battery(new ContextProperty("Battery.ChargePercentage", this))
{
    QDeviceInfo di;

    m_serialNo = SsuDeviceInfo().deviceUid();

    m_deviceVersion = QString("%1 HW: %2").arg(di.version(QDeviceInfo::Os))
                                          .arg(di.version(QDeviceInfo::Firmware));

    m_manufacturer = di.manufacturer().isEmpty() ? m_manufacturer : di.manufacturer();
    m_model = di.model().isEmpty() ? m_model : di.model();

    connect(battery, SIGNAL(valueChanged()), this, SLOT(onBatteryPercentageChanged()));
    // Initialize the battery charge value.
    onBatteryPercentageChanged();
}

/**********************************************
 * DeviceInfoProvider::~DeviceInfoProvider
 *********************************************/
DeviceInfoProvider::~DeviceInfoProvider()
{
}

void DeviceInfoProvider::onBatteryPercentageChanged()
{
    setBatteryLevel(battery->value().toUInt());
}
