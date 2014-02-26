/*
* This file is part of libmeegomtp package
*
* Copyright (C) 2010 Nokia Corporation. All rights reserved.
*
* Contact: Santosh Puranik <santosh.puranik@nokia.com>
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

#include <QVariant>
#include "propertypod.h"
#include "trace.h"
#include "deviceinfo.h"
#include "mtpextensionmanager.h"

using namespace meegomtp1dot0;

PropertyPod* PropertyPod::m_instance = 0;

// NOTE: These must me changed when adding new properties

MtpObjPropDesc PropertyPod::m_commonPropDesc[] =
{
    {
        MTP_OBJ_PROP_StorageID, MTP_DATA_TYPE_UINT32,
        false, QVariant(0), 
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Obj_Format, MTP_DATA_TYPE_UINT16,
        false, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Protection_Status, MTP_DATA_TYPE_UINT16,
        false, QVariant(0),
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Obj_Size, MTP_DATA_TYPE_UINT64,
        false, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Obj_File_Name, MTP_DATA_TYPE_STR,
        true, QVariant(QString()),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Date_Created, MTP_DATA_TYPE_STR,
        false, QVariant(QString()),
        0, MTP_FORM_FLAG_DATE_TIME,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Date_Modified, MTP_DATA_TYPE_STR,
        false, QVariant(QString()),
        0, MTP_FORM_FLAG_DATE_TIME,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Parent_Obj, MTP_DATA_TYPE_UINT32,
        false, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Persistent_Unique_ObjId, MTP_DATA_TYPE_UINT128,
        false, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Name, MTP_DATA_TYPE_STR,
        true, QVariant(QString()),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Non_Consumable, MTP_DATA_TYPE_UINT8,
        false, QVariant(0),
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
};

MtpObjPropDesc PropertyPod::m_imagePropDesc[] =
{
    {
        MTP_OBJ_PROP_Width, MTP_DATA_TYPE_UINT32,
        false, QVariant(0),
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Height, MTP_DATA_TYPE_UINT32,
        false, QVariant(0),
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Rep_Sample_Format, MTP_DATA_TYPE_UINT16,
        false, QVariant(0),
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Rep_Sample_Width, MTP_DATA_TYPE_UINT32,
        false, QVariant(0),
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Rep_Sample_Height, MTP_DATA_TYPE_UINT32,
        false, QVariant(0),
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Rep_Sample_Data, MTP_DATA_TYPE_AUINT8,
        false, QVariant(0),
        0, MTP_FORM_FLAG_BYTE_ARRAY,
        QVariant()
    }
};

MtpObjPropDesc PropertyPod::m_audioPropDesc[] =
{
    {
        MTP_OBJ_PROP_Artist, MTP_DATA_TYPE_STR,
        true, QVariant(QString()),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Album_Name, MTP_DATA_TYPE_STR,
        true, QVariant(QString()),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
#if 0
    {
        MTP_OBJ_PROP_Album_Artist, MTP_DATA_TYPE_STR,
        true, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
#endif
    {
        MTP_OBJ_PROP_Track, MTP_DATA_TYPE_UINT16,
        true, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Genre, MTP_DATA_TYPE_STR,
        true, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Use_Count, MTP_DATA_TYPE_UINT32,
        true, QVariant(0), 
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Duration, MTP_DATA_TYPE_UINT32,
        true, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
#if 0
    {
        MTP_OBJ_PROP_Original_Release_Date, MTP_DATA_TYPE_STR,
        true, QVariant(0),
        0, MTP_FORM_FLAG_DATE_TIME,
        QVariant()
    },
#endif
    {
        MTP_OBJ_PROP_Bitrate_Type, MTP_DATA_TYPE_UINT16,
        true, QVariant(0),
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Sample_Rate, MTP_DATA_TYPE_UINT32,
        true, QVariant(0), 
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Nbr_Of_Channels, MTP_DATA_TYPE_UINT16,
        true, QVariant(0), 
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Audio_WAVE_Codec, MTP_DATA_TYPE_UINT32,
        true, QVariant(0),
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Audio_BitRate, MTP_DATA_TYPE_UINT32,
        true, QVariant(0),
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
#if 0
    {
        MTP_OBJ_PROP_Rating, MTP_DATA_TYPE_UINT16,
        false, QVariant(0), 
        0, MTP_FORM_FLAG_RANGE,
        QVariant::fromValue(MtpRangeForm(QVariant(0), QVariant(100), QVariant(1)))
    },
#endif
    {
        MTP_OBJ_PROP_DRM_Status, MTP_DATA_TYPE_UINT16,
        true, QVariant(0), 
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    }
#if 0
    // This property is disabled for now as there is no concrete use case for
    // this yet.
    {
        MTP_OBJ_PROP_Purchase_Album, MTP_DATA_TYPE_UINT8,
        true, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    }
#endif
};

MtpObjPropDesc PropertyPod::m_videoPropDesc[] =
{
    {
        MTP_OBJ_PROP_Sample_Rate, MTP_DATA_TYPE_UINT32,
        true, QVariant(0), 
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Video_FourCC_Codec, MTP_DATA_TYPE_UINT32,
        true, QVariant(0), 
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Video_BitRate, MTP_DATA_TYPE_UINT32,
        true, QVariant(0),
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Frames_Per_Thousand_Secs, MTP_DATA_TYPE_UINT32,
        true, QVariant(0), 
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
#if 0
    // No use case for the below properties yet.
    {
        MTP_OBJ_PROP_KeyFrame_Distance, MTP_DATA_TYPE_UINT32,
        false, QVariant(0), 
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Encoding_Profile, MTP_DATA_TYPE_STR,
        false, QVariant(QString()),
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Scan_Type, MTP_DATA_TYPE_UINT16,
        false, QVariant(0),
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
#endif
    {
        MTP_OBJ_PROP_Genre, MTP_DATA_TYPE_STR,
        true, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Nbr_Of_Channels, MTP_DATA_TYPE_UINT16,
        true, QVariant(0), 
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Audio_WAVE_Codec, MTP_DATA_TYPE_UINT32,
        true, QVariant(0),
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Audio_BitRate, MTP_DATA_TYPE_UINT32,
        true, QVariant(0),
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Width, MTP_DATA_TYPE_UINT32,
        true, QVariant(0),
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Height, MTP_DATA_TYPE_UINT32,
        true, QVariant(0),
        0, MTP_FORM_FLAG_RANGE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Artist, MTP_DATA_TYPE_STR,
        true, QVariant(QString()),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Album_Name, MTP_DATA_TYPE_STR,
        true, QVariant(QString()),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
#if 0
    {
        MTP_OBJ_PROP_Album_Artist, MTP_DATA_TYPE_STR,
        true, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
#endif
    {
        MTP_OBJ_PROP_Use_Count, MTP_DATA_TYPE_UINT32,
        true, QVariant(0), 
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Duration, MTP_DATA_TYPE_UINT32,
        true, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
    {
        MTP_OBJ_PROP_Track, MTP_DATA_TYPE_UINT16,
        true, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    },
#if 0
    {
        MTP_OBJ_PROP_Rating, MTP_DATA_TYPE_UINT16,
        false, QVariant(0), 
        0, MTP_FORM_FLAG_RANGE,
        QVariant::fromValue(MtpRangeForm(QVariant(0), QVariant(100), QVariant(1)))
    },
    {
        MTP_OBJ_PROP_Original_Release_Date, MTP_DATA_TYPE_STR,
        true, QVariant(0),
        0, MTP_FORM_FLAG_DATE_TIME,
        QVariant()
    },
#endif
    {
        MTP_OBJ_PROP_DRM_Status, MTP_DATA_TYPE_UINT16,
        true, QVariant(0), 
        0, MTP_FORM_FLAG_ENUM,
        QVariant()
    }
#if 0
    // This property is disabled for now as there is no concrete use case for
    // this yet.
    {
        MTP_OBJ_PROP_Purchase_Album, MTP_DATA_TYPE_UINT8,
        true, QVariant(0),
        0, MTP_FORM_FLAG_NONE,
        QVariant()
    }
#endif
};

MtpDevPropDesc PropertyPod::m_devicePropDesc[] =
{
    {
        MTP_DEV_PROPERTY_BatteryLevel,
        static_cast<MTPDataType>(MTP_DATA_TYPE_UINT8), false,
        QVariant(), QVariant(),
        MTP_FORM_FLAG_NONE, QVariant()
    },
    {
        MTP_DEV_PROPERTY_Synchronization_Partner,
        static_cast<MTPDataType>(MTP_DATA_TYPE_STR), true,
        QVariant(), QVariant(),
        MTP_FORM_FLAG_NONE, QVariant()
    },
    {
        MTP_DEV_PROPERTY_Device_Friendly_Name,
        static_cast<MTPDataType>(MTP_DATA_TYPE_STR), true,
        QVariant(), QVariant(),
        MTP_FORM_FLAG_NONE, QVariant()
    },
    {
        MTP_DEV_PROPERTY_DeviceIcon,
        static_cast<MTPDataType>(MTP_DATA_TYPE_AUINT8), false,
        QVariant(), QVariant(),
        MTP_FORM_FLAG_NONE, QVariant()
    },
    {
        MTP_DEV_PROPERTY_Perceived_Device_Type,
        static_cast<MTPDataType>(MTP_DATA_TYPE_UINT32), false,
        QVariant(), QVariant(),
        MTP_FORM_FLAG_NONE, QVariant()
    }
    #if 0
    ,
    {
        MTP_DEV_PROPERTY_Volume,
        static_cast<MTPDataType>(MTP_DATA_TYPE_INT32), true,
        QVariant(0), QVariant(),
        MTP_FORM_FLAG_RANGE, QVariant()
    }
    #endif
};

PropertyPod::PropertyPod(DeviceInfo* devInfoProvider, MTPExtensionManager* extManager) : m_provider(devInfoProvider), m_extManager(extManager)
{
    MtpObjPropDesc* propDesc = 0;
    // Populate the mapping from object property to descriptions
    for(quint32 i = 0; i < sizeof(m_commonPropDesc)/sizeof(MtpObjPropDesc); i++)
    {
        propDesc = &m_commonPropDesc[i];
        m_objPropMapCommon.insert((MTPObjPropertyCode)propDesc->uPropCode, propDesc);
        if(MTP_FORM_FLAG_ENUM == propDesc->formFlag)
        {
            populateEnumDesc(propDesc, MTP_COMMON_FORMAT);
        }
    }
    for(quint32 i = 0; i < sizeof(m_imagePropDesc)/sizeof(MtpObjPropDesc); i++)
    {
        propDesc = &m_imagePropDesc[i];
        m_objPropMapImage.insert((MTPObjPropertyCode)propDesc->uPropCode, propDesc);
        if(MTP_FORM_FLAG_ENUM == propDesc->formFlag)
        {
            populateEnumDesc(propDesc, MTP_IMAGE_FORMAT);
        }
        else if(isTechObjProp((MTPObjPropertyCode)propDesc->uPropCode))
        {
            populateTechObjPropDesc(propDesc, MTP_IMAGE_FORMAT);
        }
    }
    for(quint32 i = 0; i < sizeof(m_audioPropDesc)/sizeof(MtpObjPropDesc); i++)
    {
        propDesc = &m_audioPropDesc[i];
        m_objPropMapAudio.insert((MTPObjPropertyCode)propDesc->uPropCode, propDesc);
        if(MTP_FORM_FLAG_ENUM == propDesc->formFlag)
        {
            populateEnumDesc(propDesc, MTP_AUDIO_FORMAT);
        }
        else if(isTechObjProp((MTPObjPropertyCode)propDesc->uPropCode))
        {
            populateTechObjPropDesc(propDesc, MTP_AUDIO_FORMAT);
        }
    }
    for(quint32 i = 0; i < sizeof(m_videoPropDesc)/sizeof(MtpObjPropDesc); i++)
    {
        propDesc = &m_videoPropDesc[i];
        m_objPropMapVideo.insert((MTPObjPropertyCode)propDesc->uPropCode, propDesc);
        if(MTP_FORM_FLAG_ENUM == propDesc->formFlag)
        {
            populateEnumDesc(propDesc, MTP_VIDEO_FORMAT);
        }
        else if(isTechObjProp((MTPObjPropertyCode)propDesc->uPropCode))
        {
            populateTechObjPropDesc(propDesc, MTP_AUDIO_FORMAT);
        }
    }
    MtpDevPropDesc *propDescDev = 0;
    for(quint32 i = 0; i < sizeof(m_devicePropDesc)/sizeof(MtpDevPropDesc); i++)
    {
        propDescDev = &m_devicePropDesc[i];
        m_devPropMap.insert(propDescDev->uPropCode, propDescDev);

        switch (propDescDev->uPropCode) {
            case MTP_DEV_PROPERTY_BatteryLevel: {
                propDescDev->formField = m_provider->batteryLevelForm();

                int type = propDescDev->formField.userType();
                if (type == qMetaTypeId<MtpRangeForm>()) {
                    propDescDev->formFlag = MTP_FORM_FLAG_RANGE;
                } else if (type == qMetaTypeId<MtpEnumForm>()) {
                    propDescDev->formFlag = MTP_FORM_FLAG_ENUM;
                }
                break;
            }
            case MTP_DEV_PROPERTY_Synchronization_Partner:
                propDescDev->defValue = m_provider->syncPartner();
                break;
            case MTP_DEV_PROPERTY_Device_Friendly_Name:
                propDescDev->defValue = m_provider->deviceFriendlyName();
                break;
            case MTP_DEV_PROPERTY_Volume:
                propDescDev->defValue =
                        QVariant::fromValue(MtpRangeForm(0, 100, 1));
                break;
            case MTP_DEV_PROPERTY_Perceived_Device_Type:
                propDescDev->defValue = m_provider->deviceType();
                break;
        }
    }
}

PropertyPod* PropertyPod::instance(DeviceInfo* devInfoProvider, MTPExtensionManager* extManager)
{
    if(0 == m_instance)
    {
        m_instance = new PropertyPod(devInfoProvider, extManager);
    }
    return m_instance;
}

void PropertyPod::releaseInstance()
{
    if(m_instance)
    {
        delete m_instance;
        m_instance = 0;
    }
}

PropertyPod::~PropertyPod()
{

}

MTPResponseCode PropertyPod::getObjectPropsSupportedByType(MTPObjectFormatCategory category, QVector<MTPObjPropertyCode>& propsSupported)
{
    MTPResponseCode ret = MTP_RESP_OK;
    static QVector<MTPObjPropertyCode> propsCommon = QVector<MTPObjPropertyCode>::fromList(m_objPropMapCommon.keys());
    static QVector<MTPObjPropertyCode> propsImage = propsCommon + QVector<MTPObjPropertyCode>::fromList(m_objPropMapImage.keys());
    static QVector<MTPObjPropertyCode> propsAudio = propsCommon + QVector<MTPObjPropertyCode>::fromList(m_objPropMapAudio.keys());
    static QVector<MTPObjPropertyCode> propsVideo = propsCommon + QVector<MTPObjPropertyCode>::fromList(m_objPropMapVideo.keys());
    switch(category)
    {
        case MTP_COMMON_FORMAT:
            propsSupported = propsCommon;
            break;
        case MTP_IMAGE_FORMAT:
            propsSupported = propsImage;
            break;
        case MTP_AUDIO_FORMAT:
            propsSupported = propsAudio;
            break;
        case MTP_VIDEO_FORMAT:
            propsSupported = propsVideo;
            break;
        case MTP_UNSUPPORTED_FORMAT:
        default:
            ret = MTP_RESP_Invalid_ObjectProp_Format;
            break;
    }
    return ret;
}

MTPResponseCode PropertyPod::getInterdependentPropDesc(MTPObjectFormatCategory /*category*/, QVector<MtpObjPropDesc*>& /*propDesc*/)
{
    // FIXME: Needs to be implemented
    MTPResponseCode ret = MTP_RESP_OK;
    return ret;
}

