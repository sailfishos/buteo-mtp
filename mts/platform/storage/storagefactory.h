/*
* This file is part of libmeegomtp package
*
* Copyright (C) 2010 Nokia Corporation. All rights reserved.
*
* Contact: Deepak Kodihalli <deepak.kodihalli@nokia.com>
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

#ifndef STORAGEFACTORY_H
#define STORAGEFACTORY_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QVector>

#include "mtptypes.h"

class QDir;

namespace meegomtp1dot0
{
class StoragePlugin;
class ObjectPropertyCache;

const QString pluginLocation = MTP_PLUGINDIR;
const QString CREATE_STORAGE_PLUGINS = "createStoragePlugins";
const QString DESTROY_STORAGE_PLUGIN = "destroyStoragePlugin";

typedef QList<StoragePlugin*> (*CREATE_STORAGE_PLUGINS_FPTR)( quint32 startingStorageId );
typedef void (*DESTROY_STORAGE_PLUGIN_FPTR)( StoragePlugin *storagePlugin );
}


/// StorageFactory is a factory class for creating storages. New storages are pluggable.

/// StorageFactory creates new storages on construction. It looks for storage plug-ins and loads them.
/// The class provides an interface to add, delete and access objects to the storages exposed in
/// the MTP session. It should abstract the details of the storage - be it internal memory or an
/// external card. The user only provides a storage id for interfacing, this class assigns unique
/// storage id's for the storages it creates and passes on the list of id's to the user.

namespace meegomtp1dot0
{
class StorageFactory : public QObject
{
    Q_OBJECT

public:
    /// Constructor.
    StorageFactory();

    /// Destructor.
    ~StorageFactory();

    /// Enumerates all the loaded storage plug-ins
    /// This helps in quicker construction of the factory( above ) and catches errors during enumeration.
    /// Call this immediately after creating the factory.
    /// \return tru or false depending success or failure of enumeration respectively.
    bool enumerateStorages( QVector<quint32>& failedStorageIds );

    /// Adds an item to the Storage Factory.
    /// \param storageId [in/out] which storage this item goes to.
    /// \param handle [out] the handle of the item's parent.
    /// \param parentHandle [out] the handle of this item.
    /// \param info pointer to a structure holding the object info dataset
    /// for this item.
    /// \return MTP response.
    MTPResponseCode addItem( quint32 &storageId, ObjHandle &parentHandle,
                              ObjHandle &handle, MTPObjectInfo *info ) const;

    /// Deletes an item from the Storage Factory
    /// \param handle [in] the handle of the object that needs to be deleted.
    /// \param formatCode [in] this optional arg can be used to delete all objects of a certain format.
    /// \return MTP response.
    MTPResponseCode deleteItem( const ObjHandle& handle, const MTPObjFormatCode& formatCode ) const;

    /// Returns the no. of items belonging to a certain format and/or contained in a certain folder.
    /// \return storageId [in] which storage to look for.
    /// \param formatCode [in] this optional arg can be used to delete all objects of a certain format.
    /// \param associationHandle [in] this optional argument can specify the containing folder.
    /// \param noOfObjects [out] no. of objects found.
    /// \return MTP response.
    MTPResponseCode getObjectHandles( const quint32& storageId, const MTPObjFormatCode& formatCode,
                                       const quint32& associationHandle, QVector<ObjHandle> &objectHandles ) const;

    /// Gets the id's of all the created storages.
    /// \param storageIds [out] A vector containing the storage id's.
    /// \return MTP response.
    MTPResponseCode storageIds( QVector<quint32>& storageIds );

    /// Checks the availability and validity of a storage.
    /// \param storageId [in] storage id.
    /// \return MTP response.
    MTPResponseCode checkStorage( quint32 storageId ) const;

    /// Searches for the given handle in all storages.
    /// \param handle [in] the object handle.
    /// \return MTP response.
    MTPResponseCode checkHandle( const ObjHandle &handle ) const;

    /// Gets the MTP storage info.
    /// \param storageId [in] storage id.
    /// \param storageInfo [out] populated storageInfo structure
    /// \return MTP response.
    MTPResponseCode storageInfo( const quint32& storageId, MTPStorageInfo &info );

    /// Given an object handle, gets the objects referenced by this object in a storage.
    /// \param handle [in] object handle.
    /// \param references [out] list of object references.
    /// \return MTP response.
    MTPResponseCode getReferences( const ObjHandle &handle , QVector<ObjHandle> &references ) const;

    /// For an object handle, sets objects references in a storage.
    /// \param handle [in] object handle.
    /// \param references [in] list of object references.
    /// \return MTP response.
    MTPResponseCode setReferences( const ObjHandle &handle , const QVector<ObjHandle> &references ) const;

    /// Copies an object within or across storages.
    /// \param handle [in] object to be copied.
    /// \param parentHandle [in] parent in destination location.
    /// \param storageId [in] destination storage.
    MTPResponseCode copyObject( const ObjHandle &handle, const ObjHandle &parentHandle, const quint32 &destinationStorageId, ObjHandle &copiedObjectHandle ) const;

    /// Moves an object within or across storages.
    /// \param handle [in] object to be moved.
    /// \param parentHandle [in] parent in destination location.
    /// \param storageId [in] destination storage.
    MTPResponseCode moveObject( const ObjHandle &handle, const ObjHandle &parentHandle, const quint32 &destinationStorageId ) const;

    /// Given an object handle, provides it's absolute path in its storage.
    /// \param handle [in] object handle.
    /// \param path [out] the absolute path.
    /// \return MTP response.
    MTPResponseCode getPath( const quint32 &handle, QString &path ) const;

    MTPResponseCode getEventsEnabled( const quint32 &handle, bool &eventsEnabled ) const;

    /// Given an object handle, provides the object's objectinfo dataset.
    /// \param handle [in] the object handle.
    /// \param objectInfr [out] pointer to a structure holding the objectinfo dataset.
    /// \return MTP response.
    MTPResponseCode getObjectInfo( const ObjHandle &handle, const MTPObjectInfo *&objectInfo ) const;

    /// Writes data onto a storage item.
    /// \param handle [in] the object handle.
    /// \writeBuffer [in] the data to be written.
    /// \bufferLen [in] the length of the data to be written.
    /// \param isFirstSegment [in] If true, this is the first segment in a multi segment write operation
    /// \param isLastSegment [in] If true, this is the final segment in a multi segment write operation
    MTPResponseCode writeData( const ObjHandle &handle, char *writeBuffer, quint32 bufferLen, bool isFirstSegment, bool isLastSegment ) const;

    /// Reads data from a storage item.
    /// \param handle [in] the object handle.
    /// \readBuffer [in] the buffer where data will written. The buffer must be allocated by the caller
    /// \readBufferLen [in, out] the length of the input buffer. At most this amount of data will be read from the object. The function will return the actual number of bytes read in this buffer
    /// \param readOffset [in] The offset, in bytes, into the object to be read from
    MTPResponseCode readData( const ObjHandle &handle, char *readBuffer, qint32 &readBufferLen, quint32 readOffset ) const;

    /// Truncates an item to a certain size.
    /// \param handle [in] the object handle.
    /// \size [in] the size in bytes.
    MTPResponseCode truncateItem( const ObjHandle &handle, const quint32 &size ) const;

    MTPResponseCode getObjectPropertyValue(const ObjHandle &handle,
            QList<MTPObjPropDescVal> &propValList);

    MTPResponseCode setObjectPropertyValue( const ObjHandle &handle, QList<MTPObjPropDescVal> &propValList, bool sendObjectPropList = false);

    /// \return true iff all storage plugins have completed enumeration
    bool storageIsReady();


public Q_SLOTS:
    /// This slot will take care of providing an object handle which a storage plug-in can assign to an object.
    /// A storage plug-in will emit an objectCreated signal when it needs a handle.
    /// \param handle [out] the populated object handle
    /// this handle, this would be a case when handles are maintained across sessions.
    void getObjectHandle( ObjHandle& );

    /// This slot will take care of providing a puoid which a storage plug-in can assign to an object.
    /// A storage plug-in will emit a puoid signal when it needs a handle.
    /// \param puoid [out] the puoid
    void getPuoid( MtpInt128& puoid );

    /// Storage plugins are connected to this to announce that they're
    /// done enumerating their object handles.
    void onStoragePluginReady(quint32 storageId);

    //// Session was opened/closed
    void sessionOpenChanged(bool isOpen);

Q_SIGNALS:
    /// Storage factory will emit this signal when it needs to know the largest value of a puoid
    /// a storage plug-in used for an object. This is so that for this session it can assign handles greater than the
    /// previous session's largest value, in case we are preserving puoids across sessions.
    /// \param handle [out] the populated puoid.
    void largestPuoid( MtpInt128& puoid );

    /// This signal is emitted for now to check if a transaction is cancelled.
    /// TODO If needed this can be generalized to receive other transport specific events
    /// \param txCancelled [out] If set to true, this indicates that the current MTP tx got cancelled.
    void checkTransportEvents( bool &txCancelled );

    /// Emitted when all storages have completed enumeration
    void storageReady();

private:
    quint32 m_storageId; ///< unique id for each storage.
    QHash<quint32,StoragePlugin*> m_allStorages; ///< all created storages, mapped by storage id.
    QString m_storagePluginsPath; ///<the path where StorageFactory will look for storage plug-ins.
    /// This structure helps mapping a StoragePlugin poiner to the handle to the plug-in library
    /// that created this storage plug-in.
    struct PluginHandlesInfo_
    {
        StoragePlugin *storagePluginPtr;
        void *storagePluginHandle;
    };
    QVector<PluginHandlesInfo_> m_pluginHandlesInfoVector; ///< This vector keeps track of all loaded plug-ins.

    QSet<quint32> m_readyStorages; ///< Storage ids of plugins that have emitted storageReady

    /// Assigns a unique storage id for the next storage.
    /// Storage id's are in the range [0x00000000,0xFFFFFFFF];
    /// \return the storage id.
    quint32 assignStorageId( quint16 storageNo, quint16 partitionNo = 0) const;

    StoragePlugin *storageOfHandle(ObjHandle handle) const;

    ObjHandle m_newObjectHandle;
    MtpInt128 m_newPuoid;

    QScopedPointer<ObjectPropertyCache> m_objectPropertyCache;

    /// Improves performance by preventing repeat mass fills of object property
    /// cache with StoragePlugin::getChildPropertyValues().
    QSet<ObjHandle> m_massQueriedAssociations;

private slots:
    /// This slot is called when some of the underlying storage plugins
    /// generates an MTP event.
    ///
    /// \param event [in] an MTP event code.
    /// \param params [in] a collection of event parameters.
    void onStorageEvent(MTPEventCode event, const QVector<quint32> &params);

#ifdef UT_ON
    friend class StorageFactory_test;
#endif
};
}

#endif
