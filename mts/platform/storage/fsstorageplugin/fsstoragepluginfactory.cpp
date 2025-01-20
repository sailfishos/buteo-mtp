/*
 * This file is a part of buteo-mtp package.
 *
 * Copyright (c) 2014 - 2022 Jolla Ltd.
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
#include <QProcessEnvironment>
#include <QDBusInterface>
#include <QDBusReply>
#include <nemo-dbus/dbus.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mntent.h>
#include <unistd.h>
#include <glob.h>

#include <udisks2defines.h> // systemsettings

using namespace meegomtp1dot0;

#define UDISKS2_MANAGER_METHOD_GETBLOCKDEVICES "GetBlockDevices"
#define UDISKS2_BLOCK_PROPERTY_IDLABEL "IdLabel"
#define UDISKS2_FILESYSTEM_PROPERTY_MOUNTPOINTS "MountPoints"
#define DBUS_OBJECT_PROPERTIES_METHOD_GET "Get"

static QHash<QString, QString> *generateDeviceLabelHash()
{
    QHash<QString, QString> *deviceLabelHash = new QHash<QString, QString>;
    /* Buteo-mtp does not otherwise need / connect to systembus -> use a connection that can be closed */
    QString connectionName("privateSystemBusConnection");
    QDBusConnection systemBus(QDBusConnection::connectToBus(QDBusConnection::SystemBus, connectionName));
    QDBusInterface managerInterface(UDISKS2_SERVICE, UDISKS2_MANAGER_PATH, UDISKS2_MANAGER_INTERFACE, systemBus);
    QVariantMap defaultOptions({{QLatin1String("auth.no_user_interaction"), QVariant(true)}});
    QDBusReply<QList<QDBusObjectPath>> blockDevicesReply(
        managerInterface.call(UDISKS2_MANAGER_METHOD_GETBLOCKDEVICES, defaultOptions));
    if (!blockDevicesReply.isValid()) {
        QDBusError err(blockDevicesReply.error());
        MTP_LOG_WARNING(
            QString("failed to get block device list from udisks: %1: %2").arg(err.name()).arg(err.message()));
    } else {
        for (const QDBusObjectPath &object : blockDevicesReply.value()) {
            QDBusInterface
                propertiesInterface(UDISKS2_SERVICE, object.path(), DBUS_OBJECT_PROPERTIES_INTERFACE, systemBus);
            QDBusReply<QVariant> deviceLabelReply(
                propertiesInterface
                    .call(DBUS_OBJECT_PROPERTIES_METHOD_GET, UDISKS2_BLOCK_INTERFACE, UDISKS2_BLOCK_PROPERTY_IDLABEL));
            if (!deviceLabelReply.isValid()) {
                QDBusError err(deviceLabelReply.error());
                MTP_LOG_WARNING(QString("failed to get disk label for %1: %2: %3")
                                    .arg(object.path())
                                    .arg(err.name())
                                    .arg(err.message()));
                continue;
            }
            QString deviceLabel(deviceLabelReply.value().toString());
            if (deviceLabel.isEmpty())
                continue;
            QDBusReply<QVariant> mountPointsReply(propertiesInterface.call(
                DBUS_OBJECT_PROPERTIES_METHOD_GET,
                UDISKS2_FILESYSTEM_INTERFACE,
                UDISKS2_FILESYSTEM_PROPERTY_MOUNTPOINTS));
            if (!mountPointsReply.isValid()) {
                QDBusError err(mountPointsReply.error());
                MTP_LOG_WARNING(QString("failed to get mountpoints for %1: %2: %3")
                                    .arg(object.path())
                                    .arg(err.name())
                                    .arg(err.message()));
                continue;
            }
            QByteArrayList mountPointsList(NemoDBus::demarshallArgument<QByteArrayList>(mountPointsReply.value()));
            for (const QByteArray &bytes : mountPointsList)
                deviceLabelHash->insert(QString(bytes.constData()), deviceLabel);
        }
    }
    QDBusConnection::disconnectFromBus(connectionName);
    return deviceLabelHash;
}

