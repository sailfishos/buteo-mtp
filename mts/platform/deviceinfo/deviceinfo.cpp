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


#include <QFile>
#include <QXmlSimpleReader>
#include <QDomDocument>
#include <QTextStream>
#include <QDir>
#include "xmlhandler.h"
#include "deviceinfo.h"
#include "trace.h"

using namespace meegomtp1dot0;

#define BATTERYLEVEL_DEFAULT 0
#define COPYRIGHTINFO_DEFAULT "Do Not Copy"
#define SYNCPARTNER_DEFAULT "Nemo"
#define DEVFRNDNAME_DEFAULT "Friendly"
#define STDVER_DEFAULT 100
#define VENDOREXTN_DEFAULT 0x00000006
#define DEVTYPE_DEFAULT 0x00000003
#define MTPVER_DEFAULT 100
#define MTPEXTN_DEFAULT "microsoft.com: 1.0; microsoft.com/WMPPD: 11.0; "
#define FNMODE_DEFAULT 0
#define MFR_DEFAULT "Nemo"
#define MODEL_DEFAULT "Nemo"
#define SERNO_DEFAULT "00000000000000000000000000000001"
#define DEVVER_DEFAULT "Nemo"
#define IMAGE_MIN_WIDTH 0
#define IMAGE_MAX_WIDTH 5000
#define IMAGE_MIN_HEIGHT 0
#define IMAGE_MAX_HEIGHT 5000
#define VIDEO_MIN_WIDTH 0
#define VIDEO_MAX_WIDTH 1920
#define VIDEO_MIN_HEIGHT 0
#define VIDEO_MAX_HEIGHT 1080


quint16 DeviceInfo::m_operationsSupportedTable[] = {
    MTP_OP_GetDeviceInfo,
    MTP_OP_OpenSession,
    MTP_OP_CloseSession,
    MTP_OP_GetStorageIDs,
    MTP_OP_GetStorageInfo,
    MTP_OP_GetNumObjects,
    MTP_OP_GetObjectHandles,
    MTP_OP_GetObjectInfo,
    MTP_OP_GetObject,
    //MTP_OP_GetThumb,
    MTP_OP_DeleteObject,
    MTP_OP_SendObjectInfo,
    MTP_OP_SendObject,
    MTP_OP_CopyObject,
    MTP_OP_MoveObject,
    MTP_OP_SetObjectPropList,
    MTP_OP_GetObjectPropList,
    MTP_OP_SendObjectPropList,
    //MTP_OP_GetInterdependentPropDesc,
    MTP_OP_GetPartialObject,
    MTP_OP_GetDevicePropDesc,
    MTP_OP_GetDevicePropValue,
    MTP_OP_SetDevicePropValue,
    MTP_OP_GetObjectPropsSupported,
    MTP_OP_GetObjectPropDesc,
    MTP_OP_GetObjectPropValue,
    MTP_OP_SetObjectPropValue,
    MTP_OP_GetObjectReferences,
    MTP_OP_SetObjectReferences
};

quint16 DeviceInfo::m_audChannelTable[] = {
    MTP_CH_CONF_MONO,
    MTP_CH_CONF_STEREO
};

quint16 DeviceInfo::m_vidChannelTable[] = {
    MTP_CH_CONF_MONO,
    MTP_CH_CONF_STEREO
};

quint32 DeviceInfo::m_supportedCodecsTable[] = {
    0x00000000,
    MTP_WAVE_FORMAT_ALAW,
    MTP_WAVE_FORMAT_MULAW,
    MTP_WAVE_FORMAT_MPEGLAYER3,
    MTP_WAVE_FORMAT_MSAUDIO1,
    MTP_WAVE_FORMAT_MSAUDIO2,
    MTP_WAVE_FORMAT_MSAUDIO3,
    MTP_WAVE_FORMAT_NOKIA_AMR,
    MTP_WAVE_FORMAT_AAC,
    MTP_WAVE_FORMAT_DSPGROUP_TRUESPEECH
};

