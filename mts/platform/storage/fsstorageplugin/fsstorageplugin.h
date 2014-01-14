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

#ifndef FSSTORAGEPLUGIN_H
#define FSSTORAGEPLUGIN_H

#include <sys/inotify.h>
#include "storageplugin.h"
#include <QVector>
#include <QList>
#include <QStringList>

class QFile;
class QDir;

namespace meegomtp1dot0
{
class FSInotify;
class StorageTracker;
class Thumbnailer;
class StorageItem;
}

/// FSStoragePlugin implements StoragePlugin for the case of a filesystem storage.

namespace meegomtp1dot0
{
class FSStoragePlugin : public StoragePlugin
{
    Q_OBJECT
#ifdef UT_ON
friend class FSStoragePlugin_test;
#endif

public:
    /// Constructor.
    FSStoragePlugin( quint32 storageId = 0, MTPStorageType storageType = MTP_STORAGE_TYPE_FixedRAM,
                     QString storagePath = "", QString volumeLabel = "", QString storageDescription = "" );

    /// Destructor.
    ~FSStoragePlugin();

    /// Enumerate the storage.
    /// \return true or false depending on whether storage succeeded or failed.
    bool enumerateStorage();

    /// Adds an item to the Storage Server.
    /// \param parentHandle [out] the handle of the item's parent.
    /// \param handle [out] the handle of this item.
    /// \param info pointer to a structure holding the object info dataset
    /// for this item.
    /// \return MTP response.
    MTPResponseCode addItem( ObjHandle &parentHandle, ObjHandle &handle, MTPObjectInfo *info );

    /// Deletes an item from the Storage Server
    /// \param handle [in] the handle of the object that needs to be deleted.
    /// \param formatCode [in] this optional arg can be used to delete all objects of a certain format.
    /// \return MTP response.
    MTPResponseCode deleteItem( const ObjHandle& handle, const MTPObjFormatCode& formatCode );

    /// Returns the no. of items belonging to a certain format and/or contained in a certain folder.
    /// \return storageId [in] which storage to look for.
    /// \param formatCode [in] this optional arg can be used to delete all objects of a certain format.
    /// \param associationHandle [in] this optional argument can specify the containing folder.
    /// \param noOfObjects [out] no. of objects found.
    /// \return MTP response.
    MTPResponseCode getObjectHandles( const MTPObjFormatCode& formatCode, const quint32& associationHandle, QVector<ObjHandle> &objectHandles ) const;

    /// Searches for the given object handle in this storage.
    /// \param handle [in] the object handle.
    /// \return true if this object handle exists, false otherwise.
    bool checkHandle( const ObjHandle &handle ) const;

    /// Gets the storage info.
    /// \param storageId [in] storage id.
    /// \param storageType [out] populated storage info structure.
    /// \return MTP response.
    MTPResponseCode storageInfo( MTPStorageInfo &info );

    /// Given an object handle, gets the objects referenced by this object in a storage.
    /// \param handle [in] object handle.
    /// \param references [out] list of object references.
    /// \return MTP response.
    MTPResponseCode getReferences( const ObjHandle &handle , QVector<ObjHandle> &references );

    /// For an object handle, sets objects references in a storage.
    /// \param handle [in] object handle.
    /// \param references [in] list of object references.
    /// \return MTP response.
    MTPResponseCode setReferences( const ObjHandle &handle , const QVector<ObjHandle> &references );

    /// Copies an object within or across storages.
    /// \param handle [in] object to be copied.
    /// \param parentHandle [in] parent in destination location.
    /// \param destinationStorageId [in] destination storage.
    /// \param copiedObjectHandle [out] The handle to the copied object is returned in this
    /// \param recursionCounter [in] The recursion depth
    MTPResponseCode copyObject( const ObjHandle &handle, const ObjHandle &parentHandle, const quint32 &destinationStorageId, ObjHandle &copiedObjectHandle , quint32 recursionCounter = 0);

