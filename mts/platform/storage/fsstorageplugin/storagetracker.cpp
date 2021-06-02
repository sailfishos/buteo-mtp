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

// System headers
#include <QString>
#include <QRegExp>
#include <QUuid>
#include <QUrl>
#include <QDBusPendingReply>
#include <QDBusConnection>
#include <QSparqlConnection>
#include <QSparqlError>
#include <QSparqlQuery>
#include <QSparqlResult>

// Local headers
#include "storagetracker.h"
#include "trace.h"

using namespace meegomtp1dot0;

// Static declarations
static const QString IRI_PREFIX = "file://";
static const QString RDF_TYPE = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
static QString getDateCreated (const QString&);
static QString getName (const QString&);
static QString getArtist (const QString&);
static QString getWidth (const QString&);
static QString getHeight (const QString&);
static QString getDuration (const QString&);
static QString getTrack (const QString&);
static QString getGenre (const QString&);
static QString getUseCount (const QString&);
static QString getAlbumName (const QString&);
#if 0
static QString getBitrateType (const QString&);
#endif
static QString getSampleRate (const QString&);
static QString getNbrOfChannels (const QString&);
#if 0
static QString getAudioBitDepth (const QString&);
static QString getAudioWAVECodec (const QString&);
static QString getAudioBitRate (const QString&);
static QString getVideoFourCCCodec (const QString&);
static QString getVideoBitRate (const QString&);
#endif
static QString getFramesPerThousandSecs (const QString&);
static QString getDRMStatus (const QString&);

