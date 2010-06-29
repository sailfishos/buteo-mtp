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


#ifndef MTP_TYPES_H
#define MTP_TYPES_H

#include <QString>
#include <QVector>
#include <QByteArray>
#include <QVariant>
#include <QDebug>

#define     MTP_INITIAL_SESSION_ID       0x00000000
#define     MTP_NO_SESSION_ID            0xFFFFFFFF
#define     MTP_NO_TRANSACTION_ID        0xFFFFFFFF

enum TransportType
{
    INVALID = 0,
    USB = 1,
    DUMMY = 2
};

typedef quint32 ObjHandle;
 
/* object format category */
#define     MTP_UNSUPPORTED_FORMAT 0x0001
#define     MTP_AUDIO_FORMAT 0x0002
#define     MTP_VIDEO_FORMAT 0x0003
#define     MTP_IMAGE_FORMAT 0x0004
#define     MTP_COMMON_FORMAT 0x0005

typedef quint16 MTPObjectFormatCategory;
    

/* data types */
#define MTP_DATA_TYPE_UNDEF     0x0000
#define MTP_DATA_TYPE_INT8      0x0001
#define MTP_DATA_TYPE_UINT8     0x0002
#define MTP_DATA_TYPE_INT16     0x0003
#define MTP_DATA_TYPE_UINT16    0x0004
#define MTP_DATA_TYPE_INT32     0x0005
#define MTP_DATA_TYPE_UINT32    0x0006
#define MTP_DATA_TYPE_INT64     0x0007
#define MTP_DATA_TYPE_UINT64    0x0008
#define MTP_DATA_TYPE_INT128    0x0009
#define MTP_DATA_TYPE_UINT128   0x000A
#define MTP_DATA_TYPE_AINT8     0x4001
#define MTP_DATA_TYPE_AUINT8    0x4002
#define MTP_DATA_TYPE_AINT16    0x4003
#define MTP_DATA_TYPE_AUINT16   0x4004
#define MTP_DATA_TYPE_AINT32    0x4005
#define MTP_DATA_TYPE_AUINT32   0x4006
#define MTP_DATA_TYPE_AINT64    0x4007
#define MTP_DATA_TYPE_AUINT64   0x4008
#define MTP_DATA_TYPE_AINT128   0x4009
#define MTP_DATA_TYPE_AUINT128  0x400A
#define MTP_DATA_TYPE_STR       0xFFFF

typedef quint16 MTPDataType;

/* operation codes */
#define MTP_OP_GetDeviceInfo                    0x1001
#define MTP_OP_OpenSession                      0x1002
#define MTP_OP_CloseSession                     0x1003
#define MTP_OP_GetStorageIDs                    0x1004
#define MTP_OP_GetStorageInfo                   0x1005
#define MTP_OP_GetNumObjects                    0x1006
#define MTP_OP_GetObjectHandles                 0x1007
#define MTP_OP_GetObjectInfo                    0x1008
#define MTP_OP_GetObject                        0x1009
#define MTP_OP_GetThumb                         0x100A
#define MTP_OP_DeleteObject                     0x100B
#define MTP_OP_SendObjectInfo                   0x100C
#define MTP_OP_SendObject                       0x100D
#define MTP_OP_InitiateCapture                  0x100E
#define MTP_OP_FormatStore                      0x100F
#define MTP_OP_ResetDevice                      0x1010
#define MTP_OP_SelfTest                         0x1011
#define MTP_OP_SetObjectProtection              0x1012
#define MTP_OP_PowerDown                        0x1013
#define MTP_OP_GetDevicePropDesc                0x1014
#define MTP_OP_GetDevicePropValue               0x1015
#define MTP_OP_SetDevicePropValue               0x1016
#define MTP_OP_ResetDevicePropValue             0x1017
#define MTP_OP_TerminateOpenCapture             0x1018
#define MTP_OP_MoveObject                       0x1019
#define MTP_OP_CopyObject                       0x101A
#define MTP_OP_GetPartialObject                 0x101B
#define MTP_OP_InitiateOpenCapture              0x101C
#define MTP_OP_GetObjectPropsSupported          0x9801
#define MTP_OP_GetObjectPropDesc                0x9802
#define MTP_OP_GetObjectPropValue               0x9803
#define MTP_OP_SetObjectPropValue               0x9804
#define MTP_OP_GetObjectPropList                0x9805
#define MTP_OP_SetObjectPropList                0x9806
#define MTP_OP_GetInterdependentPropDesc        0x9807
#define MTP_OP_SendObjectPropList               0x9808
#define MTP_OP_GetObjectReferences              0x9810
#define MTP_OP_SetObjectReferences              0x9811
#define MTP_OP_Skip                             0x9820

typedef quint16 MTPOperationCode;

