/*
* This file is part of libmeegomtp package
*
* Copyright (c) 2010 Nokia Corporation. All rights reserved.
* Copyright (c) 2012 - 2022 Jolla Ltd.
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
#include <stdlib.h>
#include <QDomDocument>
#include <QDir>
#include "deviceinfoprovider_test.h"
#include "deviceinfoprovider.h"

using namespace meegomtp1dot0;

void DeviceInfoProvider_Test::initTestCase()
{
    m_Provider = 0;
    m_xmlFile = 0;
    QFile::remove("/tmp/deviceinfo.xml");
    // Copy the XML file over to tmp directory
    QCOMPARE(QFile::copy("/opt/tests/buteo-mtp/data/deviceinfo.xml", "/tmp/deviceinfo.xml"), true);
    m_xmlDoc = new QDomDocument();
    m_xmlFile = new QFile("/tmp/deviceinfo.xml");
    QCOMPARE(m_xmlDoc->setContent(m_xmlFile), true);

    // Original deviceinfo.xml may be changed during the test execution, so
    // backup its current state.
    QFile originalDeviceInfoXml(MtpDeviceInfo::getDeviceInfoXmlPath());
    if (originalDeviceInfoXml.exists()) {
        QCOMPARE(originalDeviceInfoXml.copy("/tmp/deviceinfo.xml.orig"), true);
    }
}

void DeviceInfoProvider_Test::cleanupTestCase()
{
    if (m_Provider) {
        delete m_Provider;
        m_Provider = 0;
    }
    if (m_xmlDoc) {
        delete m_xmlDoc;
        m_xmlDoc = 0;
    }
    if (m_xmlFile) {
        m_xmlFile->close();
        delete m_xmlFile;
        m_xmlFile = 0;
    }

    QFile::remove("/tmp/deviceinfo.xml");

    // Restore deviceinfo.xml to the state it had before the tests were run.
    QFile::remove(MtpDeviceInfo::getDeviceInfoXmlPath());
    QFile originalDeviceInfoXml("/tmp/deviceinfo.xml.orig");
    if (originalDeviceInfoXml.exists()) {
        originalDeviceInfoXml.copy(MtpDeviceInfo::getDeviceInfoXmlPath());
        originalDeviceInfoXml.remove();
    }
}

void DeviceInfoProvider_Test::testDeviceInfoProviderDefaultConstruction()
{
    MtpDeviceInfo::setDeviceInfoXmlPath("/tmp/m_deviceinfo.xml");
    m_Provider = new DeviceInfoProvider();
    QCOMPARE(m_Provider->syncPartner(), QString("Nemo"));
    QCOMPARE(m_Provider->copyrightInfo(), QString("Do Not Copy"));
    QVERIFY(m_Provider->MTPOperationsSupported().size() != 0);
    QVERIFY(m_Provider->MTPEventsSupported().size() != 0);
    QVERIFY(m_Provider->MTPDevicePropertiesSupported().size() != 0);
    delete m_Provider;
    m_Provider = 0;
}

void DeviceInfoProvider_Test::testDeviceInfoProviderConstruction()
{
    MtpDeviceInfo::setDeviceInfoXmlPath("/tmp/deviceinfo.xml");
    m_Provider = new DeviceInfoProvider();
    // Test some properties to verify that the construction succeeded
    QVERIFY(m_Provider->syncPartner().size() != 0);
    QCOMPARE(m_Provider->copyrightInfo(), QString("Do Not Copy"));
    QVERIFY(m_Provider->MTPOperationsSupported().size() != 0);
    QVERIFY(m_Provider->MTPEventsSupported().size() != 0);
    QVERIFY(m_Provider->MTPDevicePropertiesSupported().size() != 0);
}

void DeviceInfoProvider_Test::testGetSyncPartner()
{
    QVERIFY(m_Provider->syncPartner().size() != 0);
}

void DeviceInfoProvider_Test::testGetCopyrightInfo()
{
    QCOMPARE(m_Provider->copyrightInfo(), QString("Do Not Copy"));
}

void DeviceInfoProvider_Test::testGetDeviceFriendlyName()
{
    if (!m_Provider->deviceFriendlyName().length()) {
        QFAIL("Empty friendly name");
    }
}

void DeviceInfoProvider_Test::testGetStandardVesrion()
{
    QCOMPARE(
        (quint32) m_Provider->standardVersion(),
        m_xmlDoc->elementsByTagName("StdVersion").at(0).firstChild().nodeValue().toUInt());
}

void DeviceInfoProvider_Test::testGetVendorExtension()
{
    QCOMPARE(
        (quint32) m_Provider->vendorExtension(),
        m_xmlDoc->elementsByTagName("MTPVendorExtn").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetMTPVersion()
{
    QCOMPARE(
        (quint32) m_Provider->MTPVersion(),
        m_xmlDoc->elementsByTagName("MTPVersion").at(0).firstChild().nodeValue().toUInt(0, 10));
}

void DeviceInfoProvider_Test::testGetMTPExtension()
{
    QCOMPARE(m_Provider->MTPExtension(), m_xmlDoc->elementsByTagName("MTPExtn").at(0).firstChild().nodeValue());
}

void DeviceInfoProvider_Test::testGetManufacturer()
{
    QVERIFY(!m_Provider->manufacturer().isEmpty());
}

void DeviceInfoProvider_Test::testGetModel()
{
    QVERIFY(!m_Provider->model().isEmpty());
}

void DeviceInfoProvider_Test::testGetDeviceVersion()
{
    if (!m_Provider->deviceVersion().length()) {
        QFAIL("Empty device version!");
    }
}

void DeviceInfoProvider_Test::testGetSerialNumber()
{
    if (!m_Provider->serialNo().length()) {
        QFAIL("Empty serial number!");
    }
}

void DeviceInfoProvider_Test::testGetFunctionalMode()
{
    QCOMPARE(
        (quint32) m_Provider->functionalMode(),
        m_xmlDoc->elementsByTagName("FnMode").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testSetSyncPartner()
{
    m_Provider->setSyncPartner("Windows");
    QCOMPARE(m_Provider->syncPartner(), QString("Windows"));
}

void DeviceInfoProvider_Test::testSetDeviceFriendlyName()
{
    m_Provider->setDeviceFriendlyName("UT_Name");
    if (!m_Provider->deviceFriendlyName().length()) {
        QFAIL("Failed to fetch the device friendly name");
    }
}

void DeviceInfoProvider_Test::testGetBatteryLevelForm()
{
    QVariant var = m_Provider->batteryLevelForm();

    QCOMPARE(var.userType(), qMetaTypeId<MtpRangeForm>());

    MtpRangeForm batteryLevelForm = var.value<MtpRangeForm>();

    QCOMPARE(batteryLevelForm.minValue.toInt(), 0);
    QCOMPARE(batteryLevelForm.maxValue.toInt(), 100);
    QCOMPARE(batteryLevelForm.stepSize.toInt(), 10);
}

void DeviceInfoProvider_Test::testGetMTPOperationsSupported()
{
    QVector<quint16> ops = m_Provider->MTPOperationsSupported();
    QCOMPARE((quint32) ops.size(), (quint32) m_xmlDoc->elementsByTagName("OpCode").length()); // Dirty -- to disregard comments
    for (int i = 0; i < ops.size(); i++) {
        QCOMPARE((quint32) ops[i], m_xmlDoc->elementsByTagName("OpCode").at(i).firstChild().nodeValue().toUInt(0, 16));
    }
}

void DeviceInfoProvider_Test::testGetMTPEventsSupported()
{
    QVector<quint16> evts = m_Provider->MTPEventsSupported();
    QCOMPARE((quint32) evts.size(), (quint32) m_xmlDoc->elementsByTagName("EvCode").length()); // Dirty -- to disregard comments
    for (int i = 0; i < evts.size(); i++) {
        QCOMPARE((quint32) evts[i], m_xmlDoc->elementsByTagName("EvCode").at(i).firstChild().nodeValue().toUInt(0, 16));
    }
}

void DeviceInfoProvider_Test::testGetMTPDevicePropertiesSupported()
{
    QVector<quint16> devp = m_Provider->MTPDevicePropertiesSupported();
    QCOMPARE(
        (quint32) devp.size(),
        (quint32) m_xmlDoc->elementsByTagName("DevPropCode").length()); // Dirty -- to disregard comments
    for (int i = 0; i < devp.size(); i++) {
        QCOMPARE(
            (quint32) devp[i], m_xmlDoc->elementsByTagName("DevPropCode").at(i).firstChild().nodeValue().toUInt(0, 16));
    }
}

void DeviceInfoProvider_Test::testGetSupportedFormats()
{
    QVector<quint16> devp = m_Provider->supportedFormats();

    for (int i = 0; i < m_xmlDoc->elementsByTagName("ImageFormat").length(); i++) {
        quint16 imageFormat(m_xmlDoc->elementsByTagName("ImageFormat").at(i).firstChild().nodeValue().toUInt(0, 16));
        QVERIFY(devp.contains(imageFormat));
    }

    for (int i = 0; i < m_xmlDoc->elementsByTagName("AudioFormat").length(); i++) {
        quint16 audioFormat(m_xmlDoc->elementsByTagName("AudioFormat").at(i).firstChild().nodeValue().toUInt(0, 16));
        QVERIFY(devp.contains(audioFormat));
    }

    for (int i = 0; i < m_xmlDoc->elementsByTagName("VideoFormat").length(); i++) {
        quint16 videoFormat(m_xmlDoc->elementsByTagName("VideoFormat").at(i).firstChild().nodeValue().toUInt(0, 16));
        QVERIFY(devp.contains(videoFormat));
    }

    for (int i = 0; i < m_xmlDoc->elementsByTagName("CommonFormat").length(); i++) {
        quint16 commonFormat(m_xmlDoc->elementsByTagName("CommonFormat").at(i).firstChild().nodeValue().toUInt(0, 16));
        QVERIFY(devp.contains(commonFormat));
    }
}

void DeviceInfoProvider_Test::testGetFormatCodeCategory()
{
    for (int i = 0; i < m_xmlDoc->elementsByTagName("CommonFormat").length(); i++) {
        QCOMPARE(
            (quint32) m_Provider->getFormatCodeCategory(
                m_xmlDoc->elementsByTagName("CommonFormat").at(i).firstChild().nodeValue().toUInt(0, 16)),
            (quint32) MTP_COMMON_FORMAT);
    }

    for (int i = 0; i < m_xmlDoc->elementsByTagName("ImageFormat").length(); i++) {
        QCOMPARE(
            (quint32) m_Provider->getFormatCodeCategory(
                m_xmlDoc->elementsByTagName("ImageFormat").at(i).firstChild().nodeValue().toUInt(0, 16)),
            (quint32) MTP_IMAGE_FORMAT);
    }

    for (int i = 0; i < m_xmlDoc->elementsByTagName("AudioFormat").length(); i++) {
        QCOMPARE(
            (quint32) m_Provider->getFormatCodeCategory(
                m_xmlDoc->elementsByTagName("AudioFormat").at(i).firstChild().nodeValue().toUInt(0, 16)),
            (quint32) MTP_AUDIO_FORMAT);
    }

    for (int i = 0; i < m_xmlDoc->elementsByTagName("VideoFormat").length(); i++) {
        QCOMPARE(
            (quint32) m_Provider->getFormatCodeCategory(
                m_xmlDoc->elementsByTagName("VideoFormat").at(i).firstChild().nodeValue().toUInt(0, 16)),
            (quint32) MTP_VIDEO_FORMAT);
    }
}

void DeviceInfoProvider_Test::testGetVideoChannels()
{
    const QVector<quint16> devp = m_Provider->videoChannels();
    QCOMPARE(
        (quint32) devp.size(),
        (quint32) m_xmlDoc->elementsByTagName("VideoChannel").length()); // Dirty -- to disregard comments
    for (int i = 0; i < devp.size(); i++) {
        QCOMPARE(
            (quint32) devp[i], m_xmlDoc->elementsByTagName("VideoChannel").at(i).firstChild().nodeValue().toUInt(0, 16));
    }
}

void DeviceInfoProvider_Test::testGetAudioChannels()
{
    const QVector<quint16> devp = m_Provider->audioChannels();
    QCOMPARE(
        (quint32) devp.size(),
        (quint32) m_xmlDoc->elementsByTagName("AudioChannel").length()); // Dirty -- to disregard comments
    for (int i = 0; i < devp.size(); i++) {
        QCOMPARE(
            (quint32) devp[i], m_xmlDoc->elementsByTagName("AudioChannel").at(i).firstChild().nodeValue().toUInt(0, 16));
    }
}

void DeviceInfoProvider_Test::testGetVideoDimesions()
{
    QCOMPARE(m_Provider->videoMinWidth(),
             m_xmlDoc->elementsByTagName("VideoMinWidth").at(0).firstChild().nodeValue().toUInt(0, 10));
    QCOMPARE(m_Provider->videoMaxWidth(),
             m_xmlDoc->elementsByTagName("VideoMaxWidth").at(0).firstChild().nodeValue().toUInt(0, 10));
    QCOMPARE(m_Provider->videoMinHeight(),
             m_xmlDoc->elementsByTagName("VideoMinHeight").at(0).firstChild().nodeValue().toUInt(0, 10));
    QCOMPARE(m_Provider->videoMaxHeight(),
             m_xmlDoc->elementsByTagName("VideoMaxHeight").at(0).firstChild().nodeValue().toUInt(0, 10));
}

void DeviceInfoProvider_Test::testGetImageDimesions()
{
    QCOMPARE(m_Provider->imageMinWidth(),
             m_xmlDoc->elementsByTagName("ImageMinWidth").at(0).firstChild().nodeValue().toUInt(0, 10));
    QCOMPARE(m_Provider->imageMaxWidth(),
             m_xmlDoc->elementsByTagName("ImageMaxWidth").at(0).firstChild().nodeValue().toUInt(0, 10));
    QCOMPARE(m_Provider->imageMinHeight(),
             m_xmlDoc->elementsByTagName("ImageMinHeight").at(0).firstChild().nodeValue().toUInt(0, 10));
    QCOMPARE(m_Provider->imageMaxHeight(),
             m_xmlDoc->elementsByTagName("ImageMaxHeight").at(0).firstChild().nodeValue().toUInt(0, 10));
}

void DeviceInfoProvider_Test::testGetVideoMinFPS()
{
    QCOMPARE(
        m_Provider->videoMinFPS(),
        m_xmlDoc->elementsByTagName("VideoMinFPS").at(0).firstChild().nodeValue().toUInt(0, 10));
}

void DeviceInfoProvider_Test::testGetVideoMaxFPS()
{
    QCOMPARE(
        m_Provider->videoMaxFPS(),
        m_xmlDoc->elementsByTagName("VideoMaxFPS").at(0).firstChild().nodeValue().toUInt(0, 10));
}

void DeviceInfoProvider_Test::testGetVideoScanType()
{
    QCOMPARE(
        (quint32) m_Provider->videoScanType(),
        m_xmlDoc->elementsByTagName("VideoScanType").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetVideoSampleRate()
{
    QCOMPARE(
        m_Provider->videoSampleRate(),
        m_xmlDoc->elementsByTagName("VideoSampleRate").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetAudioSampleRate()
{
    QCOMPARE(
        m_Provider->audioSampleRate(),
        m_xmlDoc->elementsByTagName("AudioSampleRate").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetVideoMinBitrate()
{
    QCOMPARE(
        m_Provider->videoMinBitRate(),
        m_xmlDoc->elementsByTagName("VideoMinBitRate").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetVideoMaxBitrate()
{
    QCOMPARE(
        m_Provider->videoMaxBitRate(),
        m_xmlDoc->elementsByTagName("VideoMaxBitRate").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetAudioMinBitrate()
{
    QCOMPARE(
        m_Provider->audioMinBitRate(),
        m_xmlDoc->elementsByTagName("AudioMinBitRate").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetAudioMaxBitrate()
{
    QCOMPARE(
        m_Provider->audioMaxBitRate(),
        m_xmlDoc->elementsByTagName("AudioMaxBitRate").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetVideoAudioMinBitrate()
{
    QCOMPARE(
        m_Provider->videoAudioMinBitRate(),
        m_xmlDoc->elementsByTagName("VideoAudioMinBitRate").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetVideoAudioMaxBitrate()
{
    QCOMPARE(
        m_Provider->videoAudioMaxBitRate(),
        m_xmlDoc->elementsByTagName("VideoAudioMaxBitRate").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetVideoMinKFD()
{
    QCOMPARE(
        m_Provider->videoMinKFD(),
        m_xmlDoc->elementsByTagName("VideoMinKeyFrameDist").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetVideoMaxKFD()
{
    QCOMPARE(
        m_Provider->videoMaxKFD(),
        m_xmlDoc->elementsByTagName("VideoMaxKeyFrameDist").at(0).firstChild().nodeValue().toUInt(0, 16));
}

void DeviceInfoProvider_Test::testGetSupportedAudioCodecs()
{
    const QVector<quint32> devp = m_Provider->supportedAudioCodecs();
    QCOMPARE((quint32) devp.size(), (quint32) m_xmlDoc->elementsByTagName("Codec").length()); // Dirty -- to disregard comments
    for (int i = 0; i < devp.size(); i++) {
        QCOMPARE((quint32) devp[i], m_xmlDoc->elementsByTagName("Codec").at(i).firstChild().nodeValue().toUInt(0, 16));
    }
}

void DeviceInfoProvider_Test::testGetDeviceInfoXmlPath()
{
    MtpDeviceInfo::setDeviceInfoXmlPath("");
    QString path = MtpDeviceInfo::getDeviceInfoXmlPath();
    QVERIFY(!path.isEmpty());
    QVERIFY(QDir(QDir::homePath()).exists());
}

void DeviceInfoProvider_Test::testGetDeviceIcon()
{
    int s = m_Provider->deviceIcon().size();
    if (s <= 0) {
        QFAIL("Empty Device Icon");
    }
}

QTEST_APPLESS_MAIN(DeviceInfoProvider_Test);
