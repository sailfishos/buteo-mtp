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

#include <QString>

namespace meegomtp1dot0 {

class StoragePlugin;

class FSStoragePluginFactory {
public:
    static QList<StoragePlugin *> create(quint32 startingStorageId);

private:
    FSStoragePluginFactory();
    Q_DISABLE_COPY(FSStoragePluginFactory);

    static const char *CONFIG_DIR;
};

} // namespace meegomtp1dot0

extern "C" {

/// A single storage plug-in library may offer multiple StoragePlugin instances,
/// e.g. one for internal memory of a device and another for removable memory
/// card. Each storage has its configuration stored in a separate XML file.
/// This function returns a list of created instances, with have storage ids
/// numbered sequentially from storageId.
QList<meegomtp1dot0::StoragePlugin *> createStoragePlugins(quint32 storageId);

/// The StorageFactory uses this interface to destroy loaded storage plug-ins.
void destroyStoragePlugin(meegomtp1dot0::StoragePlugin *storagePlugin);

} // extern "C"

#endif // FSSTORAGEPLUGINFACTORY_H
