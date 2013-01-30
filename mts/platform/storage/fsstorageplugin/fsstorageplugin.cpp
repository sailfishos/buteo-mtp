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

#include "fsstorageplugin.h"
#include "fsinotify.h"
#include "storagetracker.h"
#include "storageitem.h"
#include "thumbnailer.h"
#include "trace.h"
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QVariant>
#include <QFileInfo>
#include <QDateTime>

using namespace meegomtp1dot0;

const quint32 THUMB_MAX_SIZE  =    (1024 * 48);/* max thumbnailsize */
// Default width and height for thumbnails
const quint32 THUMB_WIDTH     =    100;
const quint32 THUMB_HEIGHT    =    100;
const quint32 MAX_READ_LEN    =    64 * 1024;

static quint32 fourcc_wmv3 = 0x574D5633;
static const QString FILENAMES_FILTER_REGEX("[<>:\\\"\\/\\\\\\|\\?\\*\\x0000-\\x001F]");

extern "C" StoragePlugin* createStoragePlugin( const quint32& storageId )
{
    QString storagePath = QDir::homePath() + "/.config/mtpstorage";
    return new FSStoragePlugin( storageId, MTP_STORAGE_TYPE_FixedRAM, storagePath, "media", "Phone Memory" );
}

extern "C" void destroyStoragePlugin( StoragePlugin* storagePlugin )
{
    if( storagePlugin )
    {
        delete storagePlugin;
        storagePlugin = 0;
    }
}

/************************************************************
 * FSStoragePlugin::FSStoragePlugin
 ***********************************************************/
FSStoragePlugin::FSStoragePlugin( quint32 storageId, MTPStorageType storageType, QString storagePath,
                                  QString volumeLabel, QString storageDescription ) :
                                  m_storageId(storageId), m_root(0), m_rootFolderPath(storagePath),
                                  m_uniqueObjectHandle(0), m_writeObjectHandle(0),
                                  m_largestPuoid(0), m_dataFile(0)
{
    m_storageInfo.storageType = storageType;
    m_storageInfo.accessCapability = MTP_STORAGE_ACCESS_ReadWrite;
    m_storageInfo.filesystemType = MTP_FILE_SYSTEM_TYPE_GenHier;
    m_storageInfo.freeSpaceInObjects = 0xFFFFFFFF;
    m_storageInfo.storageDescription = storageDescription;
    m_storageInfo.volumeLabel = volumeLabel;
    m_storagePath = storagePath;

    QByteArray ba = m_storagePath.toUtf8();
    struct statvfs stat;
    if( statvfs(ba.constData(), &stat) )
    {
        m_storageInfo.maxCapacity = 0;
        m_storageInfo.freeSpace = 0;
    }
    else
    {
        m_storageInfo.maxCapacity = (quint64)stat.f_blocks * stat.f_bsize;
        m_storageInfo.freeSpace = (quint64)stat.f_bavail * stat.f_bsize;
    }

    m_mtpPersistentDBPath = QDir::homePath() + "/.mtp";
    QDir dir = QDir( m_mtpPersistentDBPath );
    if( !dir.exists() )
    {
        dir.mkdir( m_mtpPersistentDBPath );
    }

    m_puoidsDbPath = m_mtpPersistentDBPath + "/.mtppuoids";
    m_objectReferencesDbPath = m_mtpPersistentDBPath + "/.mtpreferences";
    m_internalPlaylistPath = m_mtpPersistentDBPath + "/.Playlists";
    m_playlistPath = storagePath + "/Playlists";

    buildSupportedFormatsList();

    // Populate puoids stored persistently and store them in the puoids map.
    populatePuoids();

    m_tracker = new StorageTracker();
    m_thumbnailer = new Thumbnailer();
    QObject::connect( m_thumbnailer, SIGNAL( thumbnailReady( const QString& ) ), this, SLOT( receiveThumbnail( const QString& ) ) );
    m_inotify = new FSInotify( IN_MOVE | IN_CREATE | IN_DELETE | IN_CLOSE_WRITE );
    QObject::connect( m_inotify, SIGNAL(inotifyEventSignal( struct inotify_event* )), this, SLOT(inotifyEventSlot( struct inotify_event* )) );
}

/************************************************************
 * bool FSStoragePlugin::enumerateStorage()
 ***********************************************************/
bool FSStoragePlugin::enumerateStorage()
{
    bool result = true;
    // Create the root folder for this storage, if it doesn't already exist.
    QDir dir = QDir( m_storagePath );
    if( !dir.exists() )
    {
        dir.mkdir( m_storagePath );
    }

    // Make the Playlists directory, if one does not exist
    dir = QDir( m_storagePath );
    if( !dir.exists( "Playlists" ) )
    {
        dir.mkdir( "Playlists" );
    }

    // Now read all existing and new playlists from the device (tracker)
    m_tracker->getPlaylists(m_existingPlaylists.playlistPaths, m_existingPlaylists.playlistEntries, true);
    m_tracker->getPlaylists(m_newPlaylists.playlistNames, m_newPlaylists.playlistEntries, false);

    // Add the root folder to storage
    m_root = new StorageItem;
    m_root->m_path = m_storagePath;
    populateObjectInfo( m_root );
    addDirToStorage( m_root, true );

    removeUnusedPuoids();

    // Populate object references stored persistently and add them to the storage.
    populateObjectReferences();

    // TODO: The playlist handling is yet unclear. For now, playlists are
    // implemented as abstract 0 byte objects that contain references to other
    // objects.
    // Create playlist folders and sync .pla files with real playlists.
    assignPlaylistReferences();

    return result;
}

/************************************************************
 * FSStoragePlugin::~FSStoragePlugin
 ***********************************************************/
FSStoragePlugin::~FSStoragePlugin()
{
    storePuoids();
    storeObjectReferences();

    for( QHash<ObjHandle, StorageItem*>::iterator i = m_objectHandlesMap.begin() ; i != m_objectHandlesMap.end(); ++i )
    {
        if( i.value() )
        {
            delete i.value();
        }
    }

    delete m_tracker;
    m_tracker = 0;
    delete m_thumbnailer;
    m_thumbnailer = 0;
    delete m_inotify;
    m_inotify = 0;
}

#if 0
/************************************************************
 * void FSStoragePlugin::syncPlaylists
 ***********************************************************/
void FSStoragePlugin::syncPlaylists()
{
    // Ensure that both abstract and normal playlist directory are there
    QDir dir( m_storagePath );
    if( !dir.exists( "Playlists" ) )
    {
        dir.mkdir( "Playlists" );
    }

    // We need to synchronize the directories (while the dev was offline something could happened)

    // First read the Playlists dir and store in a vector
    QList<QString> playlistItems;
    dir = QDir( m_playlistPath );
    QStringList contents = dir.entryList();
    for( int i = 0; i < contents.size(); ++i )
    {
        QString name = contents.at(i);
        if( name.endsWith( ".pla" ) )
        {
            QString path = m_playlistPath + QString("/") + name;
            // Check if the playlist is in tracker
            if(false == m_tracker->isPlaylistExisting(path))
            {
                // Delete file from disk
                QFile f(path);
                f.remove();
            }
        }

    }

    // Second read the internal playlist dir and check if there exists a corresponding file in the
    // Playlists directory, if not, create it.
    // TODO In the future this should synchronize the device playlists
    dir = QDir( m_internalPlaylistPath );
    contents = dir.entryList();
    for( int i = 0; i < contents.size(); ++i )
    {
        QString name = contents.at(i);
        if( name.endsWith( ".m3u" ) )
        {
            if( !playlistItems.contains( name.replace( ".m3u", ".pla" ) ) )
            {
                QFile file( m_playlistPath + "/" + name );
                file.open( QIODevice::ReadWrite );
                file.close();
            }
            else
            {
                playlistItems.removeOne( name );
            }
        }
    }

    for(QList<QString>::const_iterator i = playlistItems.constBegin(); i != playlistItems.constEnd(); ++i)
    {
        QFile file( m_playlistPath + "/" + *i );
        file.remove();
    }
}
#endif
/************************************************************
 * void FSStoragePlugin::readPlaylists
 ***********************************************************/
void FSStoragePlugin::assignPlaylistReferences()
{
    // Get the handle for the playlist path
    ObjHandle playlistDirHandle = m_pathNamesMap[m_playlistPath];
    if(0 == playlistDirHandle)
    {
        MTP_LOG_CRITICAL("No handle found for playlists directory!, playlists will be unavailable!");
        return;
    }
    // Assign references based on the playlists we read from tracker
    // First the existing playlists (that is those for which we already have a .pla file for)
    QVector<ObjHandle> references;
    ObjHandle refHandle = 0;
    for(int i = 0; i < m_existingPlaylists.playlistPaths.size(); i++)
    {
        references.clear();
        QString playlistPath = m_existingPlaylists.playlistPaths[i];
        // Iterate over all entries, get their object handles, and assign references
        if(m_pathNamesMap.contains(playlistPath))
        {
            refHandle = m_pathNamesMap[playlistPath];
            // Iterate entries now
            QStringList entries = m_existingPlaylists.playlistEntries[i];
            foreach(QString entry, entries)
            {
                if(m_pathNamesMap.contains(entry))
                {
                    references.append(m_pathNamesMap[entry]);
                }
            }
            m_objectReferencesMap[refHandle] = references;
        }
    }

    // Now we do the same for the playlists that are new on the device
    for(int i = 0; i < m_newPlaylists.playlistNames.size(); i++)
    {
        references.clear();
        // Iterate over all entries, get their object handles, and assign references
        QString playlistName = m_newPlaylists.playlistNames[i];
        QString playlistPath = m_playlistPath + "/" + playlistName + ".pla";

        // Also create a .pla file under <root>/Playlists add an nie:url for these playlists
        MTPObjectInfo objInfo;
        ObjHandle newHandle = 0;
        objInfo.mtpFileName = playlistName + ".pla";
        objInfo.mtpObjectFormat = MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist;
        objInfo.mtpStorageId = m_storageId;
        objInfo.mtpParentObject = playlistDirHandle;
        if(MTP_RESP_OK == addItem(playlistDirHandle, newHandle, &objInfo))
        {
            QStringList entries = m_newPlaylists.playlistEntries[i];
            foreach(QString entry, entries)
            {
                if(m_pathNamesMap.contains(entry))
                {
                    references.append(m_pathNamesMap[entry]);
                }
            }
            m_objectReferencesMap[newHandle] = references;
            // Set the nie:identifier field in the playlist to "sync" our pla file with tracker
            m_tracker->setPlaylistPath(playlistName, playlistPath);
        }
    }
}

#if 0
/************************************************************
 * void FSStoragePlugin::openPlaylists
 ***********************************************************/
QVector<ObjHandle> FSStoragePlugin::readInternalAbstractPlaylist( StorageItem *item )
{
    QVector<ObjHandle> playlistRefs;
    if( !item )
    {
        return playlistRefs;
    }
    char filePath[256]; // the max path length // FIXME
    QFile file( item->m_path );
    if( file.open( QIODevice::ReadOnly ) )
    {
        while( !file.atEnd() )
        {
            int bytesRead = file.readLine( filePath, 256 );
            if( -1 == bytesRead || filePath[bytesRead -1] != '\n' || filePath[0] == '#' )
            {
                continue;
            }
            filePath[bytesRead -1] = '\0';
            if( m_pathNamesMap.contains( QString( filePath ) ) )
            {
                playlistRefs.append( m_pathNamesMap.value( QString( filePath ) ) );
            }
        }
    }
    return playlistRefs;
}
#endif

/************************************************************
 * void FSStoragePlugin::removePlaylist
 ***********************************************************/
void FSStoragePlugin::removePlaylist(const QString &path)
{
    // Delete the playlist from tracker
    m_tracker->deletePlaylist(path);
}

/************************************************************
 * void FSStoragePlugin::populatePuoids
 ***********************************************************/
