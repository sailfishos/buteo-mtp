/*
* This file is part of libmeegomtp package
*
* Copyright (c) 2010 Nokia Corporation. All rights reserved.
* Copyright (c) 2016 - 2022 Jolla Ltd.
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

#ifndef MTPDEVICEINFO_H
#define MTPDEVICEINFO_H

#include <QVector>
#include <QObject>

#include "mtptypes.h"

/// This class provides an interface to get and set MTP device properties.

/// This class mainly serves as an interface to get/set device info and properties
/// as specified by the MTP 1.0 spec. This provides default implementation for fetching
/// device proprties. Device specific functionality is implemented by the
/// derived class DeviceInfoProvider.

/// Device properties comprise of the device info dataset, outlined in section 5.1.1 of the MTP
/// 1.0 spec, as well as device specific properties specific to audio video objects for instance,
/// like supported codecs, bit rates, etc. The latter are device capabailities to render a certain
/// type of object. These can be found in appendix B of the spec.Apart from these, there are also
/// MTP specific device properties like battery level for eg, these are outlined in Section 5.1.2 of the spec.

namespace meegomtp1dot0 {
class MtpDeviceInfo : public QObject
{
    Q_OBJECT

#ifdef UT_ON
    friend class DeviceInfoProvider_Test;
#endif
    /// class XMLHandler sets default values for device properties by picking them from an xml file.
    friend class XMLHandler;

public:
    /// Gets MTP FORM (MTP 1.1 specification 5.1.2.1) of battery level property.
    ///
    /// \return a \c QVariant holding either \c MtpRangeForm or \c MtpEnumForm
    /// structure depending on the actual format of the device property.
    virtual QVariant batteryLevelForm() const;

    /// Gets the device's battery charge level.
    ///
    /// \return the battery charge value as specified by getBatteryLevelForm()
    virtual quint8 batteryLevel() const;

    /// Gets the name of sync partner for this device.
    /// \param current [in] boolean which indicates whether to get the current
    /// value(the default behavior) or the default value.
    /// \return the sync partner's name.
    virtual const QString &syncPartner(bool current = true) const;

    /// Gets copyright info for this device.
    /// \param current [in] boolean which indicates whether to get the current
    /// value(the default behavior) or the default value.
    /// \return the copyright info.
    virtual const QString &copyrightInfo(bool current = true) const;

    /// Gets the friendly name of this device.
    /// \param current [in] boolean which indicates whether to get the current
    /// value(the default behavior) or the default value.
    /// \return the device friendly name.
    virtual const QString &deviceFriendlyName(bool current = true);

    /// Gets the device icon for this device.
    /// \return the device icon data as a vector.
    virtual const QVector<quint8> deviceIcon();

    /// Gets the PTP version this device can support.
    /// \return the PTP version supported.
    virtual const quint16 &standardVersion() const;

    /// Gets the MTP vendor extension id.
    /// \return the MTP vendor extension id.
    virtual const quint32 &vendorExtension() const;

    /// Gets the MTP version this device can support.
    /// \return the MTP version supported.
    virtual const quint16 &MTPVersion() const;

    /// Gets the MTP extension sets.
    /// \return the list of strings representing MTP extensions.
    virtual const QString &MTPExtension() const;

    /// Gets the name of the device's manufacturer.
    /// \return the device manufacturer's name.
    virtual const QString &manufacturer() const;

    /// Gets the device's model name.
    /// \return the device model's name.
    virtual const QString &model() const;

    /// Gets the device's firmware version.
    /// \return the device firmware version.
    virtual const QString &deviceVersion() const;

    /// Gets the device's unique serial no.
    /// \return the device serial no.
    virtual const QString &serialNo() const;

    /// Gets the device's type.
    /// \return The device's perceived type.
    virtual quint32 deviceType() const;

    /// Gets the device's functional mode.
    /// \return the functional mode.
    virtual const quint16 &functionalMode() const;

    /// Sets the new sync partner for the device.
    /// \param syncPartner [in] the new sync partner.
    virtual void setSyncPartner(const QString &syncPartner);

    /// Set the new friendly name for the device.
    /// \param deviceFriendlyName [in] the new device friendly name.
    virtual void setDeviceFriendlyName(const QString &deviceFriendlyName);

    /// Gets the list of MTP operations supported by this device.
    /// \param [out] no no. of operations suppported.
    /// \param [out] len size of the array holding the list of operations.
    /// \return pointer to the array of operations.
    const QVector<quint16> &MTPOperationsSupported() const;

    /// Gets the list of MTP events supported by this device.
    /// \param [out] no no. of events suppported.
    /// \param [out] len size of the array holding the list of events.
    /// \return pointer to the array of events.
    const QVector<quint16> &MTPEventsSupported() const;

    /// Gets the list of MTP device properties supported by this device.
    /// \param [out] no no. of device properties suppported.
    /// \param [out] len size of the array holding the list of device properties.
    /// \return pointer to the array of device properties.
    const QVector<quint16> &MTPDevicePropertiesSupported() const;

    /// Gets the list of object formats supported by this device.
    /// \param [out] no no. of object formats suppported.
    /// \param [out] len size of the array holding the list of object formats.
    /// \return pointer to the array of object formats.
    const QVector<quint16> &supportedFormats() const;

    /// Given an object format code, this method returns the category to which
    /// that formats belongs to - common, audio, video, image, undefined.
    /// \param formatCode the object format code.
    /// \return category code.
    quint16 getFormatCodeCategory(quint16 formatCode);

    /// Gets the minimum image width supported by the device
    /// \return The minimum image width
    virtual quint32 imageMinWidth();

    /// Gets the maximum image width supported by the device
    /// \return The maximum image width
    virtual quint32 imageMaxWidth();

    /// Gets the minimum image height supported by the device
    /// \return The minimum image height
    virtual quint32 imageMinHeight();

    /// Gets the maximum image height supported by the device
    /// \return The maximum image height
    virtual quint32 imageMaxHeight();

    /// Gets the minimum video width supported by the device
    /// \return The minimum video width
    virtual quint32 videoMinWidth();

    /// Gets the maximum video width supported by the device
    /// \return The maximum video width
    virtual quint32 videoMaxWidth();

    /// Gets the minimum video height supported by the device
    /// \return The minimum video height
    virtual quint32 videoMinHeight();

    /// Gets the maximum video height supported by the device
    /// \return The maximum video height
    virtual quint32 videoMaxHeight();

    /// Gets the list of supported video channels.
    /// \param [out] no no. of video channels supported.
    /// \return array of supported video channel types.
    virtual const QVector<quint16> &videoChannels() const;

    /// Gets the list of supported audio channels.
    /// \param [out] no no. of audio channels supported.
    /// \return array of supported audio channel types.
    virtual const QVector<quint16> &audioChannels() const;

    /// Gets the minimum possible frames per second for videos.
    /// \return min FPS.
    virtual const quint32 &videoMinFPS() const;

    /// Gets the maximum possible frames per second for videos.
    /// \return max FPS.
    virtual const quint32 &videoMaxFPS() const;

    /// Gets the supported video scan type.
    /// \return the video scan type.
    virtual const quint16 &videoScanType() const;

    /// Gets the default video sample rate.
    /// \return video sample rate.
    virtual const quint32 &videoSampleRate() const;

    /// Gets the default audio sample rate.
    /// \return audio sample rate.
    virtual const quint32 &audioSampleRate() const;

    /// Gets the minimum possible bit rate for videos.
    /// \return min video bit rate.
    virtual const quint32 &videoMinBitRate() const;

    /// Gets the maximum possible bit rate for videos.
    /// \return max video bit rate.
    virtual const quint32 &videoMaxBitRate() const;

    /// Gets the minimum possible bit rate for audio.
    /// \return min audio bit rate.
    virtual const quint32 &audioMinBitRate() const;

    /// Gets the maximum possible bit rate for audio.
    /// \return max audio bit rate.
    virtual const quint32 &audioMaxBitRate() const;

    /// Gets the minimum possible bit rate for audio in video.
    /// \return min audio bit rate contained in video.
    virtual const quint32 &videoAudioMinBitRate() const;

    /// Gets the maximum possible bit rate for audio in video.
    /// \return max audio bit rate contained in video.
    virtual const quint32 &videoAudioMaxBitRate() const;

    /// Gets the minimum key frame distance possible for video.
    /// \return min KFD for video.
    virtual const quint32 &videoMinKFD() const;

    /// Gets the maximum key frame distance possible for video.
    /// \return max KFD for video.
    virtual const quint32 &videoMaxKFD() const;

    /// Gets the list of supported audio codecs
    /// \param [out] no no. of audio codecs supported.
    /// \return array of supported audio codecs.
    const QVector<quint32> &supportedAudioCodecs() const;

    /// Constructor.
    MtpDeviceInfo(QObject *parent = 0);

    /// Destructor.
    virtual ~MtpDeviceInfo();

signals:
    void devicePropertyChanged(MTPDevPropertyCode property, QVariant value) const;

protected:
    void setBatteryLevel(quint8 level);

    QString m_copyrightInfo;      ///< Device copyright info.
    QString m_syncPartner;        ///< This device's sync partner.
    QString m_deviceFriendlyName; ///< The device's friendly name.
    QString m_deviceIconPath;     ///< The device's icon path.
    quint16 m_standardVersion;    ///< The PTP version supported.
    quint32 m_vendorExtension;    ///< MTP vendor extension id.
    quint16 m_mtpVersion;         ///< The MTP version supported.
    QString m_mtpExtension;       ///< MTP vendor extension sets.
    quint16 m_functionalMode;     ///< The device's functional mode.
    QString m_manufacturer;       ///< The devie manufacturer.
    QString m_model;              ///< The device model.
    QString m_serialNo;           ///< The device's unique serial no.
    QString m_deviceVersion;      ///< The device's firmware version.
    quint32 m_deviceType;         ///< The perceived device type.

    quint32 m_imageMinWidth;          ///< Minimum image width
    quint32 m_imageMaxWidth;          ///< Minimum image width
    quint32 m_imageMinHeight;         ///< Minimum image width
    quint32 m_imageMaxHeight;         ///< Minimum image width
    quint32 m_videoMinWidth;          ///< Minimum image width
    quint32 m_videoMaxWidth;          ///< Minimum image width
    quint32 m_videoMinHeight;         ///< Minimum image width
    quint32 m_videoMaxHeight;         ///< Minimum image width
    QVector<quint16> m_videoChannels; ///< supported video channels.
    quint32 m_videoMinFPS;            ///< minimum video frames per second.
    quint32 m_videoMaxFPS;            ///< maximum video frames per second.
    quint16 m_videoScanType;          ///< default video scan type.
    quint32 m_videoSampleRate;        ///< default video sample rate.
    quint32 m_videoMinBitRate;        ///< min video bit rate.
    quint32 m_videoMaxBitRate;        ///< max video bit rate.
    quint32 m_audioMinBitRate;        ///< min audio bit rate.
    quint32 m_audioMaxBitRate;        ///< max audio bit rate.
    quint32 m_videoAudioMinBitRate;   ///< min audio bit rate contained in video.
    quint32 m_videoAudioMaxBitRate;   ///< max audio bit rate contained in video.
    quint32 m_videoMinKFD;            ///< min video key frame distance.
    quint32 m_videoMaxKFD;            ///< max video key frame distance.
    QVector<quint16> m_audioChannels; ///< supported audio channels.
    quint32 m_audioSampleRate;        ///< default audio sample rate.

    /// MTP form flags
    enum m_formFlag {
        FORM_NONE,
        FORM_RANGE,
        FORM_ENUM,
        FORM_ARRAY,
        FORM_BYTEARRAY,
        FORM_DATETIME,
        FORM_STRING
    };

    bool m_newConfigFileWasCreated; ///< Whether a new copy of global configuration file was taken in use

private:
    static QString m_devinceInfoXmlPath;             ///< The xml file that stores default values for device properties.
    QVector<quint16> m_mtpOperationsSupported;       ///< supported MTP operations.
    QVector<quint16> m_mtpEventsSupported;           ///< supported MTP events.
    QVector<quint16> m_mtpDevicePropertiesSupported; ///< supported MTP device properties.
    QVector<quint32> m_supportedCodecs;              ///< list of supported audio codecs.
    QVector<quint16> m_commonFormats;                ///< supported common formats.
    QVector<quint16> m_imageFormats;                 ///< supported image formats.
    QVector<quint16> m_audioFormats;                 ///< supported audio formats.
    QVector<quint16> m_videoFormats;                 ///< supported video formats.
    QVector<quint16> m_supportedFormats;             ///< supported object formats.

    quint8 m_batteryLevel;

    bool m_xmlOk; ///< indicates the device info xml file was parsed succesfully.

    ///< default values for supported audio codecs.
    enum {
        MTP_WAVE_FORMAT_ALAW          = 0x00000006,
        MTP_WAVE_FORMAT_MULAW         = 0x00000007,
        MTP_WAVE_FORMAT_MPEGLAYER3    = 0x00000055,
        MTP_WAVE_FORMAT_MSAUDIO1      = 0x00000160,
        MTP_WAVE_FORMAT_MSAUDIO2      = 0x00000161,
        MTP_WAVE_FORMAT_MSAUDIO3      = 0x00000162,
        MTP_WAVE_FORMAT_NOKIA_AMR     = 0x00004201,
        MTP_WAVE_FORMAT_AAC           = 0x0000A106,
        MTP_WAVE_FORMAT_DSPGROUP_TRUESPEECH = 0x00000022
    };

    ///< default values for supported MTP operations.
    static quint16 m_operationsSupportedTable[];
    ///< default values for supported audio channels.
    static quint16 m_audChannelTable[];
    ///< default values for supported video channels.
    static quint16 m_vidChannelTable[];
    ///< default values for supported audio codecs.
    static quint32 m_supportedCodecsTable[];
    ///< default values for supported common formats.
    static quint16 m_commonFormatsTable[];
    ///< default values for supported image formats.
    static quint16 m_imageFormatsTable[];
    ///< default values for supported audio formats.
    static quint16 m_audioFormatsTable[];
    ///< default values for supported video formats.
    static quint16 m_videoFormatsTable[];
    ///< default values for supported MTP events.
    static quint16 m_eventsSupportedTable[];
    ///< default values for supported MTP device properties.
    static quint16 m_devPropsSupportedTable[];
    ///< The xml file that stores default values for device properties.
    static QString m_deviceInfoXmlPath;
    ///< Getter for m_deviceInfoXmlFile.
    static QString getDeviceInfoXmlPath();
    ///< Setter for m_deviceInfoXmlFile.
    static void setDeviceInfoXmlPath(const QString path);
    void modifyDeviceInfoXml(QString devPropName, QString value);
};
}

#endif //#ifndef MTPDEVICEINFO_H