#define MTP_RESP_Undefined                               0x2000
#define MTP_RESP_OK                                      0x2001
#define MTP_RESP_GeneralError                            0x2002
#define MTP_RESP_SessionNotOpen                          0x2003
#define MTP_RESP_InvalidTransID                          0x2004
#define MTP_RESP_OperationNotSupported                   0x2005
#define MTP_RESP_ParameterNotSupported                   0x2006
#define MTP_RESP_IncompleteTransfer                      0x2007
#define MTP_RESP_InvalidStorageID                        0x2008
#define MTP_RESP_InvalidObjectHandle                     0x2009
#define MTP_RESP_DevicePropNotSupported                  0x200A
#define MTP_RESP_InvalidObjectFormatCode                 0x200B
#define MTP_RESP_StoreFull                               0x200C
#define MTP_RESP_ObjectWriteProtected                    0x200D
#define MTP_RESP_StoreReadOnly                           0x200E
#define MTP_RESP_AccessDenied                            0x200F
#define MTP_RESP_NoThumbnailPresent                      0x2010
#define MTP_RESP_SelfTestFailed                          0x2011
#define MTP_RESP_PartialDeletion                         0x2012
#define MTP_RESP_StoreNotAvailable                       0x2013
#define MTP_RESP_SpecByFormatUnsupported                 0x2014
#define MTP_RESP_NoValidObjectInfo                       0x2015
#define MTP_RESP_InvalidCodeFormat                       0x2016
#define MTP_RESP_UnknowVendorCode                        0x2017
#define MTP_RESP_CaptureAlreadyTerminated                0x2018
#define MTP_RESP_DeviceBusy                              0x2019
#define MTP_RESP_InvalidParentObject                     0x201A
#define MTP_RESP_InvalidDevicePropFormat                 0x201B
#define MTP_RESP_InvalidDevicePropValue                  0x201C
#define MTP_RESP_InvalidParameter                        0x201D
#define MTP_RESP_SessionAlreadyOpen                      0x201E
#define MTP_RESP_TransactionCancelled                    0x201F
#define MTP_RESP_SpecificationOfDestinationUnsupported   0x2020
#define MTP_RESP_Invalid_ObjectPropCode                  0xA801
#define MTP_RESP_Invalid_ObjectProp_Format               0xA802
#define MTP_RESP_Invalid_ObjectProp_Value                0xA803
#define MTP_RESP_Invalid_ObjectReference                 0xA804
#define MTP_RESP_Invalid_Dataset                         0xA806
#define MTP_RESP_Specification_By_Group_Unsupported      0xA807
#define MTP_RESP_Specification_By_Depth_Unsupported      0xA808
#define MTP_RESP_Object_Too_Large                        0xA809
#define MTP_RESP_ObjectProp_Not_Supported                0xA80A

    
typedef quint16 MTPResponseCode;

/* event codes */
#define MTP_EV_Undefined                    0x4000
#define MTP_EV_CancelTransaction            0x4001
#define MTP_EV_ObjectAdded                  0x4002
#define MTP_EV_ObjectRemoved                0x4003
#define MTP_EV_StoreAdded                   0x4004
#define MTP_EV_StoreRemoved                 0x4005
#define MTP_EV_DevicePropChanged            0x4006
#define MTP_EV_ObjectInfoChanged            0x4007
#define MTP_EV_DeviceInfoChanged            0x4008
#define MTP_EV_RequestObjectTransfer        0x4009
#define MTP_EV_StoreFull                    0x400A
#define MTP_EV_DeviceReset                  0x400B
#define MTP_EV_StorageInfoChanged           0x400C
#define MTP_EV_CaptureComplete              0x400D
#define MTP_EV_UnreportedStatus             0x400E
#define MTP_EV_ObjectPropChanged        0xC801
#define MTP_EV_ObjectPropDescChanged        0xC802

typedef quint16 MTPEventCode;
    

/* container types */
#define MTP_CONTAINER_TYPE_UNDEFINED     0x0000
#define MTP_CONTAINER_TYPE_COMMAND       0x0001           
#define MTP_CONTAINER_TYPE_DATA          0x0002
#define MTP_CONTAINER_TYPE_RESPONSE      0x0003
#define MTP_CONTAINER_TYPE_EVENT         0x0004

typedef quint16 MTPContainerType;


#define MTP_HEADER_SIZE  (2*sizeof(quint32) + 2*sizeof(quint16) )
#define MTP_STORAGE_INFO_SIZE ((2 * sizeof(quint64)) +\
                               (1 * sizeof(quint32)) +\
                               (2 * sizeof(quint8)) +\
                               (3 * sizeof(quint16)))

#define    MTP_PROTECTION_NoProtection          0x0000
#define    MTP_PROTECTION_ReadOnly              0x0001
#define    MTP_PROTECTION_ReadOnlyData          0x8002
#define    MTP_PROTECTION_NonTransferrableData  0x8003

typedef quint16 MTPProtectStatus;

/* storage types */
#define    MTP_STORAGE_TYPE_Undefined       0x0000
#define    MTP_STORAGE_TYPE_FixedROM        0x0001
#define    MTP_STORAGE_TYPE_RemovableROM    0x0002
#define    MTP_STORAGE_TYPE_FixedRAM        0x0003
#define    MTP_STORAGE_TYPE_RemovableRAM    0x0004
typedef quint16 MTPStorageType;

/* storage access capability */
#define    MTP_STORAGE_ACCESS_ReadWrite       0x0000
#define    MTP_STORAGE_ACCESS_ReadOnly_NoDel  0x0001
#define    MTP_STORAGE_ACCESS_ReadOnly_Del    0x0002

typedef quint16 MTPStorageAccess;

/* file system types */
#define    MTP_FILE_SYSTEM_TYPE_Undefined    0x0000
#define    MTP_FILE_SYSTEM_TYPE_GenFlat      0x0001
#define    MTP_FILE_SYSTEM_TYPE_GenHier      0x0002
#define    MTP_FILE_SYSTEM_TYPE_DCF          0x0003