MTPResponseCode PropertyPod::getDevicePropDesc(MTPDevPropertyCode propCode,
                                               MtpDevPropDesc **propDesc)
{
    *propDesc = m_devPropMap.value(propCode);
    if (!*propDesc) {
        return MTP_RESP_DevicePropNotSupported;
    }

    switch (propCode) {
        case MTP_DEV_PROPERTY_BatteryLevel:
            (*propDesc)->currentValue = m_provider->batteryLevel();
            break;
        case MTP_DEV_PROPERTY_Synchronization_Partner:
            (*propDesc)->currentValue = m_provider->syncPartner();
            break;
        case MTP_DEV_PROPERTY_Device_Friendly_Name:
            (*propDesc)->currentValue = m_provider->deviceFriendlyName();
            break;
        case MTP_DEV_PROPERTY_Volume:
            // Do nothing.
            break;
        case MTP_DEV_PROPERTY_DeviceIcon:
            (*propDesc)->currentValue =
                    QVariant::fromValue(m_provider->deviceIcon());
            break;
        case MTP_DEV_PROPERTY_Perceived_Device_Type:
            (*propDesc)->currentValue = m_provider->deviceType();
            break;
        default: {
            // Fetch the current value from the extension.
            MTPResponseCode result = MTP_RESP_OK;
            if (!m_extManager->getDevPropValue(propCode, (*propDesc)->currentValue, result)) {
                // We should never get here!
                return MTP_RESP_DevicePropNotSupported;
            }
            return result;
            break;
        }
    }

    return MTP_RESP_OK;
}

