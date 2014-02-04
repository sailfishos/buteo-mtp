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

#include <QDirIterator>
#include <QDomDocument>
#include <sys/stat.h>

using namespace meegomtp1dot0;

const char *FSStoragePluginFactory::CONFIG_DIR = "/etc/fsstorage.d";

QList<QString> FSStoragePluginFactory::configFiles() {
    QList<QString> result;

    QDirIterator it(CONFIG_DIR, QDir::Files);
    while (it.hasNext()) {
        QString fileName(it.next());
        if (fileName.endsWith(".xml", Qt::CaseInsensitive)) {
            result.append(fileName);
        }
    }

    MTP_LOG_INFO("Found" << result.size() << "FSStoragePlugin configurations.");

    return result;
}

StoragePlugin *FSStoragePluginFactory::create(QString configFileName,
                                              quint32 storageId) {

    QFile configFile(configFileName);
    if (!configFile.exists()) {
        MTP_LOG_CRITICAL(configFileName << "doesn't exist.");
        return 0;
    }

    if (!configFile.open(QFile::ReadOnly)) {
        MTP_LOG_CRITICAL(configFileName << "couldn't be opened for reading.");
        return 0;
    }

    QDomDocument configuration;
    configuration.setContent(&configFile);

    QDomElement storage = configuration.documentElement();

    if (storage.tagName() != "storage") {
        MTP_LOG_CRITICAL(configFileName << "is not a storage configuration.");
        return 0;
    }

    if (!storage.hasAttribute("path") ||
        !storage.hasAttribute("name") ||
        !storage.hasAttribute("description")) {
        MTP_LOG_WARNING("Storage" << configFileName << "is missing some of "
                "mandatory attributes 'name', 'path', and 'description'");
        return 0;
    }

    QDir dir(storage.attribute("path"));

    bool removable =
            !storage.attribute("removable").compare("true", Qt::CaseInsensitive);

    if (removable && !dir.isRoot()) {
        if (!dir.exists()) {
            MTP_LOG_WARNING("Storage path" << storage.attribute("path")
                    << "is not a directory");
            return 0;
        }

        struct stat storageStat;
        if (stat(dir.absolutePath().toUtf8(), &storageStat) != 0) {
            MTP_LOG_WARNING("Couldn't stat" << dir.absolutePath());
            return 0;
        }

        if (!dir.cdUp()) {
            MTP_LOG_WARNING("Couldn't get parent directory of " << dir);
            return 0;
        }

        struct stat parentStat;
        if (stat(dir.absolutePath().toUtf8(), &parentStat) != 0) {
            MTP_LOG_WARNING("Couldn't stat" << dir.absolutePath());
            return 0;
        }

        if (storageStat.st_dev == parentStat.st_dev) {
            MTP_LOG_INFO(dir << "is not mounted");
            return 0;
        }
    }

    FSStoragePlugin *result =
            new FSStoragePlugin(storageId,
                    removable ? MTP_STORAGE_TYPE_RemovableRAM: MTP_STORAGE_TYPE_FixedRAM,
                    storage.attribute("path"),
                    storage.attribute("name"),
                    storage.attribute("description"));

    const QDomNodeList &blacklist = storage.elementsByTagName("blacklist");
    for (int i = 0; i != blacklist.size(); ++i) {
        QString blacklistFileName(blacklist.at(i).toElement().text().trimmed());
        // Allow blacklist file paths relative to CONFIG_DIR.
        if (!blacklistFileName.startsWith('/')) {
            blacklistFileName.prepend('/').prepend(CONFIG_DIR);
        }

        QFile blacklistFile(blacklistFileName);
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

extern "C" QList<QString> storageConfigurations() {
    return FSStoragePluginFactory::configFiles();
}

extern "C" StoragePlugin* createStoragePlugin(QString configFileName,
                                              quint32 storageId) {
    return FSStoragePluginFactory::create(configFileName, storageId);
}

extern "C" void destroyStoragePlugin(StoragePlugin* storagePlugin) {
    if(storagePlugin) {
        delete storagePlugin;
    }
}