typedef quint16 MTPFileSystemType;

/* non-consumable values */
#define    MTP_OBJECT_Consumable             0x00
#define    MTP_OBJECT_NonConsumable          0x01
typedef quint16 MTPNonConsumableValue;

/* DRM status values */
#define    MTP_OBJECT_NoDrmProtection        0x0000
#define    MTP_OBJECT_DrmProtected           0x0001
typedef quint16 MTPDrmStatus;

/* device property codes */
#define MTP_DEV_PROPERTY_Undefined                      0x5000
#define MTP_DEV_PROPERTY_BatteryLevel                   0x5001
#define MTP_DEV_PROPERTY_FunctionalMode                 0x5002
#define MTP_DEV_PROPERTY_ImageSize                      0x5003
#define MTP_DEV_PROPERTY_CompressionSetting             0x5004
#define MTP_DEV_PROPERTY_WhiteBalance                   0x5005
#define MTP_DEV_PROPERTY_RGB_Gain                       0x5006
#define MTP_DEV_PROPERTY_F_Number                       0x5007
#define MTP_DEV_PROPERTY_FocalLength                    0x5008
#define MTP_DEV_PROPERTY_FocusDistance                  0x5009
#define MTP_DEV_PROPERTY_FocusMod                       0x500A
#define MTP_DEV_PROPERTY_ExposureMeteringMode           0x500B
#define MTP_DEV_PROPERTY_FlashMode                      0x500C
#define MTP_DEV_PROPERTY_ExposureTime                   0x500D
#define MTP_DEV_PROPERTY_ExposureProgramMode            0x500E
#define MTP_DEV_PROPERTY_ExposureInde                   0x500F
#define MTP_DEV_PROPERTY_ExposureBiasCompensation       0x5010
#define MTP_DEV_PROPERTY_DateTime                       0x5011
#define MTP_DEV_PROPERTY_CaptureDelay                   0x5012
#define MTP_DEV_PROPERTY_StillCaptureMode               0x5013
#define MTP_DEV_PROPERTY_Contrast                       0x5014
#define MTP_DEV_PROPERTY_Sharpness                      0x5015
#define MTP_DEV_PROPERTY_DigitalZoom                    0x5016
#define MTP_DEV_PROPERTY_EffectMode                     0x5017
#define MTP_DEV_PROPERTY_BurstNumber                    0x5018
#define MTP_DEV_PROPERTY_BurstInterval                  0x5019
#define MTP_DEV_PROPERTY_TimelapseNumber                0x501A
#define MTP_DEV_PROPERTY_TimelapseInterval              0x501B
#define MTP_DEV_PROPERTY_FocusMeteringMode              0x501C
#define MTP_DEV_PROPERTY_UploadURL                      0x501D
#define MTP_DEV_PROPERTY_Artist                         0x501E
#define MTP_DEV_PROPERTY_CopyrightInfo                  0x501F
#define MTP_DEV_PROPERTY_Synchronization_Partner        0xD401
#define MTP_DEV_PROPERTY_Device_Friendly_Name           0xD402
#define MTP_DEV_PROPERTY_Volume                         0xD403
#define MTP_DEV_PROPERTY_Supported_Formats_Ordered      0xD404
#define MTP_DEV_PROPERTY_Perceived_Device_Type          0xD407

typedef quint16 MTPDevPropertyCode;

/* object format codes */
    
