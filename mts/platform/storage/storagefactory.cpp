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

#include <dlfcn.h>
#include "storagefactory.h"
#include <QDir>
#include "storageplugin.h"
#include "mtpevent.h"
#include "trace.h"

using namespace meegomtp1dot0;

/*******************************************************
 * StorageFactory::StorageFactory
 ******************************************************/
StorageFactory::StorageFactory() :  m_storageId(0), m_storagePluginsPath( pluginLocation ), m_newObjectHandle(0), m_newPuoid(0)
{
    //TODO For now handle only the file system storage plug-in. As we have more storages
    // make this generic.
#if 0
    void *pluginHandle;
    QDir dir( m_storagePluginsPath );
    dir.setFilter( QDir::Files | QDir::NoDotAndDotDot );
    QStringList dirContents = dir.entryList();
    for( quint32 i = 0; i < dirContents.size(); ++i )
    {
        QString pluginName = dirContents.at(i);
        if( !pluginName.endsWith( ".so" ) )
        {
            continue;
        }
        QString pluginFullPath = m_storagePluginsPath + "/" + pluginName;
        pluginHandle = dlopen( pluginFullPath.toStdString().c_str(), RTLD_NOW );
        if( !pluginHandle )
        {
            MTP_LOG_WARNING("Failed to dlopen::" << pluginFullPath << dlerror());
            continue;
        }
        CREATE_STORAGE_PLUGIN_FPTR createStoragePluginFptr = ( CREATE_STORAGE_PLUGIN_FPTR )dlsym( pluginHandle, CREATE_STORAGE_PLUGIN.toStdString().c_str() );
        if( char * error = dlerror() )
        {
            MTP_LOG_WARNING("Failed to dlsym because " << error);
            dlclose( pluginHandle );
            continue;
        }

        quint32 storageId = assignStorageId(i,1);
        StoragePlugin *storagePlugin = (*createStoragePluginFptr)( storageId );

        // Connect the storage plugin's eventGenerated signal
        QObject::connect(storagePlugin, SIGNAL(eventGenerated(MTPEventCode, const QVector<quint32>&, QString)), this, SLOT(eventReceived(MTPEventCode, const QVector<quint32>&, QString)));

        // Add this storage to all storages list.
        m_allStorages[storageId] = storagePlugin;

        PluginHandlesInfo_ pluginHandlesInfo;
        pluginHandlesInfo.storagePluginPtr = storagePlugin;
        pluginHandlesInfo.storagePluginHandle = pluginHandle;
        m_pluginHandlesInfoVector.append( pluginHandlesInfo );
    }
#endif

    void *pluginHandle = 0;
    QString pluginFullPath = m_storagePluginsPath + "/libfsstorage.so";
    QByteArray ba = pluginFullPath.toUtf8();

    pluginHandle = dlopen( ba.constData(), RTLD_NOW );
    if( !pluginHandle )
    {
        MTP_LOG_WARNING("Failed to dlopen::" << pluginFullPath << dlerror());
    }
    else
    {
        if( char *error = dlerror() )
        {
            MTP_LOG_WARNING("Failed to dlsym because " << error);
            dlclose( pluginHandle );
        }
        else
        {
            ba = CREATE_STORAGE_PLUGIN.toUtf8();
            CREATE_STORAGE_PLUGIN_FPTR createStoragePluginFptr =
            ( CREATE_STORAGE_PLUGIN_FPTR )dlsym( pluginHandle,
                                                 ba.constData() );
            if( dlerror() )
            {
                MTP_LOG_WARNING("Failed to dlsym because " << dlerror());
                dlclose( pluginHandle );
            }
            else
            {
                QList<QString> configFiles = (*storageConfigurationsFptr)();
                for( quint8 i = 0; i < configFiles.count(); ++i )
                {
                    quint32 storageId = assignStorageId(1, i + 1);
                    StoragePlugin *storagePlugin =
                            (*createStoragePluginFptr)( configFiles[i], storageId );
                    if (!storagePlugin) {
                        MTP_LOG_WARNING("Couldn't create StoragePlugin for id" << i);
                        continue;
                    }

                    // Add this storage to all storages list.
                    m_allStorages[storageId] = storagePlugin;

                    PluginHandlesInfo_ pluginHandlesInfo;
                    pluginHandlesInfo.storagePluginPtr = storagePlugin;
                    pluginHandlesInfo.storagePluginHandle = pluginHandle;
                    m_pluginHandlesInfoVector.append( pluginHandlesInfo );
                }
            }
        }
    }
}