quint16 DeviceInfo::m_supportedFormatsTable[] = {
    MTP_OBF_FORMAT_Undefined,
    MTP_OBF_FORMAT_Association,
    MTP_OBF_FORMAT_Text,
    MTP_OBF_FORMAT_HTML,
    MTP_OBF_FORMAT_Abstract_Audio_Album,
    MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist,
    MTP_OBF_FORMAT_vCard2,
    MTP_OBF_FORMAT_vCard3,
    MTP_OBF_FORMAT_vCal1,

    MTP_OBF_FORMAT_EXIF_JPEG,
    MTP_OBF_FORMAT_GIF,
    MTP_OBF_FORMAT_PNG,
    MTP_OBF_FORMAT_TIFF,
    MTP_OBF_FORMAT_BMP,

    MTP_OBF_FORMAT_WAV,
    MTP_OBF_FORMAT_MP3,
    MTP_OBF_FORMAT_WMA,
    MTP_OBF_FORMAT_AAC,

    MTP_OBF_FORMAT_AVI,
    MTP_OBF_FORMAT_MPEG,
    MTP_OBF_FORMAT_3GP_Container,
    MTP_OBF_FORMAT_MP4_Container,
    MTP_OBF_FORMAT_WMV
};

quint16 DeviceInfo::m_commonFormatsTable[] = {
    MTP_OBF_FORMAT_Undefined,
    MTP_OBF_FORMAT_Association,
    MTP_OBF_FORMAT_Text,
    MTP_OBF_FORMAT_HTML,
    MTP_OBF_FORMAT_Abstract_Audio_Album,
    MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist,
    MTP_OBF_FORMAT_vCard2,
    MTP_OBF_FORMAT_vCard3,
    MTP_OBF_FORMAT_vCal1
};

quint16 DeviceInfo::m_imageFormatsTable[] = {
    MTP_OBF_FORMAT_EXIF_JPEG,
    MTP_OBF_FORMAT_GIF,
    MTP_OBF_FORMAT_PNG,
    MTP_OBF_FORMAT_TIFF,
    MTP_OBF_FORMAT_BMP
};

quint16 DeviceInfo::m_audioFormatsTable[] = {
    MTP_OBF_FORMAT_MP3,
    MTP_OBF_FORMAT_WMA,
    MTP_OBF_FORMAT_WAV,
    MTP_OBF_FORMAT_AAC
};

quint16 DeviceInfo::m_videoFormatsTable[] = {
    MTP_OBF_FORMAT_AVI,
    MTP_OBF_FORMAT_MPEG,
    MTP_OBF_FORMAT_3GP_Container,
    MTP_OBF_FORMAT_MP4_Container,
    MTP_OBF_FORMAT_WMV
};

quint16 DeviceInfo::m_eventsSupportedTable[] = {
    MTP_EV_RequestObjectTransfer,
    MTP_EV_ObjectRemoved,
    MTP_EV_ObjectAdded,
    MTP_EV_StoreAdded,
    MTP_EV_StoreRemoved,
    MTP_EV_StoreFull,
    MTP_EV_StorageInfoChanged,
    MTP_EV_ObjectInfoChanged,
    MTP_EV_DeviceInfoChanged,
    MTP_EV_DevicePropChanged,
    MTP_EV_ObjectPropChanged
};

quint16 DeviceInfo::m_devPropsSupportedTable[] = {
    MTP_DEV_PROPERTY_Synchronization_Partner,
    MTP_DEV_PROPERTY_Device_Friendly_Name
};

QString DeviceInfo::m_deviceInfoXmlPath;

//Constructor, first store default values for device properties. Next,
//fetch the same from an xml file. If xml parsing fails, the hardcoded
//default values will be used.

/*******************************************
 * DeviceInfo::DeviceInfo
 ******************************************/