MTPResponseCode PropertyPod::getObjectPropDesc(MTPObjectFormatCategory category, MTPObjPropertyCode propCode, const MtpObjPropDesc*& propDesc)
{
    if ((propDesc = m_objPropMapCommon.value(propCode, 0))) {
        return MTP_RESP_OK;
    }

    switch (category) {
        case MTP_IMAGE_FORMAT:
            propDesc = m_objPropMapImage.value(propCode, 0);
            break;
        case MTP_AUDIO_FORMAT:
            propDesc = m_objPropMapAudio.value(propCode, 0);
            break;
        case MTP_VIDEO_FORMAT:
            propDesc = m_objPropMapVideo.value(propCode, 0);
            break;
        default:
            break;
    }

    return propDesc ? MTP_RESP_OK : MTP_RESP_Invalid_ObjectPropCode;
}

void PropertyPod::populateEnumDesc(MtpObjPropDesc* desc, MTPObjectFormatCategory category)
{
    if(0 != desc && MTP_FORM_FLAG_ENUM == desc->formFlag)
    {
        QVector<QVariant> v;
        // NOTE: The below properties have the same descriptions for all the categories that they are supported for. However, this may change by category in the future.
        switch(desc->uPropCode)
        {
            case MTP_OBJ_PROP_DRM_Status:
                {
                    v.append(QVariant(0x0000));
                    v.append(QVariant(0x0001));
                    desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                }
                break;
            case MTP_OBJ_PROP_Protection_Status:
                {
                    v.append(QVariant(MTP_PROTECTION_NoProtection));
                    v.append(QVariant(MTP_PROTECTION_ReadOnly));
                    v.append(QVariant(MTP_PROTECTION_ReadOnlyData));
                    v.append(QVariant(MTP_PROTECTION_NonTransferrableData));
                    desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                }
                break;
            case MTP_OBJ_PROP_Non_Consumable:
                {
                    v.append(QVariant(MTP_OBJECT_Consumable));
                    v.append(QVariant(MTP_OBJECT_NonConsumable));
                    desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                }
                break;
            case MTP_OBJ_PROP_Rep_Sample_Format:
                {
                    v.append(QVariant(MTP_OBF_FORMAT_JFIF));
                    v.append(QVariant(MTP_OBF_FORMAT_PNG));
                    desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                }
                break;
            case MTP_OBJ_PROP_Bitrate_Type:
                {
                    v.append(QVariant(MTP_BITRATE_TYPE_UNUSED));
                    v.append(QVariant(MTP_BITRATE_TYPE_DISCRETE));
                    v.append(QVariant(MTP_BITRATE_TYPE_VARIABLE));
                    v.append(QVariant(MTP_BITRATE_TYPE_FREE));
                    desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                }
                break;
            case MTP_OBJ_PROP_Sample_Rate:
                {
                    v.append(QVariant(8000));
                    v.append(QVariant(11025));
                    v.append(QVariant(22050));
                    v.append(QVariant(44000));
                    v.append(QVariant(44100));
                    v.append(QVariant(48000));
                    desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                }
                break;
            case MTP_OBJ_PROP_Nbr_Of_Channels:
                {
                    QVector<quint16> channels;
                    if(MTP_AUDIO_FORMAT == category)
                    {
                        channels = m_provider->audioChannels();
                    }
                    else if (MTP_VIDEO_FORMAT == category)
                    {
                        channels = m_provider->videoChannels();
                    }
                    for(int i = 0; i < channels.size(); i++)
                    {
                        v.append(QVariant(channels[i]));
                    }
                    desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                }
                break;
            case MTP_OBJ_PROP_Audio_WAVE_Codec:
                {
                    QVector<quint32> codecs = m_provider->supportedAudioCodecs();
                    for(int i = 0; i < codecs.size(); i++)
                    {
                        v.append(QVariant(codecs[i]));
                    }
                    desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                }
                break;
            case MTP_OBJ_PROP_Video_FourCC_Codec:
                {
                    v.append(QVariant(0x44495643));
                    v.append(QVariant(0x454C524D));
                    v.append(QVariant(0x3153534D));
                    v.append(QVariant(0x31564D57));
                    v.append(QVariant(0x33564D57));
                    v.append(QVariant(0x34363248));
                    desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                }
                break;
            case MTP_OBJ_PROP_Encoding_Profile:
                // This has one profile for now : SP@LL
                v.append(QVariant("SP@LL"));
                desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                break;
            case MTP_OBJ_PROP_Scan_Type:
                {
                    quint16 scanType = 0;
                    scanType = m_provider->videoScanType();
                    v.append(QVariant(scanType));
                    desc->formField = QVariant::fromValue(MtpEnumForm(v.size(), v));
                }
                break;
            default:
                break;
        }
    }
}