/*******************************************************
 * StorageFactory::~StorageFactory
 ******************************************************/
StorageFactory::~StorageFactory()
{
    for( int i = 0; i < m_pluginHandlesInfoVector.count() ; ++i )
    {
        PluginHandlesInfo_ &pluginHandlesInfo = m_pluginHandlesInfoVector[i];
       
        DESTROY_STORAGE_PLUGIN_FPTR destroyStoragePluginFptr =
                ( DESTROY_STORAGE_PLUGIN_FPTR )dlsym( pluginHandlesInfo.storagePluginHandle,
                                                      DESTROY_STORAGE_PLUGIN.toUtf8().constData() );
        if( char *error = dlerror() )
        {
            MTP_LOG_WARNING( "Failed to destroy storage because" << error );
            continue;
        }

        (*destroyStoragePluginFptr)( pluginHandlesInfo.storagePluginPtr );
    }

    /* TODO: Make generic. We want to dlclose all plugins and each one only
     * once. So far it doesn't matter as we have only file system storage
     * plug-in. */
    dlclose( m_pluginHandlesInfoVector[0].storagePluginHandle );
}

/*******************************************************
 * bool StorageFactory::enumerateStorages
 ******************************************************/
bool StorageFactory::enumerateStorages( QVector<quint32>& failedStorageIds )
{
    //TODO For now handle only the file system storage plug-in. As we have more storages
    // make this generic.
    bool result = true;
    ObjHandle handle;

    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        // Connect the storage plugin's eventGenerated signal
        qRegisterMetaType<MTPEventCode>("MTPEventCode");
        qRegisterMetaType<QVector<quint32> >("QVector<quint32>");
        QObject::connect( itr.value(), SIGNAL(eventGenerated(MTPEventCode, const QVector<quint32>&, QString)),
                          this, SLOT(eventReceived(MTPEventCode, const QVector<quint32>&, QString)), Qt::QueuedConnection );

        // Connects for assigning object handles
        QObject::connect( itr.value(), SIGNAL(objectHandle( ObjHandle& )),
                          this, SLOT(getObjectHandle( ObjHandle& )) );
        QObject::connect( this, SIGNAL(largestObjectHandle( ObjHandle& )),
                          itr.value(), SLOT(getLargestObjectHandle( ObjHandle& )) );

        // Connect for puoids
        QObject::connect( itr.value(), SIGNAL(puoid( MtpInt128& )),
                          this, SLOT(getPuoid( MtpInt128& )) );
        QObject::connect( this, SIGNAL(largestPuoid( MtpInt128& )),
                          itr.value(), SLOT(getLargestPuoid( MtpInt128& )) );

        // Connect for transport events.
        QObject::connect( itr.value(), SIGNAL(checkTransportEvents( bool& )),
                          this, SIGNAL(checkTransportEvents( bool& )) );

        // This needs to be done before enumerating the storage plug-in, so that the storage factory
        // knows the largest object handle used while assigning new object handles to the storage plug-in
        // during enumeration.
        emit largestObjectHandle( handle );
        if( handle > m_newObjectHandle )
        {
            m_newObjectHandle = handle;
        }

        MtpInt128 puoid;
        emit largestPuoid( puoid );
        if( 0 < puoid.compare(m_newPuoid) )
        {
            m_newPuoid = puoid;
        }

        QObject::disconnect( this, SIGNAL(largestObjectHandle( ObjHandle & )), 0, 0 );
        QObject::disconnect( this, SIGNAL(largestPuoid( MtpInt128& )), 0, 0 );

        if( !itr.value()->enumerateStorage() )
        {
             result = false;
             failedStorageIds.append(itr.key());
        }
    }

    return result;
}

/*******************************************************
 * quint32 StorageFactory::assignStorageId
 ******************************************************/
quint32 StorageFactory::assignStorageId( quint16 storageNo, quint16 partitionNo ) const
{
    quint32 storageId = storageNo;
    storageId = ( storageId << 16 ) | partitionNo;
    return storageId;
}

/*******************************************************
 * MTPResponseCode StorageFactory::addItem
 ******************************************************/