DeviceInfo::DeviceInfo( QObject *parent ) :
    QObject(parent),
    m_batteryLevel(BATTERYLEVEL_DEFAULT),
    m_copyrightInfo(COPYRIGHTINFO_DEFAULT),
    m_syncPartner(SYNCPARTNER_DEFAULT),
    m_deviceFriendlyName(DEVFRNDNAME_DEFAULT),
    m_standardVersion(STDVER_DEFAULT),
    m_vendorExtension(VENDOREXTN_DEFAULT),
    m_mtpVersion(MTPVER_DEFAULT),
    m_mtpExtension(MTPEXTN_DEFAULT),
    m_functionalMode(FNMODE_DEFAULT),
    m_manufacturer(MFR_DEFAULT),
    m_model(MODEL_DEFAULT),
    m_serialNo(SERNO_DEFAULT),
    m_deviceVersion(DEVVER_DEFAULT),
    m_deviceType(DEVTYPE_DEFAULT),
    m_imageMinWidth(IMAGE_MIN_WIDTH), m_imageMaxWidth(IMAGE_MAX_WIDTH),
    m_imageMinHeight(IMAGE_MIN_HEIGHT), m_imageMaxHeight(IMAGE_MAX_HEIGHT),
    m_videoMinWidth(VIDEO_MIN_WIDTH), m_videoMaxWidth(VIDEO_MAX_WIDTH),
    m_videoMinHeight(VIDEO_MIN_HEIGHT), m_videoMaxHeight(VIDEO_MAX_HEIGHT),
    m_videoMinFPS(0), m_videoMaxFPS(100000), m_videoScanType(0x0001), m_videoSampleRate(0),
    m_videoMinBitRate(0), m_videoMaxBitRate(0xFFFFFFFF),
    m_audioMinBitRate(0), m_audioMaxBitRate(0xFFFFFFFF),
    m_videoAudioMinBitRate(0), m_videoAudioMaxBitRate(0xFFFFFFFF),
    m_videoMinKFD(0), m_videoMaxKFD(0xFFFFFFFF),
    m_audioSampleRate(0)
{
    //Parse deviceinfo.xml to populate default device values.

    // Kludge : till we know how and where to securely install a file
    // that can be modifed by an apllication.
    QFile fileDst(getDeviceInfoXmlPath());
#ifndef UT_ON
    QFile fileSrc("/usr/share/mtp/deviceinfo.xml");
    if( !fileDst.exists() )
    {
        fileSrc.copy(m_deviceInfoXmlPath);
    }
#endif // UT_ON
    fileDst.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlSimpleReader xmlReader;
    QXmlInputSource source(&fileDst);
    XMLHandler handler(this);
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    m_xmlOk = false;

    if(0 == xmlReader.parse(&source))
    {
        MTP_LOG_CRITICAL("Failure reading deviceinfo.xml, using default hard-coded values\n");
        //FIXME Hard code the QVectors themselves by default? Then we can avoid the memcpy.
        for( quint32 i = 0 ; i < sizeof(m_operationsSupportedTable)/sizeof(m_operationsSupportedTable[0]); i++ )
        {
            m_mtpOperationsSupported.append( m_operationsSupportedTable[i] );
        }
        for( quint32 i = 0 ; i < sizeof(m_eventsSupportedTable)/sizeof(m_eventsSupportedTable[0]); i++ )
        {
            m_mtpEventsSupported.append( m_eventsSupportedTable[i] );
        }
        for( quint32 i = 0 ; i < sizeof(m_devPropsSupportedTable)/sizeof(m_devPropsSupportedTable[0]); i++ )
        {
            m_mtpDevicePropertiesSupported.append( m_devPropsSupportedTable[i] );
        }
        for( quint32 i = 0 ; i < sizeof(m_commonFormatsTable)/sizeof(m_commonFormatsTable[0]); i++ )
        {
            m_commonFormats.append( m_commonFormatsTable[i] );
        }
        for( quint32 i = 0 ; i < sizeof(m_audioFormatsTable)/sizeof(m_audioFormatsTable[0]); i++ )
        {
            m_audioFormats.append( m_audioFormatsTable[i] );
        }
        for( quint32 i = 0 ; i < sizeof(m_imageFormatsTable)/sizeof(m_imageFormatsTable[0]); i++ )
        {
            m_imageFormats.append( m_imageFormatsTable[i] );
        }
        for( quint32 i = 0 ; i < sizeof(m_videoFormatsTable)/sizeof(m_videoFormatsTable[0]); i++ )
        {
            m_videoFormats.append( m_videoFormatsTable[i] );
        }
        for( quint32 i = 0 ; i < sizeof(m_audChannelTable)/sizeof(m_audChannelTable[0]); i++ )
        {
            m_audioChannels.append( m_audChannelTable[i] );
        }
        for( quint32 i = 0 ; i < sizeof(m_vidChannelTable)/sizeof(m_vidChannelTable[0]); i++ )
        {
            m_videoChannels.append( m_vidChannelTable[i] );
        }
        for( quint32 i = 0 ; i < sizeof(m_supportedCodecsTable)/sizeof(m_supportedCodecsTable[0]); i++ )
        {
            m_supportedCodecs.append( m_supportedCodecsTable[i] );
        }
    }
    else
    {
        m_xmlOk = true;
    }
    m_supportedFormats = m_imageFormats + m_audioFormats + m_videoFormats + m_commonFormats;
}

