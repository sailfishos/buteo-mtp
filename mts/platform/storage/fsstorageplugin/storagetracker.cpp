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
static QString getSampleRate (const QString&);
static QString getNbrOfChannels (const QString&);
static QString getFramesPerThousandSecs (const QString&);
static QString getDRMStatus (const QString&);

static void trackerQuery(const QString&, QVector<QStringList> &res);
static void trackerUpdateQuery(const QString&);
static void convertResultByTypeAndCode(const QString&, QString&, MTPDataType, MTPObjPropertyCode, QVariant&);
static QString generateIriForTracker(const QString& path);

StorageTracker::StorageTracker()
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
    m_handlerTable[MTP_OBJ_PROP_Date_Created] = getDateCreated;
    m_handlerTable[MTP_OBJ_PROP_Name] = getName;
    m_handlerTable[MTP_OBJ_PROP_Artist] = getArtist;
    m_handlerTable[MTP_OBJ_PROP_Width] = getWidth;
    m_handlerTable[MTP_OBJ_PROP_Height] = getHeight;
    m_handlerTable[MTP_OBJ_PROP_Duration] = getDuration;
    m_handlerTable[MTP_OBJ_PROP_Track] = getTrack;
    m_handlerTable[MTP_OBJ_PROP_Genre] = getGenre;
    m_handlerTable[MTP_OBJ_PROP_Use_Count] = getUseCount;
    m_handlerTable[MTP_OBJ_PROP_Album_Name] = getAlbumName;
    m_handlerTable[MTP_OBJ_PROP_Sample_Rate] = getSampleRate;
    m_handlerTable[MTP_OBJ_PROP_Nbr_Of_Channels] = getNbrOfChannels;
    m_handlerTable[MTP_OBJ_PROP_Frames_Per_Thousand_Secs] = getFramesPerThousandSecs;
    m_handlerTable[MTP_OBJ_PROP_DRM_Status] = getDRMStatus;

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
    m_trackerPropertyTable.insert(QString("nfo:sampleRate"));
    m_trackerPropertyTable.insert(QString("nfo:channels"));
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
        MTP_LOG_CRITICAL("Failed query for tracker URN for path" << fromPath);
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
