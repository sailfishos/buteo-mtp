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

#include "xmlhandler.h"

using namespace meegomtp1dot0;

/******************************************
 * XMLHandler::XMLHandler
 *****************************************/
XMLHandler::XMLHandler(DeviceInfo *d) : m_state(DEFAULT), m_devInfo(d), m_devpropcode(MTP_DEV_PROPERTY_Undefined)
{
}

/******************************************
 * XMLHandler::~XMLHandler
 *****************************************/
XMLHandler::~XMLHandler()
{
}

/******************************************
 * XMLHandler::startElement
 *****************************************/
bool XMLHandler::startElement(const QString&, 
                              const QString&,
                              const QString& aName, 
                              const QXmlAttributes&)
{
    if (aName == "DeviceInfo")
    {
        m_state = START;
    }
    else if (aName == "StdVersion")
    {
        m_state = STDVERSION;
    }
    else if (aName == "MTPVendorExtn")
    {
        m_state = MTPVNDEXTN;
    }
    else if (aName == "MTPVersion")
    {
        m_state = MTPVERSION;
    }
    else if (aName == "MTPExtn")
    {
        m_state = MTPEXTN;
    }
    else if (aName == "FnMode")
    {
        m_state = FNMODE;
    }
    else if (aName == "Manufacturer")
    {
        m_state = MANU;
    }
    else if (aName == "Model")
    {
        m_state = MODEL;
    }
    else if (aName == "DeviceVersion")
    {
        m_state = DEVVER;
    }
    else if (aName == "SerialNumber")
    {
        m_state = SERIAL;
    }
    else if (aName == "OpCode")
    {
        m_state = OPCODE;
    }
    else if (aName == "EvCode")
    {
        m_state = EVCODE;
    }
    else if (aName == "DevPropCode")
    {
        m_state = DEVPROPCODE;
    }
    else if (aName == "DevPropValue")
    {
        m_state = DEVPROPVAL;
    }
    else if (aName == "Codec")
    {
        m_state = CODEC;
    }
    else if(aName == "ImageMinWidth")
    {
        m_state = IMINWIDTH;
    }
    else if(aName == "ImageMaxWidth")
    {
        m_state = IMAXWIDTH;
    }
    else if(aName == "ImageMinHeight")
    {
        m_state = IMINHEIGHT;
    }
    else if(aName == "ImageMaxHeight")
    {
        m_state = IMAXHEIGHT;
    }
    else if(aName == "VideoMinWidth")
    {
        m_state = VMINWIDTH;
    }
    else if(aName == "VideoMaxWidth")
    {
        m_state = VMAXWIDTH;
    }
    else if(aName == "VideoMinHeight")
    {
        m_state = VMINHEIGHT;
    }
    else if(aName == "VideoMaxHeight")
    {
        m_state = VMAXHEIGHT;
    }
    else if (aName == "VideoChannel")
    {
        m_state = VCHANNEL;
    }
    else if (aName == "VideoMinFPS")
    {
        m_state = VMINFPS;
    }
    else if (aName == "VideoMaxFPS")
    {
        m_state = VMAXFPS;
    }
    else if (aName == "VideoScanType")
    {
        m_state = VSCANTYPE;
    }
    else if (aName == "VideoSampleRate")
    {
        m_state = VSAMPLERATE;
    }
    else if (aName == "VideoMinBitRate")
    {
        m_state = VMINBR;
    }
    else if (aName == "VideoMaxBitRate")
    {
        m_state = VMAXBR;
    }
    else if (aName == "AudioMinBitRate")
    {
        m_state = AMINBR;
    }
    else if (aName == "AudioMaxBitRate")
    {
        m_state = AMAXBR;
    }
    else if (aName == "VideoAudioMinBitRate")
    {
        m_state = VAUDMINBR;
    }
    else if (aName == "VideoAudioMaxBitRate")
    {
        m_state = VAUDMAXBR;
    }
    else if (aName == "VideoMinKeyFrameDist")
    {
        m_state = VMINKFD;
    }
    else if (aName == "VideoMaxKeyFrameDist")
    {
        m_state = VMAXKFD;
    }
    else if (aName == "AudioChannel")
    {
        m_state = ACHANNEL;
    }
    else if (aName == "AudioSampleRate")
    {
        m_state = ASAMPLERATE;
    }
    else if(aName == "CommonFormat")
    {
        m_state = COMMONF;
    }
    else if(aName == "ImageFormat")
    {
        m_state = IMAGEF;
    }
    else if(aName == "AudioFormat")
    {
        m_state = AUDIOF;
    }
    else if(aName == "VideoFormat")
    {
        m_state = VIDEOF;
    }
    else
    {
        m_state = DEFAULT;
    }
    return true;
}

/******************************************
 * XMLHandler::endElement
 *****************************************/
bool XMLHandler::endElement(const QString&, 
                            const QString&,
                            const QString&)
{
    if (m_state == DEFAULT)
    {
        return false;
    }
    return true;
}

/******************************************
 * XMLHandler::characters
 *****************************************/