    /// Moves an object within or across storages.
    /// \param handle [in] object to be moved.
    /// \param parentHandle [in] parent in destination location.
    /// \param storageId [in] destination storage.
    MTPResponseCode moveObject( const ObjHandle &handle, const ObjHandle &parentHandle, const quint32 &destinationStorageId, bool movePhysically = true );

    /// Given an object handle, provides it's absolute path in its storage.
    /// \param handle [in] object handle.
    /// \param path [out] the absolute path.
    /// \return MTP response.
    MTPResponseCode getPath( const quint32 &handle, QString &path ) const;

    /// Given an object handle, provides the object's objectinfo dataset.
    /// \param handle [in] the object handle.
    /// \param objectInfo [out] pointer to a structure holding the objectinfo dataset.
    /// \return MTP response.
    MTPResponseCode getObjectInfo( const ObjHandle &handle, const MTPObjectInfo *&objectInfo );

    /// Writes data onto a storage item.
    /// \param handle [in] the object handle.
    /// \writeBuffer [in] the data to be written.
    /// \bufferLen [in] the length of the data to be written.
    /// \param isFirstSegment [in] If true, this is the first segment in a multi segment write operation
    /// \param isLastSegment [in] If true, this is the final segment in a multi segment write operation
    MTPResponseCode writeData( const ObjHandle &handle, char *writeBuffer, quint32 bufferLen, bool isFirstSegment, bool isLastSegment );

    /// Reads data from a storage item.
    /// \param handle [in] the object handle.
    /// \readBuffer [in] the buffer where data will written. The buffer must be allocated by the caller
    /// \readBufferLen [in, out] the length of the input buffer. At most this amount of data will be read from the object. The function will return the actual number of bytes read in this buffer
    /// \param readOffset [in] The offset, in bytes, into the object to start reading from
    MTPResponseCode readData( const ObjHandle &handle, char *readBuffer, qint32 &readBufferLen, quint32 readOffset );

    /// Truncates an item to a certain size.
    /// \param handle [in] the object handle.
    /// \size [in] the size in bytes.
    MTPResponseCode truncateItem( const ObjHandle &handle, const quint32 &size );

    void excludePath( const QString & path );

public slots:
    /// This slot gets notified when an inotify event is received, and takes appropriate action.
    void inotifyEventSlot( struct inotify_event* );
    void receiveThumbnail(const QString &path);
    void getLargestObjectHandle( ObjHandle& handle );
    void getLargestPuoid( MtpInt128& puoid );

private:
    /// Creates playlist folders and sync .pla files with real playlists.
    void syncPlaylists();

    /// Reads internal playlists.
    void assignPlaylistReferences();

    /// Opens internal playlists.
    QVector<ObjHandle> readInternalAbstractPlaylist( StorageItem* );

    /// Removes an internal playlist.
    void removePlaylist(const QString &path);

    void setPlaylistReferences( const ObjHandle &handle , const QVector<ObjHandle> &references );

    /// Reads puoids from the puoids db, so that are preserved across MTP sessions.
    void populatePuoids();

    /// Writes puoids to a db, so that are preserved across MTP sessions.
    void storePuoids();

    /// After reading puoids the db, this gets rid of any puoids that are no longer valid ( the corresponding object doesn't exist ).
    void removeUnusedPuoids();

    /// Adds a directory to our filesystem storage.
    /// \param storageItem [out] pointer to the newly created StorageItem.
    /// \param isRootDir [in] boolean that indicates if this is the root folder.
    /// \param sendEvent [in] indicates whether to send an event to the MTP initiator or not.
    /// \param createIfNotExist [in] boolean which indicates whether to create the item on the filesystem if one does not exist.
    /// \return MTP response.
    MTPResponseCode addDirToStorage(  StorageItem *&thisStorageItem, bool isRootDir = false, bool sendEvent = false, bool createIfNotExist = false );

