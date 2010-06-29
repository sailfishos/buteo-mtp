/*
* This file is part of libmeegomtp package
*
* Copyright (C) 2010 Nokia Corporation. All rights reserved.
*
* Contact: Sateesh Kavuri <sateesh.kavuri@nokia.com>
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

#ifndef THUMBNAILERPROXY_H_1262667623
#define THUMBNAILERPROXY_H_1262667623

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface org.freedesktop.thumbnails.Thumbnailer1
 */
class ThumbnailerProxy: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.freedesktop.thumbnails.Thumbnailer1"; }

public:
    ThumbnailerProxy(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~ThumbnailerProxy();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> Dequeue(uint handle)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(handle);
        return asyncCallWithArgumentList(QLatin1String("Dequeue"), argumentList);
    }

    inline QDBusPendingReply<QStringList> GetFlavors()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("GetFlavors"), argumentList);
    }

    inline QDBusPendingReply<QStringList> GetSchedulers()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("GetSchedulers"), argumentList);
    }

    inline QDBusPendingReply<QStringList, QStringList> GetSupported()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("GetSupported"), argumentList);
    }
    inline QDBusReply<QStringList> GetSupported(QStringList &mime_types)
    {
        QList<QVariant> argumentList;
        QDBusMessage reply = callWithArgumentList(QDBus::Block, QLatin1String("GetSupported"), argumentList);
        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() == 2) {
            mime_types = qdbus_cast<QStringList>(reply.arguments().at(1));
        }
        return reply;
    }

    inline QDBusPendingReply<uint> Queue(const QStringList &uris, const QStringList &mime_types, const QString &flavor, const QString &scheduler, uint handle_to_dequeue)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(uris) << qVariantFromValue(mime_types) << qVariantFromValue(flavor) << qVariantFromValue(scheduler) << qVariantFromValue(handle_to_dequeue);
        return asyncCallWithArgumentList(QLatin1String("Queue"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void Error(uint handle, const QStringList &failed_uris, int error_code, const QString &message);
    void Finished(uint handle);
    void Ready(uint handle, const QStringList &uris);
    void Started(uint handle);
};

namespace org {
  namespace freedesktop {
    namespace thumbnails {
      typedef ::ThumbnailerProxy Thumbnailer1;
    }
  }
}
#endif