bool XMLHandler::characters(const QString& aStr)
{
    if (aStr.simplified().size() == 0 || !m_devInfo)
    {
      return true;
    }

    bool ok;
    bool result = false;

    switch (m_state)
    {
    case DEFAULT:
         result = false;
        break;
    case START:
         result = true;
        break;
    case STDVERSION:
        m_devInfo->m_standardVersion = aStr.toUShort(&ok);
        result = ok;
        break;
    case MTPVNDEXTN:
        m_devInfo->m_vendorExtension = aStr.toUInt(&ok,16);
        result = ok;
        break;
    case MTPVERSION:
        m_devInfo->m_mtpVersion = aStr.toUShort(&ok);
        result = ok;
        break;
    case MTPEXTN:
        m_devInfo->m_mtpExtension = aStr;
        result = true;
        break;
    case FNMODE:
        m_devInfo->m_functionalMode = aStr.toUShort(&ok,16);
        result = ok;
        break;
    case MANU:
        m_devInfo->m_manufacturer = aStr;
        result = true;
        break;
    case MODEL:
        m_devInfo->m_model = aStr;
        result = true;
        break;
    case DEVVER:
        m_devInfo->m_deviceVersion = aStr;
        result = true;
        break;
    case SERIAL:
        m_devInfo->m_serialNo = aStr;
        result = true;
        break;
    case OPCODE:
        m_devInfo->m_mtpOperationsSupported.append(aStr.toUShort(&ok,16));
        result = ok;
        break;
    case EVCODE:
        m_devInfo->m_mtpEventsSupported.append(aStr.toUShort(&ok,16));
        result = ok;
        break;
    case DEVPROPCODE:
        m_devpropcode = aStr.toUShort(&ok,16);
        m_devInfo->m_mtpDevicePropertiesSupported.append(m_devpropcode);
        result = ok;
        break;
    case DEVPROPVAL:
    {
        switch(m_devpropcode)
        {
            case 0xD402:
                m_devInfo->m_deviceFriendlyName = aStr;
                break;
            case 0xD401:
                m_devInfo->m_syncPartner = aStr;
                break;
            case 0x501F:
                m_devInfo->m_copyrightInfo = aStr;
                break;
            case 0xD407:
                m_devInfo->m_deviceType = aStr.toUInt(&ok, 16);
                result = ok;
                break;
        }
        result = true;
        break;
    }
    case CODEC:
        m_devInfo->m_supportedCodecs.append(aStr.toUInt(&ok,16));
        result = ok;
        break;
    case IMINWIDTH:
        m_devInfo->m_imageMinWidth = aStr.toUInt(&ok, 10);
        result = ok;
        break;
    case IMAXWIDTH:
        m_devInfo->m_imageMaxWidth = aStr.toUInt(&ok, 10);
        result = ok;
        break;
    case IMINHEIGHT:
        m_devInfo->m_imageMinHeight = aStr.toUInt(&ok, 10);
        result = ok;
        break;
    case IMAXHEIGHT:
        m_devInfo->m_imageMaxHeight = aStr.toUInt(&ok, 10);
        result = ok;
        break;
    case VMINWIDTH:
        m_devInfo->m_videoMinWidth = aStr.toUInt(&ok, 10);
        result = ok;
        break;
    case VMAXWIDTH:
        m_devInfo->m_videoMaxWidth = aStr.toUInt(&ok, 10);
        result = ok;
        break;
    case VMINHEIGHT:
        m_devInfo->m_videoMinHeight = aStr.toUInt(&ok, 10);
        result = ok;
        break;
    case VMAXHEIGHT:
        m_devInfo->m_videoMaxHeight = aStr.toUInt(&ok, 10);
        result = ok;
        break;
    case VCHANNEL:
        m_devInfo->m_videoChannels.append(aStr.toUShort(&ok,16));
        result = ok;
        break;
    case VMINFPS:
        m_devInfo->m_videoMinFPS = aStr.toUInt(&ok);
        result = ok;
        break;
    case VMAXFPS:
        m_devInfo->m_videoMaxFPS = aStr.toUInt(&ok);
        result = ok;
        break;
    case VSCANTYPE:
        m_devInfo->m_videoScanType = aStr.toUShort(&ok,16);
        result = ok;
        break;
    case VSAMPLERATE:
        m_devInfo->m_videoSampleRate = aStr.toUInt(&ok,16);
        result = ok;
        break;
    case VMINBR:
        m_devInfo->m_videoMinBitRate = aStr.toUInt(&ok,16);
        result = ok;
        break;
    case VMAXBR:
        m_devInfo->m_videoMaxBitRate = aStr.toUInt(&ok,16);
        result = ok;
        break;
    case AMINBR:
        m_devInfo->m_audioMinBitRate = aStr.toUInt(&ok,16);
        result = ok;
        break;
    case AMAXBR:
        m_devInfo->m_audioMaxBitRate = aStr.toUInt(&ok,16);
        result = ok;
        break;
    case VAUDMINBR:
        m_devInfo->m_videoAudioMinBitRate = aStr.toUInt(&ok,16);
        result = ok;
        break;
    case VAUDMAXBR:
        m_devInfo->m_videoAudioMaxBitRate = aStr.toUInt(&ok,16);
        result = ok;
        break;
    case VMINKFD:
        m_devInfo->m_videoMinKFD = aStr.toUInt(&ok,16);
        result = ok;
        break;
    case VMAXKFD:
        m_devInfo->m_videoMaxKFD = aStr.toUInt(&ok,16);
        result = ok;
        break;
    case ACHANNEL:
        m_devInfo->m_audioChannels.append(aStr.toUShort(&ok,16));
        result = ok;
        break;
    case ASAMPLERATE:
        m_devInfo->m_audioSampleRate = aStr.toUShort(&ok,16);
        result = ok;
        break;
    case COMMONF:
        m_devInfo->m_commonFormats.append(aStr.toUShort(&ok,16));
        result = ok;
        break;
    case IMAGEF:
        m_devInfo->m_imageFormats.append(aStr.toUShort(&ok,16));
        result = ok;
        break;
    case AUDIOF:
        m_devInfo->m_audioFormats.append(aStr.toUShort(&ok,16));
        result = ok;
        break;
    case VIDEOF:
        m_devInfo->m_videoFormats.append(aStr.toUShort(&ok,16));
        result = ok;
        break;
    default:
        result = false;
        break;
    }
    return result;
}