    /// Adds a file to our filesystem storage.
    /// \param storageItem [in/out] pointer to the newly created StorageItem.
    /// \param sendEvent [in] indicates whether to send an event to the MTP initiator or not.
    /// \param createIfNotExist [in] boolean which indicates whether to create the item on the filesystem if one does not exist.
    /// \return MTP response.
    MTPResponseCode addFileToStorage( StorageItem *&thisStorageItem, bool sendEvent = false, bool createIfNotExist = false );

    /// Gets a new object handle that can be assigned to an item.
    /// \return the object handle
    quint32 requestNewObjectHandle();
    void requestNewPuoid( MtpInt128& puoid );

    // FIXME This should be in the protocol layer.
    /// Build a list of all the object formats that we support.
    void buildSupportedFormatsList();

    /// This method helps to send an MTP event through the MTP Event class.
    /// \handle handle  [in] of the object for which this event is sent.
    /// \eventCode [in]  the MTP event code.
    /// \eventParams [in]  the set of parameters for this event.
    /// \partOfTransaction [in]  bool which indicates if this event is associated with an MTP transaction.
    void dispatchMTPEvent( const ObjHandle &handle, const MTPEventCode &eventCode, const QVector<quint32>& eventParams, const bool &partOfTransaction );

    /// Links a child storage item to it's parent storage item
    /// \param childStorageItem [in] pointer to the child storage item.
    /// \param parentStorageItem [in] pointer to the parent storage item.
    void linkChildStorageItem( StorageItem *childStorageItem, StorageItem *parentStorageItem );

    /// Unlinks a child storage item from it's parent storage item
    /// \param childStorageItem [in] pointer to the child storage item.
    void unlinkChildStorageItem( StorageItem *childStorageItem );

    /// Given a pathname, gives the corresponding storage item if the item exists in the filesystem.
    /// \param path [in] the pathname of the item.
    /// \return the storage item.
    StorageItem* findStorageItemByPath( const QString &path );

    /// Creates a file or dir in the filesystem.
    /// \param storageItem [in/out] pointer to the newly created StorageItem.
    /// \param info [in] the object's objectinfo dataset.
    /// \return MTP response.
    MTPResponseCode addToStorage( StorageItem *&storageItem, MTPObjectInfo *info );

    /// Removes a storage item.
    /// \param handle [in] the handle of the object that needs to be removed.
    /// \sendEvent [in] indicates whether to send an ObjectRemoved event to the inititiator.
    MTPResponseCode removeFromStorage( const ObjHandle& handle, bool sendEvent = false );

    /// Populates the object info for a storage item if that's not done by the initiator.
    /// \param storageItem [in] the item's whose object info needs to be populated.
    void populateObjectInfo( StorageItem *&storageItem );

    /// This method helps recursively modify the "path" field of a StorageItem ther has been moved.
    /// \param newAncestorPath [in] the new ancestor for the moved item and it's children.
    /// \movedItem [in] the moved item.
    /// \updateInTracker [in] If true, the function also updates the URIs in
    /// tracker
    void adjustMovedItemsPath( QString newAncestorPath, StorageItem* movedItem, bool updateInTracker = false );

    /// Gets the object format of a storage item.
    /// \param storageItem [in] the storage item.
    /// \return object format code.
    quint16 getObjectFormatByExtension( StorageItem *storageItem );

    /// Gets the protection status of a storage item.
    /// \param storageItem [in] the storage item.
    /// \return the protection status code.
    quint16 getMTPProtectionStatus( StorageItem *storageItem );

    /// Gets the size of a storage item in bytes.
    /// \param storageItem [in] the storage item.
    /// \return size in bytes.
    quint64 getObjectSize( StorageItem *storageItem );

    /// Gets the format of a thumbnail item.
    /// \return the thumbnail format code.
    /// \param storageItem [in] the storage item.
    quint16 getThumbFormat( StorageItem *storageItem );

