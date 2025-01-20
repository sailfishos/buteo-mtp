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

#include "thumbnailer.h"
#include "trace.h"
#include "mtpresponder.h"

#include <QtDBus>

using namespace meegomtp1dot0;

static const unsigned THUMB_SIZE = 128;
static const bool UNBOUNDED_SIZE = true;
static const bool CROP_THUMBS = false;

const auto DBUS_ERROR_NAME_HAS_NO_OWNER = QStringLiteral("org.freedesktop.DBus.Error.NameHasNoOwner");
const auto THUMBNAILER_SERVICE = QStringLiteral("org.nemomobile.Thumbnailer");
const auto THUMBNAILER_OBJECT = QStringLiteral("/");
const auto THUMBNAILER_INTERFACE = QStringLiteral("org.nemomobile.Thumbnailer");
const auto THUMBNAILER_FETCH = QStringLiteral("Fetch");
const auto THUMBNAILER_FAILED = QStringLiteral("Failed");
const auto THUMBNAILER_FINISHED = QStringLiteral("Finished");
const auto THUMBNAILER_READY = QStringLiteral("Ready");

static const QString IRI_PREFIX = "file://";

/* How long to wait on startup before starting to generate
 * missing thumbnails [ms] */
#define THUMBNAIL_DELAY_ON_STARTUP 3000

/* How long to wait before generating thumbnails when new
 * pictures are spotted on the file system [ms] */
#define THUMBNAIL_DELAY_IN_RUNTIME 1000

/* Maximum number of images to pass in one thumbnail request */
#define THUMBNAIL_MAX_IMAGES_PER_REQUEST 128

QDBusArgument &operator<<(QDBusArgument &argument, const ThumbnailPath &item)
{
    argument.beginStructure();
    argument << item.filePath;
    argument << item.thumbnailPath;
    argument.endStructure();
    return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, ThumbnailPath &item)
{
    argument.beginStructure();
    argument >> item.filePath;
    argument >> item.thumbnailPath;
    argument.endStructure();
    return argument;
}

void Thumbnailer::registerTypes()
{
    static bool done = false;
    if (!done) {
        done = true;
        qRegisterMetaType<ThumbnailPath>("ThumbnailPath");
        qDBusRegisterMetaType<ThumbnailPath>();
        qRegisterMetaType<ThumbnailPathList>("ThumbnailPathList");
        qDBusRegisterMetaType<ThumbnailPathList>();
    }
}

Thumbnailer::Thumbnailer()
    : m_thumbnailerEnabled(false)
    , m_thumbnailerSuspended(false)
    , m_sessionBus(QDBusConnection::sessionBus())
{
    registerTypes();

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

    /* Do not issue new thumbnail requests while handling mtp commands */
    MTPResponder *responder = MTPResponder::instance();
    QObject::connect(responder, &MTPResponder::commandPending,
                     this, &Thumbnailer::suspendThumbnailing);
    QObject::connect(responder, &MTPResponder::commandFinished,
                     this, &Thumbnailer::resumeThumbnailing);

    m_sessionBus.connect(THUMBNAILER_SERVICE, THUMBNAILER_OBJECT, THUMBNAILER_INTERFACE, THUMBNAILER_FINISHED,
                        this, SLOT(slotFinished(quint32)));
    m_sessionBus.connect(THUMBNAILER_SERVICE, THUMBNAILER_OBJECT, THUMBNAILER_INTERFACE, THUMBNAILER_FAILED,
                        this, SLOT(slotFailed(quint32, QStringList)));
    m_sessionBus.connect(THUMBNAILER_SERVICE, QString(), THUMBNAILER_INTERFACE, THUMBNAILER_READY,
                        this, SLOT(slotReady(quint32, ThumbnailPathList)));

}

void Thumbnailer::slotReady(uint handle, ThumbnailPathList thumbnails)
{
    Q_UNUSED(handle);

    for (auto it = thumbnails.cbegin(), end = thumbnails.cend(); it != end; ++it) {
        const QString &uri((*it).filePath);
        const QString &thumbnailPath((*it).thumbnailPath);

        /* Thumbnailer may use signals directed to us only
         * but could as well use regular broadcasts. Ignore
         * notifications that we are not interested in. */
        if (!m_uriAlreadyRequested.contains(uri))
            continue;

        m_uriAlreadyRequested.remove(uri);

        QString filePath = QUrl(uri).path();
        m_thumbnailPaths.insert(filePath, thumbnailPath);
        MTP_LOG_TRACE("Thumbnail ready for::" << filePath << ":" << thumbnailPath);
        emit thumbnailReady(filePath);
    }
}

