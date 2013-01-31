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

#ifndef DEVICEINFOPROVIDER_TEST_H
#define DEVICEINFOPROVIDER_TEST_H

#include <QtTest/QtTest>
class QDomDocument;

namespace meegomtp1dot0
{
class DeviceInfoProvider;
}

namespace meegomtp1dot0
{
class DeviceInfoProvider_Test : public QObject
{
    Q_OBJECT
    private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testDeviceInfoProviderDefaultConstruction();
    void testDeviceInfoProviderConstruction();
    /* Context subscriber fails on SB
    void testGetBatteryLevel();
    */
    void testGetSyncPartner();
    void testGetCopyrightInfo();
    void testGetDeviceFriendlyName();
    void testGetStandardVesrion();
    void testGetVendorExtension();
    void testGetMTPVersion();
    void testGetMTPExtension();
    void testGetManufacturer();
    void testGetModel();
    void testGetDeviceVersion();
    void testGetSerialNumber();
    void testGetFunctionalMode();
    void testSetSyncPartner();
    void testSetDeviceFriendlyName();
    void testGetBatteryLevelForm();
    void testGetMTPOperationsSupported();
    void testGetMTPEventsSupported();
    void testGetMTPDevicePropertiesSupported();
    void testGetSupportedFormats();
    void testGetFormatCodeCategory();
    void testGetVideoChannels();
    void testGetAudioChannels();
    void testGetVideoDimesions();
    void testGetImageDimesions();
    void testGetVideoMinFPS();
    void testGetVideoMaxFPS();
    void testGetVideoScanType();
    void testGetVideoSampleRate();
    void testGetAudioSampleRate();
    void testGetVideoMinBitrate();
    void testGetVideoMaxBitrate();
    void testGetAudioMinBitrate();
    void testGetAudioMaxBitrate();
    void testGetVideoAudioMinBitrate();
    void testGetVideoAudioMaxBitrate();
    void testGetVideoMinKFD();
    void testGetVideoMaxKFD();
    void testGetSupportedAudioCodecs();
    void testGetDeviceInfoXmlPath();
    void testGetDeviceIcon();
    private:
    DeviceInfoProvider* m_Provider;
    QDomDocument*       m_xmlDoc;
    QFile*              m_xmlFile;
};
}
#endif