/*******************************************
 * DeviceInfo::~DeviceInfo
 ******************************************/
DeviceInfo::~DeviceInfo()
{
}

/*******************************************
 * quint8 DeviceInfo::batteryLevel
 ******************************************/
quint8 DeviceInfo::batteryLevel( bool /*current*/ ) const
{
    return m_batteryLevel;
}

/*******************************************
 * const QString& DeviceInfo::syncPartner
 ******************************************/
const QString& DeviceInfo::syncPartner( bool /*current*/ ) const
{
    return m_syncPartner;
}

/*******************************************
 * QString& DeviceInfo::copyrightInfo
 ******************************************/
const QString& DeviceInfo::copyrightInfo( bool /*current*/ ) const
{
    return m_copyrightInfo;
}

/*******************************************
 * const QString& DeviceInfo::deviceFriendlyName
 ******************************************/
const QString& DeviceInfo::deviceFriendlyName( bool /*current*/ )
{
    return m_deviceFriendlyName;
}

/*******************************************
 * const quint16& DeviceInfo::standardVersion
 ******************************************/
const quint16& DeviceInfo::standardVersion() const
{
    return m_standardVersion;
}

/*******************************************
 * const quint32& DeviceInfo::vendorExtension
 ******************************************/
const quint32& DeviceInfo::vendorExtension() const
{
    return m_vendorExtension;
}

/*******************************************
 * const quint16& DeviceInfo::MTPVersion
 ******************************************/
const quint16& DeviceInfo::MTPVersion() const
{
    return m_mtpVersion;
}

/*******************************************
 * const QString& DeviceInfo::MTPExtension
 ******************************************/
const QString& DeviceInfo::MTPExtension() const
{
    return m_mtpExtension;
}

/*******************************************
 * const QString& DeviceInfo::manufacturer
 ******************************************/
const QString& DeviceInfo::manufacturer() const
{
    return m_manufacturer;
}

/*******************************************
 * const QString& DeviceInfo::model
 ******************************************/
const QString& DeviceInfo::model() const
{
    return m_model;
}

/*******************************************
 * const QString& DeviceInfo::deviceVersion
 ******************************************/
const QString& DeviceInfo::deviceVersion() const
{
    return m_deviceVersion;
}

/*******************************************
 * const QString& DeviceInfo::serialNo
 ******************************************/
const QString& DeviceInfo::serialNo() const
{
    return m_serialNo;
}

/*******************************************
 * quint32 DeviceInfo::deviceType
 ******************************************/
quint32 DeviceInfo::deviceType() const
{
    return m_deviceType;
}

/*******************************************
 * const quint16& DeviceInfo::functionalMode
 ******************************************/
const quint16& DeviceInfo::functionalMode() const
{
    return m_functionalMode;
}

/*******************************************
 * void DeviceInfo::setSyncPartner
 ******************************************/
void DeviceInfo::setSyncPartner( const QString& syncPartner )
{
    m_syncPartner = syncPartner;

    // Also modify the xml file
    modifyDeviceInfoXml( "syncpartner", syncPartner );
}

/*******************************************
 * void DeviceInfo::setDeviceFriendlyName
 ******************************************/