static void setName (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
#if 0
static void setDateCreated (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setArtist (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setWidth (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setHeight (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setDuration (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setDRMStatus (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setTrack (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setGenre (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setUseCount (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setAlbumName (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setBitrateType (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setSampleRate (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setNbrOfChannels (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setAudioBitDepth (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setAudioWAVECodec (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setAudioBitRate (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setVideoFourCCCodec (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setVideoBitRate (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
static void setFramesPerThousandSecs (const QString& iri, QString& val, QStringList& domains, QString &extraInserts);
#endif

static void trackerQuery(const QString&, QVector<QStringList> &res);
static void trackerUpdateQuery(const QString&);
static void convertResultByTypeAndCode(const QString&, QString&, MTPDataType, MTPObjPropertyCode, QVariant&);
#if 0
static QString getValAsString(const QVariant& propVal, MTPDataType type, MTPObjPropertyCode code);
#endif
static QString generateIriForTracker(const QString& path);
static void deletePlaylistByIri(const QString &iri);

static const QString MINER_DEST("org.freedesktop.Tracker1.Miner.Files");
static const QString MINER_PATH("/org/freedesktop/Tracker1/Miner/Files");
static const QString MINER_IF("org.freedesktop.Tracker1.Miner");

StorageTracker::StorageTracker() : m_minerInterface(MINER_DEST, MINER_PATH, MINER_IF, QDBusConnection::sessionBus())
{
    populateFunctionMap();
}

StorageTracker::~StorageTracker()
{

}

// Populates the lookup table with functions to fetch respective Object info.
void StorageTracker::populateFunctionMap()
{
    // getters
    //m_handlerTable[MTP_OBJ_PROP_Obj_Format] = getObjFormat;
    m_handlerTable[MTP_OBJ_PROP_Date_Created] = getDateCreated;
    m_handlerTable[MTP_OBJ_PROP_Name] = getName;
    m_handlerTable[MTP_OBJ_PROP_Artist] = getArtist;
    m_handlerTable[MTP_OBJ_PROP_Width] = getWidth;
    m_handlerTable[MTP_OBJ_PROP_Height] = getHeight;
    m_handlerTable[MTP_OBJ_PROP_Duration] = getDuration;
    //m_handlerTable[MTP_OBJ_PROP_Rating] = getRating;
    m_handlerTable[MTP_OBJ_PROP_Track] = getTrack;
    m_handlerTable[MTP_OBJ_PROP_Genre] = getGenre;
    m_handlerTable[MTP_OBJ_PROP_Use_Count] = getUseCount;
    //m_handlerTable[MTP_OBJ_PROP_Original_Release_Date] = getOriginalReleaseDate;
    m_handlerTable[MTP_OBJ_PROP_Album_Name] = getAlbumName;
    //m_handlerTable[MTP_OBJ_PROP_Album_Artist] = getAlbumArtist;
    //m_handlerTable[MTP_OBJ_PROP_Bitrate_Type] = getBitrateType;
    m_handlerTable[MTP_OBJ_PROP_Sample_Rate] = getSampleRate;
    m_handlerTable[MTP_OBJ_PROP_Nbr_Of_Channels] = getNbrOfChannels;
    //m_handlerTable[MTP_OBJ_PROP_Audio_BitDepth] = getAudioBitDepth;
    //m_handlerTable[MTP_OBJ_PROP_Scan_Type] = getScanType;
    //m_handlerTable[MTP_OBJ_PROP_Audio_WAVE_Codec] = getAudioWAVECodec;
    //m_handlerTable[MTP_OBJ_PROP_Audio_BitRate] = getAudioBitRate;
    //m_handlerTable[MTP_OBJ_PROP_Video_FourCC_Codec] = getVideoFourCCCodec;
    //m_handlerTable[MTP_OBJ_PROP_Video_BitRate] = getVideoBitRate;
    m_handlerTable[MTP_OBJ_PROP_Frames_Per_Thousand_Secs] = getFramesPerThousandSecs;
    m_handlerTable[MTP_OBJ_PROP_DRM_Status] = getDRMStatus;
    //m_handlerTable[MTP_OBJ_PROP_KeyFrame_Distance] = getKeyFrameDistance;
    //m_handlerTable[MTP_OBJ_PROP_Encoding_Profile] = getEncodingProfile;
    // We don't support the below as of now.
#if 0
    m_handlerTable[MTP_OBJ_PROP_Keywords] = getKeywords;
    m_handlerTable[MTP_OBJ_PROP_Hidden] = getHidden;
    m_handlerTable[MTP_OBJ_PROP_Sys_Obj] = getSysObj;
    m_handlerTable[MTP_OBJ_PROP_SyncID] = getSyncID;
    m_handlerTable[MTP_OBJ_PROP_Property_Bag] = getPropertyBag;
    m_handlerTable[MTP_OBJ_PROP_Created_By] = getCreatedBy;
    m_handlerTable[MTP_OBJ_PROP_Date_Authored] = getDateAuthored;
    m_handlerTable[MTP_OBJ_PROP_Description] = getDescription;
    m_handlerTable[MTP_OBJ_PROP_URL_Reference] = getURLReference;
    m_handlerTable[MTP_OBJ_PROP_Language_Locale] = getLanguageLocale;
    m_handlerTable[MTP_OBJ_PROP_Copyright_Information] = getCopyrightInformation;
    m_handlerTable[MTP_OBJ_PROP_Source] = getSource;
    m_handlerTable[MTP_OBJ_PROP_Origin_Location] = getOriginLocation;
    m_handlerTable[MTP_OBJ_PROP_Credits] = getCredits;
    m_handlerTable[MTP_OBJ_PROP_Lyrics] = getLyrics;
    m_handlerTable[MTP_OBJ_PROP_Subscription_Content_ID] = getSubscriptionContentID;
    m_handlerTable[MTP_OBJ_PROP_Produced_By] = getProducedBy;
    m_handlerTable[MTP_OBJ_PROP_Skip_Count] = getSkipCount;
    m_handlerTable[MTP_OBJ_PROP_Last_Accessed] = getLastAccessed;
    m_handlerTable[MTP_OBJ_PROP_Parental_Rating] = getParentalRating;
    m_handlerTable[MTP_OBJ_PROP_Meta_Genre] = getMetaGenre;
    m_handlerTable[MTP_OBJ_PROP_Composer] = getComposer;
    m_handlerTable[MTP_OBJ_PROP_Effective_Rating] = getEffectiveRating;
    m_handlerTable[MTP_OBJ_PROP_Subtitle] = getSubtitle;
    m_handlerTable[MTP_OBJ_PROP_Mood] = getMood;
    m_handlerTable[MTP_OBJ_PROP_Sub_Description] = getSubDescription;
    m_handlerTable[MTP_OBJ_PROP_Is_Cropped] = getIsCropped;
    m_handlerTable[MTP_OBJ_PROP_Is_Colour_Corrected] = getIsColourCorrected;
    m_handlerTable[MTP_OBJ_PROP_Exposure_Time] = getExposureTime;
    m_handlerTable[MTP_OBJ_PROP_Exposure_Index] = getExposureIndex;
    m_handlerTable[MTP_OBJ_PROP_Total_BitRate] = getTotalBitRate;
    m_handlerTable[MTP_OBJ_PROP_Buffer_Size] = getBufferSize;
    m_handlerTable[MTP_OBJ_PROP_Encoding_Quality] = getEncodingQuality;
#endif
/* These direct settings of file properties aren't expected by tracker, and don't match what tracker
 * is expecting. Disable them as they cause an error in tracker-extract when it tries to update them
 */
#if 0
    // Setters
    m_handlerTableUpdate[MTP_OBJ_PROP_Date_Created] = setDateCreated;
    m_handlerTableUpdate[MTP_OBJ_PROP_Name] = setName;
    m_handlerTableUpdate[MTP_OBJ_PROP_Artist] = setArtist;
    m_handlerTableUpdate[MTP_OBJ_PROP_Width] = setWidth;
    m_handlerTableUpdate[MTP_OBJ_PROP_Height] = setHeight;
    m_handlerTableUpdate[MTP_OBJ_PROP_Duration] = setDuration;
    //m_handlerTableUpdate[MTP_OBJ_PROP_Rating] = setRating;
    m_handlerTableUpdate[MTP_OBJ_PROP_Track] = setTrack;
    m_handlerTableUpdate[MTP_OBJ_PROP_Genre] = setGenre;
    //m_handlerTableUpdate[MTP_OBJ_PROP_Original_Release_Date] = setOriginalReleaseDate;
    m_handlerTableUpdate[MTP_OBJ_PROP_Album_Name] = setAlbumName;
    //m_handlerTableUpdate[MTP_OBJ_PROP_Album_Artist] = setAlbumArtist;
    m_handlerTableUpdate[MTP_OBJ_PROP_Bitrate_Type] = setBitrateType;
    m_handlerTableUpdate[MTP_OBJ_PROP_Sample_Rate] = setSampleRate;
    m_handlerTableUpdate[MTP_OBJ_PROP_Nbr_Of_Channels] = setNbrOfChannels;
    m_handlerTableUpdate[MTP_OBJ_PROP_Audio_BitDepth] = setAudioBitDepth;
    //m_handlerTableUpdate[MTP_OBJ_PROP_Scan_Type] = setScanType;
    m_handlerTableUpdate[MTP_OBJ_PROP_Audio_WAVE_Codec] = setAudioWAVECodec;
    m_handlerTableUpdate[MTP_OBJ_PROP_Audio_BitRate] = setAudioBitRate;
    m_handlerTableUpdate[MTP_OBJ_PROP_Video_FourCC_Codec] = setVideoFourCCCodec;
    m_handlerTableUpdate[MTP_OBJ_PROP_Video_BitRate] = setVideoBitRate;
    m_handlerTableUpdate[MTP_OBJ_PROP_Frames_Per_Thousand_Secs] = setFramesPerThousandSecs;
    //m_handlerTableUpdate[MTP_OBJ_PROP_KeyFrame_Distance] = setKeyFrameDistance;
    //m_handlerTableUpdate[MTP_OBJ_PROP_Encoding_Profile] = setEncodingProfile;
    m_handlerTableUpdate[MTP_OBJ_PROP_Use_Count] = setUseCount;
    m_handlerTableUpdate[MTP_OBJ_PROP_DRM_Status] = setDRMStatus;
    // We don't support the below as of now.

    m_handlerTableUpdate[MTP_OBJ_PROP_Keywords] = setKeywords;
    m_handlerTableUpdate[MTP_OBJ_PROP_Subtitle] = setSubtitle;
    m_handlerTableUpdate[MTP_OBJ_PROP_Hidden] = setHidden;
    m_handlerTableUpdate[MTP_OBJ_PROP_Sys_Obj] = setSysObj;
    m_handlerTableUpdate[MTP_OBJ_PROP_SyncID] = setSyncID;
    m_handlerTableUpdate[MTP_OBJ_PROP_Property_Bag] = setPropertyBag;
    m_handlerTableUpdate[MTP_OBJ_PROP_Created_By] = setCreatedBy;
    m_handlerTableUpdate[MTP_OBJ_PROP_Date_Authored] = setDateAuthored;
    m_handlerTableUpdate[MTP_OBJ_PROP_Description] = setDescription;
    m_handlerTableUpdate[MTP_OBJ_PROP_URL_Reference] = setURLReference;
    m_handlerTableUpdate[MTP_OBJ_PROP_Language_Locale] = setLanguageLocale;
    m_handlerTableUpdate[MTP_OBJ_PROP_Copyright_Information] = setCopyrightInformation;
    m_handlerTableUpdate[MTP_OBJ_PROP_Source] = setSource;
    m_handlerTableUpdate[MTP_OBJ_PROP_Origin_Location] = setOriginLocation;
    m_handlerTableUpdate[MTP_OBJ_PROP_Credits] = setCredits;
    m_handlerTableUpdate[MTP_OBJ_PROP_Lyrics] = setLyrics;
    m_handlerTableUpdate[MTP_OBJ_PROP_Subscription_Content_ID] = setSubscriptionContentID;
    m_handlerTableUpdate[MTP_OBJ_PROP_Produced_By] = setProducedBy;
    m_handlerTableUpdate[MTP_OBJ_PROP_Skip_Count] = setSkipCount;
    m_handlerTableUpdate[MTP_OBJ_PROP_Last_Accessed] = setLastAccessed;
    m_handlerTableUpdate[MTP_OBJ_PROP_Parental_Rating] = setParentalRating;
    m_handlerTableUpdate[MTP_OBJ_PROP_Meta_Genre] = setMetaGenre;
    m_handlerTableUpdate[MTP_OBJ_PROP_Composer] = setComposer;
    m_handlerTableUpdate[MTP_OBJ_PROP_Effective_Rating] = setEffectiveRating;
    m_handlerTableUpdate[MTP_OBJ_PROP_Mood] = setMood;
    m_handlerTableUpdate[MTP_OBJ_PROP_Sub_Description] = setSubDescription;
    m_handlerTableUpdate[MTP_OBJ_PROP_Is_Cropped] = setIsCropped;
    m_handlerTableUpdate[MTP_OBJ_PROP_Is_Colour_Corrected] = setIsColourCorrected;
    m_handlerTableUpdate[MTP_OBJ_PROP_Exposure_Time] = setExposureTime;
    m_handlerTableUpdate[MTP_OBJ_PROP_Exposure_Index] = setExposureIndex;
    m_handlerTableUpdate[MTP_OBJ_PROP_Total_BitRate] = setTotalBitRate;
    m_handlerTableUpdate[MTP_OBJ_PROP_Buffer_Size] = setBufferSize;
    m_handlerTableUpdate[MTP_OBJ_PROP_Encoding_Quality] = setEncodingQuality;
#endif

    // Populate the map of tracker properties supported
    m_trackerPropertyTable.insert(QString("nie:url"));
    m_trackerPropertyTable.insert(QString("nie:contentCreated"));
    m_trackerPropertyTable.insert(QString("nie:title"));
    m_trackerPropertyTable.insert(QString("nmm:performer"));
    m_trackerPropertyTable.insert(QString("nfo:width"));
    m_trackerPropertyTable.insert(QString("nfo:height"));
    m_trackerPropertyTable.insert(QString("nfo:duration"));
    m_trackerPropertyTable.insert(QString("nmm:trackNumber"));
    m_trackerPropertyTable.insert(QString("nfo:genre"));
    m_trackerPropertyTable.insert(QString("nmm:musicAlbum"));
    //m_trackerPropertyTable.insert(QString("nfo:bitrateType"));
    m_trackerPropertyTable.insert(QString("nfo:sampleRate"));
    m_trackerPropertyTable.insert(QString("nfo:channels"));
    //m_trackerPropertyTable.insert(QString("nfo:bitDepth"));
    //m_trackerPropertyTable.insert(QString("mtp:waveFormat"));
    //m_trackerPropertyTable.insert(QString("nfo:averageAudioBitrate"));
    //m_trackerPropertyTable.insert(QString("mtp:fourCC"));
    //m_trackerPropertyTable.insert(QString("nfo:averageVideoBitrate"));
    m_trackerPropertyTable.insert(QString("nfo:frameRate"));
    m_trackerPropertyTable.insert(QString("nie:usageCounter"));
    m_trackerPropertyTable.insert(QString("nfo:isContentEncrypted"));
}

static void convertResultByTypeAndCode(const QString& filePath, QString& res, MTPDataType type, MTPObjPropertyCode code, QVariant& convertedResult)
{
    switch(type)
    {
        case MTP_DATA_TYPE_STR:
            {
                if(MTP_OBJ_PROP_Name == code && (res == ""))
                {
                    int idx = filePath.lastIndexOf(QChar('/'));
                    res = filePath.mid(idx + 1);
                }
                convertedResult = QVariant::fromValue(res);
            }
            break;
        case MTP_DATA_TYPE_INT8:
        case MTP_DATA_TYPE_UINT8:
            {
                // Just true and false for now!
                if(res == "true")
                {
                    convertedResult = QVariant::fromValue((quint8)1);
                }
                else
                {
                    convertedResult = QVariant::fromValue((quint8)0);
                }
            }
            break;
        case MTP_DATA_TYPE_INT16:
            {
                // Remove any thing after the decimal point...
                res = res.section('.', 0, 0);
                qint16 val = res.toShort();
                convertedResult = QVariant::fromValue(val);
            }
            break;
        case MTP_DATA_TYPE_UINT16:
            {
                if( MTP_OBJ_PROP_DRM_Status == code )
                {
                    if( "true" == res )
                    {
                        convertedResult = QVariant(0x0001);
                    }
                    else
                    {
                        convertedResult = QVariant(0x0000);
                    }
                }
                else
                {
                    // Remove any thing after the decimal point...
                    res = res.section('.', 0, 0);
                    quint16 val = res.toUShort();
                    convertedResult = QVariant::fromValue(val);
                }
            }
            break;
        case MTP_DATA_TYPE_INT32:
            {
                // Remove any thing after the decimal point...
                res = res.section('.', 0, 0);
                qint32 val = res.toLong();
                convertedResult = QVariant::fromValue(val);
            }
            break;
        case MTP_DATA_TYPE_UINT32:
            {
                // Remove any thing after the decimal point...
                res = res.section('.', 0, 0);
                quint32 val = res.toULong();
                convertedResult = QVariant::fromValue(val);
            }
            break;
        case MTP_DATA_TYPE_INT64:
            {
                // Remove any thing after the decimal point...
                res = res.section('.', 0, 0);
                qint64 val = res.toLongLong();
                convertedResult = QVariant::fromValue(val);
            }
            break;
        case MTP_DATA_TYPE_UINT64:
            {
                // Remove any thing after the decimal point...
                res = res.section('.', 0, 0);
                quint64 val = res.toULongLong();
                convertedResult = QVariant::fromValue(val);
            }
            break;
        // The below types are not currently used in tracker queries
        case MTP_DATA_TYPE_INT128:
        case MTP_DATA_TYPE_UINT128:
        case MTP_DATA_TYPE_AINT8:
        case MTP_DATA_TYPE_AUINT8:
        case MTP_DATA_TYPE_AINT16:
        case MTP_DATA_TYPE_AUINT16:
        case MTP_DATA_TYPE_AINT32:
        case MTP_DATA_TYPE_AUINT32:
        case MTP_DATA_TYPE_AINT64:
        case MTP_DATA_TYPE_AUINT64:
        case MTP_DATA_TYPE_AINT128:
        case MTP_DATA_TYPE_AUINT128:
        case MTP_DATA_TYPE_UNDEF:
        default:
            break;
    }

    if(MTP_OBJ_PROP_Duration == code)
    {
        quint32 v = convertedResult.value<quint32>();
        v = v * 1000;
        convertedResult = QVariant::fromValue(v);
    }
}

#if 0
static QString getValAsString(const QVariant& propVal, MTPDataType type, MTPObjPropertyCode code)
{
    QString ret;
    switch(type)
    {
        case MTP_DATA_TYPE_INT8:
        case MTP_DATA_TYPE_UINT8:
            {
                // Only booleans for now!
                quint8 num = propVal.value<quint8>();
                if(0 == num)
                {
                    ret = "false";
                }
                else
                {
                    ret = "true";
                }
            }
            break;
        case MTP_DATA_TYPE_INT16:
        case MTP_DATA_TYPE_INT32:
            {
               long num = propVal.value<long>();
               ret = QString::number(num);
            }
            break;
        case MTP_DATA_TYPE_UINT16:
        case MTP_DATA_TYPE_UINT32:
            {
               ulong num = propVal.value<ulong>();
               if(MTP_OBJ_PROP_Duration == code)
               {
                   num /= 1000;
               }
               ret = QString::number(num);
               if( MTP_OBJ_PROP_DRM_Status == code )
               {
                   if( "1" == ret )
                   {
                       ret = "true";
                   }
                   else
                   {
                       ret = "false";
                   }
               }
            }
            break;
        case MTP_DATA_TYPE_INT64:
            {
                qlonglong num = propVal.value<qlonglong>();
                ret = QString::number(num);
            }
            break;
        case MTP_DATA_TYPE_UINT64:
            {
                qulonglong num = propVal.value<qulonglong>();
                ret = QString::number(num);
            }
            break;
        case MTP_DATA_TYPE_STR:
            {
                ret = propVal.value<QString>();
                if(MTP_OBJ_PROP_Date_Created == code)
                {
                    // Add : after +/-, for the timezone, if any
                    int idx = ret.lastIndexOf(QRegExp("\\+|\\-"));
                    if(-1 != idx)
                    {
                        ret.insert(idx + 3, ":");
                    }

                    // Covert date from MTP representation to ISO 8601
                    // Remove any tenths of seconds first...
                    ret.replace(QRegExp("\\.[0-9]"), "");
                    // Add -'s and :'s
                    ret.insert(4, "-");
                    ret.insert(7, "-");
                    ret.insert(13, ":");
                    ret.insert(16, ":");
                }
            }
            break;
    // The below types are not currently supported for writing to tracker
        case MTP_DATA_TYPE_INT128:
        case MTP_DATA_TYPE_UINT128:
        case MTP_DATA_TYPE_AINT8:
        case MTP_DATA_TYPE_AUINT8:
        case MTP_DATA_TYPE_AINT16:
        case MTP_DATA_TYPE_AUINT16:
        case MTP_DATA_TYPE_AINT32:
        case MTP_DATA_TYPE_AUINT32:
        case MTP_DATA_TYPE_AINT64:
        case MTP_DATA_TYPE_AUINT64:
        case MTP_DATA_TYPE_AINT128:
        case MTP_DATA_TYPE_AUINT128:
        default:
            MTP_LOG_WARNING("Unsupported data type in tracker write::" << type);
            break;
    }
    // Escape any 's in the value, as the query encloses the values in single quotes
    ret.replace(QRegExp("'"), "\\'");
    return ret;
}
#endif

QString StorageTracker::buildQuery(const QString &filePath, QList<MTPObjPropDescVal> &propValList)
{
    QString iri = generateIriForTracker(filePath);
    QString select("SELECT ");
    QString where("WHERE{?file a nie:DataObject;nie:url '" + iri + "'");
    QString end("}");
    QString trackerName;
    quint32 count = 0;

    // Populate our query...
    for(QList<MTPObjPropDescVal>::iterator i = propValList.begin(); i != propValList.end(); )
    {
        if(false == i->propVal.isValid())
        {
            MTPObjPropertyCode propCode = i->propDesc->uPropCode;
            // Get the query string for each property...
            if(!m_handlerTable.contains(propCode))
            {
                MTP_LOG_WARNING("Handler for object property not found!::" << propCode);
                i = propValList.erase(i);
                // Silently ignore properties that we don't handle
                continue;
            }
            count++;
            // Only consider properties whose variants are invalid, i.e, they
            // have not yet been populated
            QString propString = QString("?f%1").arg(count);
            fpTrackerQueryHandler pFunc = m_handlerTable[propCode];
            where += QString(". OPTIONAL{?file ");
            trackerName = pFunc(QString());

            where += trackerName;
            where += QString(" ") + propString;
            if(trackerName.contains(QChar('[')))
            {
                where += QString("]");
            }
            where += QString("} ");
            select += propString + QString(" ");
        }
        ++i;
    }
    if(0 == count)
    {
        return QString();
    }
    else
    {
        return (select + where + end);
    }
}

QString StorageTracker::buildMassQuery(const QString &path,
                                       const QList<const MtpObjPropDesc *> &properties)
{
    QString select;
    QString where;
    int variableId = 0;
    bool hasNullQuery = false;

    foreach (const MtpObjPropDesc *property, properties) {
        if (m_handlerTable.contains(property->uPropCode)) {
            const QString &variable = QString("?f%1 ").arg(variableId++);
            const QString &predicate =
                    m_handlerTable[property->uPropCode](QString());

            select += variable;
            where += QString("OPTIONAL{?x %1 %2 %3}")
                    .arg(predicate).arg(variable)
                    .arg(predicate.contains('[') ? ']' : ' ');
        } else {
            select += "?null ";
            if (!hasNullQuery) {
                where += " OPTIONAL{?null nie:url '123null321'}";
                hasNullQuery = true;
            }
        }
    }

    if (variableId == 0) {
        // Nothing to query.
        return QString();
    }

    return QString("SELECT ?iri %1 WHERE { "
                   "?x a nie:DataObject; nie:url ?iri. "
                   "%2 "
                   "FILTER regex(?iri, '^%3/[^/]*$') }")
                   .arg(select).arg(where).arg(generateIriForTracker(path));
}

bool StorageTracker::getPropVals(const QString &filePath, QList<MTPObjPropDescVal> &propValList)
{
    bool ret = false;
    QString query = buildQuery(filePath, propValList);
    if(false == query.isNull())
    {
        QVector<QStringList> result;
        trackerQuery(query, result);
        // Ensure we got what we asked for
        if(1 <= result.size())
        {
            qint32 count = 0;
            QStringList resList = result[0];
            ret = true;
            // Convert each resList string into the corresponding variant
            for(QList<MTPObjPropDescVal>::iterator i = propValList.begin(); i != propValList.end();)
            {
                if(false == i->propVal.isValid())
                {
                    QString str = resList.at(count);
                    count++;
                    // If the prop value turns out to be empty (other than Name,
                    // for which we can return the file name), we remove the
                    // property from the list
                    if(("" == str) && (MTP_OBJ_PROP_Name != i->propDesc->uPropCode))
                    {
                        i = propValList.erase(i);
                        continue;
                    }
                    convertResultByTypeAndCode(filePath, str, i->propDesc->uDataType,
                            i->propDesc->uPropCode, i->propVal);
                }
                ++i;
            }
        }
    }
    return ret;
}

void StorageTracker::getChildPropVals(const QString& parentPath,
        const QList<const MtpObjPropDesc *>& properties,
        QMap<QString, QList<QVariant> > &values)
{
    QString query(buildMassQuery(parentPath, properties));
    if (query.isEmpty()) {
        return;
    }

    QVector<QStringList> result;
    trackerQuery(query, result);

    QVector<QStringList>::iterator it;
    for (it = result.begin(); it != result.end(); ++it) {
        QStringList &row = *it;
        const QString &path = QUrl::fromEncoded(row[0].toUtf8()).toLocalFile();
        QList<QVariant> &resultRow =
                values.insert(path, QList<QVariant>()).value();

        for (int i = 1; i != row.size(); ++i) {
            resultRow.append(QVariant());

            QString &val = row[i];
            if (!val.isEmpty()) {
                const MtpObjPropDesc *desc = properties[i - 1];
                convertResultByTypeAndCode(path, val, desc->uDataType,
                        desc->uPropCode, resultRow.last());
            }
        }
    }
}

// Fetch the property value for the object referenced by the iri (ex: file:///home/user/MyDocs/1.mp3).
// Caller must free the pResult by calling storage_tracker_free_tracker_result
bool StorageTracker::getObjectProperty(const QString& path, MTPObjPropertyCode ePropertyCode, MTPDataType type, QVariant& result)
{
    if(!m_handlerTable.contains(ePropertyCode))
    {
        MTP_LOG_WARNING("Handler for object property not found!::" << ePropertyCode);
        return false;
    }
    fpTrackerQueryHandler pFunc = m_handlerTable[ePropertyCode];
    QString iri = generateIriForTracker(path);
    QString ret = pFunc(iri);
    convertResultByTypeAndCode(path, ret, type, ePropertyCode,result);
    return true;
}

#if 0
QString StorageTracker::buildUpdateQuery(const QString &filePath, QList<MTPObjPropDescVal> &propValList)
{
    QString iri = generateIriForTracker(filePath);
    QString insert = QString("DELETE {?file a rdfs:Resource} WHERE {?file nie:url <") + iri + QString(">} INSERT { _:x a nie:DataObject ");
    QString props("; nie:url '" + iri + "'");
    QString end("}");
    QString extraInserts;
    QString trackerName;
    QStringList domains;
    QString propVal;
    QString propValEncoded;
    quint32 count = 0;

    // Populate our query...
    for(QList<MTPObjPropDescVal>::iterator i = propValList.begin(); i != propValList.end(); ++i)
    {
        QString propInsert;
        MTPObjPropertyCode propCode = i->propDesc->uPropCode;
        // Get the query string for each property...
        if(!m_handlerTableUpdate.contains(propCode))
        {
            MTP_LOG_WARNING("Handler for object property not found!::" << propCode);
            // Silently ignore properties that we don't handle
            continue;
        }
        propVal = getValAsString(i->propVal, i->propDesc->uDataType, propCode);
        if(!propVal.isEmpty() && ! propVal.isNull())
        {
            count++;
            // Only consider properties whose variants are invalid, i.e, they
            // have not yet been populated
            fpTrackerUpdateQueryHandler pFunc = m_handlerTableUpdate[propCode];
            pFunc(QString(), trackerName, domains, propInsert);
            props += QString("; ") + trackerName;
            propValEncoded = QUrl(propVal).toEncoded();

            if(false == propInsert.isNull())
            {
                propInsert = propInsert.arg(propValEncoded, propVal);
                extraInserts += propInsert;
            }

            if(trackerName.contains(QChar('<')))
            {
                props += propValEncoded;
                props += QString(">");
            }
            else
            {
                props += QString(" '");
                props += propVal;
                props += QString("' ");
            }
        }
    }
    if(domains.size() > 0)
    {
        domains.removeDuplicates();
        insert += QString(",");
        insert += domains.join(QString(","));
    }
    if(0 == count)
    {
        return QString();
    }
    else
    {
        return (insert + props + end + extraInserts);
    }
}
#endif

void StorageTracker::setPropVals(const QString &filePath, QList<MTPObjPropDescVal> &propValList)
{
    Q_UNUSED(filePath);
    Q_UNUSED(propValList);
#if 0 // update table not populated at the moment
    QString query = buildUpdateQuery(filePath, propValList);
    if(false == query.isNull())
    {
        trackerUpdateQuery(query);
    }
#endif
}

bool StorageTracker::setObjectProperty(const QString& path, MTPObjPropertyCode ePropertyCode, MTPDataType type, const QVariant& propVal)
{
    Q_UNUSED(path);
    Q_UNUSED(ePropertyCode);
    Q_UNUSED(type);
    Q_UNUSED(propVal);
    return false;
#if 0
    if(!m_handlerTableUpdate.contains(ePropertyCode))
    {
        MTP_LOG_WARNING("Write handler for object property not found!::" << ePropertyCode);
        return false;
    }
    fpTrackerUpdateQueryHandler pFunc = m_handlerTableUpdate[ePropertyCode];
    QString iri = generateIriForTracker(path);
    QString val = getValAsString(propVal, type, ePropertyCode);
    QStringList domains;
    QString extraInserts;
    pFunc(iri, val, domains, extraInserts);
    return true;
#endif
}

static QSparqlConnection *trackerConnection()
{
    // QTRACKER_DIRECT does not handle the case when DB is removed and
    // recreated, like with `tracker-control -r` (storage-test does this)
    static QSparqlConnection *trackerConnection = new QSparqlConnection("QTRACKER");
    return trackerConnection;
}

static void trackerQuery(const QString& query, QVector<QStringList> &res)
{
    MTP_LOG_INFO(query);

    QSparqlResult* result = trackerConnection()->syncExec(QSparqlQuery(query));

    while (result->next()) {
        QStringList row;
        for (int i = 0; i < result->current().count(); ++i) {
            row.append(result->current().value(i).toString());
        }
        res.append(row);
    }

    delete result;
}

static void trackerUpdateQuery(const QString& query)
{
    // Ignore result for now...
    MTP_LOG_INFO(query);

    delete trackerConnection()->syncExec(QSparqlQuery(query, QSparqlQuery::InsertStatement));
}

static QString generateIriForTracker(const QString& path)
{
    QUrl url(IRI_PREFIX + path);
    QString iri = url.toEncoded();
    iri.replace(QRegExp("'"), "\\'");
    return iri;
}

QString getDateCreated (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nie:contentCreated");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nie:contentCreated ?fld}");
    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    trackerQuery(query, res);

    if((1 <= res.size()) && (1 <= res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getName (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nie:title");
    }
    QVector<QStringList> res;
    QString ret;
    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nie:title ?fld}");

    trackerQuery(query, res);
    if((1 <= res.size()) && (1 <= res[0].size()))
    {
        ret = (res[0])[0];
        if("" == ret)
        {
            // Empty title? return name...
            int idx = iri.lastIndexOf(QChar('/'));
            ret = iri.mid(idx + 1);
            ret = QUrl::fromEncoded(ret.toLatin1()).toString();
        }
    }
    return ret;
}

QString getArtist (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nmm:performer [nmm:artistName");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nmm:performer [nmm:artistName ?fld]}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getWidth (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:width");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?width WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:width ?width}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getHeight (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:height");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?height WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:height ?height}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getDRMStatus (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:isContentEncrypted");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?isContentEncrypted WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:isContentEncrypted ?isContentEncrypted}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getDuration (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:duration");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:duration ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getTrack (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nmm:trackNumber");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nmm:trackNumber ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getGenre (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:genre");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:genre ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getUseCount (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nie:usageCounter");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nie:usageCounter ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getAlbumName (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nmm:musicAlbum [nie:title");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nmm:musicAlbum [nie:title ?fld]}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

#if 0
QString getBitrateType (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:bitrateType");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:bitrateType ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}
#endif

QString getSampleRate (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:sampleRate");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:sampleRate ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getNbrOfChannels (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:channels");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:channels ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

#if 0
QString getAudioBitDepth (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nmm:bitDepth");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nmm:bitDepth ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getAudioWAVECodec (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("mtp:waveformat");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; mtp:waveformat ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getAudioBitRate (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:averageAudioBitrate");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:averageAudioBitrate ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getVideoFourCCCodec (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("mtp:fourCC");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; mtp:fourCC ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

QString getVideoBitRate (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:averageVideoBitrate");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:averageVideoBitrate ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}
#endif

QString getFramesPerThousandSecs (const QString& iri)
{
    if(iri.isNull())
    {
        return QString("nfo:frameRate");
    }
    QVector<QStringList> res;
    QString ret;

    QString query = QString("SELECT ?fld WHERE{?f a nie:InformationElement ; nie:url '") + iri + QString("' ; nfo:frameRate ?fld}");
    trackerQuery(query, res);

    if((1 == res.size()) && (1 == res[0].size()))
    {
        ret = (res[0])[0];
    }
    return ret;
}

void setName (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nie:title");
        domains.append("nie:InformationElement");
    }
    else
    {
        QString query = QString("DELETE{?f nie:title ?fld} WHERE{?f nie:url '") + iri + QString("' ; nie:title ?fld} INSERT{?f nie:title '") + val + QString("'} WHERE{?f a nie:InformationElement; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

#if 0
void setDateCreated (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nie:contentCreated");
        domains.append("nie:InformationElement");
    }
    else
    {
        QString query = QString("DELETE{?f nie:contentCreated ?fld} WHERE{?f nie:url '") + iri + QString("' ; nie:contentCreated ?fld} INSERT{?f nie:contentCreated '") + val + QString("'} WHERE{?f a nie:InformationElement; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setArtist (const QString& iri, QString& val, QStringList& domains, QString &extraInserts)
{
    if(iri.isNull())
    {
        val = QString("nmm:performer <urn:artist:");
        domains.append("nmm:MusicPiece");
        extraInserts = QString(" INSERT{<urn:artist:%1> a nmm:Artist; nmm:artistName '%2'}");
    }
    else
    {
        QString query = QString("DELETE{?f nmm:performer ?fld} WHERE{?f nie:url '") + iri + QString("' ; nmm:performer ?fld} INSERT{?f nmm:performer[ a nmm:Artist nmm:artistName '") + val + QString("']} WHERE{?f a nmm:MusicPiece; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setWidth (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:width");
        domains.append("nfo:Visual");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:width ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:width ?fld} INSERT{?f nfo:width '") + val + QString("'} WHERE{?f a nfo:Visual; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setHeight (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:height");
        domains.append("nfo:Visual");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:height ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:height ?fld} INSERT{?f nfo:height '") + val + QString("'} WHERE{?f a nfo:Visual; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setDRMStatus (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:isContentEncrypted");
        domains.append("nie:InformationElement");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:isContentEncrypted ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:isContentEncrypted ?fld} INSERT{?f nfo:isContentEncrypted '") + val + QString("'} WHERE{?f a nie:InformationElement; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setDuration (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:duration");
        domains.append("nfo:Media");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:duration ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:duration ?fld} INSERT{?f nfo:duration '") + val + QString("'} WHERE{?f a nfo:Media; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setTrack (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nmm:trackNumber");
        domains.append("nmm:MusicPiece");
    }
    else
    {
        QString query = QString("DELETE{?f nmm:trackNumber ?fld} WHERE{?f nie:url '") + iri + QString("' ; nmm:trackNumber ?fld} INSERT{?f nmm:trackNumber '") + val + QString("'} WHERE{?f a nmm:MusicPiece; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setGenre (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:genre");
        domains.append("nfo:Media");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:genre ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:genre ?fld} INSERT{?f nfo:genre '") + val + QString("'} WHERE{?f a nfo:Media; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setUseCount (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nie:usageCounter");
        domains.append("nie:InformationElement");
    }
    else
    {
        QString query = QString("DELETE{?f nie:usageCounter ?fld} WHERE{?f nie:url '") + iri + QString("' ; nie:usageCounter ?fld} INSERT{?f nie:usageCounter '") + val + QString("'} WHERE{?f a nie:InformationElement; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setAlbumName (const QString& iri, QString& val, QStringList& domains, QString &extraInserts)
{
    if(iri.isNull())
    {
        val = QString("nmm:musicAlbum <urn:album:");
        domains.append("nmm:MusicPiece");
        extraInserts = QString(" INSERT{<urn:album:%1> a nmm:MusicAlbum; nie:title '%2'}");
    }
    else
    {
        QString query = QString("DELETE{?f nmm:musicAlbum ?fld} WHERE{?f nie:url '") + iri + QString("' ; nmm:musicAlbum ?fld} INSERT{?f nmm:musicAlbum[ a nmm:MusicAlbum ; nie:title '") + val + QString("']} WHERE{?f a nmm:MusicPiece; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setBitrateType (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:bitrateType");
        domains.append("nfo:Media");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:bitrateType ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:bitrateType ?fld} INSERT{?f nfo:bitrateType '") + val + QString("'} WHERE{?f a nfo:Media; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setSampleRate (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:sampleRate");
        domains.append("nfo:Audio");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:sampleRate ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:sampleRate ?fld} INSERT{?f nfo:sampleRate '") + val + QString("'} WHERE{?f a nfo:Audio; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setNbrOfChannels (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:channels");
        domains.append("nfo:Audio");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:channels ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:channels ?fld} INSERT{?f nfo:channels '") + val + QString("'} WHERE{?f a nfo:Audio; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setAudioBitDepth (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:bitDepth");
        domains.append("nfo:Audio");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:bitDepth ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:bitDepth ?fld} INSERT{?f nfo:bitDepth '") + val + QString("'} WHERE{?f a nfo:Audio; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setAudioWAVECodec (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("mtp:waveformat");
        domains.append("nmm:Video");
    }
    else
    {
        QString query = QString("DELETE{?f mtp:waveformat ?fld} WHERE{?f nie:url '") + iri + QString("' ; mtp:waveformat ?fld} INSERT{?f mtp:waveformat '") + val + QString("'} WHERE{?f a nmm:Video; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setAudioBitRate (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:averageAudioBitrate");
        domains.append("nfo:Audio");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:averageAudioBitrate ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:averageAudioBitrate ?fld} INSERT{?f nfo:averageAudioBitrate '") + val + QString("'} WHERE{?f a nfo:Audio; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setVideoFourCCCodec (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("mtp:fourCC");
        domains.append("nmm:Video");
    }
    else
    {
        QString query = QString("DELETE{?f mtp:fourCC ?fld} WHERE{?f nie:url '") + iri + QString("' ; mtp:fourCC ?fld} INSERT{?f mtp:fourCC '") + val + QString("'} WHERE{?f a nmm:Video; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setVideoBitRate (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:averageVideoBitrate");
        domains.append("nfo:Video");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:averageVideoBitrate ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:averageVideoBitrate ?fld} INSERT{?f nfo:averageVideoBitrate '") + val + QString("'} WHERE{?f a nfo:Video; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}

void setFramesPerThousandSecs (const QString& iri, QString& val, QStringList& domains, QString &/*extraInserts*/)
{
    if(iri.isNull())
    {
        val = QString("nfo:frameRate");
        domains.append("nfo:Video");
    }
    else
    {
        QString query = QString("DELETE{?f nfo:frameRate ?fld} WHERE{?f nie:url '") + iri + QString("' ; nfo:frameRate ?fld} INSERT{?f nfo:frameRate '") + val + QString("'} WHERE{?f a nfo:Video; nie:url '") + iri + QString("'}");
        return trackerUpdateQuery(query);
    }
}
#endif

void StorageTracker::ignoreNextUpdateFinished(QDBusPendingCallWatcher *pcw)
{
    QDBusPendingReply<> reply = *pcw;

    if (!reply.isValid())
        MTP_LOG_WARNING("Ignore next update failed!!");

    pcw->deleteLater();
}

void StorageTracker::ignoreNextUpdate(const QStringList &iris)
{
    if(m_minerInterface.isValid())
    {
        QDBusPendingCall pc = m_minerInterface.asyncCall("IgnoreNextUpdate", iris);
        QDBusPendingCallWatcher *pcw = new QDBusPendingCallWatcher(pc, this);
        connect(pcw, &QDBusPendingCallWatcher::finished,
                this, &StorageTracker::ignoreNextUpdateFinished);
    }
}

QString StorageTracker::savePlaylist(const QString &playlistPath, QStringList &entries)
{
    QString playlistPathLocal = playlistPath;
    int idx = playlistPathLocal.lastIndexOf(QString(".pla"));
    if(-1 != idx)
    {
        playlistPathLocal.truncate(idx);
    }
    QString playlistUri = generateIriForTracker(playlistPath);
    deletePlaylistByIri(playlistUri);
    QString playlistTitle = playlistPathLocal.mid(playlistPathLocal.lastIndexOf('/') + 1);
    playlistTitle.replace(QRegExp("'"), "\\'");
    // Create a new nmm:Playlist
    QString playlistId = QString("urn:playlist:");
    QString playlistUuid = QString("mafw") + QUuid::createUuid().toString();
    playlistUuid.remove(QRegExp("[-{}]"));
    playlistId += playlistUuid;
    QString createPlaylist = QString("INSERT {<") + playlistId +
                             QString("> a nmm:Playlist ; nie:identifier '") +
                             playlistUri + QString("'; nie:title '") + playlistTitle + ("'}");
    trackerUpdateQuery(createPlaylist);
    // Add entries to the playlist
    // Retrive URNs for all entry URIs
    QString entryUri;
    QString urnQuery = "SELECT ?f ?dur WHERE{?f a nie:InformationElement";
    for(int i = 0; i < entries.size(); i++)
    {
        if(i > 0)
        {
            urnQuery += QString(" UNION ");
        }
        entryUri = generateIriForTracker(entries[i]);
        urnQuery += QString("{?f nie:url '") + entryUri + QString("' . OPTIONAL{?f nfo:duration ?dur}}");
    }
    urnQuery += QString("}");
    QVector<QStringList> allEntries;
    trackerQuery(urnQuery, allEntries);
    QString insertEntries = QString("INSERT {<") + playlistId +
                            QString("> nfo:entryCounter '") + QString::number(allEntries.size()) +
                            QString("' . ");
    quint32 listDuration = 0;
    // Finally add all these entries to the created playlist's nfo:hasMediaFileListEntry property
    for(int i = 0; i < allEntries.size(); i++)
    {
        QString listEntryUri = QString("urn:playlist-entry:") + playlistUuid + ":" + QString::number(i);
        QString entryDuration = allEntries[i][1];
        insertEntries += QString("<") + playlistId + QString("> nfo:hasMediaFileListEntry <") +
                         listEntryUri + QString("> . ");
        insertEntries += QString("<") + listEntryUri + QString("> a nfo:MediaFileListEntry . ");
        insertEntries += QString("<") + listEntryUri + QString("> nfo:entryUrl '") +
                         generateIriForTracker(entries[i]) + QString("' . ");
        insertEntries += QString("<") + listEntryUri + QString("> nfo:listPosition \'") +
                         QString::number(i) + QString("\' . ");
        entryDuration = entryDuration.section('.', 0, 0);
        listDuration += entryDuration.toULong();
    }
    insertEntries += QString("}");
    trackerUpdateQuery(insertEntries);
    // Add list duration that was calculated above
    insertEntries = QString("INSERT{<") + playlistId + QString("> nfo:listDuration '") +
                    QString::number(listDuration) + ("'}");
    trackerUpdateQuery(insertEntries);
    return playlistId;
}

void StorageTracker::getPlaylists(QStringList &playlistPaths, QList<QStringList> &entries, bool getExisting /*= false*/)
{
    // Make a query for all playlist names and the iri's it refers
    QVector<QStringList> resultSet;
    QStringList resultRow;
    QString query;

    if(true == getExisting)
    {
        // Query for all "existing" playlists, i.e, all those with valid url's
        query = QString("SELECT ?f1 ?fld1 WHERE{?f a nmm:Playlist . OPTIONAL{?f nie:identifier ?f1} . ?f nfo:hasMediaFileListEntry ?fld . ?fld nfo:entryUrl ?fld1 . FILTER (bound(?f1))} ORDER BY ?f1");
    }
    else
    {
        // Query for all "new" playlists, that is those that were added on the device
        query = QString("SELECT ?f3 ?fld1 WHERE{?f a nmm:Playlist ; nie:title ?f3 . OPTIONAL{?f nie:identifier ?f1} . ?f nfo:hasMediaFileListEntry ?fld . ?fld nfo:entryUrl ?fld1 . FILTER (! bound(?f1))} ORDER BY ?f1");
    }
    trackerQuery(query, resultSet);
    QString lastPlaylistUrl;
    int j = -1;
    // Iterate over the result set and populate our out parameters
    for(int i=0; i < resultSet.size(); i++)
    {
        resultRow = resultSet.at(i);
        if(resultRow.size() != 2)
        {
            continue;
        }
        QString thisPlaylistUrl = resultRow.at(0);
        // Check if the URL of the playlist was same as the previous row
        if(thisPlaylistUrl != lastPlaylistUrl)
        {
            lastPlaylistUrl = thisPlaylistUrl;
            if(true == getExisting)
            {
                // Add a new item to the playlist path
                thisPlaylistUrl = QUrl::fromEncoded(thisPlaylistUrl.toLatin1()).toString();
                playlistPaths.append(thisPlaylistUrl.remove("file://"));
            }
            else
            {
                playlistPaths.append(thisPlaylistUrl);
            }
            entries.append(QStringList());
            j++;
        }
        QString entryUrl = resultRow.at(1);
        entryUrl = QUrl::fromEncoded(entryUrl.toLatin1()).toString();
        if(-1 != j)
        {
            // Append the entry URL to the list of entries
            entries[j].append(entryUrl.remove("file://"));
        }
    }
}

bool StorageTracker::isPlaylistExisting(const QString &path)
{
    bool ret = false;
    QString iri = generateIriForTracker(path);
    QString query = QString("SELECT ?f WHERE {?f a nmm:Playlist ; nie:identifier '") + iri + QString("'}");
    QVector<QStringList> resultSet;
    trackerQuery(query, resultSet);
    if(resultSet.size() > 0)
    {
        ret = true;
    }
    return ret;
}

void StorageTracker::setPlaylistPath(const QString &name, const QString &path)
{
    // Set the nie:url for this playlist
    // INSERT{?f a nmm:Playlist, nie:DataObject ; nie:url <file:///home/puranik/MyDocs/pl3>} WHERE{?f a nmm:Playlist ; nie:title 'pl3'}
    QString iri = generateIriForTracker(path);
    QString query = QString("INSERT{?f a nmm:Playlist ; nie:identifier '") + iri +
                    QString("'} WHERE{?f a nmm:Playlist ; nie:title '") + name + QString("'}");
    trackerUpdateQuery(query);
}

void StorageTracker::deletePlaylist(const QString &path)
{
    QString iri = generateIriForTracker(path);
    deletePlaylistByIri(iri);
}

static void deletePlaylistByIri(const QString &playlistUri)
{
    // Delete playlist entries, if any
    QString deleteEntries = QString("DELETE {?f a rdfs:Resource} WHERE {?fld a nmm:Playlist; nie:identifier '") +
                            playlistUri +
                            QString("' ; nfo:hasMediaFileListEntry ?f}");
    trackerUpdateQuery(deleteEntries);
    // Delete the old playlist, if any
    QString deletePlaylist = QString("DELETE {?f a rdfs:Resource} WHERE {?f a nmm:Playlist ; nie:identifier '")
                          + playlistUri + QString("'}");
    trackerUpdateQuery(deletePlaylist);
}

void StorageTracker::movePlaylist(const QString &fromPath, const QString &toPath)
{
    // Move the nie:url of the playlist fromPath --> toPath
    QString fromIri = generateIriForTracker(fromPath);
    QString toIri = generateIriForTracker(toPath);
    QVector<QStringList> resultSet;

    // Get the tracker URN for the from IRI
    trackerQuery(QString("SELECT ?f WHERE{?f a nmm:Playlist ; nie:identifier '" + fromIri + QString("'}")), resultSet);
    if(0 == resultSet.size() || 0 == resultSet[0].size())
    {
        MTP_LOG_CRITICAL("Failed query for tracker URN" << fromPath);
        return;
    }
    QString urn = resultSet[0][0];
    // Delete the old nie:url and insert a new one
    QString query = QString("DELETE {<") + urn + QString("> nie:identifier ?f} WHERE {<") + urn + QString("> nie:identifier ?f}");
    trackerUpdateQuery(query);
    query = QString("INSERT {<") + urn + ("> a nie:DataObject ; nie:identifier '") + toIri + ("'}");
    trackerUpdateQuery(query);
    // Update nie:title as well
    QStringList list;
    QString temp;
    QString title = toPath.mid(toPath.lastIndexOf('/') + 1);
    setName(toIri, title, list, temp);
}

void StorageTracker::move(const QString &fromPath, const QString &toPath)
{
    // Move the nie:url of the file fromPath --> toPath
    QString fromIri = generateIriForTracker(fromPath);
    QString toIri = generateIriForTracker(toPath);
    QVector<QStringList> resultSet;

    // Get the tracker URN for the from IRI
    trackerQuery(QString("SELECT ?f WHERE{?f a nfo:FileDataObject ; nie:url '" + fromIri + QString("'}")), resultSet);
    if(0 == resultSet.size() || 0 == resultSet[0].size())
    {
        MTP_LOG_CRITICAL("Failed query for tracker URN for playlist" << fromPath);
        return;
    }
    QString urn = resultSet[0][0];
    // Delete the old nie:url and insert a new one
    QString query = QString("DELETE {<") + urn + QString("> nie:url ?f} WHERE {<") + urn + QString("> nie:url ?f}");
    trackerUpdateQuery(query);
    query = QString("INSERT {<") + urn + ("> a nie:DataObject ; nie:url '") + toIri + ("'}");
    trackerUpdateQuery(query);
}

QString StorageTracker::generateIri(const QString &path)
{
    return generateIriForTracker(path);
}

bool StorageTracker::supportsProperty(MTPObjPropertyCode code) const
{
    return m_handlerTable.contains(code);
}

void StorageTracker::copy(const QString &fromPath, const QString &toPath)
{
    QString fromIri = generateIriForTracker(fromPath);
    QString toIri = generateIriForTracker(toPath);
    QString query;
    QVector<QStringList> resultSet;

    // Get the tracker URN for the from IRI
    trackerQuery(QString("SELECT ?f WHERE{?f a nie:DataObject ; nie:url '" + fromIri + QString("'}")), resultSet);
    if(0 == resultSet.size() || 0 == resultSet[0].size())
    {
        MTP_LOG_CRITICAL("Failed query for tracker URN" << fromPath);
        return;
    }
    QString urn = resultSet[0][0];

    resultSet.clear();

    // Now get all available properties for the from URN
    query = QString("SELECT ?p ?v WHERE{<") + urn + QString("> ?p ?v}");
    trackerQuery(query, resultSet);

    QStringList domains;
    QString insert = QString("DELETE {?file a rdfs:Resource} WHERE {?file nie:url '") + toIri + QString("'} INSERT { _:x a ");
    QString props;
    QString end = QString("}");
    // Iterate over the result set and get a map of properties that we support
    for(int i = 0; i < resultSet.size(); i++)
    {
        QString predicate = resultSet[i][0];
        QString value = resultSet[i][1];

        if(predicate == RDF_TYPE)
        {
            // Add this value to the domains field...
            domains.append(QString("<") + value + QString(">"));
        }
        else
        {
            // Check if the property is supported, if so, we retain it, else
            // remove it from the vector
            predicate = predicate.mid(predicate.lastIndexOf('/') + 1);
            predicate.replace('#', ':');
            if(false == isTrackerPropertySupported(predicate))
            {
                resultSet.remove(i);
                i--;
                continue;
            }
            else if(QString("nie:url") == predicate)
            {
                // Replace the value in nie:url with the new url
                value = QString(" '") + toIri + QString("' ");
            }
            else
            {
                value = QString(" '") + value + QString("' ");
            }
            props += QString(";") + predicate + value;
        }
    }
    // Formulate and execute query
    insert += domains.join(QString(","));
    insert += props;
    insert += end;

    trackerUpdateQuery(insert);
}

bool StorageTracker::isTrackerPropertySupported(const QString &property)
{
    return m_trackerPropertyTable.contains(property);
}
