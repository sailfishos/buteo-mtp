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

#ifndef DEVINFOPROVIDER_H
#define DEVINFOPROVIDER_H

#include "deviceinfo.h"

class BatteryStatus;

/// This class implements DeviceInfo for getting and setting device info and properties for an MTP session.

/// This class uses services like context subscriber to get values for device
/// properties ( the ones mentioned in MTP 1.0 spec ) dynamically. This extends DeviceInfo class,
/// for properties that we don't get values from any system service, the default implementation falls back
/// to DeviceInfo.

namespace meegomtp1dot0 {
class DeviceInfoProvider : public DeviceInfo
{
    Q_OBJECT
#ifdef UT_ON
    friend class DeviceInfoProvider_Test;
#endif
public:
    /// Constructor.
    DeviceInfoProvider();

    /// Destructor.
    ~DeviceInfoProvider();

private:
    BatteryStatus *m_batteryStatus;

private slots:
    void onBatteryPercentageChanged(int percentage);
};
}

#endif //#ifndef DEVINFOPROVIDER_H