static QString generateLabel(const QString &path)
{
    static QHash<QString, QString> *deviceLabelHash = nullptr;
    if (!deviceLabelHash)
        deviceLabelHash = generateDeviceLabelHash();
    QString label(deviceLabelHash->value(path));
    if (label.isEmpty()) {
        label = QLatin1String("Card");
        (*deviceLabelHash)[path] = label;
    }
    return label;
}

const char *FSStoragePluginFactory::CONFIG_DIR = "/etc/fsstorage.d";

static void makeLabelsUnique(QMap<QString, QString> &pathLabels, QSet<QString> &reservedLabels)
{
    /* The idea is, given a set of labels with duplicates like:
     * - "foo", "foo", "foo 2", "foo 3"
     *
     * The set is iterated and modified until all are unique:
     * - "foo 1", "foo 2", "foo 2", "foo 3"
     * - "foo 1", "foo 2.1", "foo 2.2", "foo 3"
     *
     * ... or maximum iteration count is reached - in which
     * case the rest of the logic ignores duplicate entries.
     */
    for (int iteration = 0; iteration < 5; ++iteration) {
        QMap<QString, int> labelCount;
        QMap<QString, int> labelIndex;
        // Seed counts from labels that have already been taken in use
        for (const QString &label : reservedLabels.values())
            labelCount[label] = labelCount.value(label, 0) + 1;
        // Check if current labels-to-add are unique and unused
        int maxCount = 0;
        for (const QString &label : pathLabels.values()) {
            int count = labelCount.value(label, 0) + 1;
            labelCount[label] = count;
            maxCount = qMax(maxCount, count);
        }
        if (maxCount < 2)
            break;
        // Modify conflicting labels
        for (const QString &path : pathLabels.keys()) {
            QString label = pathLabels[path];
            if (labelCount[label] < 2)
                continue;
            int index = labelIndex.value(label, 0) + 1;
            labelIndex[label] = index;
            if (iteration == 0)
                pathLabels[path] = QString("%1 %2").arg(label).arg(index);
            else
                pathLabels[path] = QString("%1.%2").arg(label).arg(index);
        }
    }
}