    /// Gets the width of a thumbnail item in pixels.
    /// \param storageItem [in] the storage item.
    /// \return width in pixels.
    quint32 getThumbPixelWidth( StorageItem *storageItem );

    /// Gets the height of a thumbnail item in pixels.
    /// \param storageItem [in] the storage item.
    /// \return height in pixels.
    quint32 getThumbPixelHeight( StorageItem *storageItem );

    /// Gets the size of a thumbnail item in bytes.
    /// \param storageItem [in] the storage item.
    /// \return size in bytes.
    quint32 getThumbCompressedSize( StorageItem *storageItem );

    /// Gets the width of a image item in pixels.
    /// \param storageItem [in] the storage item.
    /// \return width in pixels.
    quint32 getImagePixelWidth( StorageItem *storageItem );

    /// Gets the height of a image item in pixels.
    /// \param storageItem [in] the storage item.
    /// \return height in pixels.
    quint32 getImagePixelHeight( StorageItem *storageItem );

    /// Gets the association type of a storage item.
    /// \param storageItem [in] the storage item.
    /// \return a code specifying the association type.
    quint16 getAssociationType( StorageItem *storageItem );

    /// Gets the association description of a storage item.
    /// \param storageItem [in] the storage item.
    /// \return a code describing the association type.
    quint32 getAssociationDescription( StorageItem *storageItem );


    /// Gets the but depth of a image.
    /// \param storageItem [in] the storage item.
    /// \return depth in bits.
    quint32 getImageBitDepth( StorageItem *storageItem );

    /// Gets the sequence no. of a storage item.
    /// \param storageItem [in] the storage item.
    /// \return the sequence no.
    quint32 getSequenceNumber( StorageItem *storageItem );

    /// Gets a storage item's creation date.
    /// \param storageItem [in] the storage item.
    /// \return date created.
    QString getCreatedDate( StorageItem *storageItem );

    /// Gets a storage item's last modification date.
    /// \param storageItem [in] the storage item.
    /// \return date modified.
    QString getModifiedDate( StorageItem *storageItem );

    /// Gets key words for a storage item.
    /// \param storageItem [in] the storage item.
    /// \return keywords.
    char* getKeywords( StorageItem *storageItem );

    /// Dump item info < node handle, node path, parent handle, parent path >, recursively if reqd.
    /// \param storageItem [in] the storage item.
    /// \param recurse indiicates whether to dump info recursively or not.
    void dumpStorageItem( StorageItem *storageItem, bool recurse = false );

    /// Reads object references from the references db and populates them to the references map.
    void populateObjectReferences();

    /// Writes object references from the references map onto ther persistent references db.
    void storeObjectReferences();

    /// This method removes invalid object handles and invalid references from the references map.
    void removeInvalidObjectReferences( const ObjHandle &handle );

    /// This handles IN_DELETE/IN_MOVED_FROM iNotify events
    void handleFSDelete(const struct inotify_event *event, const char* name);

    /// This handles IN_CREATE/IN_MOVED_TO iNotify events
    void handleFSCreate(const struct inotify_event *event, const char* name);

    /// This handles IN_MOVED_TO/IN_MOVED_FROM iNotify events
    void handleFSMove(const struct inotify_event *fromEvent, const char* fromName,
            const struct inotify_event *toEvent, const char* toName);

    /// This handles IN_MODIFY iNotify events
    void handleFSModify(const struct inotify_event *event, const char* name);
    
    /// Caches IN_MOVED_FROM events for future pairing
    void cacheInotifyEvent(const struct inotify_event *event, const char* name);
    
    /// Retrieves the cached iNotify event
    void getCachedInotifyEvent(const struct inotify_event **event, QString &name);
    
    /// Clears the internal iNotify event cache
    void clearCachedInotifyEvent();