#define MTP_OBF_FORMAT_Undefined                       0x3000
#define MTP_OBF_FORMAT_Association                     0x3001
#define MTP_OBF_FORMAT_Script                          0x3002
#define MTP_OBF_FORMAT_Executable                      0x3003
#define MTP_OBF_FORMAT_Text                            0x3004
#define MTP_OBF_FORMAT_HTML                            0x3005
#define MTP_OBF_FORMAT_DPOF                            0x3006
#define MTP_OBF_FORMAT_AIFF                            0x3007  
#define MTP_OBF_FORMAT_WAV                             0x3008  
#define MTP_OBF_FORMAT_MP3                             0x3009 
#define MTP_OBF_FORMAT_AVI                             0x300A  
#define MTP_OBF_FORMAT_MPEG                            0x300B  
#define MTP_OBF_FORMAT_ASF                             0x300C
#define MTP_OBF_FORMAT_Unknown_Image_Object            0x3800
#define MTP_OBF_FORMAT_EXIF_JPEG                       0x3801
#define MTP_OBF_FORMAT_TIFF_EP                         0x3802
#define MTP_OBF_FORMAT_FlashPix                        0x3803
#define MTP_OBF_FORMAT_BMP                             0x3804
#define MTP_OBF_FORMAT_CIFF                            0x3805
/* 0x3806 Undefined Reserved */
#define MTP_OBF_FORMAT_GIF                             0x3807 
#define MTP_OBF_FORMAT_JFIF                            0x3808
#define MTP_OBF_FORMAT_PCD                             0x3809
#define MTP_OBF_FORMAT_PICT                            0x380A
#define MTP_OBF_FORMAT_PNG                             0x380B
/* 0x380C Undefined  Reserved */
#define MTP_OBF_FORMAT_TIFF                            0x380D
#define MTP_OBF_FORMAT_TIFF_IT                         0x380E
#define MTP_OBF_FORMAT_JP2                             0x380F   
#define MTP_OBF_FORMAT_JPX                             0x3810
#define MTP_OBF_FORMAT_Undefined_Firmware           0xB802
#define MTP_OBF_FORMAT_Windows_Image_Format           0xB881
#define MTP_OBF_FORMAT_Undefined_Audio               0xB900
#define MTP_OBF_FORMAT_WMA                             0xB901
#define MTP_OBF_FORMAT_OGG                             0xB902
#define MTP_OBF_FORMAT_AAC                             0xB903
#define MTP_OBF_FORMAT_Audible                         0xB904
#define MTP_OBF_FORMAT_FLAC                            0xB906
#define MTP_OBF_FORMAT_Undefined_Video               0xB980
#define MTP_OBF_FORMAT_WMV                             0xB981
#define MTP_OBF_FORMAT_MP4_Container               0xB982
#define MTP_OBF_FORMAT_3GP_Container               0xB984
#define MTP_OBF_FORMAT_Undefined_Collection           0xBA00
#define MTP_OBF_FORMAT_Abstract_Multimedia_Album       0xBA01
#define MTP_OBF_FORMAT_Abstract_Image_Album           0xBA02
#define MTP_OBF_FORMAT_Abstract_Audio_Album           0xBA03
#define MTP_OBF_FORMAT_Abstract_Video_Album           0xBA04
#define MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist   0xBA05
#define MTP_OBF_FORMAT_Abstract_Contact_Group          0xBA06
#define MTP_OBF_FORMAT_Abstract_Message_Folder         0xBA07
#define MTP_OBF_FORMAT_Abstract_Chaptered_Production   0xBA08
#define MTP_OBF_FORMAT_Abstract_Audio_Playlist       0xBA09
#define MTP_OBF_FORMAT_Abstract_Video_Playlist       0xBA0A
#define MTP_OBF_FORMAT_WPL_Playlist                    0xBA10
#define MTP_OBF_FORMAT_M3U_Playlist                    0xBA11
#define MTP_OBF_FORMAT_MPL_Playlist                    0xBA12
#define MTP_OBF_FORMAT_ASX_Playlist                    0xBA13
#define MTP_OBF_FORMAT_PLS_Playlist                   0xBA14
#define MTP_OBF_FORMAT_Undefined_Document           0xBA80
#define MTP_OBF_FORMAT_Abstract_Document           0xBA81
#define MTP_OBF_FORMAT_Undefined_Message               0xBB00
#define MTP_OBF_FORMAT_Abstract_Message                0xBB01
#define MTP_OBF_FORMAT_Undefined_Contact               0xBB80
#define MTP_OBF_FORMAT_Abstract_Contact                0xBB81
#define MTP_OBF_FORMAT_vCard2                          0xBB82
#define MTP_OBF_FORMAT_vCard3                          0xBB83
#define MTP_OBF_FORMAT_Undefined_Calendar_Item         0xBE00
#define MTP_OBF_FORMAT_Abstract_Calendar_Item          0xBE01
#define MTP_OBF_FORMAT_vCal1                           0xBE02
#define MTP_OBF_FORMAT_vCal2                           0xBE03
#define MTP_OBF_FORMAT_Undefined_Windows_Executable   0xBE80

typedef quint16 MTPObjFormatCode;

