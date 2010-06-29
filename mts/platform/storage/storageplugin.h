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
    StoragePlugin(){}

    /// Destructor.
    virtual ~StoragePlugin(){}

    /// Enumerate the storage.
    /// \return true or false depending on whether storage succeeded or failed.
    virtual bool enumerateStorage() = 0;

    /// Adds an item to the Storage Factory.
    /// \param storageId [in/out] which storage this item goes to.
    /// \param handle [out] the handle of the item's parent.
    /// \param parentHandle [out] the handle of this item.
    /// \param info pointer to a structure holding the object info dataset
    /// for this item.
    /// \return MTP response.
    virtual MTPResponseCode addItem( ObjHandle &parentHandle, ObjHandle &handle, MTPObjectInfo *info ) = 0;

    /// Deletes an item from the Storage Factory
    /// \param handle [in] the handle of the object that needs to be deleted.
    /// \param formatCode [in] this optional arg can be used to delete all objects of a certain format.
    /// \return MTP response.
    virtual MTPResponseCode deleteItem( const ObjHandle& handle, const MTPObjFormatCode& formatCode ) = 0;

    /// Returns the no. of items belonging to a certain format and/or contained in a certain folder.
    /// \return storageId [in] which storage to look for.
    /// \param formatCode [in] this optional arg can be used to delete all objects of a certain format.
    /// \param associationHandle [in] this optional argument can specify the containing folder.
    /// \param noOfObjects [out] no. of objects found.
    /// \return MTP response.
    virtual MTPResponseCode getObjectHandles( const MTPObjFormatCode& formatCode, const quint32& associationHandle,
                                              QVector<ObjHandle> &objectHandles ) const = 0;

    /// Searches for the given handle in all storages.
    /// \param handle [in] the object handle.
    /// \return MTP response.
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
    /// \param storageId [in] destination storage.
    virtual MTPResponseCode copyObject( const ObjHandle &handle, const ObjHandle &parentHandle, const quint32 &destinationStorageId, ObjHandle &copiedObjectHandle, quint32 recursionDepth = 0) = 0;

    /// Moves an object within or across storages.
    /// \param handle [in] object to be moved.
    /// \param parentHandle [in] parent in destination location.
    /// \param storageId [in] destination storage.
    virtual MTPResponseCode moveObject( const ObjHandle &handle, const ObjHandle &parentHandle, const quint32 &destinationStorageId, bool movePhysically = true ) = 0;

    /// Given an object handle, provides it's absolute path in its storage.
    /// \param handle [in] object handle.
    /// \param path [out] the absolute path.
    /// \return MTP response.
    virtual MTPResponseCode getPath( const quint32 &handle, QString &path ) const = 0;

    /// Given an object handle, provides the object's objectinfo dataset.
    /// \param handle [in] the object handle.
    /// \param objectInfo [out] pointer to a structure holding the objectinfo dataset.
    /// \return MTP response.
    virtual MTPResponseCode getObjectInfo( const ObjHandle &handle, const MTPObjectInfo *&objectInfo ) = 0;

    /// Writes data onto a storage item.
    /// \param handle [in] the object handle.
    /// \writeBuffer [in] the data to be written.
    /// \bufferLen [in] the length of the data to be written.
    /// \param isFirstSegment [in] If true, this is the first segment in a multi segment write operation
    /// \param isLastSegment [in] If true, this is the final segment in a multi segment write operation
    virtual MTPResponseCode writeData( const ObjHandle &handle, char *writeBuffer, quint32 bufferLen, bool isFirstSegment, bool isLastSegment ) = 0;

    /// Reads data from a storage item.
    /// \param handle [in] the object handle.
    /// \readBuffer [in] the buffer where data will written. The buffer must be allocated by the caller
    /// \readBufferLen [in, out] the length of the input buffer. At most this amount of data will be read from the object. The function will return the actual number of bytes read in this buffer
    /// \param readOffset [in] The offset, in bytes, into the object to start reading from
    virtual MTPResponseCode readData( const ObjHandle &handle, char *readBuffer, qint32 &readBufferLen, quint32 readOffset ) = 0;

    /// Truncates an item to a certain size.
    /// \param handle [in] the object handle.
    /// \size [in] the size in bytes.
    virtual MTPResponseCode truncateItem( const ObjHandle &handle, const quint32 &size ) = 0;

    virtual MTPResponseCode getObjectPropertyValue( const ObjHandle &handle, QList<MTPObjPropDescVal> &propValList, bool getFromObjInfo = true, bool getDynamically = true ) = 0;
    virtual MTPResponseCode setObjectPropertyValue( const ObjHandle &handle, QList<MTPObjPropDescVal> &propValList, bool sendObjectPropList = false ) = 0;

Q_SIGNALS:
    /// The signal must be emitted by the storage plugin whenever an MTP event is captured by it
    /// \param eventCode [in] The MTP event code to be sent
    /// \param params [in] A vector of event parameters
    /// \param filePath [in] The key (= file name + parent handle + storage id) to the file that generated this event. For events that do not have a file associated with them, this must be an empty QString
    void eventGenerated(MTPEventCode eventCode, const QVector<quint32> &params, QString filePath);

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

public Q_SLOTS:
    /// This slot gets called when storagefactory needs to know the value of the largest
    /// object handle a plug-in used in the previous MTP session.
    /// \param handle [out] largest object handle
    virtual void getLargestObjectHandle( ObjHandle& handle ) = 0;

    /// This slot gets called when the storage factory needs to know the value of the largest
    /// puoid used for any object in a storage plug-in.
    /// \param puoid[out] the largest puoid assigned to any object handle in this storage plug-in.
    virtual void getLargestPuoid( MtpInt128& puoid ) = 0;

protected:
    MTPStorageInfo m_storageInfo;
    QString m_storagePath;
    QHash<ObjHandle, QVector<ObjHandle> > m_objectReferencesMap; ///< this map maintains references (if any) for each object.
};
}

#endif