void DeviceInfo::setDeviceFriendlyName( const QString& deviceFriendlyName )
{
    m_deviceFriendlyName = deviceFriendlyName;

    // Also modify the xml file
    modifyDeviceInfoXml( "friendlyname", deviceFriendlyName );
}

/*******************************************
 * void DeviceInfo::modifyDeviceInfoXml
 ******************************************/
void DeviceInfo::modifyDeviceInfoXml( QString devPropName, QString value )
{
    QDomDocument document;
    QDomElement element;
    QFile file(getDeviceInfoXmlPath());
    if( file.open( QIODevice::ReadOnly) )
    {
        document.setContent(&file);
        file.close();
        QDomNodeList elementList = document.elementsByTagName("DevPropValue");
        for( int i = 0 ; i < elementList.count(); i++ )
        {
            element = elementList.item( i ).toElement();
            if( devPropName == element.attribute( "id" ) )
            {
                // Assuming first child is the text node containing the friendly name
                element.removeChild( element.firstChild() );
                QDomText text = document.createTextNode( value );
                element.appendChild( text );
                if( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
                {
                    QTextStream ts( &file );
                    ts << document.toString();
                }
                break;
            }
        }
    }
}

/*******************************************
 * QString DeviceInfo::getDeviceInfoXmlPath
 ******************************************/
QString DeviceInfo::getDeviceInfoXmlPath()
{
    if (m_deviceInfoXmlPath.isEmpty()) {
        QString tmpPath = QDir::homePath();
        if (tmpPath.isEmpty()) {
            qFatal("DeviceInfo: can't determine home directory");
        }
        m_deviceInfoXmlPath = tmpPath + "/.mtpdeviceinfo.xml";
    }
    return m_deviceInfoXmlPath;
}

/*******************************************
 * void DeviceInfo::setDeviceInfoXmlPath
 ******************************************/
void DeviceInfo::setDeviceInfoXmlPath(const QString path)
{
    m_deviceInfoXmlPath = path;
}

/*******************************************
 * m_formFlag DeviceInfo::getBatteryLevelForm
 ******************************************/
quint8 DeviceInfo::getBatteryLevelForm( quint8& min, quint8& max, quint8& stepSize, quint32& /*noOfVals*/, QVector<quint8>& /*vals*/ ) const
{
    min = 0;
    max = 100;
    stepSize = 10;
    return FORM_RANGE;
}

/*******************************************
 * m_formFlag DeviceInfo::getBatteryLevelForm
 ******************************************/
const QVector<quint16>& DeviceInfo::MTPOperationsSupported() const
{
    return m_mtpOperationsSupported;
}

/*******************************************
 * quint16* DeviceInfo::MTPEventsSupported
 ******************************************/
const QVector<quint16>& DeviceInfo::MTPEventsSupported() const
{
    return m_mtpEventsSupported;
}

/*******************************************
 * quint16* DeviceInfo::MTPDevicePropertiesSupported
 ******************************************/
const QVector<quint16>& DeviceInfo::MTPDevicePropertiesSupported() const
{
    return m_mtpDevicePropertiesSupported;
}

/*******************************************
 * quint16* DeviceInfo::supportedFormats
 ******************************************/
const QVector<quint16>& DeviceInfo::supportedFormats() const
{
    return m_supportedFormats;
}

/*******************************************
 * quint16 DeviceInfo::getFormatCodeCategory
 ******************************************/
quint16 DeviceInfo::getFormatCodeCategory(quint16 formatCode)
{
    quint16 formatCategory = MTP_UNSUPPORTED_FORMAT;

    if(m_commonFormats.contains(formatCode))
    {
        formatCategory = MTP_COMMON_FORMAT;
    }
    else if(m_audioFormats.contains(formatCode))
    {
        formatCategory = MTP_AUDIO_FORMAT;
    }
    else if(m_videoFormats.contains(formatCode))
    {
        formatCategory = MTP_VIDEO_FORMAT;
    }
    else  if(m_imageFormats.contains(formatCode))
    {
        formatCategory = MTP_IMAGE_FORMAT;
    }
    return formatCategory;
}

quint32 DeviceInfo::imageMinWidth()
{
    return m_imageMinWidth;
}

quint32 DeviceInfo::imageMaxWidth()
{
    return m_imageMaxWidth;
}

quint32 DeviceInfo::imageMinHeight()
{
    return m_imageMinHeight;
}

quint32 DeviceInfo::imageMaxHeight()
{
    return m_imageMaxHeight;
}

quint32 DeviceInfo::videoMinWidth()
{
    return m_videoMinWidth;
}

quint32 DeviceInfo::videoMaxWidth()
{
    return m_videoMaxWidth;
}

quint32 DeviceInfo::videoMinHeight()
{
    return m_videoMinHeight;
}

quint32 DeviceInfo::videoMaxHeight()
{
    return m_videoMaxHeight;
}

/*******************************************
 * quint16* DeviceInfo::videoChannels(quint32& no)
 ******************************************/
const QVector<quint16>& DeviceInfo::videoChannels() const
{
    return m_videoChannels;
}

/*******************************************
 * quint16* DeviceInfo::audioChannels
 ******************************************/
const QVector<quint16>& DeviceInfo::audioChannels() const
{
    return m_audioChannels;
}

/*******************************************
 * const quint32& DeviceInfo::videoMinFPS
 ******************************************/
const quint32& DeviceInfo::videoMinFPS() const
{
    return m_videoMinFPS;
}

/*******************************************
 * const quint32& DeviceInfo::videoMaxFPS
 ******************************************/
const quint32& DeviceInfo::videoMaxFPS() const
{
    return m_videoMaxFPS;
}

/*******************************************
 * const quint16& DeviceInfo::videoScanType
 ******************************************/
const quint16& DeviceInfo::videoScanType() const
{
    return m_videoScanType;
}

/*******************************************
 * const quint32& DeviceInfo::videoSampleRate
 ******************************************/
const quint32& DeviceInfo::videoSampleRate() const
{
    return m_videoSampleRate;
}

/*******************************************
 * const quint32& DeviceInfo::audioSampleRate()
 ******************************************/
const quint32& DeviceInfo::audioSampleRate() const
{
    return m_audioSampleRate;
}

/*******************************************
 * const quint32& DeviceInfo::videoMinBitRate
 ******************************************/
const quint32& DeviceInfo::videoMinBitRate() const
{
    return m_videoMinBitRate;
}

/*******************************************
 * const quint32& DeviceInfo::videoMaxBitRate
 ******************************************/
const quint32& DeviceInfo::videoMaxBitRate() const
{
    return m_videoMaxBitRate;
}

/*******************************************
 * const quint32& DeviceInfo::audioMinBitRate
 ******************************************/
const quint32& DeviceInfo::audioMinBitRate() const
{
    return m_audioMinBitRate;
}

/*******************************************
 * const quint32& DeviceInfo::audioMaxBitRate
 ******************************************/
const quint32& DeviceInfo::audioMaxBitRate() const
{
    return m_audioMaxBitRate;
}

/*******************************************
 * const quint32& DeviceInfo::videoAudioMinBitRate
 ******************************************/
const quint32& DeviceInfo::videoAudioMinBitRate() const
{
    return m_videoAudioMinBitRate;
}

/*******************************************
 * const quint32& DeviceInfo::videoAudioMaxBitRate
 ******************************************/
const quint32& DeviceInfo::videoAudioMaxBitRate() const
{
    return m_videoAudioMaxBitRate;
}

/*******************************************
 * const quint32& DeviceInfo::videoMinKFD
 ******************************************/
const quint32& DeviceInfo::videoMinKFD() const
{
    return m_videoMinKFD;
}

/*******************************************
 * const quint32& DeviceInfo::videoMaxKFD
 ******************************************/
const quint32& DeviceInfo::videoMaxKFD() const
{
    return m_videoMaxKFD;
}

/*******************************************
 * quint32* DeviceInfo::supportedAudioCodecs
 ******************************************/
const QVector<quint32>& DeviceInfo::supportedAudioCodecs() const
{
    return m_supportedCodecs;
}