#define MTP_OBJ_PROP_StorageID                    0xDC01
#define MTP_OBJ_PROP_Obj_Format                     0xDC02
#define MTP_OBJ_PROP_Protection_Status            0xDC03
#define MTP_OBJ_PROP_Obj_Size                       0xDC04
#define MTP_OBJ_PROP_Association_Type            0xDC05
#define MTP_OBJ_PROP_Association_Desc            0xDC06
#define MTP_OBJ_PROP_Obj_File_Name                0xDC07
#define MTP_OBJ_PROP_Date_Created                   0xDC08
#define MTP_OBJ_PROP_Date_Modified                0xDC09
#define MTP_OBJ_PROP_Keywords                    0xDC0A
#define MTP_OBJ_PROP_Parent_Obj                     0xDC0B
#define MTP_OBJ_PROP_Allowed_Folder_Contents    0xDC0C
#define MTP_OBJ_PROP_Hidden                         0xDC0D
#define MTP_OBJ_PROP_Sys_Obj                        0xDC0E
#define MTP_OBJ_PROP_Persistent_Unique_ObjId        0xDC41
#define MTP_OBJ_PROP_SyncID                         0xDC42
#define MTP_OBJ_PROP_Property_Bag                   0xDC43
#define MTP_OBJ_PROP_Name                           0xDC44
#define MTP_OBJ_PROP_Created_By                     0xDC45
#define MTP_OBJ_PROP_Artist                         0xDC46
#define MTP_OBJ_PROP_Date_Authored                  0xDC47
#define MTP_OBJ_PROP_Description                    0xDC48
#define MTP_OBJ_PROP_URL_Reference                  0xDC49
#define MTP_OBJ_PROP_Language_Locale                0xDC4A
#define MTP_OBJ_PROP_Copyright_Information          0xDC4B
#define MTP_OBJ_PROP_Source                         0xDC4C
#define MTP_OBJ_PROP_Origin_Location                0xDC4D
#define MTP_OBJ_PROP_Date_Added                     0xDC4E
#define MTP_OBJ_PROP_Non_Consumable                 0xDC4F
#define MTP_OBJ_PROP_Corrupt_Unplayable            0xDC50
#define MTP_OBJ_PROP_Rep_Sample_Format              0xDC81
#define MTP_OBJ_PROP_Rep_Sample_Size                0xDC82
#define MTP_OBJ_PROP_Rep_Sample_Height              0xDC83
#define MTP_OBJ_PROP_Rep_Sample_Width               0xDC84
#define MTP_OBJ_PROP_Rep_Sample_Duration            0xDC85
#define MTP_OBJ_PROP_Rep_Sample_Data                0xDC86
#define MTP_OBJ_PROP_Width                        0xDC87
#define MTP_OBJ_PROP_Height                        0xDC88
#define MTP_OBJ_PROP_Duration                    0xDC89
#define MTP_OBJ_PROP_Rating                        0xDC8A
#define MTP_OBJ_PROP_Track                        0xDC8B
#define MTP_OBJ_PROP_Genre                        0xDC8C
#define MTP_OBJ_PROP_Credits                    0xDC8D
#define MTP_OBJ_PROP_Lyrics                        0xDC8E
#define MTP_OBJ_PROP_Subscription_Content_ID    0xDC8F
#define MTP_OBJ_PROP_Produced_By                0xDC90
#define MTP_OBJ_PROP_Use_Count                    0xDC91
#define MTP_OBJ_PROP_Skip_Count                    0xDC92
#define MTP_OBJ_PROP_Last_Accessed                0xDC93
#define MTP_OBJ_PROP_Parental_Rating            0xDC94
#define MTP_OBJ_PROP_Meta_Genre                    0xDC95
#define MTP_OBJ_PROP_Composer                    0xDC96
#define MTP_OBJ_PROP_Effective_Rating            0xDC97
#define MTP_OBJ_PROP_Subtitle                    0xDC98
#define MTP_OBJ_PROP_Original_Release_Date        0xDC99
#define MTP_OBJ_PROP_Album_Name                    0xDC9A
#define MTP_OBJ_PROP_Album_Artist                0xDC9B
#define MTP_OBJ_PROP_Mood                        0xDC9C
#define MTP_OBJ_PROP_DRM_Status                    0xDC9D
#define MTP_OBJ_PROP_Sub_Description            0xDC9E
#define MTP_OBJ_PROP_Is_Cropped                    0xDCD1
#define MTP_OBJ_PROP_Is_Colour_Corrected        0xDCD2
#define MTP_OBJ_PROP_Exposure_Time            0xDCD5
#define MTP_OBJ_PROP_Exposure_Index            0xDCD6
#define MTP_OBJ_PROP_Total_BitRate                0xDE91
#define MTP_OBJ_PROP_Bitrate_Type                0xDE92
#define MTP_OBJ_PROP_Sample_Rate                0xDE93
#define MTP_OBJ_PROP_Nbr_Of_Channels            0xDE94
#define MTP_OBJ_PROP_Audio_BitDepth                0xDE95
#define MTP_OBJ_PROP_Scan_Type                    0xDE97
#define MTP_OBJ_PROP_Audio_WAVE_Codec            0xDE99
#define MTP_OBJ_PROP_Audio_BitRate                0xDE9A
#define MTP_OBJ_PROP_Video_FourCC_Codec            0xDE9B
#define MTP_OBJ_PROP_Video_BitRate                0xDE9C
#define MTP_OBJ_PROP_Frames_Per_Thousand_Secs       0xDE9D
#define MTP_OBJ_PROP_KeyFrame_Distance            0xDE9E
#define MTP_OBJ_PROP_Buffer_Size                0xDE9F
#define MTP_OBJ_PROP_Encoding_Quality            0xDEA0
#define MTP_OBJ_PROP_Encoding_Profile            0xDEA1
#define MTP_OBJ_PROP_Display_Name                0xDCE0
#define MTP_OBJ_PROP_Body_Text                    0xDCE1
#define MTP_OBJ_PROP_Subject                    0xDCE2
#define MTP_OBJ_PROP_Priority                    0xDCE3
#define MTP_OBJ_PROP_Given_Name                    0xDD00
#define MTP_OBJ_PROP_Middle_Names                0xDD01
#define MTP_OBJ_PROP_Family_Name                0xDD02
#define MTP_OBJ_PROP_Prefix                        0xDD03
#define MTP_OBJ_PROP_Suffix                        0xDD04
#define MTP_OBJ_PROP_Phonetic_Given_Name        0xDD05
#define MTP_OBJ_PROP_Phonetic_Family_Name        0xDD06
#define MTP_OBJ_PROP_Email_Primary                0xDD07
#define MTP_OBJ_PROP_Email_Personal1            0xDD08
#define MTP_OBJ_PROP_Email_Personal2            0xDD09
#define MTP_OBJ_PROP_Email_Business1            0xDD0A
#define MTP_OBJ_PROP_Email_Business2            0xDD0B
#define MTP_OBJ_PROP_Email_Others                0xDD0C
#define MTP_OBJ_PROP_Phone_Nbr_Primary              0xDD0D
#define MTP_OBJ_PROP_Phone_Nbr_Personal             0xDD0E
#define MTP_OBJ_PROP_Phone_Nbr_Personal2        0xDD0F
#define MTP_OBJ_PROP_Phone_Nbr_Business             0xDD10
#define MTP_OBJ_PROP_Phone_Nbr_Business2        0xDD11
#define MTP_OBJ_PROP_Phone_Nbr_Mobile               0xDD12
#define MTP_OBJ_PROP_Phone_Nbr_Mobile2              0xDD13
#define MTP_OBJ_PROP_Fax_Nbr_Primary            0xDD14
#define MTP_OBJ_PROP_Fax_Nbr_Personal               0xDD15
#define MTP_OBJ_PROP_Fax_Nbr_Business               0xDD16
#define MTP_OBJ_PROP_Pager_Nbr                      0xDD17
#define MTP_OBJ_PROP_Phone_Nbr_Others               0xDD18
#define MTP_OBJ_PROP_Primary_Web_Addr               0xDD19
#define MTP_OBJ_PROP_Personal_Web_Addr            0xDD1A
#define MTP_OBJ_PROP_Business_Web_Addr              0xDD1B
#define MTP_OBJ_PROP_Instant_Messenger_Addr        0xDD1C
#define MTP_OBJ_PROP_Instant_Messenger_Addr2    0xDD1D
#define MTP_OBJ_PROP_Instant_Messenger_Addr3    0xDD1E
#define MTP_OBJ_PROP_Post_Addr_Personal_Full        0xDD1F
#define MTP_OBJ_PROP_Post_Addr_Personal_Line1       0xDD20
#define MTP_OBJ_PROP_Post_Addr_Personal_Line2       0xDD21
#define MTP_OBJ_PROP_Post_Addr_Personal_City        0xDD22
#define MTP_OBJ_PROP_Post_Addr_Personal_Region      0xDD23
#define MTP_OBJ_PROP_Post_Addr_Personal_Post_Code   0xDD24
#define MTP_OBJ_PROP_Post_Addr_Personal_Country     0xDD25
#define MTP_OBJ_PROP_Post_Addr_Business_Full    0xDD26
#define MTP_OBJ_PROP_Post_Addr_Business_Line1    0xDD27
#define MTP_OBJ_PROP_Post_Addr_Business_Line2    0xDD28
#define MTP_OBJ_PROP_Post_Addr_Business_City    0xDD29
#define MTP_OBJ_PROP_Post_Addr_Business_Region    0xDD2A
#define MTP_OBJ_PROP_Post_Addr_Business_Post_Code   0xDD2B
#define MTP_OBJ_PROP_Post_Addr_Business_Country     0xDD2C
#define MTP_OBJ_PROP_Post_Addr_Other_Full           0xDD2D
#define MTP_OBJ_PROP_Post_Addr_Other_Line1          0xDD2E
#define MTP_OBJ_PROP_Post_Addr_Other_Line2          0xDD2F
#define MTP_OBJ_PROP_Post_Addr_Other_City           0xDD30
#define MTP_OBJ_PROP_Post_Addr_Other_Region         0xDD31
#define MTP_OBJ_PROP_Post_Addr_Other_Post_Code      0xDD32
#define MTP_OBJ_PROP_Post_Addr_Other_Country        0xDD33
#define MTP_OBJ_PROP_Organization_Name              0xDD34
#define MTP_OBJ_PROP_Phonetic_Organization_Name     0xDD35
#define MTP_OBJ_PROP_Role                           0xDD36
#define MTP_OBJ_PROP_Birthdate                      0xDD37
#define MTP_OBJ_PROP_Message_To                     0xDD40
#define MTP_OBJ_PROP_Message_CC                     0xDD41
#define MTP_OBJ_PROP_Message_BCC                    0xDD42
#define MTP_OBJ_PROP_Message_Read                   0xDD43
#define MTP_OBJ_PROP_Message_Received_Time          0xDD44
#define MTP_OBJ_PROP_Message_Sender                 0xDD45
#define MTP_OBJ_PROP_Activity_Begin_Time            0xDD50
#define MTP_OBJ_PROP_Activity_End_Time              0xDD51
#define MTP_OBJ_PROP_Activity_Location              0xDD52
#define MTP_OBJ_PROP_Activity_Required_Attendees    0xDD54
#define MTP_OBJ_PROP_Activity_Optional_Attendees    0xDD55
#define MTP_OBJ_PROP_Activity_Resources             0xDD56
#define MTP_OBJ_PROP_Activity_Accepted              0xDD57
#define MTP_OBJ_PROP_Activity_Tentative             0xDD58
#define MTP_OBJ_PROP_Activity_Declined              0xDD59
#define MTP_OBJ_PROP_Activity_Reminder_Time         0xDD5A
#define MTP_OBJ_PROP_Activity_Owner                 0xDD5B
#define MTP_OBJ_PROP_Activity_Status                0xDD5C
#define MTP_OBJ_PROP_Media_GUID                     0xDD72
#define MTP_OBJ_PROP_Purchase_Album                 0xD901