    MTPResponseCode getObjectPropertyValue( const ObjHandle &handle, QList<MTPObjPropDescVal> &propValList, bool getFromObjInfo = true, bool getDynamically = true );
    MTPResponseCode setObjectPropertyValue( const ObjHandle &handle, QList<MTPObjPropDescVal> &propValList, bool sendObjectPropList = false);
    MTPResponseCode getObjectPropertyValueFromStorage( const ObjHandle &handle,
                                                       MTPObjPropertyCode propCode,
                                                       QVariant &value, MTPDataType type );
    MTPResponseCode getObjectPropertyValueFromTracker( const ObjHandle &handle,
                                                       MTPObjPropertyCode propCode,
                                                       QVariant &value, MTPDataType type );
    bool isImage(StorageItem*);

    /// Removes watch descriptors on a directory and it's sub directories if any.
    void removeWatchDescriptorRecursively( StorageItem* item );

    /// Adds inotify watch on a directory and sub dirs if any.
    void addWatchDescriptorRecursively( StorageItem *item );

    /// Returns recursively the list of files (and directories) under a given item,
    /// and the new paths for all those file, if they were moved under a new
    /// root
    void getFileListRecursively(const StorageItem *storageItem, const QString &destinationPath,
                                QStringList &fileList);

    /// Removes watch descriptors on a directory.
    void removeWatchDescriptor( StorageItem* item );

    /// Adds inotify watch on a directory.
    void addWatchDescriptor( StorageItem *item );

    
private:
    MTPResponseCode deleteItemHelper( const ObjHandle& handle, bool removePhysically = true, bool sendEvent = false );
    bool isFileNameValid(const QString &fileName, const StorageItem *parent);

    quint32 m_storageId; ///< storage id assigned to this storage by the storage factory.
    QHash<int,ObjHandle> m_watchDescriptorMap; ///< map from an inotify watch on an object to it's object handle.
    QHash<QString,ObjHandle> m_pathNamesMap;
    QHash<QString,MtpInt128> m_puoidsMap;
    QHash<MtpInt128, ObjHandle> m_puoidToHandleMap; ///< Maps the PUOID to the corresponding object handle
    StorageItem *m_root; ///< the root folder
    QString m_rootFolderPath; ///< the root folder path
    QString m_puoidsDbPath; ///< path where puoids will be stored persistently.
    QString m_objectReferencesDbPath; ///< path where references will be stored persistently.
    QString m_playlistPath; ///< the path where playlists are stored.
    QString m_internalPlaylistPath; ///< the path where internal abstract playlists are stored.
    ObjHandle m_uniqueObjectHandle; ///< The last alloted object handle
    ObjHandle m_writeObjectHandle; ///< The obj handle for which a write operation is currently is progress. 0 means invalid handle, NOT root node!!
    StorageTracker* m_tracker; ///< pointer to the tracker object
    Thumbnailer* m_thumbnailer; ///< pointer to the thumbnailer object
    FSInotify* m_inotify; ///< pointer to the inotify wrapper
    QHash<QString,quint16> m_formatByExtTable;
    QHash<MTPObjFormatCode, QString> m_imageMimeTable; ///< Maps the MTP object format code (for image types only) to MIME type string
    QString m_mtpPersistentDBPath;
    MtpInt128 m_largestPuoid;
    struct INotifyCache
    {
        struct inotify_event    fromEvent;
        QString                 fromName;
    }m_iNotifyCache; ///< A cache for iNotify events

    struct ExistingPlaylists
    {
        QStringList             playlistPaths;
        QList<QStringList>      playlistEntries;
    }m_existingPlaylists;

    struct NewPlaylists
    {
        QStringList             playlistNames;
        QList<QStringList>      playlistEntries;
    }m_newPlaylists;

    QHash<ObjHandle, StorageItem*> m_objectHandlesMap; ///< each storage has a map of all it's object's handles to corresponding storage item.
    QFile *m_dataFile;

    QStringList m_excludePaths; ///< Paths that should not be indexed
    
};
}

#endif