void PropertyPod::populateTechObjPropDesc(MtpObjPropDesc* desc, MTPObjectFormatCategory category)
{
    if(MTP_AUDIO_FORMAT == category || MTP_VIDEO_FORMAT == category || MTP_IMAGE_FORMAT == category)
    {
        switch(desc->uPropCode)
        {
            case MTP_OBJ_PROP_Audio_BitRate:
                {
                    desc->formField = QVariant::fromValue(MtpRangeForm(QVariant(m_provider->audioMinBitRate()), QVariant(m_provider->audioMaxBitRate()), QVariant(1)));
                }
                break;
            case MTP_OBJ_PROP_Video_BitRate:
                {
                    desc->formField = QVariant::fromValue(MtpRangeForm(QVariant(m_provider->videoMinBitRate()), QVariant(m_provider->videoMaxBitRate()), QVariant(1)));
                }
                break;
            case MTP_OBJ_PROP_KeyFrame_Distance:
                {
                    desc->formField = QVariant::fromValue(MtpRangeForm(QVariant(m_provider->videoMinKFD()), QVariant(m_provider->videoMaxKFD()), QVariant(1)));
                }
                break;
            case MTP_OBJ_PROP_Frames_Per_Thousand_Secs:
                {
                    desc->formField = QVariant::fromValue(MtpRangeForm(QVariant(m_provider->videoMinFPS()), QVariant(m_provider->videoMaxFPS()), QVariant(1)));
                }
                break;
            case MTP_OBJ_PROP_Width:
            case MTP_OBJ_PROP_Rep_Sample_Width:
                if(MTP_IMAGE_FORMAT == category)
                {
                    desc->formField = QVariant::fromValue(MtpRangeForm(QVariant(m_provider->imageMinWidth()), QVariant(m_provider->imageMaxWidth()), QVariant(1)));
                }
                else
                {
                    desc->formField = QVariant::fromValue(MtpRangeForm(QVariant(m_provider->videoMinWidth()), QVariant(m_provider->videoMaxWidth()), QVariant(1)));
                }
                break;
            case MTP_OBJ_PROP_Height:
            case MTP_OBJ_PROP_Rep_Sample_Height:
                if(MTP_IMAGE_FORMAT == category)
                {
                    desc->formField = QVariant::fromValue(MtpRangeForm(QVariant(m_provider->imageMinHeight()), QVariant(m_provider->imageMaxHeight()), QVariant(1)));
                }
                else
                {
                    desc->formField = QVariant::fromValue(MtpRangeForm(QVariant(m_provider->videoMinHeight()), QVariant(m_provider->videoMaxHeight()), QVariant(1)));
                }
                break;
            default:
                break;
        }
    }
}

bool PropertyPod::isTechObjProp(MTPObjPropertyCode code)
{
    bool ret = false;
    switch(code)
    {
        case MTP_OBJ_PROP_Audio_WAVE_Codec:
        case MTP_OBJ_PROP_Audio_BitRate:
        case MTP_OBJ_PROP_Video_BitRate:
        case MTP_OBJ_PROP_KeyFrame_Distance:
        case MTP_OBJ_PROP_Frames_Per_Thousand_Secs:
        case MTP_OBJ_PROP_Nbr_Of_Channels:
        case MTP_OBJ_PROP_Encoding_Profile:
        case MTP_OBJ_PROP_Width:
        case MTP_OBJ_PROP_Height:
        case MTP_OBJ_PROP_Rep_Sample_Width:
        case MTP_OBJ_PROP_Rep_Sample_Height:
            ret = true;
            break;
        default:
            break;
    }
    return ret;
}