QList<StoragePlugin *> FSStoragePluginFactory::create(quint32 storageId)
{
    QSet<QString> alreadyExported;
    QSet<QString> reservedLabels;
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

        if (!storage.hasAttribute("path") && !storage.hasAttribute("blockdev")) {
            MTP_LOG_WARNING(
                "Storage" << fileName
                          << "has neither 'path' nor"
                             " 'blockdev' attributes.");
            continue;
        }

        if (storage.hasAttribute("path") && storage.hasAttribute("blockdev")) {
            MTP_LOG_WARNING(
                "Storage" << fileName
                          << "has mutually exclusive"
                             " 'path' and 'blockdev' attributes.");
            continue;
        }

        if (!storage.hasAttribute("name") || !storage.hasAttribute("description")) {
            MTP_LOG_WARNING(
                "Storage" << fileName
                          << "is missing some of "
                             "mandatory attributes 'name' and 'description'");
            continue;
        }

        bool removable = !storage.attribute("removable").compare("true", Qt::CaseInsensitive);

        QStringList blacklistPaths;
        const QDomNodeList &blacklist = storage.elementsByTagName("blacklist");
        for (int i = 0; i != blacklist.size(); ++i) {
            QString blacklistFileName(blacklist.at(i).toElement().text().trimmed());
            // Allow blacklist file paths relative to CONFIG_DIR.
            if (!blacklistFileName.startsWith('/'))
                blacklistFileName.prepend('/').prepend(CONFIG_DIR);

            QFile blacklistFile(blacklistFileName);
            if (!blacklistFile.open(QFile::ReadOnly)) {
                MTP_LOG_WARNING(blacklistFile.fileName() << "couldn't be opened for reading.");
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

        // directory path to storage label mapping
        QMap<QString, QString> pathLabels;
        if (storage.hasAttribute("path")) {
            /* Treat 'path' attribute as a glob pattern. If it actually
             * contains wildcard characters, use basenames of matching
             * directories as substitute for 'description' attribute. */
            QString pattern = storage.attribute("path");
            // If path contains "%u" replace it with the current user name
            if (pattern.contains("%u")) {
                pattern.replace("%u", QProcessEnvironment::systemEnvironment().value("USER"));
            }
            // If path contains "%h" replace it with the current home directory
            if (pattern.contains("%h")) {
                pattern.replace("%h", QProcessEnvironment::systemEnvironment().value("HOME"));
            }
            QString description;
            if (!pattern.contains('*') && !pattern.contains('?')) {
                description = storage.attribute("description");
            }
            glob_t gl;
            memset(&gl, 0, sizeof gl);
            glob(qUtf8Printable(pattern), GLOB_ONLYDIR, 0, &gl);
            for (size_t i = 0; i < gl.gl_pathc; ++i) {
                struct stat st;
                if (lstat(gl.gl_pathv[i], &st) == -1)
                    continue;
                if (!S_ISDIR(st.st_mode))
                    continue;
                QString path = QString::fromUtf8(gl.gl_pathv[i]);
                if (pathLabels.contains(path) || alreadyExported.contains(path))
                    continue;
                QString desc(description);
                description.clear();
                if (desc.isEmpty())
                    desc = generateLabel(path);
                pathLabels[path] = desc;
            }
            globfree(&gl);
        } else {
            // to be useful.
            QString blockdev = storage.attribute("blockdev");
            FILE *mntf = setmntent(_PATH_MOUNTED, "r");
            if (!mntf) {
                MTP_LOG_WARNING("could not list mounted filesystems");
                continue;
            }
            while (struct mntent *ent = getmntent(mntf)) {
                QString path = QString::fromUtf8(ent->mnt_dir);
                if (pathLabels.contains(path) || alreadyExported.contains(path))
                    continue;
                QString devname(ent->mnt_fsname);
                // Use startsWith to catch partitions of the main dev
                if (devname.startsWith(blockdev)) {
                    // Build the desc from the description attribtute
                    // and the partition part of the device name.
                    // TODO: look up fs label using libblkid?
                    devname.remove(0, blockdev.size());
                    QString description = storage.attribute("description");
                    if (!devname.isEmpty() && devname != "p1")
                        description.append(" ").append(devname);
                    pathLabels[path] = description;
                }
            }
        }

        makeLabelsUnique(pathLabels, reservedLabels);

        for (const QString &path : pathLabels.keys()) {
            /* Make sure a directory gets exported only once even if
             * it would match multiple configuration files. */
            if (alreadyExported.contains(path))
                continue;
            const QString desc = pathLabels[path];
            if (reservedLabels.contains(desc))
                continue;
            reservedLabels.insert(desc);
            alreadyExported.insert(path);

            // The description is the important part; name is mostly internal.
            // descs[desc] is the directory to export.
            FSStoragePlugin *plugin =
                new FSStoragePlugin(storageId,
                                    removable ? MTP_STORAGE_TYPE_RemovableRAM
                                    : MTP_STORAGE_TYPE_FixedRAM,
                                    path,
                                    storage.attribute("name"),
                                    desc);

            foreach (QString line, blacklistPaths) {
                plugin->excludePath(line);
            }

            result.append(plugin);
            storageId++;
        }
    }

    return result;
}

extern "C" QList<StoragePlugin *> createStoragePlugins(quint32 storageId)
{
    return FSStoragePluginFactory::create(storageId);
}

extern "C" void destroyStoragePlugin(StoragePlugin *storagePlugin)
{
    if (storagePlugin) {
        delete storagePlugin;
    }
}
