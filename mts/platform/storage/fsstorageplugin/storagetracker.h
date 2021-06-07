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

#ifndef STORAGE_TRACKER_H
#define STORAGE_TRACKER_H

#include <QVariant>
#include <QList>
#include <QSet>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include "mtptypes.h"

class QString;

typedef QString (*fpTrackerQueryHandler)(const QString&);
typedef void (*fpTrackerUpdateQueryHandler)(const QString&, QString&, QStringList&, QString&);

namespace meegomtp1dot0
{
class StorageTracker : public QObject
{
#ifdef UT_ON
friend class FSStoragePlugin_test;
#endif
    public:
        StorageTracker();
        ~StorageTracker();
        bool getPropVals(const QString &filePath, QList<MTPObjPropDescVal> &propValList);
        //void setPropVals(const QString &filePath, QList<MTPObjPropDescVal> &propValList);
        //bool getObjectProperty(const QString& filePath, MTPObjPropertyCode ePropertyCode, MTPDataType type, QVariant& propVal);
        //bool setObjectProperty(const QString& filePath, MTPObjPropertyCode ePropertyCode, MTPDataType type, const QVariant& propVal);
        void getChildPropVals(const QString& parentPath,
                const QList<const MtpObjPropDesc *>& properties,
                QMap<QString, QList<QVariant> > &values);
        //void ignoreNextUpdate(const QStringList &filePaths);
        QString savePlaylist(const QString &playlistPath, QStringList &entries);
        void getPlaylists(QStringList &playlistIds, QList<QStringList> &lists, bool getExisting = false);
        bool isPlaylistExisting(const QString &path);
        void setPlaylistPath(const QString &name, const QString &path);
        void deletePlaylist(const QString &path);
        void movePlaylist(const QString &fromPath, const QString &toPath);
        void move(const QString &fromPath, const QString &toPath);
        void copy(const QString &fromPath, const QString &toPath);
        QString generateIri(const QString &path);
        bool supportsProperty(MTPObjPropertyCode code) const;

#if 0
    public Q_SLOTS:
        void ignoreNextUpdateFinished(QDBusPendingCallWatcher *pcw);
#endif

    private:
        QHash<MTPObjPropertyCode, fpTrackerQueryHandler> m_handlerTable;
#if 0
        QHash<MTPObjPropertyCode, fpTrackerUpdateQueryHandler> m_handlerTableUpdate;
#endif
        QSet<QString> m_trackerPropertyTable;
        void populateFunctionMap();
        QString buildQuery(const QString &filePath, QList<MTPObjPropDescVal> &propValList);
        QString buildMassQuery(const QString &path,
                const QList<const MtpObjPropDesc *> &properties);
#if 0
        QString buildUpdateQuery(const QString &filePath, QList<MTPObjPropDescVal> &propValList);
#endif
        bool isTrackerPropertySupported(const QString &property);
        //QDBusInterface m_minerInterface;
};
}
#endif

