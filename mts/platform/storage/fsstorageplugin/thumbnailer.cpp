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

#include <QtDBus>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <buteosyncfw5/SyncDBusConnection.h>
#else
#include <buteosyncfw/SyncDBusConnection.h>
#endif
#include "thumbnailer.h"
#include "trace.h"

using namespace meegomtp1dot0;

static const int THUMB_WIDTH = 124;
static const int THUMB_HEIGHT = 124;
static const QString IRI_PREFIX = "file://";
static const QString THUMBNAILER_SERVICE = "org.freedesktop.thumbnails.Thumbnailer1";
static const QString THUMBNAILER_GENERIC_PATH = "/org/freedesktop/thumbnails/Thumbnailer1";
static const QString NORMAL_DIR = "/normal";
static const QString THUMB_DIR = "/.thumbnails";


Thumbnailer::Thumbnailer() :
    ThumbnailerProxy(THUMBNAILER_SERVICE, THUMBNAILER_GENERIC_PATH, Buteo::SyncDBusConnection::sessionBus()), MAX_REQ_MAP_SIZE(50),
    m_scheduler("foreground"), m_flavor("normal")
{
    QString thumbBaseDir = QDir::homePath() + THUMB_DIR;

    m_thumbnailDirs << (thumbBaseDir + NORMAL_DIR);

    QObject::connect(this, SIGNAL(Finished(uint)),
                     this, SLOT(slotRequestFinished(uint)));
    QObject::connect(this, SIGNAL(Started(uint)),
                     this, SLOT(slotRequestStarted(uint)));
    QObject::connect(this, SIGNAL(Ready(uint, const QStringList &)),
                     this, SLOT(slotThumbnailReady(uint, const QStringList &)));
    QObject::connect(this, SIGNAL(Error(uint, const QStringList &, int, const QString &)),
                     this, SLOT(slotError(uint, const QStringList &, int, const QString &)));
}

void Thumbnailer::slotThumbnailReady(uint handle, const QStringList& uris)
{
    QString filePath;
    MTP_LOG_TRACE("Thumbnail ready!!");
    if(m_requestMap.contains(handle))
    {
        foreach(QString uri, uris)
        {
            filePath = QUrl(uri).path();
            MTP_LOG_TRACE("Thumbnail ready for::" << filePath);
            emit thumbnailReady(filePath);
            m_uriMap.remove(uri);
        }
        m_requestMap.remove(handle);
    }
}

void Thumbnailer::slotRequestStarted(uint /*handle*/)
{
    // Nothing to do here right now
}

void Thumbnailer::slotRequestFinished(uint handle)
{
    // We may get a Finished without getting a Ready? In which case, we remove
    // the request from our internal map...
    MTP_LOG_TRACE("Thumbnailer request finished::" << handle);
    QString iri = m_requestMap.value(handle);
    m_requestMap.remove(handle);
    // Remove URI...
    m_uriMap.remove(iri);
}

void Thumbnailer::slotError(uint /*handle*/, const QStringList& uris, int /*errorCode*/, const QString& /*errorMsg*/)
{
    // We should always receive a "Finished", even if there is an Error, so we
    // need not clean our maps here.
    foreach(QString uri, uris)
    {
        MTP_LOG_WARNING("Thumbnailer returned error for:: " << uri);
    }
}

QString Thumbnailer::requestThumbnail(const QString &filePath, const QString &mimeType)
{
    QString thumbPath;
    QString fileIri = IRI_PREFIX + filePath;
    // First check if a thumbnail is already present
    // If yes, then we simply return the thumbnail path
    if(!checkThumbnailPresent(filePath, thumbPath) && !m_uriMap.contains(fileIri))
    {
        // Here we initiate a request to the thumbnailer to generate a new thumbnail
        QStringList uris;
        QStringList mimes; // Currently empty -- do we need to fill these?
        uris << fileIri;
        mimes << mimeType;
        QDBusPendingReply<uint> reply = this->Queue(uris, mimes, m_flavor, m_scheduler, 0);
        reply.waitForFinished();
        if(reply.isError())
        {
            MTP_LOG_WARNING("Failed to queue request to thumbnailer::" << filePath);
            MTP_LOG_WARNING("Error::" << reply.error());
        }
        recordRequest(fileIri, reply.value());
        // TODO: Do we need to store the handle? It is not needed as we only rely on the Ready callback
    }
    return thumbPath;
}

void Thumbnailer::recordRequest(const QString& fileIri, uint reqHandle)
{
    if(m_requestMap.size() > MAX_REQ_MAP_SIZE || m_uriMap.size() > MAX_REQ_MAP_SIZE)
    {
        MTP_LOG_WARNING("Thumbnailer::Exceeded max req size!");
    }
    else
    {
        m_requestMap.insert(reqHandle, fileIri);
        m_uriMap.insert(fileIri, reqHandle);
    }
}

bool Thumbnailer::checkThumbnailPresent(const QString& filePath, QString& thumbPath)
{
    QString fileIri = IRI_PREFIX + filePath;
    // Generate the MD5 for the file iri
    QCryptographicHash hasher(QCryptographicHash::Md5);
    QByteArray bArr = fileIri.toUtf8();
    const char* fileIriUtf8 = bArr.data();
    hasher.addData(fileIriUtf8, qstrlen(fileIriUtf8));
    QByteArray fileHash = hasher.result();
    QString hexHash = QString(fileHash.toHex());

    foreach(QString candidateDir, m_thumbnailDirs)
    {
        QString file = candidateDir + "/" + hexHash;
        QString ret;
        if(QFile::exists(ret = (file + ".jpeg")) || QFile::exists(ret = (file + ".png")))
        {
            QFileInfo fileInfo(filePath);
            QFileInfo thumbInfo(ret);
            if(thumbInfo.lastModified() >= fileInfo.lastModified())
            {
                MTP_LOG_TRACE("Thumbnail file::::" << file);
                thumbPath = ret;
                return true;
            }
        }
    }
    return false;
}

#if 0
// Including this function means linking to QTGui (for QImage)
// Commenting as this function is no longer used
QByteArray Thumbnailer::generateThumbnail(const QString& path)
{
    ExifData *d;
    QByteArray ret;

    // First try to extract the EXIF thumbnail
    d = exif_data_new_from_file(path.toUtf8().data());

    if(d && d->data)
    {
        ret.resize(d->size);
        memcpy(ret.data(), d->data, d->size);
        exif_data_unref(d);
        return ret;
    }
    else
    {
        QImage img(path);
        img = img.scaled(QSize(THUMB_WIDTH, THUMB_HEIGHT));
        QBuffer buf(&ret);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "JPEG");
    }
    return ret;
}
#endif
