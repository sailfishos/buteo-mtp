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
#include <mntent.h>

using namespace meegomtp1dot0;

const char *FSStoragePluginFactory::CONFIG_DIR = "/etc/fsstorage.d";

QList<StoragePlugin *>FSStoragePluginFactory::create(quint32 storageId)
{
    QList<StoragePlugin *> result;
    QDirIterator it(CONFIG_DIR, QDir::Files);
    while (it.hasNext()) {
        QString fileName(it.next());
        if (!fileName.endsWith(".xml", Qt::CaseInsensitive))
            continue;

        MTP_LOG_INFO("FSStoragePlugin configuring " << fileName);

        QFile configFile(fileName);
        if (!configFile.open(QFile::ReadOnly)) {
            MTP_LOG_CRITICAL(fileName << "couldn't be opened for reading.");
            continue;
        }

        QDomDocument configuration;
        configuration.setContent(&configFile);

        QDomElement storage = configuration.documentElement();

        if (storage.tagName() != "storage") {
            MTP_LOG_CRITICAL(fileName << "is not a storage configuration.");
            continue;
        }

        if (!storage.hasAttribute("path")
            && !storage.hasAttribute("blockdev")) {
            MTP_LOG_WARNING("Storage" << fileName << "has neither 'path' nor"
                    "'blockdev' attributes.");
            continue;
        }

        if (!storage.hasAttribute("name") ||
            !storage.hasAttribute("description")) {
            MTP_LOG_WARNING("Storage" << fileName << "is missing some of "
                    "mandatory attributes 'name' and 'description'");
            continue;
        }

        bool removable =
            !storage.attribute("removable").compare("true", Qt::CaseInsensitive);

        QStringList blacklistPaths;
        const QDomNodeList &blacklist = storage.elementsByTagName("blacklist");
        for (int i = 0; i != blacklist.size(); ++i) {
            QString blacklistFileName(blacklist.at(i).toElement().text().trimmed());
            // Allow blacklist file paths relative to CONFIG_DIR.
            if (!blacklistFileName.startsWith('/'))
                blacklistFileName.prepend('/').prepend(CONFIG_DIR);

            QFile blacklistFile(blacklistFileName);
            if (!blacklistFile.open(QFile::ReadOnly)) {
                MTP_LOG_WARNING(blacklistFile.fileName()
                        << "couldn't be opened for reading.");
                continue;
            }

            while (!blacklistFile.atEnd()) {
                QString line = blacklistFile.readLine();
                if (line.startsWith('#')) {
                    // Comment line.
                    continue;
                }

                blacklistPaths.append(line.trimmed());
            }
        }

        QStringList paths;
        if (storage.hasAttribute("path")) {
            paths.append(storage.attribute("path"));
        } else {
            // TODO: use QStorageInfo here once it provides enough information
            // to be useful.
            QString blockdev = storage.attribute("blockdev");
            FILE *mntf = setmntent(_PATH_MOUNTED, "r");
            if (!mntf) {
                MTP_LOG_WARNING("could not list mounted filesystems");
                continue;
            }
            while (struct mntent *ent = getmntent(mntf)) {
                // Use startsWith to catch partitions of the main dev
                if (QString(ent->mnt_fsname).startsWith(blockdev))
                    paths.append(ent->mnt_dir);
            }
        }

        foreach (QString path, paths) {
            // TODO: for blockdev configs, add partition number to name
            // or look up fs label (using libblkid?)
            FSStoragePlugin *plugin =
                new FSStoragePlugin(storageId,
                    removable ? MTP_STORAGE_TYPE_RemovableRAM
                              : MTP_STORAGE_TYPE_FixedRAM,
                    path,
                    storage.attribute("name"),
                    storage.attribute("description"));

            foreach (QString line, blacklistPaths) {
                plugin->excludePath(line);
            }

            result.append(plugin);
            storageId++;
        }
    }

    return result;
}

extern "C" QList<StoragePlugin*> createStoragePlugins(quint32 storageId)
{
    return FSStoragePluginFactory::create(storageId);
}

extern "C" void destroyStoragePlugin(StoragePlugin* storagePlugin) {
    if(storagePlugin) {
        delete storagePlugin;
    }
}