void Thumbnailer::slotFinished(uint handle)
{
    Q_UNUSED(handle);

    // Nothing to do here right now
}

void Thumbnailer::slotFailed(uint handle, const QStringList &uris)
{
    Q_UNUSED(handle);
    Q_UNUSED(uris);

    MTP_LOG_TRACE("Thumbnail request failed for:" << uris);
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
    if (m_uriRequestQueue.isEmpty()) {
        MTP_LOG_INFO("Thumbnail queue is empty; stopping dequeue timer");
        m_thumbnailTimer->stop();

        /* Setup initial delay for the next patch of requests */
        m_thumbnailTimer->setInterval(THUMBNAIL_DELAY_IN_RUNTIME);
        return;
    }

    /* Dequeue image files to thumbnail */
    QStringList uris;
    for (int i = 0; i < THUMBNAIL_MAX_IMAGES_PER_REQUEST; ++i) {
        if (m_uriRequestQueue.isEmpty())
            break;
        uris << m_uriRequestQueue.takeFirst();
    }

    /* Make an asynchronous thumbnail request via D-Bus */
    MTP_LOG_TRACE("Requesting" << uris.count() << "thumbnails");
    QDBusMessage request(
        QDBusMessage::createMethodCall(THUMBNAILER_SERVICE, THUMBNAILER_OBJECT, THUMBNAILER_INTERFACE, THUMBNAILER_FETCH));
    request << uris;
    request << quint32(THUMB_SIZE);
    request << bool(UNBOUNDED_SIZE);
    request << bool(CROP_THUMBS);
    QDBusPendingReply<quint32> pc(m_sessionBus.asyncCall(request));
    QDBusPendingCallWatcher *pcw = new QDBusPendingCallWatcher(pc, this);
    connect(pcw, &QDBusPendingCallWatcher::finished, this, &Thumbnailer::requestThumbnailFinished);

    /* Continue flushing the queue when mainloop gets idle */
    m_thumbnailTimer->setInterval(0);
}

void Thumbnailer::enableThumbnailing()
{
    if (!m_thumbnailerEnabled) {
        MTP_LOG_INFO("thumbnailer enabled");
        m_thumbnailerEnabled = true;
        scheduleThumbnailing();
    }
}

void Thumbnailer::suspendThumbnailing()
{
    if (!m_thumbnailerSuspended) {
        m_thumbnailerSuspended = true;
        scheduleThumbnailing();
    }
}

void Thumbnailer::resumeThumbnailing()
{
    if (m_thumbnailerSuspended) {
        m_thumbnailerSuspended = false;
        scheduleThumbnailing();
    }
}

void Thumbnailer::scheduleThumbnailing()
{
    bool activate = m_thumbnailerEnabled && !m_thumbnailerSuspended && !m_uriRequestQueue.isEmpty();

    if (activate) {
        if (!m_thumbnailTimer->isActive()) {
            MTP_LOG_TRACE("thumbnailer dequeue timer started");
            m_thumbnailTimer->start();
        }
    } else {
        if (m_thumbnailTimer->isActive()) {
            MTP_LOG_TRACE("thumbnailer dequeue timer stopped");
            m_thumbnailTimer->stop();
            m_thumbnailTimer->setInterval(THUMBNAIL_DELAY_IN_RUNTIME);
        }
    }
}

QString Thumbnailer::requestThumbnail(const QString &filePath, const QString &mimeType)
{
    Q_UNUSED(mimeType)

    QString thumbPath;
    QHash<QString, QString>::iterator it = m_thumbnailPaths.find(filePath);
    if (it != m_thumbnailPaths.end()) {
        thumbPath = it.value();
    } else {
        QString fileIri = IRI_PREFIX + filePath;
        if (!m_uriAlreadyRequested.contains(fileIri)) {
            /* Add uri to queue, use dummy handle */
            m_uriAlreadyRequested.insert(fileIri, 0);
            m_uriRequestQueue.append(fileIri);

            /* Queue is flushed via timer to combine multiple
             * images into one thumbnail request. */
            scheduleThumbnailing();
        }
    }
    return thumbPath;
}
