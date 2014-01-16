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

#ifndef FSSTORAGEPLUGINFACTORY_H
#define FSSTORAGEPLUGINFACTORY_H

#include <QScopedPointer>

class QDomDocument;

namespace meegomtp1dot0 {

class StoragePlugin;

class FSStoragePluginFactory {
public:
    int pluginCount() const;
    StoragePlugin *createPlugin(quint8 pluginId, quint32 storageId);

    ~FSStoragePluginFactory();

    static FSStoragePluginFactory& instance();
private:
    FSStoragePluginFactory();
    Q_DISABLE_COPY(FSStoragePluginFactory);

    QScopedPointer<QDomDocument> configuration;
    static const char *CONFIG_FILE_PATH;
};

} // namespace meegomtp1dot0

extern "C" {

/// The StorageFactory uses this interface to get a number of individual
/// storages that are made available in the plug-in's configuration. A single
/// storage plug-in library may offer multiple StoragePlugin instances, e.g.
/// one for internal memory of a device and another for removable memory card.
quint8 storageCount();

/// The StorageFactory uses this interface to load new storage plug-ins.
/// The pluginId has to be >= 0 and < value returned by storageCount().
meegomtp1dot0::StoragePlugin* createStoragePlugin(const quint8 pluginId,
                                                  const quint32& storageId);

/// The StorageFactory uses this interface to destroy loaded storage plug-ins.
void destroyStoragePlugin(meegomtp1dot0::StoragePlugin *storagePlugin);

} // extern "C"

#endif // FSSTORAGEPLUGINFACTORY_H