typedef quint16 MTPObjPropertyCode;

/* object property forms. Used in the object property describing dataset */
#define MTP_OBJ_PROP_FORM_None                  0x00
#define MTP_OBJ_PROP_FORM_Range                 0x01
#define MTP_OBJ_PROP_FORM_Enumeration           0x02
#define MTP_OBJ_PROP_FORM_DateTime              0x03
#define MTP_OBJ_PROP_FORM_Fixed_length_Array    0x04
#define MTP_OBJ_PROP_FORM_Regular_Expression    0x05
#define MTP_OBJ_PROP_FORM_ByteArray             0x06
#define MTP_OBJ_PROP_FORM_LongString            0xFF

typedef quint8 MTPObjPropForm;

/* association types */
#define MTP_ASSOCIATION_TYPE_Undefined            0x0000
#define MTP_ASSOCIATION_TYPE_GenFolder            0x0001
#define MTP_ASSOCIATION_TYPE_Album                0x0002
#define MTP_ASSOCIATION_TYPE_TimeSeq              0x0003
#define MTP_ASSOCIATION_TYPE_HorizontalPanoramic  0x0004
#define MTP_ASSOCIATION_TYPE_VerticalPanoramic    0x0005
#define MTP_ASSOCIATION_TYPE_2DPanoramic          0x0006
#define MTP_ASSOCIATION_TYPE_AncillaryData        0x0007