void FSStoragePlugin::populatePuoids()
{
    QFile file( m_puoidsDbPath );
    if( !file.open( QIODevice::ReadOnly ) || !file.size() )
    {
        return;
    }

    qint32 bytesRead = 0;
    char *name = 0;
    quint32 noOfPuoids = 0, pathnameLen = 0;
    MtpInt128 puoid;

    // Read the last used puoid
    bytesRead = file.read( reinterpret_cast<char*>(&m_largestPuoid), sizeof(MtpInt128) );
            quint32 id = *(reinterpret_cast<quint32*>(&m_largestPuoid));
    if( 0 >= bytesRead )
    {
        return;
    }

    // Read the no. of puoids
    bytesRead = file.read( reinterpret_cast<char*>(&noOfPuoids), sizeof(quint32) );
    if( 0 >= bytesRead )
    {
        return;
    }

    for( quint32 i = 0 ; i < noOfPuoids; ++i )
    {
        // read pathname len
        bytesRead = file.read( reinterpret_cast<char*>(&pathnameLen), sizeof(quint32) );
        if( 0 >= bytesRead )
        {
            return;
        }

        // read the pathname
        name = new char[pathnameLen + 1];
        bytesRead = file.read( name, pathnameLen );
        if( 0 >= bytesRead )
        {
            delete [] name;
            return;
        }
        name[pathnameLen] = '\0';

        // read the puoid
        bytesRead = file.read( reinterpret_cast<char*>(&puoid), sizeof(MtpInt128) );
            id = *(reinterpret_cast<quint32*>(&puoid));
        if( 0 >= bytesRead )
        {
            delete [] name;
            return;
        }

        // Store this in puoids map
        m_puoidsMap[name] = puoid;

        delete [] name;
    }
}

/************************************************************
 * void FSStoragePlugin::removeUnusedPuoids
 ***********************************************************/
void FSStoragePlugin::removeUnusedPuoids()
{
    QHash<QString, MtpInt128>::iterator i = m_puoidsMap.begin();
    while( i != m_puoidsMap.end() )
    {
        if( !m_pathNamesMap.contains( i.key() ) )
        {
            i = m_puoidsMap.erase(i);
        }
        else
        {
            ++i;
        }
    }
}

/************************************************************
 * void FSStoragePlugin::storePuoids
 ***********************************************************/
void FSStoragePlugin::storePuoids()
{
    // Store in the following order
    // Last used PUOID (128 bits)
    // No. Of puoids ( 4 bytes )
    // length of object's pathname : pathname : puoid ( for each puoid )
    qint32 bytesWritten = -1;
    QFile file( m_puoidsDbPath );
    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        return;
    }

    // Write the last used puoid.
    bytesWritten = file.write( reinterpret_cast<const char*>(&m_largestPuoid), sizeof(MtpInt128) );
            quint32 id = *(reinterpret_cast<quint32*>(&m_largestPuoid));
    if( -1 == bytesWritten )
    {
        MTP_LOG_WARNING("ERROR writing last used puoid to db!!");
        file.resize(0);
        file.close();
        return;
    }

    // Write the no of puoids
    quint32 noOfPuoids = m_puoidsMap.size();
    bytesWritten = file.write( reinterpret_cast<const char*>(&noOfPuoids), sizeof(quint32) );
    if( -1 == bytesWritten )
    {
        MTP_LOG_WARNING("ERROR writing no of puoids to db!!");
        file.resize(0);
        file.close();
        return;
    }

    // Write info for each puoid
    for( QHash<QString,MtpInt128>::const_iterator i = m_puoidsMap.constBegin() ; i != m_puoidsMap.constEnd(); ++i )
    {
        quint32 pathnameLen = i.key().size();
        QString pathname = i.key();
        MtpInt128 puoid = i.value();

        // Write length of path name
        bytesWritten = file.write( reinterpret_cast<const char*>(&pathnameLen), sizeof(quint32) );
        if( -1 == bytesWritten )
        {
            MTP_LOG_WARNING("ERROR writing pathname len to db!!");
            file.resize(0);
            file.close();
            return;
        }

        // Write path name
        QByteArray ba = pathname.toUtf8();
        bytesWritten = file.write( reinterpret_cast<const char*>(ba.constData()), pathnameLen );
        if( -1 == bytesWritten )
        {
            MTP_LOG_WARNING("ERROR writing pathname to db!!");
            file.resize(0);
            file.close();
            return;
        }

        // Write puoid
        bytesWritten = file.write( reinterpret_cast<const char*>(&puoid), sizeof(MtpInt128) );
            id = *(reinterpret_cast<quint32*>(&puoid));
        if( -1 == bytesWritten )
        {
            MTP_LOG_WARNING("ERROR writing puoid to db!!");
            file.resize(0);
            file.close();
            return;
        }
    }
}

/************************************************************
 * void FSStoragePlugin::buildSupportedFormatsList
 ***********************************************************/
void FSStoragePlugin::buildSupportedFormatsList()
{
    m_formatByExtTable["pla"] = MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist;
    m_formatByExtTable["wav"] = MTP_OBF_FORMAT_WAV;
    m_formatByExtTable["mp3"] = MTP_OBF_FORMAT_MP3;
    m_formatByExtTable["ogg"] = MTP_OBF_FORMAT_OGG;
    m_formatByExtTable["txt"] = MTP_OBF_FORMAT_Text;
    m_formatByExtTable["htm"] = MTP_OBF_FORMAT_HTML;
    m_formatByExtTable["html"] = MTP_OBF_FORMAT_HTML;
    m_formatByExtTable["wmv"] = MTP_OBF_FORMAT_WMV;
    m_formatByExtTable["avi"] = MTP_OBF_FORMAT_AVI;
    m_formatByExtTable["mpg"] = MTP_OBF_FORMAT_MPEG;
    m_formatByExtTable["mpeg"] = MTP_OBF_FORMAT_MPEG;
    m_formatByExtTable["bmp"] = MTP_OBF_FORMAT_BMP;
    m_formatByExtTable["gif"] = MTP_OBF_FORMAT_GIF;
    m_formatByExtTable["jpg"] = MTP_OBF_FORMAT_EXIF_JPEG;
    m_formatByExtTable["jpeg"] = MTP_OBF_FORMAT_EXIF_JPEG;
    m_formatByExtTable["png"] = MTP_OBF_FORMAT_PNG;
    m_formatByExtTable["tif"] = MTP_OBF_FORMAT_TIFF;
    m_formatByExtTable["tiff"] = MTP_OBF_FORMAT_TIFF;
    m_formatByExtTable["wma"] = MTP_OBF_FORMAT_WMA;
    m_formatByExtTable["aac"] = MTP_OBF_FORMAT_AAC;
    m_formatByExtTable["mp4"] = MTP_OBF_FORMAT_MP4_Container;
    m_formatByExtTable["3gp"] = MTP_OBF_FORMAT_3GP_Container;
    m_formatByExtTable["pls"] = MTP_OBF_FORMAT_PLS_Playlist;
    m_formatByExtTable["alb"] = MTP_OBF_FORMAT_Abstract_Audio_Album;

    // Populate format code->MIME type map
    m_imageMimeTable[MTP_OBF_FORMAT_BMP] = "image/bmp";
    m_imageMimeTable[MTP_OBF_FORMAT_GIF] = "image/gif";
    m_imageMimeTable[MTP_OBF_FORMAT_EXIF_JPEG] = "image/jpeg";
    m_imageMimeTable[MTP_OBF_FORMAT_PNG] = "image/png";
    m_imageMimeTable[MTP_OBF_FORMAT_TIFF] = "image/tiff";
}

/************************************************************
 * quint32 FSStoragePlugin::requestNewObjectHandle
 ***********************************************************/
quint32 FSStoragePlugin::requestNewObjectHandle()
{
    ObjHandle handle = 0;
    emit objectHandle( handle );
    if( 0 < handle )
    {
        m_uniqueObjectHandle = handle;
    }
#ifdef UT_ON
    // While ut'ing this, we won't have storagefactory giving us handles
    // so we have our own object handle provider below. The 0 == handle
    // check is to avoid recomputation of the handle in case of ut'ing along
    // with the release image.
    if( 0 == handle )
    {
        handle = ++m_uniqueObjectHandle;
    }
#endif
    return handle;
}

void FSStoragePlugin::requestNewPuoid( MtpInt128& newPuoid )
{
    emit puoid( newPuoid );
    m_largestPuoid = newPuoid;
}


void FSStoragePlugin::getLargestObjectHandle( ObjHandle& handle )
{
    handle = m_uniqueObjectHandle;
}

void FSStoragePlugin::getLargestPuoid( MtpInt128& puoid )
{
    puoid = m_largestPuoid;
}
/************************************************************
 * MTPrespCode FSStoragePlugin::addFileToStorage
 ***********************************************************/
MTPResponseCode FSStoragePlugin::addFileToStorage( StorageItem *&thisStorageItem, bool sendEvent, bool createIfNotExist )
{
    // Create the file in the file system.
    QFile file(thisStorageItem->m_path);
    QIODevice::OpenModeFlag openMode = ((createIfNotExist) ? (QIODevice::ReadWrite) : (QIODevice::ReadOnly));
    // If the file already exists, we do not have to open it in read-write mode
    if ( !file.open( openMode ) )
    {
        // Also remove it from the path names map, just in case...
        m_pathNamesMap.remove(thisStorageItem->m_path);
        unlinkChildStorageItem(thisStorageItem);
        delete thisStorageItem;
        thisStorageItem = 0;
        return MTP_RESP_GeneralError;
    }

#if 0
    if(createIfNotExist)
    {
        // Ask tracker to ignore the next update (close) on this file
        m_tracker->ignoreNextUpdate(QStringList(m_tracker->generateIri(thisStorageItem->m_path)));
    }
#endif
    // Next check if this is a playlist item.
    if((false == createIfNotExist) &&
       (MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist == thisStorageItem->m_objectInfo->mtpObjectFormat))
    {
        // Check is the playlist is also in tracker
        if(false == m_tracker->isPlaylistExisting(thisStorageItem->m_path))
        {
            // Delete the file from the filesystem and unlink the node
            unlinkChildStorageItem(thisStorageItem);
            delete thisStorageItem;
            thisStorageItem = 0;
            file.remove();
            return MTP_RESP_OK;
        }
    }

    file.close();

    // Assign a handle for this item.
    thisStorageItem->m_handle = requestNewObjectHandle();
    // Add the item to the path names map.
    m_pathNamesMap[ thisStorageItem->m_path ] = thisStorageItem->m_handle;
    // Add the item to the object handle map.
    m_objectHandlesMap[ thisStorageItem->m_handle ] = thisStorageItem;

    if( !m_puoidsMap.contains( thisStorageItem->m_path ) )
    {
        // Assign a new puoid
        requestNewPuoid( thisStorageItem->m_puoid );
        // Add the puoid to the map.
        m_puoidsMap[thisStorageItem->m_path] = thisStorageItem->m_puoid;
    }
    else
    {
        // Use the persistent puoid.
        thisStorageItem->m_puoid = m_puoidsMap[thisStorageItem->m_path];
    }

    // Add this PUOID to the PUOID->Object Handles map
    m_puoidToHandleMap[thisStorageItem->m_puoid] = thisStorageItem->m_handle;

    // Send an event to the MTP intitator if this file got added not due to the initiator.
    if( sendEvent )
    {
        QVector<quint32> eventParams;
        eventParams.append( thisStorageItem->m_handle );
        emit eventGenerated(MTP_EV_ObjectAdded, eventParams, QString());
    }

    // Dates from our device
    thisStorageItem->m_objectInfo->mtpCaptureDate = getCreatedDate( thisStorageItem );
    thisStorageItem->m_objectInfo->mtpModificationDate = getModifiedDate( thisStorageItem );

    return MTP_RESP_OK;
}

/************************************************************
 * MTPrespCode FSStoragePlugin::addDirToStorage
 ***********************************************************/
