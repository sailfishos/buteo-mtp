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
public:
    /// Constructor.
    FSStoragePlugin( quint32 storageId = 0, MTPStorageType storageType = MTP_STORAGE_TYPE_FixedRAM,
                     QString storagePath = "", QString volumeLabel = "", QString storageDescription = "" );

    /// Destructor.
    ~FSStoragePlugin();

    void disableObjectEvents();

    bool enumerateStorage();

    MTPResponseCode addItem( ObjHandle &parentHandle, ObjHandle &handle, MTPObjectInfo *info );

    MTPResponseCode deleteItem( const ObjHandle& handle, const MTPObjFormatCode& formatCode );

    MTPResponseCode copyHandle( StoragePlugin *sourceStorage, ObjHandle source,
            ObjHandle parent );

    MTPResponseCode getObjectHandles( const MTPObjFormatCode& formatCode, const quint32& associationHandle, QVector<ObjHandle> &objectHandles ) const;

    bool checkHandle( const ObjHandle &handle ) const;

    MTPResponseCode storageInfo( MTPStorageInfo &info );

    MTPResponseCode getReferences( const ObjHandle &handle , QVector<ObjHandle> &references );

    MTPResponseCode setReferences( const ObjHandle &handle , const QVector<ObjHandle> &references );

    MTPResponseCode copyObject( const ObjHandle &handle, const ObjHandle &parentHandle, StoragePlugin *destinationStorage, ObjHandle &copiedObjectHandle , quint32 recursionCounter = 0);

    MTPResponseCode moveObject( const ObjHandle &handle, const ObjHandle &parentHandle, StoragePlugin *destinationStorage, bool movePhysically = true );

    MTPResponseCode getPath( const quint32 &handle, QString &path ) const;

    MTPResponseCode getEventsEnabled( const quint32 &handle, bool &eventsEnabled ) const;

    MTPResponseCode getObjectInfo( const ObjHandle &handle, const MTPObjectInfo *&objectInfo );

    MTPResponseCode writeData( const ObjHandle &handle, char *writeBuffer, quint32 bufferLen, bool isFirstSegment, bool isLastSegment );

    MTPResponseCode readData( const ObjHandle &handle, char *readBuffer, qint32 &readBufferLen, quint32 readOffset );

    MTPResponseCode truncateItem( const ObjHandle &handle, const quint32 &size );

    MTPResponseCode getObjectPropertyValue(const ObjHandle &handle,
            QList<MTPObjPropDescVal> &propValList);

    MTPResponseCode setObjectPropertyValue(const ObjHandle &handle,
            QList<MTPObjPropDescVal> &propValList,
            bool sendObjectPropList = false);

    MTPResponseCode getChildPropertyValues(ObjHandle handle,
            const QList<const MtpObjPropDesc *>& properties,
            QMap<ObjHandle, QList<QVariant> > &values);

    void excludePath( const QString & path );

public slots:
    /// This slot gets notified when an inotify event is received, and takes appropriate action.
    void inotifyEventSlot( struct inotify_event* );
    void receiveThumbnail(const QString &path);
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

    /// Creates a directory in the file system.
    ///
    /// \param path [in] filesystem path of the directory to create.
    /// \return MTP response.
    MTPResponseCode createDirectory( const QString &path );

    /// Creates a file in the file system.
    ///
    /// \param path [in] filesystem path of the file to create.
    /// \return MTP response.
    MTPResponseCode createFile( const QString &path, MTPObjectInfo *info);

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

    /// Creates new StorageItem representing a file or directory at \c path
    /// and creates the file or directory if asked to do so.
    ///
    /// \param path [in] absolute filesystem path to represent in the storage.
    /// \param storageItem [out] pointer where the new StorageItem is stored;
    ///                    can be null.
    /// \param info [in] if not null, the data in MTPObjectInfo are used to
    ///             initialize the StorageItem; otherwise the object info is
    ///             populated by default values (see populateObjectInfo()).
    /// \param sendEvent [in] if true, ObjectAdded event is sent to the MTP
    ///                  initiator.
    /// \param createIfNotExist [in] if true, the filesystem item at \c path is
    ///                         created if it doesn't exist yet.
    /// \param handle [in] when nonzero, assigns the specific object handle to
    ///               the newly created StorageItem.
    /// \return MTP response code.
    ///
    /// This method will call processEvents() regularly when adding
    /// a whole directory tree, so that the event loop remains responsive.
    MTPResponseCode addToStorage( const QString &path,
            StorageItem **storageItem = 0, MTPObjectInfo *info = 0,
            bool sendEvent = false, bool createIfNotExist = false,
            ObjHandle handle = 0 );

    /// Inserts a storage item into internal data structures for faster search.
    ///
    /// \param item [in] a storage item.
    void addItemToMaps( StorageItem *item );

    /// Removes a storage item.
    /// \param handle [in] the handle of the object that needs to be removed.
    /// \sendEvent [in] indicates whether to send an ObjectRemoved event to the inititiator.
    MTPResponseCode removeFromStorage( ObjHandle handle, bool sendEvent = false );

    /// Populates the object info for a storage item if that's not done by the initiator.
    /// \param storageItem [in] the item's whose object info needs to be populated.
    void populateObjectInfo( StorageItem *storageItem );

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

    // Throttle sending of MTP_EV_StorageInfoChanged events
    void sendStorageInfoChanged(void);
    
    /// Caches IN_MOVED_FROM events for future pairing
    void cacheInotifyEvent(const struct inotify_event *event, const char* name);
    
    /// Retrieves the cached iNotify event
    void getCachedInotifyEvent(const struct inotify_event **event, QString &name);
    
    /// Clears the internal iNotify event cache
    void clearCachedInotifyEvent();

    MTPResponseCode getObjectPropertyValueFromStorage( const ObjHandle &handle,
                                                       MTPObjPropertyCode propCode,
                                                       QVariant &value, MTPDataType type );
    MTPResponseCode getObjectPropertyValueFromTracker( const ObjHandle &handle,
                                                       MTPObjPropertyCode propCode,
                                                       QVariant &value, MTPDataType type );

    /// Is storage item an image file that the thumbnailer can process
    bool isThumbnailableImage(StorageItem*);

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

private slots:
    void enumerateStorage_worker();
    
private:
    MTPResponseCode deleteItemHelper( ObjHandle handle, bool removePhysically = true, bool sendEvent = false );
    bool isFileNameValid(const QString &fileName, const StorageItem *parent);
    QString filesystemUuid() const;

    QString m_storagePath;
    QHash<int,ObjHandle> m_watchDescriptorMap; ///< map from an inotify watch on an object to it's object handle.
    QHash<QString,ObjHandle> m_pathNamesMap;
    QHash<QString,MtpInt128> m_puoidsMap;
    QHash<MtpInt128, ObjHandle> m_puoidToHandleMap; ///< Maps the PUOID to the corresponding object handle
    StorageItem *m_root; ///< the root folder
    QString m_puoidsDbPath; ///< path where puoids will be stored persistently.
    QString m_objectReferencesDbPath; ///< path where references will be stored persistently.
    QString m_playlistPath; ///< the path where playlists are stored.
    QString m_internalPlaylistPath; ///< the path where internal abstract playlists are stored.
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
    quint64 m_reportedFreeSpace;
    QFile *m_dataFile;

    QStringList m_excludePaths; ///< Paths that should not be indexed

#ifdef UT_ON
    ObjHandle m_testHandleProvider;
    friend class FSStoragePlugin_test;
#endif
};
}

#endif