typedef quint16 MTPAssociationType;



#define    MTP_BITRATE_TYPE_UNUSED              0x0000
#define    MTP_BITRATE_TYPE_DISCRETE            0x0001
#define    MTP_BITRATE_TYPE_VARIABLE            0x0002
#define    MTP_BITRATE_TYPE_FREE                0x0003

typedef quint16 MTP_BitrateType;


#define MTP_CH_CONF_UNUSED      0x0000
#define MTP_CH_CONF_MONO        0x0001
#define MTP_CH_CONF_STEREO      0x0002
#define MTP_CH_CONF_2_1_CH      0x0003
#define MTP_CH_CONF_3_CH        0x0004
#define MTP_CH_CONF_3_1_CH      0x0005
#define MTP_CH_CONF_4_CH        0x0006
#define MTP_CH_CONF_4_1_CH      0x0007
#define MTP_CH_CONF_5_CH        0x0008
#define MTP_CH_CONF_5_1_CH      0x0009
#define MTP_CH_CONF_6_CH        0x000A
#define MTP_CH_CONF_6_1_CH      0x000B
#define MTP_CH_CONF_7_CH        0x000C
#define MTP_CH_CONF_7_1_CH      0x000D
#define MTP_CH_CONF_8_CH        0x000E
#define MTP_CH_CONF_8_1_CH      0x000F
#define MTP_CH_CONF_9_CH        0x0010
#define MTP_CH_CONF_9_1_CH      0x0011
#define MTP_CH_CONF_5_2_CH      0x0012
#define MTP_CH_CONF_6_2_CH      0x0013
#define MTP_CH_CONF_7_2_CH      0x0014
#define MTP_CH_CONF_8_2_CH      0x0015
#define MTP_CCH_CONF_9_2_CH     0x0016

typedef quint16 MTPChannelConf;

typedef quint32 MtpParam;

struct MtpRequest
{
  MTPOperationCode opCode;      // The operation code for the request
  QVector<MtpParam> params;     // Request parameters (size must be no more than 5)
  quint8 *data;                 // Request data 
  quint32 dataLen;              // Data length
  MtpRequest() : opCode(0), data(0), dataLen(0) {}     // Constructor
};

struct MtpResponse
{
  MTPResponseCode   respCode;   // The response code
  QVector<MtpParam> params;     // Response parameters (size must be no more than 5)
  quint8 *data;                 // Response data
  quint32 dataLen;              // Data length
  MtpResponse() :               // Constructor
    respCode(MTP_RESP_OperationNotSupported),
    data(0), dataLen(0)
  {
  }
};

#define MTP_FORM_FLAG_NONE          0x00
#define MTP_FORM_FLAG_RANGE         0x01
#define MTP_FORM_FLAG_ENUM          0x02
#define MTP_FORM_FLAG_DATE_TIME     0x03
#define MTP_FORM_FLAG_FIXED_ARRAY   0x04
#define MTP_FORM_FLAG_REGEX         0x05
#define MTP_FORM_FLAG_BYTE_ARRAY    0x06
#define MTP_FORM_FLAG_LONG_STRING   0xFF

typedef quint8 MtpFormFlag;

struct MTPStorageInfo
{
    MTPStorageType storageType;
    MTPFileSystemType filesystemType;
    MTPStorageAccess accessCapability;
    quint64 maxCapacity;
    quint64 freeSpace;
    quint32 freeSpaceInObjects;
    QString storageDescription;
    QString volumeLabel;
    MTPStorageInfo() : storageType(MTP_STORAGE_TYPE_Undefined), filesystemType(MTP_FILE_SYSTEM_TYPE_Undefined),
                       accessCapability(MTP_STORAGE_ACCESS_ReadOnly_NoDel), maxCapacity(0), freeSpace(0),
                       freeSpaceInObjects(0xFFFFFFFF){}
};

struct MtpRangeForm
{
  QVariant minValue;
  QVariant maxValue;
  QVariant stepSize;
  MtpRangeForm(QVariant v1, QVariant v2, QVariant v3) : minValue(v1), maxValue(v2), stepSize(v3) {}
  MtpRangeForm(){}
};

Q_DECLARE_METATYPE(MtpRangeForm);

struct MtpEnumForm
{
  quint16 uTotal; // Total number of enumeration values
  QVector<QVariant> values;
  MtpEnumForm() : uTotal(0) {}
  MtpEnumForm(quint16 total, const QVector<QVariant>& vals) : uTotal(total), values(vals) {}
};

Q_DECLARE_METATYPE(MtpEnumForm);

struct MtpInt128
{
    char val[16]; // 16 bytes in LE

    // Default Constructor
    MtpInt128()
    {
        memset(val, 0, sizeof(val));
    }