MTPResponseCode FSStoragePlugin::addDirToStorage( StorageItem *&thisStorageItem, bool isRootDir, bool sendEvent, bool createIfNotExist )
{
    // Create the directory in the file system.
    QDir dir = QDir( thisStorageItem->m_path );
    if( !dir.exists() && !isRootDir )
    {
        dir.mkdir( thisStorageItem->m_path );
    }

    // If this is the root dir, assign a handle of 0.
    if( isRootDir )
    {
        thisStorageItem->m_handle = 0;
    }
    // Assign a handle for this item.
    else
    {
        thisStorageItem->m_handle = requestNewObjectHandle();
    }
    // Add the item to the path names map.
    m_pathNamesMap[ thisStorageItem->m_path ] = thisStorageItem->m_handle;
    // Add the item to the object handle map.
    m_objectHandlesMap[ thisStorageItem->m_handle ] = thisStorageItem;

    if( !m_puoidsMap.contains( thisStorageItem->m_path ) )
    {
        // Assign a new puoid
        requestNewPuoid( thisStorageItem->m_puoid );
        // Add the puoid to the map.
        m_puoidsMap[thisStorageItem->m_path] = thisStorageItem->m_puoid;
    }
    else
    {
        // Use the persistent puoid.
        thisStorageItem->m_puoid = m_puoidsMap[thisStorageItem->m_path];
    }

    // Add the item to the watch descriptor maps.
    addWatchDescriptor( thisStorageItem );

    // Send an event to the MTP intitator if this file got added not due to the initiator.
    if( sendEvent )
    {
        QVector<quint32> eventParams;
        eventParams.append( thisStorageItem->m_handle );
        emit eventGenerated(MTP_EV_ObjectAdded, eventParams, QString());
    }

    // Recursively add the contents of the dir to the file system.
    dir.setFilter( QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden );
    QFileInfoList dirContents = dir.entryInfoList();
    // Ignore "." and ".."
    for( int i = 0; i < dirContents.size(); ++i )
    {
        QFileInfo dirContent = dirContents.at(i);
        // Create the storageItem.
        StorageItem *dirEntry = new StorageItem;
        dirEntry->m_path = dirContent.absoluteFilePath();
        linkChildStorageItem( dirEntry, thisStorageItem );
        populateObjectInfo( dirEntry );

        if( dirContent.isFile() )
        {
           // TODO Should we record this file in changeDB?
           addFileToStorage( dirEntry, sendEvent, createIfNotExist );
        }
        else if( dirContent.isDir() )
        {
           addDirToStorage( dirEntry, false, sendEvent );
        }
    }

    // Dates from our device
    thisStorageItem->m_objectInfo->mtpCaptureDate = getCreatedDate( thisStorageItem );
    thisStorageItem->m_objectInfo->mtpModificationDate = getModifiedDate( thisStorageItem );

    return MTP_RESP_OK;
}

/************************************************************
 * void FSStoragePlugin::linkChildStorageItem
 ***********************************************************/
void FSStoragePlugin::linkChildStorageItem( StorageItem *childStorageItem, StorageItem *parentStorageItem )
{
    if( !childStorageItem || !parentStorageItem )
    {
        return;
    }
    childStorageItem->m_parent = parentStorageItem;

    // Parent has no children
    if( !parentStorageItem->m_firstChild )
    {
        parentStorageItem->m_firstChild = childStorageItem;
    }
    // Parent already has children
    else
    {
        // Insert as first child
        StorageItem *oldFirstChild = parentStorageItem->m_firstChild;
        parentStorageItem->m_firstChild = childStorageItem;
        childStorageItem->m_nextSibling = oldFirstChild;
    }
}

/************************************************************
 * void FSStoragePlugin::unlinkChildStorageItem
 ***********************************************************/
void FSStoragePlugin::unlinkChildStorageItem( StorageItem *childStorageItem )
{
    if( !childStorageItem || !childStorageItem->m_parent )
    {
        return;
    }

    // If this is the first child.
    if( childStorageItem->m_parent->m_firstChild == childStorageItem)
    {
        childStorageItem->m_parent->m_firstChild = childStorageItem->m_nextSibling;
    }
    // not the first child
    else
    {
        StorageItem *itr = childStorageItem->m_parent->m_firstChild;
        while(itr && itr->m_nextSibling != childStorageItem)
        {
            itr = itr->m_nextSibling;
        }
        if(itr)
        {
            itr->m_nextSibling = childStorageItem->m_nextSibling;
        }
    }
    childStorageItem->m_nextSibling = 0;
}

/************************************************************
 * StorageItem* FSStoragePlugin::findStorageItemByPath
 ***********************************************************/
StorageItem* FSStoragePlugin::findStorageItemByPath( const QString &path )
{
    StorageItem *storageItem = 0;
    ObjHandle handle;
    if( m_pathNamesMap.contains( path ) )
    {
        handle = m_pathNamesMap.value( path );
        storageItem = m_objectHandlesMap.value( handle );
    }
    return storageItem;
}

/************************************************************
 * MTPrespCode FSStoragePlugin::addToStorage
 ***********************************************************/
MTPResponseCode FSStoragePlugin::addToStorage( StorageItem *&storageItem, MTPObjectInfo *info )
{
    QString parentPath = "";
    StorageItem *parentStorageItem = 0;

    // Find the absolute path of this item
    if( checkHandle( info->mtpParentObject ) )
    {
        parentStorageItem = m_objectHandlesMap[info->mtpParentObject];
        parentPath = parentStorageItem->m_path + "/";
    }

    // Create the storageItem.
    storageItem = new StorageItem;
    storageItem->m_path = parentPath + info->mtpFileName;
    storageItem->m_objectInfo = new MTPObjectInfo;
    *(storageItem->m_objectInfo) = *info;

    // In case the path already exits...
    if( m_pathNamesMap.contains( storageItem->m_path ) )
    {
        delete storageItem;
        storageItem = findStorageItemByPath(parentPath + info->mtpFileName);
        return MTP_RESP_OK;
    }

    linkChildStorageItem( storageItem, parentStorageItem );
    // Create file or directory
    switch( info->mtpObjectFormat )
    {
        // Directory.
        case MTP_OBF_FORMAT_Association:
        {
            return addDirToStorage( storageItem, false, false, true );
        }
        // File.
        default:
        {
            return addFileToStorage( storageItem, false, true);
        }
    }
}

/************************************************************
 * MTPrespCode FSStoragePlugin::addItem
 ***********************************************************/
