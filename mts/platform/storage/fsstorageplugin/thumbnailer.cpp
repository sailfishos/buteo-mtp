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

/* How long to wait on startup before starting to generate
 * missing thumbnails [ms] */
#define THUMBNAIL_DELAY_ON_STARTUP 3000

/* How long to wait before generating thumbnails when new
 * pictures are spotted on the file system [ms] */
#define THUMBNAIL_DELAY_IN_RUNTIME 1000

/* Maximum number of images to pass in one thumbnail request */
#define THUMBNAIL_MAX_IMAGES_PER_REQUEST 128

Thumbnailer::Thumbnailer() :
    ThumbnailerProxy(THUMBNAILER_SERVICE, THUMBNAILER_GENERIC_PATH, QDBusConnection::sessionBus()),
    m_scheduler("foreground"), m_flavor("normal")
{
    /* Setup interval timer for combining multiple thumbnail
     * requests into one D-Bus method call. The first batch
     * of thumbnails that need to be generated will be delayed
     * a bit to keep fs changes minimal during startup.
     */
    m_thumbnailTimer = new QTimer(this);
    m_thumbnailTimer->setSingleShot(false);
    QObject::connect(m_thumbnailTimer, &QTimer::timeout,
                     this, &Thumbnailer::thumbnailDelayTimeout);
    m_thumbnailTimer->setInterval(THUMBNAIL_DELAY_ON_STARTUP);

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
    MTP_LOG_TRACE("Thumbnail ready!!");

    foreach(const QString &uri, uris)
    {
        /* Thumbnailer may use signals directed to us only
         * but could as well use regular broadcasts. Ignore
         * notifications that we are not interested in. */
        if(!m_uriAlreadyRequested.contains(uri))
            continue;

        m_uriAlreadyRequested.remove(uri);
        QString filePath = QUrl(uri).path();
        MTP_LOG_TRACE("Thumbnail ready for::" << filePath);
        emit thumbnailReady(filePath);
    }
}

void Thumbnailer::slotRequestStarted(uint /*handle*/)
{
    // Nothing to do here right now
}

void Thumbnailer::slotRequestFinished(uint handle)
{
    // Nothing to do here right now
}

enum {
    /* Derived from D-Bus API spec */
    THUMBNAILER_ERROR_UNSUPPORTED_URI_OR_MIME         = 0,
    THUMBNAILER_ERROR_SPECIALIZED_THUMBNAILER_MISSING = 1,
    THUMBNAILER_ERROR_INVALID_DATA_OR_MIME            = 2,
    THUMBNAILER_ERROR_SOURCE_IS_THUMBNAIL             = 3,
    THUMBNAILER_ERROR_FAILED_TO_WRITE                 = 4,
    THUMBNAILER_ERROR_UNSUPPORTED_FLAVOR              = 5,
};

void Thumbnailer::slotError(uint handle, const QStringList& uris, int errorCode, const QString& errorMsg)
{
    Q_UNUSED(handle);
    Q_UNUSED(uris);

    // We should always receive a "Finished", even if there is an Error, so we
    // need not clean our maps here.

    switch(errorCode) {
    case THUMBNAILER_ERROR_SOURCE_IS_THUMBNAIL:
        // Be silent about failures to thumbnail thumbnails
        break;

    case THUMBNAILER_ERROR_INVALID_DATA_OR_MIME:
        // Be silent about missing image loaders
        break;

    default:
        MTP_LOG_WARNING("Thumbnailer returned error code:" << errorCode
                        << "message: " << errorMsg);
        /* At least the currently used thumbnailer reports just one
         * file / error and the errorMsg identifies the source file
         * too -> listing uris would be just repetitive noise. */
#if 0
        foreach(const QString &uri, uris) {
            MTP_LOG_WARNING("for uri:: " << uri);
        }
#endif
        break;
    }
}

void Thumbnailer::requestThumbnailFinished(QDBusPendingCallWatcher *pcw)
{
    QDBusPendingReply<uint> reply = *pcw;

    if (reply.isError()) {
        MTP_LOG_WARNING("Failed to queue request to thumbnailer");
        MTP_LOG_WARNING("Error::" << reply.error());
    }

    pcw->deleteLater();

    /* Note: The uris are left in m_uriAlreadyRequested, which
     * implicitly blocks repeated thumbnailing attempts. */
}

void Thumbnailer::thumbnailDelayTimeout()
{
    if(m_uriRequestQueue.isEmpty()) {
        MTP_LOG_TRACE("Thumbnail queue is empty; stopping timer");
        m_thumbnailTimer->stop();

        /* Setup initial delay for the next patch of requests */
        m_thumbnailTimer->setInterval(THUMBNAIL_DELAY_IN_RUNTIME);
        return;
    }

    /* Dequeue image files & mime-types to thumbnail */
    QStringList uris;
    QStringList mimes;

    for(int i = 0; i < THUMBNAIL_MAX_IMAGES_PER_REQUEST; ++i) {
        if(m_uriRequestQueue.isEmpty())
            break;
        uris  << m_uriRequestQueue.takeFirst();
        mimes << m_uriRequestQueue.takeFirst();
    }

    /* Make an asynchronous thumbnail request via D-Bus */
    MTP_LOG_TRACE("Requesting" << uris.count() << "thumbnails");
    QDBusPendingCall pc = this->Queue(uris, mimes, m_flavor, m_scheduler, 0);
    QDBusPendingCallWatcher *pcw = new QDBusPendingCallWatcher(pc, this);
    connect(pcw, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(requestThumbnailFinished(QDBusPendingCallWatcher*)));

    /* Continue flushing the queue when mainloop gets idle */
    m_thumbnailTimer->setInterval(0);
}

QString Thumbnailer::requestThumbnail(const QString &filePath, const QString &mimeType)
{
    QString thumbPath;
    QString fileIri = IRI_PREFIX + filePath;
    // First check if a thumbnail is already present
    // If yes, then we simply return the thumbnail path
    if(!checkThumbnailPresent(filePath, thumbPath))
    {
        if(!m_uriAlreadyRequested.contains(fileIri)) {
            /* Add uri to queue, use dummy handle */
            m_uriAlreadyRequested.insert(fileIri, 0);
            m_uriRequestQueue.append(fileIri);
            m_uriRequestQueue.append(mimeType);

            /* Queue is flushed via timer to combine multiple
             * images into one thumbnail request. */
            if(!m_thumbnailTimer->isActive())
                m_thumbnailTimer->start();
        }
    }
    return thumbPath;
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