    // Constructor taking upper and lower 8 bytes.
    MtpInt128( quint64 lower, quint64 upper = 0 )
    {
        for(qint32 i = 0; i < 8; i++)
        {
            val[i] = lower & 0xFF;
            lower >>= 8;
        }
        for(qint32 i = 8; i < 16; i++)
        {
            val[i] = upper & 0xFF;
            upper >>= 8;
        }
    }

    // Increment operator
    MtpInt128& operator++()
    {
        quint64 *lower = reinterpret_cast<quint64*>(val);
        quint64 *upper = reinterpret_cast<quint64*>(val + 8);

        ++*lower;

        // If the lower 8 bytes overflowed
        // due to this increment, increment the upper 8 bytes.
        if( !*lower )
        {
            ++*upper;
        }

        return *this;
    }

    // Compares two MtpInt128's
    int compare( MtpInt128 &rhs ) const
    {
        int diff = 0;
        for(qint32 i = 15; ((i >= 0) && (0 == diff)) ; i--)
        {
            diff = this->val[i] - rhs.val[i];
        }
        return diff;
    }

    bool operator==( MtpInt128 &rhs ) const
    {
        return (0 == compare(rhs)) ? true : false;
    }

    // Assignment operator
    MtpInt128& operator=( MtpInt128 &rhs )
    {
        for(qint32 i = 0; i < 16; i++)
        {
            val[i] = rhs.val[i];
        }
        return *this;
    }
}__attribute__((packed));

// Hashing function for MtpInt128 so that it can be used as a key in QHash
inline uint qHash(const MtpInt128 &v)
{
    // Convert the MtpInt128 to a QByteArray and use the QT provided hashing
    // function on it
    QByteArray arr = QByteArray::fromRawData(v.val, sizeof(v));
    return qHash(arr);
}

Q_DECLARE_METATYPE(char);
Q_DECLARE_METATYPE(MtpInt128);

// Some more metatypes to handle arrays
Q_DECLARE_METATYPE(QVector<char>);
Q_DECLARE_METATYPE(QVector<qint8>);
Q_DECLARE_METATYPE(QVector<qint16>);
Q_DECLARE_METATYPE(QVector<qint32>);
Q_DECLARE_METATYPE(QVector<qint64>);
Q_DECLARE_METATYPE(QVector<quint8>);
Q_DECLARE_METATYPE(QVector<quint16>);
Q_DECLARE_METATYPE(QVector<quint32>);
Q_DECLARE_METATYPE(QVector<quint64>);
Q_DECLARE_METATYPE(QVector<MtpInt128>);

struct MtpDevPropDesc
{
  MTPDevPropertyCode  uPropCode;
  MTPDataType     uDataType;
  bool            bGetSet;
  QVariant        defValue;
  QVariant        currentValue;
  MtpFormFlag     formFlag;
  QVariant        formField;
};

struct MtpObjPropDesc
{
  MTPObjPropertyCode  uPropCode;
  MTPDataType     uDataType;
  bool            bGetSet;
  QVariant        defValue;
  quint32         groupCode;
  MtpFormFlag     formFlag;
  QVariant        formField; // Can only be the types as specified in table 5.1 of the MTP 1.0 spec., 
  // ex: MtpRangeForm, MtpEnumForm, quint16, etc... Note: some types may be semantically incorrect such as
  // a range/enum of arrays
};

// A structure that is used to hold the MTP object property description
// and value together. Can be used to fetch multiple object properties
// from the storage.
struct MTPObjPropDescVal
{
    const MtpObjPropDesc *propDesc;
    QVariant propVal;

    MTPObjPropDescVal() : 
       propDesc(0)
    {
    }

    MTPObjPropDescVal(const MtpObjPropDesc *desc) :
       propDesc(desc)
    {
    }
    MTPObjPropDescVal(const MtpObjPropDesc *desc, const QVariant &val) :
       propDesc(desc), propVal(val)
    {
    }
};

struct MTPObjectInfo
{
    quint32                 mtpStorageId;
    quint16                 mtpObjectFormat;
    quint16                 mtpProtectionStatus;
    quint64                 mtpObjectCompressedSize;
    quint16                 mtpThumbFormat;
    quint32                 mtpThumbCompressedSize;
    quint32                 mtpThumbPixelWidth;
    quint32                 mtpThumbPixelHeight;
    quint32                 mtpImagePixelWidth;
    quint32                 mtpImagePixelHeight;
    quint32                 mtpImageBitDepth;
    quint32                 mtpParentObject;
    quint16                 mtpAssociationType;
    quint32                 mtpAssociationDescription;
    quint32                 mtpSequenceNumber;
    QString                 mtpFileName;
    QString                 mtpCaptureDate;
    QString                 mtpModificationDate;
    QString                 mtpKeywords;    
    MTPObjectInfo() : mtpStorageId(0), mtpObjectFormat(MTP_OBF_FORMAT_Undefined),
                      mtpProtectionStatus(MTP_PROTECTION_NoProtection), mtpObjectCompressedSize(0),
                      mtpThumbFormat(MTP_OBF_FORMAT_Undefined), mtpThumbCompressedSize(0),
                      mtpThumbPixelWidth(0), mtpThumbPixelHeight(0),
                      mtpImagePixelWidth(0), mtpImagePixelHeight(0), mtpImageBitDepth(0),
                      mtpParentObject(0), mtpAssociationType(MTP_ASSOCIATION_TYPE_Undefined),
                      mtpAssociationDescription(0), mtpSequenceNumber(0)
                      {}
                      
};
#endif

