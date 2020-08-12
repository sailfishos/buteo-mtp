/*
* This file is part of libmeegomtp package
*
* Copyright (c) 2010 Nokia Corporation. All rights reserved.
* Copyright (c) 2014 - 2020 Jolla Ltd.
* Copyright (c) 2020 Open Mobile Platform LLC.
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

#ifndef STORAGEPLUGIN_H
#define STORAGEPLUGIN_H

#include <QObject>
#include <QHash>
#include <QVector>
#include <QList>
#include "mtptypes.h"

/// StoragePlugin is a base class for MTP storage plug-ins.

/// StoragePlugin serves as an abstract interface for creating MTP storage plug-ins.
/// The StorageFactory classes loads storage plug-ins of the type StoragePlugin, thus
/// abstracting the actual plug-in type to the user.

namespace meegomtp1dot0
{
class StoragePlugin : public QObject
{
    Q_OBJECT

public:
    /// Constructor.
    StoragePlugin(quint32 storageId): m_storageId(storageId) {}

    /// Destructor.
    virtual ~StoragePlugin(){}

    quint32 storageId() const {
        return m_storageId;
    }

    /// Stop sending change events for all objects
    virtual void disableObjectEvents() = 0;


    /// Enumerate the storage.
    /// \return true or false depending on whether storage succeeded or failed.
    virtual bool enumerateStorage() = 0;

    /// Adds an item to the storage.
    ///
    /// \param parentHandle [out] the handle of the item's parent.
    /// \param handle [out] the handle of this item.
    /// \param info pointer to a structure holding the object info dataset
    /// for this item.
    ///
    /// \return MTP response.
    virtual MTPResponseCode addItem( ObjHandle &parentHandle, ObjHandle &handle, MTPObjectInfo *info ) = 0;

    /// Deletes an item from the storage.
    ///
    /// \param handle [in] the handle of the object that needs to be deleted.
    /// \param formatCode [in] this optional arg can be used to delete all objects of a certain format.
    /// \return MTP response.
    virtual MTPResponseCode deleteItem( const ObjHandle& handle, const MTPObjFormatCode& formatCode ) = 0;

    /// Recursively copies objects from another storage.
    ///
    /// The duplicated object hierarchy will retain its original object handles.
    /// It is a responsibility of the method's caller to ensure the handles are
    /// kept unique from the initiator's point of view, and thus disowned from
    /// their previous storage.
    ///
    /// \param sourceStorage [in] a source StoragePlugin.
    /// \param source [in] handle of the object to duplicate into this storage.
    /// \param parent [in] handle of an existing association in this storage
    ///               that will become parent of the copied object hierarchy.
    ///
    /// \return MTP response.
    virtual MTPResponseCode copyHandle( StoragePlugin *sourceStorage,
            ObjHandle source, ObjHandle parent ) = 0;

    /// Fills a collection with handles of child object of a given association
    /// (folder).
    ///
    /// \param formatCode [in] if not zero, filters the set of handles only to
    ///                   objects of particular type.
    /// \param associationHandle [in] the association whose children to get.
    /// \param objectHandles [out] method fills this collection with the
    ///                      requested object handles.
    ///
    /// \return MTP response.
    virtual MTPResponseCode getObjectHandles( const MTPObjFormatCode& formatCode, const quint32& associationHandle,
                                              QVector<ObjHandle> &objectHandles ) const = 0;

    /// Searches for the given object handle in this storage.
    ///
    /// \param handle [in] the object handle.
    ///
    /// \return true if this object handle exists, false otherwise.
    virtual bool checkHandle( const ObjHandle &handle ) const = 0;

    /// Gets the storage info.
    /// \param storageId [in] storage id.
    /// \param storageType [out] populated storage info structure.
    /// \return MTP response.
    virtual MTPResponseCode storageInfo( MTPStorageInfo &info ) = 0;

    /// Given an object handle, gets the objects referenced by this object in a storage.
    /// \param handle [in] object handle.
    /// \param references [out] list of object references.
    /// \return MTP response.
    virtual MTPResponseCode getReferences( const ObjHandle &handle , QVector<ObjHandle> &references ) = 0;

    /// For an object handle, sets objects references in a storage.
    /// \param handle [in] object handle.
    /// \param references [in] list of object references.
    /// \return MTP response.
    virtual MTPResponseCode setReferences( const ObjHandle &handle , const QVector<ObjHandle> &references ) = 0;

    /// Copies an object within or across storages.
    /// \param handle [in] object to be copied.
    /// \param parentHandle [in] parent in destination location.
    /// \param destinationStorage [in] destination storage; NULL means copy
    ///                           happens within a single storage.
    /// \param copiedObjectHandle [out] The handle to the copied object is returned in this
    /// \param recursionCounter [in] The recursion depth
    virtual MTPResponseCode copyObject( const ObjHandle &handle, const ObjHandle &parentHandle, StoragePlugin *destinationStorage, ObjHandle &copiedObjectHandle, quint32 recursionDepth = 0) = 0;

    /// Moves an object within or across storages.
    /// \param handle [in] object to be moved.
    /// \param parentHandle [in] parent in destination location.
    /// \param destinationStorage [in] destination storage.
    virtual MTPResponseCode moveObject( const ObjHandle &handle, const ObjHandle &parentHandle, StoragePlugin *destinationStorage, bool movePhysically = true ) = 0;

    /// Given an object handle, provides it's absolute path in its storage.
    /// \param handle [in] object handle.
    /// \param path [out] the absolute path.
    /// \return MTP response.
    virtual MTPResponseCode getPath( const quint32 &handle, QString &path ) const = 0;

    virtual MTPResponseCode getEventsEnabled( const quint32 &handle, bool &eventsEnabled ) const = 0;
    virtual MTPResponseCode setEventsEnabled( const quint32 &handle, bool eventsEnabled ) const = 0;

    /// Given an object handle, provides the object's objectinfo dataset.
    /// \param handle [in] the object handle.
    /// \param objectInfo [out] pointer to a structure holding the objectinfo dataset.
    /// \return MTP response.
    virtual MTPResponseCode getObjectInfo( const ObjHandle &handle, const MTPObjectInfo *&objectInfo ) = 0;

    /// Writes data onto a storage item.
    /// \param handle [in] the object handle.
    /// \param writeBuffer [in] the data to be written.
    /// \param bufferLen [in] the length of the data to be written.
    /// \param isFirstSegment [in] If true, this is the first segment in
    ///                       a multi-segment write operation
    /// \param isLastSegment [in] If true, this is the final segment in
    ///                      a multi-segment write operation
    virtual MTPResponseCode writeData( const ObjHandle &handle, char *writeBuffer, quint32 bufferLen, bool isFirstSegment, bool isLastSegment ) = 0;
    virtual MTPResponseCode writePartialData(const ObjHandle &handle, quint64 offset, const quint8 *dataContent, quint32 dataLength, bool isFirstSegment, bool isLastSegment) = 0;

    /// Reads data from a storage item.
    /// \param handle [in] the object handle.
    /// \param readBuffer [in] the buffer where data will written; must be
    ///                   allocated by the caller.
    /// \param readBufferLen [in] number of bytes to read
    /// \param readOffset [in] The offset, in bytes, into the object to start reading from
    virtual MTPResponseCode readData( const ObjHandle &handle, char *readBuffer, quint32 readBufferLen, quint64 readOffset ) = 0;

    /// Truncates an item to a certain size.
    /// \param handle [in] the object handle.
    /// \size [in] the size in bytes.
    virtual MTPResponseCode truncateItem( const ObjHandle &handle, const quint64 &size ) = 0;

    /// Retrieves the values of given object properties.
    ///
    /// \param handle [in] an object handle.
    /// \param propValList [in, out] a list of MTPObjPropDescVal objects whose
    ///                    descriptions determine the object property to
    ///                    retrieve. The values should be invalid QVariants,
    ///                    which will be filled by the method.
    ///
    /// \return MTP response.
    virtual MTPResponseCode getObjectPropertyValue(const ObjHandle &handle,
            QList<MTPObjPropDescVal> &propValList) = 0;

    virtual MTPResponseCode setObjectPropertyValue( const ObjHandle &handle, QList<MTPObjPropDescVal> &propValList, bool sendObjectPropList = false ) = 0;

    /// Retrieves the values of given object properties for all child objects of
    /// given association.
    ///
    /// \param handle [in] a handle of an association
    /// \param properties [in] a list describing the properties to retrieve.
    /// \param values [out] the method fills this structure with the query
    ///               result, where keys are handles of child objects and values
    ///               are the properties of the object in the same order as the
    ///               descriptions in the \c properties list have.
    virtual MTPResponseCode getChildPropertyValues(ObjHandle handle,
            const QList<const MtpObjPropDesc *>& properties,
            QMap<ObjHandle, QList<QVariant> > &values) = 0;

signals:
    /// Emitted whenever the storage plugin generates an MTP event.
    ///
    /// This happens for example when a new object has been created in the
    /// storage or an object property changed without the initiator's
    /// involvement.
    ///
    /// \param eventCode [in] an event code of the event
    /// \param params [in] the event parameters
    void eventGenerated(MTPEventCode eventCode, const QVector<quint32> &params);

    /// A storage plug-in emits this signal when it needs to get an object handle to assign to one of the objects.
    /// Since object handles need to be unique across storages, the storage factory assigns them to different storages.
    /// this handle, this would be a case when handles are maintained across sessions. 
    /// \param handle [out] the assigned object handle
    void objectHandle( ObjHandle& handle );

    /// A storage plug-in emits this signal when it needs to get a puoid to assign to one of it's objects.
    /// \param puoid[out] the largest puoid assigned to any object handle in this storage plug-in.
    void puoid( MtpInt128& puoid );

    /// This signal is emitted for now to check if a transaction is cancelled.
    /// TODO If needed this can be generalized to receive other transport specific events
    /// \param txCancelled [out] If set to true, this indicates that the current MTP tx got cancelled.
    void checkTransportEvents( bool &txCancelled );

    /// The plugin emits this signal when it is done with enumeration.
    void storagePluginReady(quint32 storageId);

public Q_SLOTS:
    /// This slot gets called when the storage factory needs to know the value of the largest
    /// puoid used for any object in a storage plug-in.
    /// \param puoid[out] the largest puoid assigned to any object handle in this storage plug-in.
    virtual void getLargestPuoid( MtpInt128& puoid ) = 0;

protected:
    /// Copies data between two storages and objects.
    ///
    /// \param sourceStorage [in] StoragePlugin from which to copy.
    /// \param source [in] handle of an object from which to copy data.
    /// \param destinationStorage [in] StoragePlugin into which to copy.
    /// \param destination [in] handle of an object to be filled with data.
    static MTPResponseCode copyData(StoragePlugin *sourceStorage,
            ObjHandle source, StoragePlugin *destinationStorage,
            ObjHandle destination);

    /// Storage id assigned to this storage by the storage factory.
    quint32 m_storageId;
    MTPStorageInfo m_storageInfo;
    QHash<ObjHandle, QVector<ObjHandle> > m_objectReferencesMap; ///< this map maintains references (if any) for each object.
};
}

#endif
