/*
 * This file is a part of buteo-mtp package.
 *
 * Copyright (C) 2014 Jolla Ltd.
 *
 * Contact: Jakub Adam <jakub.adam@jollamobile.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer. Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution. Neither the name of Jolla Ltd. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsstoragepluginfactory.h"

#include "fsstorageplugin.h"
#include "trace.h"

#include <QDomDocument>

using namespace meegomtp1dot0;

const char *FSStoragePluginFactory::CONFIG_FILE_PATH =
        "/etc/buteo/mtp-fsstorage-conf.xml";

FSStoragePluginFactory::FSStoragePluginFactory():
  configuration(new QDomDocument()) {
    QFile configFile(CONFIG_FILE_PATH);
    if (!configFile.exists()) {
        MTP_LOG_CRITICAL(CONFIG_FILE_PATH << "doesn't exist.");
        return;
    }

    if (!configFile.open(QFile::ReadOnly)) {
        MTP_LOG_CRITICAL(CONFIG_FILE_PATH << "couldn't be opened for reading.");
        return;
    }

    configuration->setContent(&configFile);

    MTP_LOG_INFO("Loaded FSStoragePlugin configuration with" << pluginCount()
            << "storages");
}

int FSStoragePluginFactory::pluginCount() const {
    return configuration->documentElement().elementsByTagName("storage").count();
}

StoragePlugin *FSStoragePluginFactory::createPlugin(quint8 pluginId,
                                                    quint32 storageId) {

    const QDomElement &storage =
            configuration->documentElement().elementsByTagName("storage").at(pluginId).toElement();
    if (storage.isNull()) {
        return 0;
    }

    if (!storage.hasAttribute("path") ||
        !storage.hasAttribute("name") ||
        !storage.hasAttribute("description")) {
        MTP_LOG_WARNING("Storage" << pluginId << "is missing some of "
                "mandatory attributes 'name', 'path', and 'description'");
        return 0;
    }

    bool removable =
            !storage.attribute("removable").compare("true", Qt::CaseInsensitive);

    FSStoragePlugin *result =
            new FSStoragePlugin(storageId,
                    removable ? MTP_STORAGE_TYPE_RemovableRAM: MTP_STORAGE_TYPE_FixedRAM,
                    storage.attribute("path"),
                    storage.attribute("name"),
                    storage.attribute("description"));

    const QDomNodeList &blacklist = storage.elementsByTagName("blacklist");
    for (int i = 0; i != blacklist.size(); ++i) {
        QFile blacklistFile(blacklist.at(i).toElement().text());
        if (!blacklistFile.open(QFile::ReadOnly)) {
            MTP_LOG_WARNING(blacklistFile.fileName() << "couldn't be opened "
                    "for reading.");
            continue;
        }

        while (!blacklistFile.atEnd()) {
            QString line = blacklistFile.readLine();
            if (line.startsWith('#')) {
                // Comment line.
                continue;
            }

            result->excludePath(line.trimmed());
        }
    }

    return result;
}

FSStoragePluginFactory::~FSStoragePluginFactory() {}

FSStoragePluginFactory& FSStoragePluginFactory::instance() {
    static QScopedPointer<FSStoragePluginFactory> instance;
    if (!instance) {
        instance.reset(new FSStoragePluginFactory());
    }

    return *instance;
}

extern "C" quint8 storageCount() {
    return FSStoragePluginFactory::instance().pluginCount();
}

extern "C" StoragePlugin* createStoragePlugin(const quint8 pluginId,
                                              const quint32& storageId) {
    return FSStoragePluginFactory::instance().createPlugin(pluginId, storageId);
}

extern "C" void destroyStoragePlugin(StoragePlugin* storagePlugin) {
    if(storagePlugin) {
        delete storagePlugin;
    }
}