MTPResponseCode StorageFactory::addItem( quint32& storageId, ObjHandle &parentHandle, ObjHandle &handle, MTPObjectInfo *info ) const
{
    MTPResponseCode response = MTP_RESP_GeneralError;
    // Add the item in the right storage.
    // Responder can choose the storage in this case
    if( 0x00000000 == info->mtpStorageId )
    {
        // Choose the first storage as of now.
        // FIXME Not a good idea to just pick the first storage. Maybe we should choose one has enough available space, etc.
        storageId = assignStorageId(1,1);
    }
    StoragePlugin *storagePlugin =  m_allStorages.value(storageId);
    if( storagePlugin )
    {
        response = storagePlugin->addItem( parentHandle, handle, info );
    }
    return response;
}

/*******************************************************
 * MTPResponseCode StorageFactory::deleteItem
 ******************************************************/
MTPResponseCode StorageFactory::deleteItem( const ObjHandle& handle, const MTPObjFormatCode& formatCode ) const
{
    MTPResponseCode response = MTP_RESP_GeneralError;
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    // a handle of 0xFFFFFFFF means delete everthing in all storages.
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( 0xFFFFFFFF == handle || itr.value()->checkHandle( handle ) )
        {
            response =  itr.value()->deleteItem( handle, formatCode );
            if( 0xFFFFFFFF != handle )
            {
                break;
            }
        }
    }
    return response;
}

/*******************************************************
 * MTPResponseCode StorageFactory::getObjectHandles
 ******************************************************/
MTPResponseCode StorageFactory::getObjectHandles( const quint32& storageId, const MTPObjFormatCode& formatCode, const quint32& associationHandle,
                                                  QVector<ObjHandle> &objectHandles ) const
{
    MTPResponseCode response = MTP_RESP_InvalidStorageID;
    // Get the total count across all storages.
    if( 0xFFFFFFFF == storageId )
    {
        QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
        for( ; itr != m_allStorages.constEnd(); ++itr )
        {
            QVector<ObjHandle> handles;
            response = itr.value()->getObjectHandles( formatCode, associationHandle, handles );
            if( MTP_RESP_OK != response )
            {
                break;
            }
            objectHandles += handles;
        }
    }
    else
    {
        //Get the no. of objects from the requested storage.
        StoragePlugin *storagePlugin =  m_allStorages.value(storageId);
        if( storagePlugin )
        {
            response = storagePlugin->getObjectHandles( formatCode, associationHandle, objectHandles );
        }
    }

    //FIXME The storage id might be valid, but the store may now be unavailable, if this is the case return a STORE_NOT_AVAILABLE response.
    return response;
}

/*******************************************************
 * static MTPResponseCode StorageFactory::storageIds
 ******************************************************/
MTPResponseCode StorageFactory::storageIds( QVector<quint32> &storageIds )
{
    // Return a list of id's of all created storages.
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        storageIds.append(itr.key());
    }
    return MTP_RESP_OK;
}

/*******************************************************
 * MTPResponseCode StorageFactory::checkStorage
 ******************************************************/
MTPResponseCode StorageFactory::checkStorage( quint32 storageId ) const
{
    // Check if the given storage is created
    return m_allStorages.contains( storageId ) ? MTP_RESP_OK : MTP_RESP_InvalidStorageID; 
}

/*******************************************************
 * MTPResponseCode StorageFactory::checkHandle
 ******************************************************/