MTPResponseCode FSStoragePlugin::addItem( ObjHandle &parentHandle, ObjHandle &handle, MTPObjectInfo *info )
{
    MTPResponseCode response;
    StorageItem *storageItem = 0;

    if( !info )
    {
        return MTP_RESP_Invalid_Dataset;
    }

    // Initiator has left it to us to choose the parent, choose root folder.
    if( 0xFFFFFFFF == info->mtpParentObject )
    {
        info->mtpParentObject = 0x00000000;
    }

    // Check if the parent is valid.
    if( !checkHandle( info->mtpParentObject ) )
    {
        return MTP_RESP_InvalidParentObject;
    }

    // Add the object ( file/dir ) to the filesystem storage.
    response = addToStorage( storageItem, info );
    if( storageItem )
    {
        handle = storageItem->m_handle;
        parentHandle = storageItem->m_parent ? storageItem->m_parent->m_handle : 0x00000000;
    }

    return response;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::deleteItem
 ***********************************************************/
MTPResponseCode FSStoragePlugin::deleteItem( const ObjHandle& handle, const MTPObjFormatCode& formatCode )
{
    // If handle == 0xFFFFFFFF, that means delete all objects that can be deleted ( this could be filered by fmtCode )
    quint32 totalNoOfObjects = m_objectHandlesMap.count() - 1;
    quint32 noOfObjectsByFormat = 0;
    StorageItem *storageItem = 0;
    MTPResponseCode response = MTP_RESP_GeneralError;

    if( 0xFFFFFFFF == handle )
    {
        for( QHash<ObjHandle,StorageItem*>::const_iterator i = m_objectHandlesMap.constBegin() ; i != m_objectHandlesMap.constEnd(); ++i )
        {
            if( formatCode && MTP_OBF_FORMAT_Undefined != formatCode )
            {
                storageItem = i.value();
                if( storageItem->m_objectInfo && storageItem->m_objectInfo->mtpObjectFormat == formatCode )
                {
                    ++noOfObjectsByFormat;
                    response = deleteItemHelper( i.key() );
                }
            }
            else
            {
                response = deleteItemHelper( i.key() );
            }
        }
    }
    else
    {
        response = deleteItemHelper( handle );
    }

    if( MTP_RESP_OK == response && 0xFFFFFFFF == handle )
    {
        if( formatCode && MTP_OBF_FORMAT_Undefined != formatCode )
        {
            if( noOfObjectsByFormat + m_objectHandlesMap.count() - 1 != totalNoOfObjects )
            {
                response = MTP_RESP_PartialDeletion;
            }
        }
        else if( m_objectHandlesMap.count() != 1 )
        {
            response = MTP_RESP_PartialDeletion;
        }
    }

    return response;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::deleteItemHelper
 ***********************************************************/
MTPResponseCode FSStoragePlugin::deleteItemHelper( const ObjHandle& handle, bool removePhysically, bool sendEvent )
{
    MTPResponseCode response = MTP_RESP_GeneralError;
    bool itemNotDeleted = false;
    if( !checkHandle( handle ) )
    {
        return MTP_RESP_InvalidObjectHandle;
    }

    // Get the corresponding storage item.
    StorageItem *storageItem = m_objectHandlesMap[handle];

    if( !storageItem )
    {
        return MTP_RESP_GeneralError;
    }

    // If this is a file or an empty dir, just delete this item.
    if( !storageItem->m_firstChild )
    {
        if( removePhysically && MTP_OBF_FORMAT_Association == storageItem->m_objectInfo->mtpObjectFormat && 0 != storageItem->m_handle )
        {
            QDir dir(storageItem->m_parent->m_path);
            if( !dir.rmdir(storageItem->m_path) )
            {
                return MTP_RESP_GeneralError;
            }
        }
        else if( removePhysically )
        {
            QFile file( storageItem->m_path );
            if( !file.remove() )
            {
                return MTP_RESP_GeneralError;
            }
        }
        // If this an abstract playlist, also remove the internal playlist.
        if(MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist == storageItem->m_objectInfo->mtpObjectFormat)
        {
            removePlaylist(storageItem->m_path);
        }

        removeFromStorage( handle, sendEvent );
    }
    // if this is a non-empty dir.
    else
    {
        StorageItem* itr = storageItem->m_firstChild;
        while(itr)
        {
            response = deleteItemHelper( itr->m_handle, removePhysically, sendEvent );
            if( MTP_RESP_OK != response )
            {
                itemNotDeleted = true;
                break;
            }
            //itr = itr->m_nextSibling;
            itr = storageItem->m_firstChild;
        }
        // Now delete the empty directory ( if empty! ).
        if( !itemNotDeleted )
        {
            response = deleteItemHelper( handle );
        }
        else
        {
            return MTP_RESP_PartialDeletion;
        }
    }
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::removeFromStorage
 ***********************************************************/
MTPResponseCode FSStoragePlugin::removeFromStorage( const ObjHandle& handle, bool sendEvent )
{
    StorageItem *storageItem = 0;
    // Remove the item from object handles map and delete the corresponding storage item.
    if( checkHandle( handle ) )
    {
        storageItem = m_objectHandlesMap.value( handle );
        // Remove the item from the watch descriptor map if present.
        if(-1 != storageItem->m_wd)
        {
            // Remove watch on the path and then remove the wd from the map
            removeWatchDescriptor( storageItem );
        }
        m_objectHandlesMap.remove( handle );
        m_pathNamesMap.remove( storageItem->m_path );
        unlinkChildStorageItem( storageItem );
        delete storageItem;
    }

    if( sendEvent )
    {
        QVector<quint32> eventParams;
        eventParams.append( handle );
        emit eventGenerated(MTP_EV_ObjectRemoved, eventParams, QString());
    }

    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::getObjectHandles
 ***********************************************************/
MTPResponseCode FSStoragePlugin::getObjectHandles( const MTPObjFormatCode& formatCode, const quint32& associationHandle,
                                                   QVector<ObjHandle> &objectHandles ) const
{

    switch( associationHandle )
    {
        // Count of all objects in this storage.
        case 0x00000000:
            if( !formatCode )
            {
                for( QHash<ObjHandle,StorageItem*>::const_iterator i = m_objectHandlesMap.constBegin() ; i != m_objectHandlesMap.constEnd(); ++i )
                {
                    // Don't enumerate the root.
                    if( 0 == i.key() )
                    {
                        continue;
                    }
                    objectHandles.append( i.key() );
                }
            }
            else
            {
                for( QHash<ObjHandle,StorageItem*>::const_iterator i = m_objectHandlesMap.constBegin() ; i != m_objectHandlesMap.constEnd(); ++i )
                {
                    // Don't enumerate the root.
                    if( 0 == i.key() )
                    {
                        continue;
                    }
                    if( i.value()->m_objectInfo && formatCode == i.value()->m_objectInfo->mtpObjectFormat )
                    {
                        objectHandles.append( i.key() );
                    }
                }
            }
            break;

        // Count of all objects present in the root storage.
        case 0xFFFFFFFF:
            if( m_root )
            {
                StorageItem *storageItem = m_root->m_firstChild;
                while( storageItem )
                {
                    if( !formatCode ||
                        ( MTP_OBF_FORMAT_Undefined != formatCode && storageItem->m_objectInfo && formatCode == storageItem->m_objectInfo->mtpObjectFormat ) )
                    {
                        objectHandles.append( storageItem->m_handle );
                    }
                    storageItem = storageItem->m_nextSibling;
                }
            }
            else
            {
                return MTP_RESP_InvalidParentObject;
            }
            break;

       // Count of all objects that are children of an object whose handle = associationHandle;
       default:
           //Check if the association handle is valid.
           if( !m_objectHandlesMap.contains( associationHandle ) )
           {
                return MTP_RESP_InvalidParentObject;
           }
           StorageItem *parentItem = m_objectHandlesMap[ associationHandle ];
           if( parentItem )
           {
               //Check if this is an association
               if( !parentItem->m_objectInfo || MTP_OBF_FORMAT_Association != parentItem->m_objectInfo->mtpObjectFormat )
               {
                   return MTP_RESP_InvalidParentObject;
               }
               StorageItem *storageItem = parentItem->m_firstChild;
               while( storageItem )
               {
                   if( ( !formatCode ) ||
                       ( MTP_OBF_FORMAT_Undefined != formatCode && storageItem->m_objectInfo &&
                         formatCode == storageItem->m_objectInfo->mtpObjectFormat ) )
                   {
                       objectHandles.append( storageItem->m_handle );
                   }
                   storageItem = storageItem->m_nextSibling;
               }
           }
           else
           {
               return MTP_RESP_InvalidParentObject;
           }
          break;
    }
    return MTP_RESP_OK;
}

/************************************************************
 * bool FSStoragePlugin::checkHandle
 ***********************************************************/
bool FSStoragePlugin::checkHandle( const ObjHandle &handle ) const
{
    return m_objectHandlesMap.contains( handle ) ? true : false;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::storageInfo
 ***********************************************************/
MTPResponseCode FSStoragePlugin::storageInfo( MTPStorageInfo &info )
{
    info = m_storageInfo;
    struct statvfs stat;
    MTPResponseCode responseCode = MTP_RESP_OK;

    //FIXME max capacity should be a const too, so can be computed once in the constructor.
    QByteArray ba = m_storagePath.toUtf8();
    if ( statvfs(ba.constData(), &stat) )
    {
        responseCode = MTP_RESP_GeneralError;
    }
    else
    {
        info.maxCapacity = m_storageInfo.maxCapacity = (quint64)stat.f_blocks * stat.f_bsize;
        info.freeSpace = m_storageInfo.freeSpace = (quint64)stat.f_bavail * stat.f_bsize;
    }
    return responseCode;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::copyObject
 ***********************************************************/
MTPResponseCode FSStoragePlugin::copyObject( const ObjHandle &handle, const ObjHandle &parentHandle, const quint32 &destinationStorageId, ObjHandle &copiedObjectHandle, quint32 recursionCounter /*= 0*/)
{
    bool txCancelled = false;

    if( !checkHandle( handle ) )
    {
        return MTP_RESP_InvalidObjectHandle;
    }
    if( !checkHandle( parentHandle ) )
    {
        return MTP_RESP_InvalidParentObject;
    }
    StorageItem *storageItem = m_objectHandlesMap[handle];
    if( !storageItem )
    {
        return MTP_RESP_GeneralError;
    }
    if( !m_objectHandlesMap[handle]->m_objectInfo )
    {
        return MTP_RESP_InvalidObjectHandle;
    }

    // Get the source object's objectinfo dataset.
    MTPObjectInfo objectInfo  = *m_objectHandlesMap[handle]->m_objectInfo;
    QString destinationPath = m_objectHandlesMap[parentHandle]->m_path + "/" + objectInfo.mtpFileName;

    // FIXME freeSpace not accurate, Check if the object fits.
    /*quint64 freeSpaceRemaining = 0;
    MTPResponseCode resp;
    resp = freeSpace( freeSpaceRemaining );
    if( freeSpaceRemaining < objectInfo.mtpObjectCompressedSize )
    {
        return MTP_RESP_StoreFull;
    }*/

    if( ( 0 == recursionCounter ) && MTP_OBF_FORMAT_Association == objectInfo.mtpObjectFormat )
    {
        // Check if we copy a dir to a place where it already exists; don't allow that
        if( m_pathNamesMap.contains( destinationPath ) )
        {
            return MTP_RESP_InvalidParentObject;
        }
    }

    // Modify the objectinfo dataset to include the new storageId and new parent object.
    objectInfo.mtpParentObject = parentHandle;
    objectInfo.mtpStorageId = destinationStorageId;

    // Work-around : remove wd on the destination dir so that we don't
    // receive inotify signals, this prevents adding them twice
    StorageItem *parentItem = m_objectHandlesMap[parentHandle];
    removeWatchDescriptor( parentItem );

    // Write the content to the new item.
    MTPResponseCode response = MTP_RESP_OK;
    QString sourcePath = storageItem->m_path;

    // Apply metadata for the destination path
    m_tracker->copy(sourcePath, destinationPath);
    // Ask tracker to ignore the next update on the destination path
    //m_tracker->ignoreNextUpdate(QStringList(m_tracker->generateIri(destinationPath)));

    // If this is a directory, copy recursively.
    if( MTP_OBF_FORMAT_Association == objectInfo.mtpObjectFormat )
    {
        // Create the new item.
        response = addItem( const_cast<ObjHandle&>(parentHandle), copiedObjectHandle, &objectInfo );
        if( MTP_RESP_OK != response )
        {
            return response;
        }

        // Save the directory handle
        ObjHandle parentHandle = copiedObjectHandle;
        StorageItem *childItem = storageItem->m_firstChild;
        for( ; childItem; childItem = childItem->m_nextSibling )
        {
            response = copyObject( childItem->m_handle, parentHandle, destinationStorageId, copiedObjectHandle, ++recursionCounter );
            if( MTP_RESP_OK != response )
            {
                copiedObjectHandle = parentHandle;
                return response;
            }
        }
        // Restore directory handle
        copiedObjectHandle = parentHandle;
    }
    // this is a file, copy the data
    else
    {
        response = addItem( const_cast<ObjHandle&>(parentHandle), copiedObjectHandle, &objectInfo );
        if( MTP_RESP_OK != response )
        {
            return response;
        }

        quint64 sourceObjSize = storageItem->m_objectInfo->mtpObjectCompressedSize;
        quint32 readOffset = 0, remainingLen = sourceObjSize;
        qint32 readLen = MAX_READ_LEN;
        char readBuffer[MAX_READ_LEN];

        while( remainingLen && response == MTP_RESP_OK )
        {
            readLen = remainingLen >= MAX_READ_LEN ? MAX_READ_LEN : remainingLen;
            response = readData( handle, readBuffer, readLen, readOffset );

            emit checkTransportEvents( txCancelled );
            if( txCancelled )
            {
                MTP_LOG_WARNING("CopyObject cancelled, aborting file copy...");
                response = deleteItem( copiedObjectHandle, MTP_OBF_FORMAT_Undefined );
                return MTP_RESP_GeneralError;
            }

            if( MTP_RESP_OK == response )
            {
                remainingLen -= readLen;
                response = writeData( copiedObjectHandle, readBuffer, readLen, readOffset == 0, false );
                readOffset += readLen;
                if( !remainingLen )
                {
                    response = writeData( copiedObjectHandle, 0, 0, false, true );
                }
            }
        }
    }

    addWatchDescriptor( parentItem );
    return response;
}

/************************************************************
 * void FSStoragePlugin::adjustMovedItemsPath
 ***********************************************************/
void FSStoragePlugin::adjustMovedItemsPath( QString newAncestorPath, StorageItem* movedItem, bool updateInTracker /*= false*/ )
{
    if( !movedItem )
    {
        return;
    }

    m_pathNamesMap.remove( movedItem->m_path );
    QString destinationPath = newAncestorPath + "/" + movedItem->m_objectInfo->mtpFileName;
    if(true == updateInTracker)
    {
        // Move the URI in tracker too
        m_tracker->move(movedItem->m_path, destinationPath);

        if( MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist == movedItem->m_objectInfo->mtpObjectFormat )
        {
            // If this is a playlist, also need to update the playlist URL
            m_tracker->movePlaylist(movedItem->m_path, destinationPath);
        }
    }
    movedItem->m_path = destinationPath;
    m_pathNamesMap[movedItem->m_path] = movedItem->m_handle;
    StorageItem *itr = movedItem->m_firstChild;
    while( itr )
    {
        adjustMovedItemsPath( movedItem->m_path, itr, updateInTracker );
        itr = itr->m_nextSibling;
    }
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::moveObject
 ***********************************************************/
MTPResponseCode FSStoragePlugin::moveObject( const ObjHandle &handle, const ObjHandle &parentHandle, const quint32 &destinationStorageId, bool movePhysically )
{
    // TODO Handle moves across storages;
    if( m_storageId != destinationStorageId )
    {
        return MTP_RESP_GeneralError;
    }

    if( !checkHandle( handle ) )
    {
        return MTP_RESP_InvalidObjectHandle;
    }
    if( !checkHandle( parentHandle ) )
    {
        return MTP_RESP_InvalidParentObject;
    }

    StorageItem *storageItem = m_objectHandlesMap[handle], *parentItem = m_objectHandlesMap[parentHandle];
    if( !storageItem || !parentItem )
    {
        return MTP_RESP_GeneralError;
    }

    if( storageItem->m_path == m_playlistPath )
    {
        MTP_LOG_WARNING("Don't play around with the Playlists directory!");
        return MTP_RESP_AccessDenied;
    }

    QString destinationPath = parentItem->m_path + "/" + storageItem->m_objectInfo->mtpFileName;

    // If this is a directory already exists, don't overwrite it.
    if( MTP_OBF_FORMAT_Association == storageItem->m_objectInfo->mtpObjectFormat )
    {
        if( m_pathNamesMap.contains( destinationPath ) )
        {
            return MTP_RESP_InvalidParentObject;
        }
    }

    // Invalidate the watch descriptor for this item and it's children, as their paths will change.
    removeWatchDescriptorRecursively( storageItem );
    // unlink this item from it's current parent;
    unlinkChildStorageItem( storageItem );

    // Do the move.
    if( movePhysically )
    {
        QDir dir;
        if ( !dir.rename( storageItem->m_path, destinationPath ) )
        {
            return MTP_RESP_InvalidParentObject;
        }
    }
    m_pathNamesMap.remove( storageItem->m_path );
    m_pathNamesMap[destinationPath] = handle;

    StorageItem *itr = storageItem->m_firstChild;
    while( itr )
    {
        adjustMovedItemsPath( destinationPath, itr, true );
        itr = itr->m_nextSibling;
    }

    // link it to the new parent
    linkChildStorageItem( storageItem, parentItem );
    //storageItem->m_nextSibling = 0;
    // Reset URI in tracker and ask it to ignore
    m_tracker->move(storageItem->m_path, destinationPath);

    if(storageItem->m_objectInfo &&
            MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist == storageItem->m_objectInfo->mtpObjectFormat)
    {
        // If this is a playlist, also need to update the playlist URL
        m_tracker->movePlaylist(storageItem->m_path, destinationPath);
    }

    // update it's path and parent object.
    storageItem->m_path = destinationPath;
    storageItem->m_objectInfo->mtpParentObject = parentHandle;
    // create new watch descriptors for the moved item.
    addWatchDescriptorRecursively( storageItem );
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::getFileListRecursively
 ***********************************************************/
void FSStoragePlugin::getFileListRecursively(const StorageItem *storageItem, const QString &destinationPath, QStringList &fileList)
{
    // For every file to be moved, get the old path and new path recursively
    if(0 == storageItem)
    {
        return;
    }
    // Add this iri to the list
    fileList.append(m_tracker->generateIri(storageItem->m_path));
    // Add the destination iri to the list
    fileList.append(m_tracker->generateIri(destinationPath));
    StorageItem *itr = storageItem->m_firstChild;
    while( itr )
    {
        getFileListRecursively( itr, destinationPath + "/" + itr->m_objectInfo->mtpFileName, fileList );
        itr = itr->m_nextSibling;
    }
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::getObjectInfo
 ***********************************************************/
MTPResponseCode FSStoragePlugin::getObjectInfo( const ObjHandle &handle, const MTPObjectInfo *&objectInfo )
{
    if( !checkHandle( handle ) )
    {
        return MTP_RESP_InvalidObjectHandle;
    }
    StorageItem *storageItem = m_objectHandlesMap[handle];
    if( !storageItem )
    {
        return MTP_RESP_GeneralError;
    }
    populateObjectInfo( storageItem );
    objectInfo = storageItem->m_objectInfo;
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::populateObjectInfo
 ***********************************************************/
void FSStoragePlugin::populateObjectInfo( StorageItem *&storageItem )
{
    if( !storageItem )
    {
        return;
    }
    // If we have already stored objectinfo for this item.
    if( storageItem->m_objectInfo )
    {
        return;
    }

    // Populate object info for this item.
    storageItem->m_objectInfo = new MTPObjectInfo;

    // storage id.
    storageItem->m_objectInfo->mtpStorageId = m_storageId;
    // file name
    QString name = storageItem->m_path;
    name = name.remove(0,storageItem->m_path.lastIndexOf("/") + 1);
    storageItem->m_objectInfo->mtpFileName = name;
    // object format.
    storageItem->m_objectInfo->mtpObjectFormat = getObjectFormatByExtension( storageItem );
    // protection status.
    storageItem->m_objectInfo->mtpProtectionStatus = getMTPProtectionStatus( storageItem );
    // object size.
    storageItem->m_objectInfo->mtpObjectCompressedSize = getObjectSize( storageItem );
    // thumb size
    storageItem->m_objectInfo->mtpThumbCompressedSize = getThumbCompressedSize( storageItem );
    // thumb format
    storageItem->m_objectInfo->mtpThumbFormat = getThumbFormat( storageItem );
    // thumb width
    storageItem->m_objectInfo->mtpThumbPixelWidth = getThumbPixelWidth( storageItem );
    // thumb height
    storageItem->m_objectInfo->mtpThumbPixelHeight = getThumbPixelHeight( storageItem );
    // image pix width
    storageItem->m_objectInfo->mtpImagePixelWidth = getImagePixelWidth( storageItem );
    // image pix height
    storageItem->m_objectInfo->mtpImagePixelHeight = getImagePixelHeight( storageItem );
    // image bit depth
    storageItem->m_objectInfo->mtpImageBitDepth = getImageBitDepth( storageItem );
    // parent object.
    storageItem->m_objectInfo->mtpParentObject = storageItem->m_parent ? storageItem->m_parent->m_handle : 0x00000000;
    // association type
    storageItem->m_objectInfo->mtpAssociationType = getAssociationType( storageItem );
    // association description
    storageItem->m_objectInfo->mtpAssociationDescription = getAssociationDescription( storageItem );
    // sequence number
    storageItem->m_objectInfo->mtpSequenceNumber = getSequenceNumber( storageItem );
    // date created
    storageItem->m_objectInfo->mtpCaptureDate = getCreatedDate( storageItem );
    // date modified
    storageItem->m_objectInfo->mtpModificationDate = getModifiedDate( storageItem );
    // keywords.
    storageItem->m_objectInfo->mtpKeywords = getKeywords( storageItem );
}

/************************************************************
 * quint16 FSStoragePlugin::getObjectFormatByExtension
 ***********************************************************/
quint16 FSStoragePlugin::getObjectFormatByExtension( StorageItem *storageItem )
{
    // TODO Fetch from tracker or determine from the file.
    quint16 format = MTP_OBF_FORMAT_Undefined;

    QFileInfo item(storageItem->m_path);
    if( item.isDir() )
    {
        format = MTP_OBF_FORMAT_Association;
    }
    else //file
    {
        QString ext = storageItem->m_path.section('.',-1).toLower();
        if( m_formatByExtTable.contains( ext ) )
        {
            format = m_formatByExtTable[ext];
        }
    }
    return format;
}

/************************************************************
 * quint16 FSStoragePlugin::getMTPProtectionStatus
 ***********************************************************/
quint16 FSStoragePlugin::getMTPProtectionStatus( StorageItem* /*storageItem*/ )
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint64 FSStoragePlugin::getObjectSize
 ***********************************************************/
quint64 FSStoragePlugin::getObjectSize( StorageItem *storageItem )
{
    if( !storageItem )
    {
        return 0;
    }
    QFileInfo item( storageItem->m_path );
    if( item.isFile() )
    {
        return item.size();
    }
    return 0;
}

/************************************************************
 * bool FSStoragePlugin::isImage
 ***********************************************************/
bool FSStoragePlugin::isImage( StorageItem *storageItem )
{
    //UGLY
    if( storageItem &&
        ( storageItem->m_path.endsWith("gif")  ||
          storageItem->m_path.endsWith("jpeg") ||
          storageItem->m_path.endsWith("jpg")  ||
          storageItem->m_path.endsWith("bmp")  ||
          storageItem->m_path.endsWith("tif")  ||
          storageItem->m_path.endsWith("tiff") ||
          storageItem->m_path.endsWith("png")
        )
      )
    {
        return true;
    }
    return false;
}

/************************************************************
 * quint16 FSStoragePlugin::getThumbFormat
 ***********************************************************/
quint16 FSStoragePlugin::getThumbFormat( StorageItem *storageItem )
{
    quint16 format = MTP_OBF_FORMAT_Undefined;
    if( isImage( storageItem ) )
    {
        format = MTP_OBF_FORMAT_JFIF;
    }
    return format;
}

/************************************************************
 * quint32 FSStoragePlugin::getThumPixelWidth
 ***********************************************************/
quint32 FSStoragePlugin::getThumbPixelWidth( StorageItem *storageItem )
{
    quint16 width = 0;
    if( isImage( storageItem ) )
    {
        width = THUMB_WIDTH;
    }
    return width;
}

/************************************************************
 * quint32 FSStoragePlugin::getThumPixelHeight
 ***********************************************************/
quint32 FSStoragePlugin::getThumbPixelHeight( StorageItem *storageItem )
{
    quint16 height = 0;
    if( isImage( storageItem ) )
    {
        height = THUMB_HEIGHT;
    }
    return height;
}

/************************************************************
 * quint32 FSStoragePlugin::getThumbCompressedSize
 ***********************************************************/
quint32 FSStoragePlugin::getThumbCompressedSize( StorageItem * /*storageItem*/ )
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint32 FSStoragePlugin::getImagePixelWidth
 ***********************************************************/
quint32 FSStoragePlugin::getImagePixelWidth( StorageItem * /*storageItem*/ )
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint32 FSStoragePlugin::getImagePixelHeight
 ***********************************************************/
quint32 FSStoragePlugin::getImagePixelHeight( StorageItem * /*storageItem*/ )
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint16 FSStoragePlugin::getAssociationType
 ***********************************************************/
quint16 FSStoragePlugin::getAssociationType( StorageItem * /*storageItem*/ )
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint32 FSStoragePlugin::getAssociationDescription
 ***********************************************************/
quint32 FSStoragePlugin::getAssociationDescription( StorageItem * /*storageItem*/ )
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint32 FSStoragePlugin::getImageBitDepth
 ***********************************************************/
quint32 FSStoragePlugin::getImageBitDepth( StorageItem * /*storageItem*/ )
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint32 FSStoragePlugin::getSequenceNumber
 ***********************************************************/
quint32 FSStoragePlugin::getSequenceNumber( StorageItem * /*storageItem*/ )
{
    // TODO Fetch from tracker
    return 0;
}

/************************************************************
 * QString FSStoragePlugin::getCreatedDate
 ***********************************************************/
QString FSStoragePlugin::getCreatedDate( StorageItem *storageItem )
{
    // Get creation date from the file system
    QFileInfo fileInfo(storageItem->m_path);
    QDateTime dt = fileInfo.created();
    dt = dt.toUTC();
    QString dateCreated = dt.toString("yyyyMMdd'T'hhmmss'Z'");
    return dateCreated;
}

/************************************************************
 * char* FSStoragePlugin::getModifiedDate
 ***********************************************************/
QString FSStoragePlugin::getModifiedDate( StorageItem *storageItem )
{
    // Get modification date from the file system
    QFileInfo fileInfo(storageItem->m_path);
    QDateTime dt = fileInfo.lastModified();
    dt = dt.toUTC();
    QString dateModified = dt.toString("yyyyMMdd'T'hhmmss'Z'");
    return dateModified;
}

/************************************************************
 * char* FSStoragePlugin::getKeywords
 ***********************************************************/
char* FSStoragePlugin::getKeywords( StorageItem * /*storageItem*/ )
{
    // Not supported.
    return 0;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::readData
 ***********************************************************/
MTPResponseCode FSStoragePlugin::readData( const ObjHandle &handle, char *readBuffer, qint32 &readBufferLen, quint32 readOffset )
{
    if( !checkHandle( handle ) )
    {
        return MTP_RESP_InvalidObjectHandle;
    }

    if(0 == readBuffer)
    {
        return MTP_RESP_GeneralError;
    }

    // Get the corresponding storage item.
    StorageItem *storageItem = m_objectHandlesMap[handle];
    if( !storageItem )
    {
        return MTP_RESP_GeneralError;
    }

    // Open the file and read from it.
    qint32 bytesToRead = readBufferLen;
    QFile file( storageItem->m_path );
    if( !file.open( QIODevice::ReadOnly ) )
    {
        return MTP_RESP_GeneralError;
    }
    qint64 fileSize = file.size();
    if(fileSize < (readOffset + bytesToRead))
    {
        return MTP_RESP_GeneralError;
    }
    if( !file.seek(readOffset) )
    {
        MTP_LOG_WARNING("ERROR seeking file");
        return MTP_RESP_GeneralError;
    }
    // TODO: Read this in a loop?
    readBufferLen = 0;
    do{
        readBufferLen = file.read( readBuffer, bytesToRead );
        if( -1 == readBufferLen )
        {
            return MTP_RESP_GeneralError;
        }
        bytesToRead -= readBufferLen;
    }while(bytesToRead);
    // FIXME: Repeated opening and closing of the file will happen when we do segmented reads.
    // This must be avoided
    file.close();
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::truncateItem
 ***********************************************************/
MTPResponseCode FSStoragePlugin::truncateItem( const ObjHandle &handle, const quint32 &size )
{
    if( !checkHandle( handle ) )
    {
        return MTP_RESP_InvalidObjectHandle;
    }

    // Get the corresponding storage item.
    StorageItem *storageItem = m_objectHandlesMap[handle];
    if( !storageItem || !storageItem->m_objectInfo || MTP_OBF_FORMAT_Association == storageItem->m_objectInfo->mtpObjectFormat )
    {
        return MTP_RESP_GeneralError;
    }

    QFile file( storageItem->m_path );
    if( !file.resize( size ) )
    {
        return MTP_RESP_GeneralError;
    }
    storageItem->m_objectInfo->mtpObjectCompressedSize = static_cast<quint64>(size);
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::writeData
 ***********************************************************/
MTPResponseCode FSStoragePlugin::writeData( const ObjHandle &handle, char *writeBuffer, quint32 bufferLen, bool isFirstSegment, bool isLastSegment )
{
    if( !checkHandle( handle ) )
    {
        return MTP_RESP_InvalidObjectHandle;
    }

    // Get the corresponding storage item.
    StorageItem *storageItem = m_objectHandlesMap[handle];
    if( !storageItem )
    {
        return MTP_RESP_GeneralError;
    }

    if( ( true == isLastSegment ) && ( 0 == writeBuffer ) )
    {
        m_writeObjectHandle = 0;
        if( m_dataFile )
        {
            m_dataFile->close();
            delete m_dataFile;
            m_dataFile = 0;
        }
    }
    else
    {
        m_writeObjectHandle = handle;
        qint32 bytesRemaining = bufferLen;
        // Resize file to zero, if first segment
        if(isFirstSegment)
        {
            // Open the file and write to it.
            m_dataFile = new QFile( storageItem->m_path );
            if( !m_dataFile->open( QIODevice::Append ) )
            {
                delete m_dataFile;
                m_dataFile = 0;
                return MTP_RESP_GeneralError;
            }
            m_dataFile->resize(0);
        }
        while( bytesRemaining && m_dataFile )
        {
            qint32 bytesWritten = m_dataFile->write( writeBuffer, bytesRemaining );
            if( -1 == bytesWritten )
            {
                MTP_LOG_WARNING("ERROR writing data to" << storageItem->m_path);
                //Send a store full event if there's no space.
                /*MTPResponseCode resp;
                MTPObjectInfo objectInfo;
                resp = getObjectInfo( handle, &objectInfo );
                quint64 freeSpaceRemaining = 0;
                resp = freeSpace( freeSpaceRemaining );
                if( freeSpaceRemaining < objectInfo.mtpObjectCompressedSize )
                {
                    QVector<quint32> params;
                    params.append(m_storageId);
                    emit eventGenerated(MTP_EV_StoreFull, params, QString());
                }*///TODO Fixme eventGenerated not working.
                return MTP_RESP_GeneralError;
            }
            bytesRemaining -= bytesWritten;
            writeBuffer += bytesWritten;
        }
    }
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::getPath
 ***********************************************************/
MTPResponseCode FSStoragePlugin::getPath( const quint32 &handle, QString &path ) const
{
    path = "";
    if( !m_objectHandlesMap.contains( handle ) )
    {
        return MTP_RESP_GeneralError;
    }
    StorageItem *storageItem = m_objectHandlesMap.value( handle );
    if( !storageItem )
    {
        return MTP_RESP_GeneralError;
    }

    path = storageItem->m_path;
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::dumpStorageItem
 ***********************************************************/
void FSStoragePlugin::dumpStorageItem( StorageItem *storageItem, bool recurse )
{
    if( !storageItem )
    {
        return;
    }

    ObjHandle parentHandle = storageItem->m_parent ? storageItem->m_parent->m_handle : 0;
    QString parentPath = storageItem->m_parent ? storageItem->m_parent->m_path : "";
    MTP_LOG_INFO("\n<" << storageItem->m_handle << "," << storageItem->m_path
                      << "," << parentHandle << "," << parentPath << ">");

    if( recurse )
    {
        StorageItem *itr = storageItem->m_firstChild;
        while( itr )
        {
            dumpStorageItem( itr, recurse );
            itr = itr->m_nextSibling;
        }
    }
}

/************************************************************
 * void FSStoragePlugin::inotifyEventSlot
 ***********************************************************/
void FSStoragePlugin::inotifyEventSlot( struct inotify_event *event )
{
    const struct inotify_event *fromEvent = 0;
    QString fromNameString;
    const char *name = 0;

    getCachedInotifyEvent( &fromEvent, fromNameString );
    QByteArray ba = fromNameString.toUtf8();

    // Trick to handle the last non-paired IN_MOVED_FROM
    if (!event)
    {
        if (fromEvent)
        {
            // File/directory was moved from the storage.
            handleFSDelete( fromEvent, ba.data() );
            clearCachedInotifyEvent();
        }
        return;
    }

    name = event->len ? event->name : NULL;
    if(!name)
    {
        return;
    }

    if( fromEvent && fromEvent->cookie != event->cookie )
    {
        // File/directory was moved from the storage.
        handleFSDelete( fromEvent, ba.data() );
        clearCachedInotifyEvent();
    }

    // File/directory was created.
    if( event->mask & IN_CREATE )
    {
        handleFSCreate( event, name );
    }

    // File/directory was deleted.
    if( event->mask & IN_DELETE )
    {
        handleFSDelete( event, name );
    }

    if( event->mask & IN_MOVED_TO )
    {
        if( fromEvent && fromEvent->cookie == event->cookie )
        {
            // File/directory was moved/renamed within the storage.
            handleFSMove( fromEvent, ba.data(), event, name );
            clearCachedInotifyEvent();
        }
        else
        {
            // File/directory was moved to the storage.
            handleFSCreate( event, name );
        }
    }

    if( event->mask & IN_MOVED_FROM )
    {
        if( fromEvent )
        {
            // File/directory was moved from the storage.
            handleFSDelete( fromEvent, ba.data() );
            clearCachedInotifyEvent();
        }
        // Don't know what to do with it yet. Save it for later.
        cacheInotifyEvent( event, name );
    }

    if( event->mask & IN_CLOSE_WRITE )
    {
        handleFSModify( event, name );
    }
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::getReferences
 ***********************************************************/
MTPResponseCode FSStoragePlugin::getReferences( const ObjHandle &handle , QVector<ObjHandle> &references )
{
    if( !m_objectHandlesMap.contains( handle ) )
    {
        removeInvalidObjectReferences( handle );
        return MTP_RESP_InvalidObjectHandle;
    }
    if( !m_objectReferencesMap.contains( handle ) )
    {
        return MTP_RESP_OK;
    }
    references = m_objectReferencesMap[handle];
    QVector<ObjHandle>::iterator i = references.begin();
    while( i != references.end() )
    {
        if( !m_objectHandlesMap.contains( *i ) )
        {
            i = references.erase( i );
            // TODO: Remove the handle from the playlist too
        }
        else
        {
            ++i;
        }
    }
    m_objectReferencesMap[handle] = references;
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::setReferences
 ***********************************************************/
//TODO Do we have cases where we need to set our own references (ie not due to initiator's request)
MTPResponseCode FSStoragePlugin::setReferences( const ObjHandle &handle , const QVector<ObjHandle> &references )
{
    StorageItem *playlist = m_objectHandlesMap.value(handle);
    StorageItem *reference = 0;
    if( 0 == playlist || 0 == playlist->m_objectInfo )
    {
        return MTP_RESP_InvalidObjectHandle;
    }
    bool savePlaylist = (MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist == playlist->m_objectInfo->mtpObjectFormat);
    QStringList entries;
    for( int i = 0; i < references.size(); ++i )
    {
        reference = m_objectHandlesMap.value(references[i]);
        if( 0 == reference || 0 == reference->m_objectInfo )
        {
            return MTP_RESP_Invalid_ObjectReference;
        }
        if(true == savePlaylist)
        {
            // Append the path to the entries list
            entries.append(reference->m_path);
        }
    }
    m_objectReferencesMap[handle] = references;
    // Trigger a save of playlists into tracker
    if(true == savePlaylist)
    {
        QString playlistId = m_tracker->savePlaylist(playlist->m_path, entries);
    }
    return MTP_RESP_OK;
}

/************************************************************
 * void FSStoragePlugin::removeInvalidObjectReferences
 ***********************************************************/
void FSStoragePlugin::removeInvalidObjectReferences( const ObjHandle &handle )
{
    QHash<ObjHandle, QVector<ObjHandle> >::iterator i = m_objectReferencesMap.begin();
    while( i != m_objectReferencesMap.end() )
    {
        QVector<ObjHandle>::iterator j = i.value().begin();
        while( j != i.value().end() )
        {
            if( handle == *j )
            {
                j = i.value().erase( j );
            }
            else
            {
                ++j;
            }
        }
        if( handle == i.key() )
        {
            i = m_objectReferencesMap.erase( i );
        }
        else
        {
            ++i;
        }
    }
}

/************************************************************
 * void FSStoragePlugin::setPlaylistReferences
 ***********************************************************/
#if 0
void FSStoragePlugin::setPlaylistReferences( const ObjHandle &handle , const QVector<ObjHandle> &references )
{
    if( !m_objectHandlesMap.contains( handle ) )
    {
        return;
    }
    StorageItem *storageItem = m_objectHandlesMap.value( handle );
    if( !storageItem || "" == storageItem->m_path || !storageItem->m_objectInfo || !storageItem->m_path.endsWith( ".pla" ) )
    {
        return;
    }
    QString name = storageItem->m_objectInfo->mtpFileName;
    name.replace( ".pla", ".m3u" );
    QString path = m_internalPlaylistPath + "/" + name;
    if( !path.endsWith( ".m3u" ) )
    {
        return;
    }
    QFile file( path );
    if( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        file.write( "#EXTM3U\n", 8 );
        for( int i = 0 ; i < references.size(); ++i )
        {
            if( !m_objectHandlesMap.contains( references[i] ) )
            {
                continue;
            }
            StorageItem *storageItem = m_objectHandlesMap.value( references[i] );
            if( !storageItem )
            {
                continue;
            }
            QString refItemName = storageItem->m_path;
            if( refItemName[refItemName.size() -1] == '\0' )
            {
                refItemName[refItemName.size() -1] = '\n';
            }
            else
            {
                refItemName += '\n';
            }
            QByteArray ba = refItemName.toUtf8();
            char *writeBuffer = ba.data();
            file.write( writeBuffer, strlen( writeBuffer ) );
        }
    }
}
#endif

/************************************************************
 * void FSStoragePlugin::storeObjectReferences
 ***********************************************************/
void FSStoragePlugin::storeObjectReferences()
{
    // Write as follows
    // 1. No. of objects that have references.
    // 2a. Object PUOID
    // 2b. No. of references for this object.
    // 2c. PUOIDs of the referred objects.

    qint32 bytesWritten = -1;
    QFile file( m_objectReferencesDbPath );
    StorageItem *item = 0;

    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        return;
    }
    quint32 noOfHandles = m_objectReferencesMap.count();
    // Recored the position to store the number of object handles at...
    qint64 posNoOfHandles = file.pos();
    bytesWritten = file.write( reinterpret_cast<const char*>(&noOfHandles), sizeof(quint32) );
    if( -1 == bytesWritten )
    {
        MTP_LOG_WARNING("ERROR writing count to persistent objrefs db!!");
        file.resize(0);
        return;
    }
    for( QHash<ObjHandle , QVector<ObjHandle> >::iterator i = m_objectReferencesMap.begin(); i != m_objectReferencesMap.end(); ++i )
    {
        ObjHandle handle = i.key();
        // Get the object PUOID from the object handle (we need to store PUOIDs
        // in the ref DB as it is persistent, not object handles)
        item = m_objectHandlesMap.value(handle);
        if(0 == item || (MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist == item->m_objectInfo->mtpObjectFormat))
        {
            // 1) Possibly, the handle was removed from the objectHandles map, but
            // still lingers in the object references map (It is cleared lazily
            // in getObjectReferences). Ignore this handle.
            // 2) This object is an abstract playlist, which is stored only in tracker.
            // Ignore this too.
            noOfHandles--;
            continue;
        }
        // No NULL check for the item here, as it can safely be assumed that it
        // will exist in the object handles map.
        bytesWritten = file.write( reinterpret_cast<const char*>(&item->m_puoid), sizeof(MtpInt128) );
        if( -1 == bytesWritten )
        {
            MTP_LOG_WARNING("ERROR writing a handle to persistent objrefs db!!");
            file.resize(0);
            return;
        }
        quint32 noOfRefs = i.value().size();
        // Record the position to write the number of object references into...
        qint64 posNoOfReferences = file.pos();
        bytesWritten = file.write( reinterpret_cast<const char*>(&noOfRefs), sizeof(quint32) );
        if( -1 == bytesWritten )
        {
            MTP_LOG_WARNING("ERROR writing a handle's ref count to persistent objrefs db!!");
            file.resize(0);
            return;
        }
        for( int j = 0; j < i.value().size(); ++j )
        {
            ObjHandle reference = (i.value())[j];
            // Get PUOID from the reference
            item = m_objectHandlesMap.value(reference);
            if(0 == item)
            {
                // Ignore this handle...
                noOfRefs--;
                continue;
            }
            bytesWritten = file.write( reinterpret_cast<const char*>(&item->m_puoid), sizeof(MtpInt128) );
            if( -1 == bytesWritten )
            {
                MTP_LOG_WARNING("ERROR writing a handle's reference to persistent objrefs db!!");
                file.resize(0);
                return;
            }
        }
        // Backup current position
        qint64 curPos = file.pos();
        // Seek back to write the number of references
        if(false == file.seek(posNoOfReferences))
        {
            MTP_LOG_WARNING("File seek failed!!");
            file.resize(0);
            return;
        }
        bytesWritten = file.write( reinterpret_cast<const char*>(&noOfRefs), sizeof(quint32) );
        if( -1 == bytesWritten )
        {
            MTP_LOG_WARNING("ERROR writing a handle's ref count to persistent objrefs db!!");
            file.resize(0);
            return;
        }
        // Seek forward again
        if(false == file.seek(curPos))
        {
            MTP_LOG_WARNING("File seek failed!!");
            file.resize(0);
            return;
        }

    }
    // Seek backwards to write the number of object handles
    if(false == file.seek(posNoOfHandles))
    {
        MTP_LOG_WARNING("File seek failed!!");
        file.resize(0);
        return;
    }
    bytesWritten = file.write( reinterpret_cast<const char*>(&noOfHandles), sizeof(quint32) );
    if( -1 == bytesWritten )
    {
        MTP_LOG_WARNING("ERROR writing count to persistent objrefs db!!");
        file.resize(0);
    }
    // No file.close() needed, handled in QFile's d'tor
}

/************************************************************
 * void FSStoragePlugin::populateObjectReferences
 ***********************************************************/
void FSStoragePlugin::populateObjectReferences()
{
    QFile file( m_objectReferencesDbPath );
    if( !file.open( QIODevice::ReadOnly ) )
    {
        return;
    }

    qint32 bytesRead = 0;
    quint32 noOfHandles = 0, noOfRefs = 0;
    MtpInt128 objPuoid, referencePuoid;
    QVector<ObjHandle> references;

    bytesRead = file.read( reinterpret_cast<char*>(&noOfHandles), sizeof(quint32) );
    if( 0 >= bytesRead )
    {
        return;
    }
    for( quint32 i = 0; i < noOfHandles; ++i )
    {
        bytesRead = file.read( reinterpret_cast<char*>(&objPuoid), sizeof(objPuoid) );
        if( 0 >= bytesRead )
        {
            return;
        }
        bytesRead = file.read( reinterpret_cast<char*>(&noOfRefs), sizeof(quint32) );
        if( 0 >= bytesRead )
        {
            return;
        }
        references.clear();
        for( quint32 j = 0; j < noOfRefs; ++j )
        {
            bytesRead = file.read( reinterpret_cast<char*>(&referencePuoid), sizeof(referencePuoid) );
            if( 0 >= bytesRead )
            {
                return;
            }
            // Get object handle from the PUOID
            if(m_puoidToHandleMap.contains(referencePuoid))
            {
                references.append( m_puoidToHandleMap[referencePuoid] );
            }
        }
        if(m_puoidToHandleMap.contains(objPuoid))
        {
            m_objectReferencesMap[m_puoidToHandleMap[objPuoid]] = references;
        }
    }
}

MTPResponseCode FSStoragePlugin:: getObjectPropertyValueFromStorage( const ObjHandle &handle,
                                                   MTPObjPropertyCode propCode,
                                                   QVariant &value, MTPDataType /*type*/ )
{
    MTPResponseCode code = MTP_RESP_OK;
    const MTPObjectInfo *objectInfo;
    code = getObjectInfo( handle, objectInfo );
    if( MTP_RESP_OK != code )
    {
        return code;
    }
    switch(propCode)
    {
        case MTP_OBJ_PROP_Association_Desc:
        {
            value = QVariant::fromValue(0);
        }
        break;
        case MTP_OBJ_PROP_Association_Type:
        {
            quint16 v = objectInfo->mtpAssociationType;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Parent_Obj:
        {
            quint32 v = objectInfo->mtpParentObject;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Obj_Size:
        {
            quint64 v = objectInfo->mtpObjectCompressedSize;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_StorageID:
        {
            quint32 v = objectInfo->mtpStorageId;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Obj_Format:
        {
            quint16 v = objectInfo->mtpObjectFormat;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Protection_Status:
        {
            quint16 v = objectInfo->mtpProtectionStatus;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Allowed_Folder_Contents:
        {
            // Not supported, return empty array
            QVector<qint16> v;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Date_Modified:
        {
            QString v = objectInfo->mtpModificationDate;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Date_Created:
        {
            QString v = objectInfo->mtpCaptureDate;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Date_Added:
        {
            QString v = objectInfo->mtpCaptureDate;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Obj_File_Name:
        {
            QString v = objectInfo->mtpFileName;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Rep_Sample_Format:
        {
            quint16 v = MTP_OBF_FORMAT_JFIF;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Rep_Sample_Size:
        {
            quint32 v = THUMB_MAX_SIZE;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Rep_Sample_Height:
        {
            value = QVariant::fromValue(THUMB_HEIGHT);
        }
        break;
        case MTP_OBJ_PROP_Rep_Sample_Width:
        {
            value = QVariant::fromValue(THUMB_WIDTH);
        }
        break;
        case MTP_OBJ_PROP_Video_FourCC_Codec:
        {
            quint32 v = fourcc_wmv3;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Corrupt_Unplayable:
        case MTP_OBJ_PROP_Hidden:
        {
            quint8 v = 0;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Persistent_Unique_ObjId:
        {
            StorageItem *storageItem = m_objectHandlesMap.value( handle );
            value = QVariant::fromValue(storageItem->m_puoid);
        }
        break;
        case MTP_OBJ_PROP_Non_Consumable:
        {
            quint8 v = 0;
            value = QVariant::fromValue(v);
        }
        break;
        case MTP_OBJ_PROP_Rep_Sample_Data:
        {
            StorageItem *storageItem = m_objectHandlesMap.value( handle );
            QString thumbPath = m_thumbnailer->requestThumbnail(storageItem->m_path, m_imageMimeTable.value(objectInfo->mtpObjectFormat));
            value = QVariant::fromValue(QVector<quint8>());
            if(false == thumbPath.isEmpty())
            {
                QFile thumbFile(thumbPath);
                if(thumbFile.open(QIODevice::ReadOnly))
                {
                    QVector<quint8> fileData(thumbFile.size());
                    // Read file data into the vector
                    // FIXME: Assumes that the entire file will be read at once
                    thumbFile.read(reinterpret_cast<char*>(fileData.data()), thumbFile.size());
                    value = QVariant::fromValue(fileData);
                }
            }
        }
        break;
        default:
            code = MTP_RESP_ObjectProp_Not_Supported;
            break;
    }
    return code;
}

MTPResponseCode FSStoragePlugin::getObjectPropertyValueFromTracker( const ObjHandle &handle,
                                                   MTPObjPropertyCode propCode,
                                                   QVariant &value, MTPDataType type )
{
    MTPResponseCode code = MTP_RESP_ObjectProp_Not_Supported;
    StorageItem *storageItem = m_objectHandlesMap.value( handle );
    if( !storageItem || storageItem->m_path.isEmpty() )
    {
        code = MTP_RESP_GeneralError;
    }
    else
    {
        code = m_tracker->getObjectProperty( storageItem->m_path, propCode, type, value ) ?
               MTP_RESP_OK : code;
    }
    return code;
}

MTPResponseCode FSStoragePlugin::getObjectPropertyValue( const ObjHandle &handle,
                                                         QList<MTPObjPropDescVal> &propValList,
                                                         bool getFromObjInfo, bool getDynamically )
{
    MTPResponseCode code = MTP_RESP_OK;
    StorageItem *storageItem = m_objectHandlesMap.value( handle );
    if( !storageItem || storageItem->m_path.isEmpty() )
    {
        code = MTP_RESP_GeneralError;
    }
    else
    {
        // First, we try to check if the object property has a value in the object info data set
        // or whether if it's statically defined ( hard-coded in other words ).

        if( getFromObjInfo )
        {
            for(QList<MTPObjPropDescVal>::iterator i = propValList.begin();
                    i != propValList.end(); ++i)
            {
                code = getObjectPropertyValueFromStorage( handle, i->propDesc->uPropCode, i->propVal, i->propDesc->uDataType );
            }
        }
        // Fetch whatever else remains from tracker
        if( getDynamically )
        {
            m_tracker->getPropVals(storageItem->m_path, propValList);
        }
    }
    return code;
}

MTPResponseCode FSStoragePlugin::setObjectPropertyValue( const ObjHandle &handle,
                                                         QList<MTPObjPropDescVal> &propValList,
                                                         bool sendObjectPropList /*=false*/ )
{
    MTPResponseCode code = MTP_RESP_OK;
    StorageItem *storageItem = m_objectHandlesMap.value( handle );
    if( !storageItem )
    {
        return MTP_RESP_GeneralError;
    }
    for(QList<MTPObjPropDescVal>::iterator i = propValList.begin(); i != propValList.end(); ++i)
    {
        const MtpObjPropDesc *propDesc = i->propDesc;
        QVariant &value = i->propVal;
        // Handle filename on our own
        if( MTP_OBJ_PROP_Obj_File_Name == propDesc->uPropCode )
        {
            QDir dir;
            QString path = storageItem->m_path;
            path.truncate( path.lastIndexOf("/") + 1 );
            QString newName = QString( value.value<QString>() );
            // Check if the file name is valid
            if(false == isFileNameValid(newName, storageItem->m_parent))
            {
                // Bad file name
                MTP_LOG_WARNING("Bad file name in setObjectProperty!" << newName);
                return MTP_RESP_Invalid_ObjectProp_Value;
            }
            path += newName;
            if( dir.rename( storageItem->m_path, path ) )
            {
                m_pathNamesMap.remove(storageItem->m_path);
                m_puoidsMap.remove(storageItem->m_path);
                // Adjust path in tracker
                m_tracker->move(storageItem->m_path, path);

                if( MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist == storageItem->m_objectInfo->mtpObjectFormat )
                {
                    // If this is a playlist, also need to update the playlist URL
                    m_tracker->movePlaylist(storageItem->m_path, path);
                }

                storageItem->m_path = path;
                storageItem->m_objectInfo->mtpFileName = newName;
                m_pathNamesMap[storageItem->m_path] = handle;
                m_puoidsMap[storageItem->m_path] = storageItem->m_puoid;
                removeWatchDescriptorRecursively( storageItem );
                addWatchDescriptorRecursively( storageItem );
                StorageItem *itr = storageItem->m_firstChild;
                while( itr )
                {
                    adjustMovedItemsPath( path, itr, true );
                    itr = itr->m_nextSibling;
                }
                code = MTP_RESP_OK;
            }
        }
        else if((false == sendObjectPropList) && (false == storageItem->m_path.isEmpty()))
        {
            // go to tracker
            if( !storageItem->m_path.isEmpty() )
            {
                code = m_tracker->setObjectProperty( storageItem->m_path, propDesc->uPropCode, propDesc->uDataType, value ) ?
                    MTP_RESP_OK : code;
            }
        }
    }
    if(true == sendObjectPropList)
    {
        m_tracker->setPropVals(storageItem->m_path, propValList);
#if 0
        // Ask tracker to ignore the current file, this is because we already have
        // all required metadata from the initiator.
        m_tracker->ignoreNextUpdate(QStringList(m_tracker->generateIri(storageItem->m_path)));
#endif
    }
    return code;
}

void FSStoragePlugin::receiveThumbnail(const QString &path)
{
    // Thumbnail for the file "path" is ready
    ObjHandle handle = m_pathNamesMap.value(path);
    if(0 != handle)
    {
        QVector<quint32> params;
        params.append(handle);
        params.append(MTP_OBJ_PROP_Rep_Sample_Data);
        emit eventGenerated(MTP_EV_ObjectPropChanged, params, QString());
    }
}

// TODO: Implement the below
void FSStoragePlugin::handleFSDelete(const struct inotify_event *event, const char* name)
{
    if(event->mask & (IN_DELETE | IN_MOVED_FROM))
    {
        MTP_LOG_INFO("Handle FS Delete::" << name);
        // Search for the object by the event's WD
        if(m_watchDescriptorMap.contains(event->wd))
        {
            ObjHandle parentHandle = m_watchDescriptorMap[event->wd];
            StorageItem *parentNode = m_objectHandlesMap[parentHandle];

            if(0 != parentNode)
            {
                QString fullPath = parentNode->m_path + QString("/") + QString(name);
                if(m_pathNamesMap.contains(fullPath))
                {
                    MTP_LOG_INFO("Handle FS Delete, deleting file::" << name);
                    ObjHandle toBeDeleted = m_pathNamesMap[fullPath];
                    deleteItemHelper( toBeDeleted, false, true );
                }
                // Emit storageinfo changed events, free space may be different from before now
                QVector<quint32> params;
                params.append(m_storageId);
                emit eventGenerated(MTP_EV_StorageInfoChanged, params, QString() );
            }
        }
    }
}

void FSStoragePlugin::handleFSCreate(const struct inotify_event *event, const char* name)
{
    if(event->mask & (IN_CREATE | IN_MOVED_TO))
    {
        ObjHandle parent = m_watchDescriptorMap.value(event->wd);
        StorageItem *parentNode = m_objectHandlesMap[parent];
        MTP_LOG_INFO("Handle FS Create::" << name);

        // The above QHash::value() may return a default constructed value of 0... so we double check the wd's here
        if(parentNode && (parentNode->m_wd == event->wd))
        {
            QString addedPath = parentNode->m_path + QString("/") + QString(name);
            // Already handled?
            ObjHandle addedNode = m_pathNamesMap.value(parentNode->m_path + QString("/") + QString(name));
            // Root node should never be returned by the above call, so the check below is safe
            if(0 == addedNode)
            {
                StorageItem *addedNode = new StorageItem;
                addedNode->m_path = addedPath;
                linkChildStorageItem( addedNode, parentNode );
                populateObjectInfo(addedNode);
                MTP_LOG_INFO("Handle FS create, adding file::" << name);
                // We must recurse into new directories (They may appear fully populated)
                if (event->mask & IN_ISDIR)
                {
                    addDirToStorage(addedNode, false, true);
                }
                else
                {
                    addFileToStorage(addedNode, true, false);
                }
                // Emit storageinfo changed events, free space may be different from before now
                QVector<quint32> params;
                params.append(m_storageId);
                emit eventGenerated(MTP_EV_StorageInfoChanged, params, QString() );
            }
        }
    }
}

void FSStoragePlugin::handleFSMove(const struct inotify_event *fromEvent, const char* fromName,
        const struct inotify_event *toEvent, const char* toName)
{
    if((fromEvent->mask & IN_MOVED_FROM) && (toEvent->mask & IN_MOVED_TO) && (fromEvent->cookie == toEvent->cookie))
    {
        ObjHandle fromHandle = m_watchDescriptorMap.value(fromEvent->wd);
        ObjHandle toHandle = m_watchDescriptorMap.value(toEvent->wd);
        StorageItem *fromNode = m_objectHandlesMap.value(fromHandle);
        StorageItem *toNode = m_objectHandlesMap.value(toHandle);

        MTP_LOG_INFO("Handle FS Move::" << fromName << toName);
        if((fromHandle == toHandle) && (0 == qstrcmp(fromName, toName)))
        {
            // No change!
            return;
        }
        if((0 != fromNode) && (0 != toNode) && (fromNode->m_wd == fromEvent->wd) && (toNode->m_wd == toEvent->wd))
        {
            MTP_LOG_INFO("Handle FS Move, moving file::" << fromName << toName);
            QString oldPath = fromNode->m_path + QString("/") + QString(fromName);
            ObjHandle movedHandle = m_pathNamesMap.value(oldPath);

            if(0 == movedHandle)
            {
                // Already handled
                return;
            }
            StorageItem *movedNode = m_objectHandlesMap.value(movedHandle);
            if(movedNode)
            {
                QString newPath = toNode->m_path + QString("/") + toName;
                if( m_pathNamesMap.contains( newPath ) ) // Already Handled
                {
                    // As the destination path is already present in our tree,
                    // we only need to delete the fromNode
                    MTP_LOG_INFO("The path to rename to is already present in our tree, hence, delete the moved node from our tree");
                    deleteItemHelper( m_pathNamesMap[oldPath], false, true );
                    return;
                }
                MTP_LOG_INFO("Handle FS Move, moving file, found!");
                if( fromHandle == toHandle ) // Rename
                {
                    MTP_LOG_INFO("Handle FS Move, renaming file::" << fromName << toName);
                    // Remove the old path from the path names map
                    m_pathNamesMap.remove(oldPath);
                    movedNode->m_path = newPath;
                    movedNode->m_objectInfo->mtpFileName = QString(toName);
                    m_pathNamesMap[movedNode->m_path] = movedHandle;
                    StorageItem *itr = movedNode->m_firstChild;
                    while( itr )
                    {
                        adjustMovedItemsPath( movedNode->m_path, itr );
                        itr = itr->m_nextSibling;
                    }
                    removeWatchDescriptorRecursively( movedNode );
                    addWatchDescriptorRecursively( movedNode );
                }
                else
                {
                    moveObject( movedHandle, toHandle, m_storageId, false );
                }

                // object info would need to be computed again
                delete movedNode->m_objectInfo;
                movedNode->m_objectInfo = 0;
                populateObjectInfo( movedNode );

                // Emit an object info changed signal
                QVector<quint32> evtParams;
                evtParams.append(movedHandle);
                emit eventGenerated(MTP_EV_ObjectInfoChanged, evtParams, QString());
            }
        }
    }
}

void FSStoragePlugin::handleFSModify(const struct inotify_event *event, const char* name)
{
    if(event->mask & IN_CLOSE_WRITE)
    {
        ObjHandle parent = m_watchDescriptorMap.value(event->wd);
        StorageItem *parentNode = m_objectHandlesMap.value(parent);
        //MTP_LOG_INFO("Handle FS Modify::" << name);
        if(parentNode && (parentNode->m_wd == event->wd))
        {
            QString changedPath = parentNode->m_path + QString("/") + QString(name);
            ObjHandle changedHandle = m_pathNamesMap.value(changedPath);
            // Don't fire the change signal in the case when there is a transfer to the device ongoing
            if ((0 != changedHandle) && (changedHandle != m_writeObjectHandle))
            {
                MTP_LOG_INFO("Handle FS Modify, file::" << name);
                StorageItem *item = m_objectHandlesMap.value(changedHandle);
                // object info would need to be computed again
                delete item->m_objectInfo;
                item->m_objectInfo = 0;
                populateObjectInfo( item );

                // Emit an object info changed event
                QVector<quint32> eventParams;
                eventParams.append(changedHandle);
                emit eventGenerated(MTP_EV_ObjectInfoChanged, eventParams, QString() );

                static quint64 freeSpace = m_storageInfo.freeSpace;
                MTPStorageInfo info;
                storageInfo( info );
                qint64 diff = freeSpace - info.freeSpace;
                if( diff < 0 ) diff *= -1;
                // Emit storageinfo changed event, if free space changes by 1% or more
                if( freeSpace && (((quint64)diff*100)/freeSpace) >= 1 )
                {
                    freeSpace = m_storageInfo.freeSpace;
                    eventParams.clear();
                    eventParams.append(m_storageId);
                    emit eventGenerated(MTP_EV_StorageInfoChanged, eventParams, QString() );
                }
            }
        }
    }
}

void FSStoragePlugin::cacheInotifyEvent(const struct inotify_event *event, const char* name)
{
    m_iNotifyCache.fromEvent = *event;
    m_iNotifyCache.fromName = QString(name);
}

void FSStoragePlugin::getCachedInotifyEvent(const struct inotify_event **event, QString &name)
{
    if(0 != m_iNotifyCache.fromEvent.cookie)
    {
        *event = &m_iNotifyCache.fromEvent;
        name = m_iNotifyCache.fromName;
    }
    else
    {
        *event = 0;
        name = "";
    }
}

void FSStoragePlugin::clearCachedInotifyEvent()
{
    m_iNotifyCache.fromName.clear();
    memset(&m_iNotifyCache.fromEvent, 0, sizeof(m_iNotifyCache.fromEvent));
}

void FSStoragePlugin::removeWatchDescriptorRecursively( StorageItem* item )
{
    StorageItem *itr;
    if( item && item->m_objectInfo && MTP_OBF_FORMAT_Association == item->m_objectInfo->mtpObjectFormat )
    {
        removeWatchDescriptor( item );
        for( itr = item->m_firstChild; itr; itr = itr->m_nextSibling )
        {
            removeWatchDescriptorRecursively( itr );
        }
    }
}

void FSStoragePlugin::removeWatchDescriptor( StorageItem* item )
{
    if( item && item->m_objectInfo && MTP_OBF_FORMAT_Association == item->m_objectInfo->mtpObjectFormat )
    {
        m_inotify->removeWatch( item->m_wd );
        m_watchDescriptorMap.remove( item->m_wd );
    }
}

void FSStoragePlugin::addWatchDescriptorRecursively( StorageItem* item )
{
    StorageItem *itr;
    if( item && item->m_objectInfo && MTP_OBF_FORMAT_Association == item->m_objectInfo->mtpObjectFormat )
    {
        addWatchDescriptor( item );
        for( itr = item->m_firstChild; itr; itr = itr->m_nextSibling )
        {
            addWatchDescriptorRecursively( itr );
        }
    }
}

void FSStoragePlugin::addWatchDescriptor( StorageItem* item )
{
    if( item && item->m_objectInfo && MTP_OBF_FORMAT_Association == item->m_objectInfo->mtpObjectFormat )
    {
        item->m_wd = m_inotify->addWatch( item->m_path );
        if( -1 != item->m_wd )
        {
            m_watchDescriptorMap[ item->m_wd ] = item->m_handle;
        }
    }
}

bool FSStoragePlugin::isFileNameValid(const QString &fileName, const StorageItem *parent)
{
    // Check if the file name contains illegal characters, or if the file with
    // the same name is already present under the parent
    if(fileName.contains(QRegExp(FILENAMES_FILTER_REGEX)) ||
            QRegExp("[\\.]+").exactMatch(fileName))
    {
        // Illegal characters, or all .'s
        return false;
    }
    if(m_pathNamesMap.contains(parent->m_path + "/" + fileName))
    {
        // Already present
        return false;
    }
    return true;
}