MTPResponseCode StorageFactory::checkHandle( const ObjHandle &handle ) const
{
    //Search for the handle in the storages.
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return MTP_RESP_OK;
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

/*******************************************************
 * MTPResponseCode StorageFactory::storageType
 ******************************************************/
MTPResponseCode StorageFactory::storageInfo( const quint32& storageId, MTPStorageInfo &info )
{
    StoragePlugin *storagePlugin =  m_allStorages.value(storageId);
    if( storagePlugin )
    {
        return storagePlugin->storageInfo( info );
    }
    return MTP_RESP_InvalidStorageID;
}

/*******************************************************
 * MTPResponseCode StorageFactory::getReferences
 ******************************************************/
MTPResponseCode StorageFactory::getReferences( const ObjHandle &handle , QVector<ObjHandle> &references ) const
{
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return itr.value()->getReferences( handle , references );
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

/*******************************************************
 * MTPResponseCode StorageFactory::setReferences
 ******************************************************/
MTPResponseCode StorageFactory::setReferences( const ObjHandle &handle , const QVector<ObjHandle> &references ) const
{
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return itr.value()->setReferences( handle , references );
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

/*******************************************************
 * MTPResponseCode StorageFactory::copyObject
 ******************************************************/
MTPResponseCode StorageFactory::copyObject( const ObjHandle &handle, const ObjHandle &parentHandle, const quint32 &destinationStorageId, ObjHandle &copiedObjectHandle ) const
{
    MTPResponseCode response;

    if( !m_allStorages.contains( destinationStorageId ) )
    {
        return MTP_RESP_InvalidStorageID;
    }

    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            response = itr.value()->copyObject( handle, parentHandle,
                    m_allStorages[destinationStorageId], copiedObjectHandle );
            if( MTP_RESP_StoreFull == response )
            {
                //Cleanup
                response = deleteItem( copiedObjectHandle, MTP_OBF_FORMAT_Undefined );
                return MTP_RESP_StoreFull;
            }
            return response;

        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

/*******************************************************
 * MTPResponseCode StorageFactory::moveObject
 ******************************************************/
MTPResponseCode StorageFactory::moveObject( const ObjHandle &handle, const ObjHandle &parentHandle, const quint32 &destinationStorageId ) const
{
    if( !m_allStorages.contains( destinationStorageId ) )
    {
        return MTP_RESP_InvalidStorageID;
    }

    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return itr.value()->moveObject( handle, parentHandle,
                    m_allStorages[destinationStorageId] );
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

/*******************************************************
 * MTPResponseCode StorageFactory::getPath
 ******************************************************/
MTPResponseCode StorageFactory::getPath( const quint32 &handle, QString &path ) const
{
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return itr.value()->getPath( handle, path );
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

/*******************************************************
 * MTPResponseCode StorageFactory::getObjectInfo
 ******************************************************/
MTPResponseCode StorageFactory::getObjectInfo( const ObjHandle &handle, const MTPObjectInfo *&objectInfo ) const
{
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return itr.value()->getObjectInfo( handle, objectInfo );
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

/*******************************************************
 * MTPResponseCode StorageFactory::writeData
 ******************************************************/
MTPResponseCode StorageFactory::writeData( const ObjHandle &handle, char *writeBuffer, quint32 bufferLen, bool isFirstSegment, bool isLastSegment ) const
{
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return itr.value()->writeData( handle, writeBuffer, bufferLen, isFirstSegment, isLastSegment );
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

/*******************************************************
 * MTPResponseCode StorageFactory::truncateItem
 ******************************************************/
MTPResponseCode StorageFactory::truncateItem( const ObjHandle &handle, const quint32& size ) const
{
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return itr.value()->truncateItem( handle, size );
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

/*******************************************************
 * MTPResponseCode StorageFactory::readData
 ******************************************************/
MTPResponseCode StorageFactory::readData( const ObjHandle &handle, char *readBuffer, qint32 &readBufferLen, quint32 readOffset ) const
{
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return itr.value()->readData( handle, readBuffer, readBufferLen, readOffset );
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

MTPResponseCode StorageFactory::getObjectPropertyValue( const ObjHandle &handle,
                                                        QList<MTPObjPropDescVal> &propValList,
                                                        bool getFromObjInfo, bool getDynamically )
{
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return itr.value()->getObjectPropertyValue( handle, propValList, getFromObjInfo, getDynamically );
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

MTPResponseCode StorageFactory::setObjectPropertyValue( const ObjHandle &handle,
                                                        QList<MTPObjPropDescVal> &propValList,
                                                        bool sendObjectPropList /*= false*/)
{
    QHash<quint32,StoragePlugin*>::const_iterator itr = m_allStorages.constBegin();
    for( ; itr != m_allStorages.constEnd(); ++itr )
    {
        if( itr.value()->checkHandle( handle ) )
        {
            return itr.value()->setObjectPropertyValue( handle, propValList, sendObjectPropList );
        }
    }
    return MTP_RESP_InvalidObjectHandle;
}

void StorageFactory::eventReceived(MTPEventCode eventCode, const QVector<quint32> &params, QString filePath)
{
    // Try to get an already exiting event object
    MTPEvent *event = MTPEvent::transMap.value(filePath);
    if(0 != event)
    {
        event->setEventParams(params);
        event->setEventCode(eventCode);
    }
    else
    {
        event = new MTPEvent(eventCode, MTP_NO_SESSION_ID, MTP_NO_TRANSACTION_ID, params);
    }
    event->dispatchEvent();
    delete event;
}

void StorageFactory::getObjectHandle( ObjHandle& handle )
{
    //TODO : Handle the case when object handle surpasses 0xFFFFFFFF,
    //but the no. of objects doesn't. 
    handle =  ++m_newObjectHandle == 0xFFFFFFFF ? 1 : m_newObjectHandle;
}

void StorageFactory::getPuoid( MtpInt128& puoid )
{
    puoid = ++m_newPuoid;
}
