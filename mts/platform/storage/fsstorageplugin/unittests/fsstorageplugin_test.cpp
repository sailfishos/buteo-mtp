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

#include "fsstorageplugin_test.h"
#include "fsstorageplugin.h"
#include "storageitem.h"
#include "storagetracker.h"
#include <QtTracker/Tracker>

using namespace meegomtp1dot0;

int totalCount = 17;

void FSStoragePlugin_test::initTestCase()
{
    QDir dir;
    QFile file( QDir::homePath() + "/.mtp/.mtphandles");
    if( file.open( QIODevice::ReadOnly ) )
    {
        m_storage = new FSStoragePlugin( 1, MTP_STORAGE_TYPE_FixedRAM, "/tmp/mtptests", "media", "Phone Memory");
        dir = QDir( m_storage->m_storagePath );
        if( !dir.exists( "Playlists" ) )
        {
            dir.mkdir( "Playlists" );
        }
        m_storage->enumerateStorage();
        QVERIFY( m_storage->m_root != 0 );
        QCOMPARE( m_storage->m_root->m_handle, static_cast<unsigned int>(0) );
        QCOMPARE( m_storage->m_objectHandlesMap.size(), m_storage->m_pathNamesMap.size() );
        QCOMPARE( m_storage->m_objectHandlesMap.size(), 12 );

        QVector<ObjHandle> references;
        MTPResponseCode response;
        quint32 handle = m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/fileA"];
        response = m_storage->getReferences( handle, references );
        QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
        QCOMPARE( references.size(), 2 );
        QCOMPARE( references[0], static_cast<unsigned int>(15) );
        QCOMPARE( references[1], static_cast<unsigned int>(16) );

        delete m_storage;

        QDir dir("/tmp/mtptests");
        dir.remove(".mtphandles");
        dir.remove("subdir2/fileA");
        dir.remove("subdir2/fileB");
        dir.remove("subdir2/fileC");
        dir.remove("subdir2/file1");
        dir.remove("subdir2/file2");
        dir.remove("subdir2/file3");
        dir.remove("subdir2/D1/D2/f1");
        dir.rmdir("subdir2/D1/D2");
        dir.rmdir("subdir2/D1");
        dir.rmdir("subdir2");
        dir = QDir("/tmp/Music");
        dir.remove("song1");
        dir.remove("song2");
        dir.remove("song3");
        dir.remove("song4");
        dir = QDir("/tmp");
        dir.rmdir("Music");
        dir.rmdir("mtptests");
    }
    // 1 root dir, 2 sub dirs, 1 nested, each dir has 3 files.
    // create dirs.
    dir = QDir("/tmp");
    dir.mkdir("mtptests");
    dir = QDir("/tmp/mtptests");
    dir.mkdir("subdir1");
    dir.mkdir("subdir2");
    dir.mkdir("Music");
    dir = QDir("/tmp/mtptests/subdir1");
    dir.mkdir("subdir3");

    // create files
    QFile file1("/tmp/mtptests/file1");
    file1.open(QIODevice::WriteOnly);
    file1.write("a",1);
    file1.close();
    QFile file2("/tmp/mtptests/file2");
    file2.open(QIODevice::WriteOnly);
    file2.write("aaaaaa",6);
    file2.close();
    QFile file3("/tmp/mtptests/file3");
    file3.open(QIODevice::WriteOnly);
    file3.write("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",100);
    file3.close();

    QFile file4("/tmp/mtptests/subdir1/file1");
    file4.open(QIODevice::WriteOnly);
    file4.write("a",1);
    file4.close();
    QFile file5("/tmp/mtptests/subdir1/file2");
    file5.open(QIODevice::WriteOnly);
    file5.write("aaaaaa",6);
    file5.close();
    QFile file6("/tmp/mtptests/subdir1/file3");
    file6.open(QIODevice::WriteOnly);
    file6.write("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",100);
    file6.close();

    QFile file7("/tmp/mtptests/subdir2/fileA");
    file7.open(QIODevice::WriteOnly);
    file7.write("a",1);
    file7.close();
    QFile file8("/tmp/mtptests/subdir2/fileB");
    file8.open(QIODevice::WriteOnly);
    file8.write("aaaaaa",6);
    file8.close();
    QFile file9("/tmp/mtptests/subdir2/fileC");
    file9.open(QIODevice::WriteOnly);
    file9.write("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",100);
    file9.close();

    QFile file10("/tmp/mtptests/subdir1/subdir3/file1");
    file10.open(QIODevice::WriteOnly);
    file10.write("a",1);
    file10.close();
    QFile file11("/tmp/mtptests/subdir1/subdir3/file2");
    file11.open(QIODevice::WriteOnly);
    file11.write("aaaaaa",6);
    file11.close();
    QFile file12("/tmp/mtptests/subdir1/subdir3/file3");
    file12.open(QIODevice::WriteOnly);
    file12.write("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",100);
    file12.close();

    QFile song1("/tmp/mtptests/Music/song1.mp3");
    song1.open(QIODevice::WriteOnly);
    song1.close();

    QFile song2("/tmp/mtptests/Music/song2.mp3");
    song2.open(QIODevice::WriteOnly);
    song2.close();

    QFile song3("/tmp/mtptests/Music/song3.mp3");
    song3.open(QIODevice::WriteOnly);
    song3.close();

    QFile song4("/tmp/mtptests/Music/song4.mp3");
    song4.open(QIODevice::WriteOnly);
    song4.close();

    sleep(5);

    // Create tracker playlist1 with song1 and song2
    QString query;

    query = "INSERT{<file:///tmp/mtptests/Music/song1.mp3> a nfo:FileDataObject, nie:InformationElement ; nie:url 'file:///tmp/mtptests/Music/song1.mp3'}";
    ::tracker()->rawSparqlUpdateQuery(query);
    
    query = "INSERT{<file:///tmp/mtptests/Music/song2.mp3> a nfo:FileDataObject, nie:InformationElement ; nie:url 'file:///tmp/mtptests/Music/song2.mp3'}";
    ::tracker()->rawSparqlUpdateQuery(query);
    
    query = "INSERT{<file:///tmp/mtptests/Music/song3.mp3> a nfo:FileDataObject, nie:InformationElement ; nie:url 'file:///tmp/mtptests/Music/song3.mp3'}";
    ::tracker()->rawSparqlUpdateQuery(query);
    
    query = "INSERT{<file:///tmp/mtptests/Music/song4.mp3> a nfo:FileDataObject, nie:InformationElement ; nie:url 'file:///tmp/mtptests/Music/song4.mp3'}";
    ::tracker()->rawSparqlUpdateQuery(query);
    
    query = "DELETE { ?entry a rdfs:Resource } WHERE { <urn:playlist:pl1> nfo:hasMediaFileListEntry ?entry . }";

    ::tracker()->rawSparqlUpdateQuery(query);

    query = "DELETE {<urn:playlist:pl1> a rdfs:Resource}";
    
    ::tracker()->rawSparqlUpdateQuery(query);

    query = "INSERT {<urn:playlist:pl1> a nmm:Playlist; nie:title 'play1'}";
    
    ::tracker()->rawSparqlUpdateQuery(query);

    query = "INSERT {<urn:playlist:pl1> nfo:hasMediaFileListEntry <urn:playlist-entry:pl1:0> . <urn:playlist-entry:pl1:0> a nfo:MediaFileListEntry . <urn:playlist-entry:pl1:0> nfo:entryContent <file:///tmp/mtptests/Music/song1.mp3> . <urn:playlist-entry:pl1:0> nfo:listPosition '0' . <urn:playlist:pl1> nfo:hasMediaFileListEntry <urn:playlist-entry:pl1:1> . <urn:playlist-entry:pl1:1> a nfo:MediaFileListEntry . <urn:playlist-entry:pl1:1> nfo:entryContent <file:///tmp/mtptests/Music/song2.mp3> . <urn:playlist-entry:pl1:1> nfo:listPosition '1' .}";
    
    ::tracker()->rawSparqlUpdateQuery(query);

    // Create tracker playlist2 with song3 and song4
    query = "DELETE { ?entry a rdfs:Resource } WHERE { <urn:playlist:pl2> nfo:hasMediaFileListEntry ?entry . }";

    ::tracker()->rawSparqlUpdateQuery(query);

    query = "DELETE {<urn:playlist:pl2> a rdfs:Resource}";
    
    ::tracker()->rawSparqlUpdateQuery(query);

    query = "INSERT {<urn:playlist:pl2> a nmm:Playlist; nie:title 'play2'}";
    
    ::tracker()->rawSparqlUpdateQuery(query);

    query = "INSERT {<urn:playlist:pl2> nfo:hasMediaFileListEntry <urn:playlist-entry:pl2:0> . <urn:playlist-entry:pl2:0> a nfo:MediaFileListEntry . <urn:playlist-entry:pl2:0> nfo:entryContent <file:///tmp/mtptests/Music/song3.mp3> . <urn:playlist-entry:pl2:0> nfo:listPosition '0' . <urn:playlist:pl2> nfo:hasMediaFileListEntry <urn:playlist-entry:pl2:1> . <urn:playlist-entry:pl2:1> a nfo:MediaFileListEntry . <urn:playlist-entry:pl2:1> nfo:entryContent <file:///tmp/mtptests/Music/song4.mp3> . <urn:playlist-entry:pl2:1> nfo:listPosition '1' .}";
    
    ::tracker()->rawSparqlUpdateQuery(query);

}

void  FSStoragePlugin_test::testStorageCreation()
{
    m_storage = new FSStoragePlugin( 1, MTP_STORAGE_TYPE_FixedRAM, "/tmp/mtptests", "media", "Phone Memory" );
    QDir dir( m_storage->m_storagePath );
    if( !dir.exists( "Playlists" ) )
    {
        dir.mkdir( "Playlists" );
    }
    m_storage->enumerateStorage();
    QVERIFY( m_storage->m_root != 0 );
    QCOMPARE( m_storage->m_root->m_handle, static_cast<unsigned int>(0) );
    QVERIFY( m_storage->m_root->m_parent == 0 );
    QVERIFY( m_storage->m_root->m_firstChild != 0 );
    QVERIFY( m_storage->m_root->m_nextSibling == 0 );
    QCOMPARE( m_storage->m_root->m_path, QString("/tmp/mtptests") );
    //m_storage->dumpStorageItem( m_storage->m_root, true );
}

void FSStoragePlugin_test::testDeleteAll()
{
    MTPResponseCode response = MTP_RESP_GeneralError;

    response = m_storage->deleteItem( 0xFFFFFFFF,  MTP_OBF_FORMAT_Undefined );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    QCOMPARE( m_storage->m_objectHandlesMap.size(), 1 );
    QCOMPARE( m_storage->m_objectHandlesMap.size(), m_storage->m_pathNamesMap.size() );

    delete m_storage;
    system("rm -rf ~/.mtp");
    system("rm -rf /tmp/mtptests");
    system("tracker-control -r");
    sleep(2);
    system("tracker-control -s");
    sleep(2);
    initTestCase();
    testStorageCreation();
}
void FSStoragePlugin_test::testObjectHandlesCountAfterCreation()
{
    QCOMPARE( m_storage->m_objectHandlesMap.size(), m_storage->m_pathNamesMap.size() );
    //QCOMPARE( m_storage->m_objectHandlesMap.size(), totalCount );
    totalCount = m_storage->m_objectHandlesMap.size();
    quint32 noOfObjects;
    QVector<ObjHandle> objectHandles;

    MTPResponseCode response = m_storage->getObjectHandles( 0x0000, 0x00000000, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(totalCount - 1) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, 0xFFFFFFFF, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(7) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(4) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir2")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(3) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1/subdir3")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(3) );
}

void FSStoragePlugin_test::testObjectHandlesAfterCreation()
{
    quint32 noOfObjects;
    QVector<ObjHandle> objectHandles;

    MTPResponseCode response = m_storage->getObjectHandles( 0x0000, 0x00000000, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), totalCount - 1 );
    QCOMPARE( objectHandles.contains(0), static_cast<bool>(false) );
    QCOMPARE( objectHandles.contains(1), static_cast<bool>(true) );
    QCOMPARE( objectHandles.contains(totalCount - 1), static_cast<bool>(true) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1/subdir3")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), 3 );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), 4 );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, 100, objectHandles );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidParentObject); 
    QCOMPARE( objectHandles.size(), 0 );
}

void FSStoragePlugin_test::testInvalidGetObjectHandle()
{
    QVector<ObjHandle> objectHandles;

    MTPResponseCode response = m_storage->getObjectHandles( 0x0000, 100, objectHandles );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidParentObject); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(0) );

    //test getObjectHandles with association param that's a valid handle but not an association.
    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1/file1")],
                                          objectHandles );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidParentObject); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(0) );
}

void FSStoragePlugin_test::testGetObjectHandleByFormat()
{
    QVector<ObjHandle> objectHandles;
    MTPResponseCode response = m_storage->getObjectHandles( MTP_OBF_FORMAT_Association, 0x00000000, objectHandles );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_OK); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(5) );
}

void FSStoragePlugin_test::testObjectInfoAfterCreation()
{
    const MTPObjectInfo *objectInfo = 0;

    MTPResponseCode response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("mtptests") );

    response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("subdir1") );

    response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir2")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("subdir2") );

    response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1/file1")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("file1") );
    QCOMPARE( objectInfo->mtpObjectCompressedSize, static_cast<quint64>(1));

    response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir2/fileB")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("fileB") );
    QCOMPARE( objectInfo->mtpObjectCompressedSize,static_cast<quint64>(6));

    response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1/subdir3")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("subdir3") );
    QCOMPARE( objectInfo->mtpObjectCompressedSize,static_cast<quint64>(0));

    response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1/subdir3/file3")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("file3") );
    QCOMPARE( objectInfo->mtpObjectCompressedSize,static_cast<quint64>(100));

    response = m_storage->getObjectInfo( 100, objectInfo );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidObjectHandle); 
}

void FSStoragePlugin_test::testFindByPath()
{
    StorageItem* storageItem;

    storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/mtptests" ) );
    QCOMPARE( storageItem != 0, true );
    QCOMPARE( storageItem->m_path, QString("/tmp/mtptests") );
    QCOMPARE( storageItem->m_handle, static_cast<quint32>(0) );

    storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/mtptests/subdir1/subdir3" ) );
    QCOMPARE( storageItem != 0, true );
    QCOMPARE( storageItem->m_path, QString("/tmp/mtptests/subdir1/subdir3") );

    storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/mtptests/subdir2/fileC" ) );
    QCOMPARE( storageItem != 0, true );
    QCOMPARE( storageItem->m_path, QString("/tmp/mtptests/subdir2/fileC") );

    storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/NOmtptests/subdir2/fileC" ) );
    QCOMPARE( storageItem == 0, true );
}

void FSStoragePlugin_test::testObjectHandle()
{
    QCOMPARE( m_storage->checkHandle(0), static_cast<bool>(true) );
    QCOMPARE( m_storage->checkHandle(16), static_cast<bool>(true) );
    QCOMPARE( m_storage->checkHandle(100), static_cast<bool>(false) );
    QCOMPARE( m_storage->checkHandle(-1), static_cast<bool>(false) );
}

void FSStoragePlugin_test::testStorageInfo()
{
    MTPResponseCode response;
    MTPStorageInfo info;

    response = m_storage->storageInfo( info );
    QCOMPARE( info.storageType, (MTPResponseCode)MTP_STORAGE_TYPE_FixedRAM );
    QCOMPARE( info.filesystemType, (MTPResponseCode)MTP_FILE_SYSTEM_TYPE_GenHier );
    QCOMPARE( info.accessCapability, (MTPResponseCode)MTP_STORAGE_ACCESS_ReadWrite );
    QCOMPARE( info.maxCapacity > 0, static_cast<bool>(true) );
    QCOMPARE( info.freeSpace > 0, static_cast<bool>(true) );
    QCOMPARE( info.storageDescription, QString("Phone Memory") );
    QCOMPARE( info.volumeLabel, QString("media") );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
}

void FSStoragePlugin_test::testWriteData()
{
    MTPResponseCode response;

    response = m_storage->writeData( m_storage->m_pathNamesMap["/tmp/mtptests/file2"], "bbb", 3, true, false );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );

    response = m_storage->writeData( m_storage->m_pathNamesMap["/tmp/mtptests/file2"], "bbb", 3, false, true );
    m_storage->writeData(m_storage->m_pathNamesMap["/tmp/mtptests/file2"], 0, 0, false, true);
    QFile file("/tmp/mtptests/file2");
    file.open( QIODevice::ReadOnly);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    char readBuf2[13];
    memset(readBuf2, 0, sizeof(readBuf2));
    readBuf2[12] = '\0';
    int readBufLen = file.read( readBuf2, 12 );
    QCOMPARE( file.size(), static_cast<long long>(6) );
    QCOMPARE( readBufLen, 6 );
    QCOMPARE( readBuf2, "bbbbbb\0" );
    file.close();

    response = m_storage->writeData( 100, "bbb", 3, true, true );
    m_storage->writeData(100, 0, 0, false, true);
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidObjectHandle);
}

void FSStoragePlugin_test::testReadData()
{
    char *readBuf = 0;
    int readBufLen = 1;
    MTPResponseCode response;

    readBuf = (char*)malloc(readBufLen);
    response = m_storage->readData( m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/subdir3/file1"], readBuf, readBufLen, 0 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( readBuf != 0, static_cast<bool>(true) );
    QCOMPARE( readBufLen, 1 );
    QCOMPARE( readBuf[0], 'a' );
    free(readBuf);
    readBuf = 0;

    readBufLen = 100;
    readBuf = (char*)malloc(readBufLen);
    response = m_storage->readData( m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/subdir3/file3"], readBuf, readBufLen, 0 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( readBuf != 0, static_cast<bool>(true) );
    QCOMPARE( readBufLen, 100 );
    QCOMPARE( readBuf[0], 'a' );
    QCOMPARE( readBuf[99], 'a' );
    free(readBuf);
    readBuf = 0;

    response = m_storage->readData( 100, readBuf, readBufLen, 0 );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidObjectHandle);
}

void FSStoragePlugin_test::testAddFile()
{
    MTPResponseCode response;
    ObjHandle parentHandle;
    ObjHandle handle;
    MTPObjectInfo objectInfo;
    //memset(&objectInfo, 0 , sizeof(MTPObjectInfo));
    objectInfo.mtpParentObject = 100;
    
    response = m_storage->addItem( parentHandle, handle, (MTPObjectInfo*)0 );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_Invalid_Dataset);

    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidParentObject);

    {
    // Add a file to root
    objectInfo.mtpParentObject = 0xFFFFFFFF;
    objectInfo.mtpFileName = "addfile";
    objectInfo.mtpObjectCompressedSize = 3;
    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( handle, static_cast<quint32>(totalCount) );
    QCOMPARE( parentHandle, static_cast<quint32>(0) );
    QCOMPARE( objectInfo.mtpParentObject, static_cast<quint32>(0) );
    QCOMPARE( objectInfo.mtpFileName, QString("addfile" ) );
    QCOMPARE( m_storage->m_pathNamesMap["/tmp/mtptests/addfile"], static_cast<quint32>(handle) );
    QCOMPARE( m_storage->m_objectHandlesMap[handle] != 0, true );
    QCOMPARE( m_storage->m_objectHandlesMap[handle]->m_parent->m_handle, static_cast<quint32>(0) );
    response = m_storage->writeData( handle, "xxx", 3, true, true );
    m_storage->writeData(handle, 0, 0, false, true);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QFile file("/tmp/mtptests/addfile");
    file.open( QIODevice::ReadOnly);
    char readBuf[4];
    readBuf[3] = '\0';
    int readBufLen = file.read( readBuf, 3 );
    QCOMPARE( file.size(), static_cast<long long>(3) );
    QCOMPARE( readBufLen, 3 );
    QCOMPARE( readBuf, "xxx\0" );
    file.close();
    }

    {
    // Add another file to root
    objectInfo.mtpFileName  = "addfile2";
    objectInfo.mtpObjectCompressedSize = 3;
    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( handle, static_cast<quint32>(++totalCount) );
    QCOMPARE( parentHandle, static_cast<quint32>(0) );
    QCOMPARE( objectInfo.mtpParentObject, static_cast<quint32>(0) );
    QCOMPARE( objectInfo.mtpFileName, QString("addfile2" ) );
    QCOMPARE( m_storage->m_pathNamesMap["/tmp/mtptests/addfile2"], static_cast<quint32>(handle) );
    QCOMPARE( m_storage->m_objectHandlesMap[handle] != 0, true );
    QCOMPARE( m_storage->m_objectHandlesMap[handle]->m_parent->m_handle, static_cast<quint32>(0) );
    response = m_storage->writeData( handle, "xxx", 3, true, true );
    m_storage->writeData(handle, 0, 0, false, true);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QFile file("/tmp/mtptests/addfile2");
    file.open( QIODevice::ReadOnly);
    char readBuf[4];
    readBuf[3] = '\0';
    int readBufLen = file.read( readBuf, 3 );
    QCOMPARE( file.size(), static_cast<long long>(3) );
    QCOMPARE( readBufLen, 3 );
    QCOMPARE( readBuf, "xxx\0" );
    file.close();
    }

    {
    // Add a file to subdir1
    objectInfo.mtpParentObject = m_storage->m_pathNamesMap["/tmp/mtptests/subdir1"];
    objectInfo.mtpFileName = "addfile";
    objectInfo.mtpObjectCompressedSize = 3;
    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( handle, static_cast<quint32>(++totalCount) );
    QCOMPARE( parentHandle, static_cast<quint32>(m_storage->m_pathNamesMap["/tmp/mtptests/subdir1"]) );
    QCOMPARE( objectInfo.mtpFileName, QString("addfile" ) );
    QCOMPARE( m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/addfile"], static_cast<quint32>(handle) );
    QCOMPARE( m_storage->m_objectHandlesMap[handle] != 0, true );
    QCOMPARE( m_storage->m_objectHandlesMap[handle]->m_parent->m_handle,  m_storage->m_pathNamesMap["/tmp/mtptests/subdir1"]);
    response = m_storage->writeData( handle, "xxx", 3, true, true );
    m_storage->writeData(handle, 0, 0, false, true);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QFile file("/tmp/mtptests/subdir1/addfile");
    file.open( QIODevice::ReadOnly);
    char readBuf[4];
    readBuf[3] = '\0';
    int readBufLen = file.read( readBuf, 3 );
    QCOMPARE( file.size(), static_cast<long long>(3) );
    QCOMPARE( readBufLen, 3 );
    QCOMPARE( readBuf, "xxx\0" );
    file.close();
    }

    {
    // Add a file to subdir3
    objectInfo.mtpParentObject = m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/subdir3"];
    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( handle, static_cast<quint32>(++totalCount) );
    QCOMPARE( parentHandle, static_cast<quint32>(m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/subdir3"]) );
    QCOMPARE( objectInfo.mtpFileName, QString("addfile" ) );
    QCOMPARE( m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/subdir3/addfile"], static_cast<quint32>(handle) );
    QCOMPARE( m_storage->m_objectHandlesMap[handle] != 0, true );
    QCOMPARE( m_storage->m_objectHandlesMap[handle]->m_parent->m_handle, m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/subdir3"] );
    response = m_storage->writeData( handle, "xxx", 3, true, true );
    m_storage->writeData(handle, 0, 0, false, true);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QFile file("/tmp/mtptests/subdir1/subdir3/addfile");
    file.open( QIODevice::ReadOnly);
    char readBuf[4];
    readBuf[3] = '\0';
    int readBufLen = file.read( readBuf, 3 );
    QCOMPARE( file.size(), static_cast<long long>(3) );
    QCOMPARE( readBufLen, 3 );
    QCOMPARE( readBuf, "xxx\0" );
    file.close();
    }

}

void FSStoragePlugin_test::testAddDir()
{
    MTPResponseCode response;
    ObjHandle parentHandle;
    ObjHandle handle;
    quint32 storageId;
    MTPObjectInfo objectInfo;
    //memset(&objectInfo, 0 , sizeof(MTPObjectInfo));

    //add a nested dir to subdir2 : D1, D1->D2, D1->D2->f
    objectInfo.mtpParentObject = m_storage->m_pathNamesMap["/tmp/mtptests/subdir2"];
    objectInfo.mtpFileName = "D1";
    objectInfo.mtpObjectCompressedSize = 0;
    objectInfo.mtpObjectFormat = MTP_OBF_FORMAT_Association;
    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( handle, static_cast<quint32>(++totalCount) );
    QCOMPARE( parentHandle, static_cast<quint32>(m_storage->m_pathNamesMap["/tmp/mtptests/subdir2"]) );
    QCOMPARE( objectInfo.mtpFileName, QString("D1" ) );
    QCOMPARE( m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/D1"], static_cast<quint32>(handle) );
    QCOMPARE( m_storage->m_objectHandlesMap[handle] != 0, true );
    QCOMPARE( m_storage->m_objectHandlesMap[handle]->m_parent->m_handle, static_cast<quint32>(m_storage->m_pathNamesMap["/tmp/mtptests/subdir2"]) );

    objectInfo.mtpParentObject = handle;
    objectInfo.mtpFileName = "D2";
    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( handle, static_cast<quint32>(++totalCount) );
    QCOMPARE( objectInfo.mtpParentObject, static_cast<quint32>(parentHandle) );
    QCOMPARE( objectInfo.mtpFileName, QString("D2" ) );
    QCOMPARE( m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/D1/D2"], static_cast<quint32>(handle) );
    QCOMPARE( m_storage->m_objectHandlesMap[handle] != 0, true );
    QCOMPARE( m_storage->m_objectHandlesMap[handle]->m_parent->m_handle, static_cast<quint32>(parentHandle) );
    QCOMPARE( m_storage->m_objectHandlesMap[parentHandle]->m_firstChild->m_handle, static_cast<quint32>(handle) );

    objectInfo.mtpParentObject = handle;
    objectInfo.mtpFileName = "f1";
    objectInfo.mtpObjectFormat = 0;
    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( handle, static_cast<quint32>(++totalCount) );
    QCOMPARE( objectInfo.mtpParentObject, static_cast<quint32>(parentHandle) );
    QCOMPARE( objectInfo.mtpFileName, QString("f1" ) );
    QCOMPARE( m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/D1/D2/f1"], static_cast<quint32>(handle) );
    QCOMPARE( m_storage->m_objectHandlesMap[handle] != 0, true );
    QCOMPARE( m_storage->m_objectHandlesMap[handle]->m_parent->m_handle, static_cast<quint32>(parentHandle) );
    QCOMPARE( m_storage->m_objectHandlesMap[parentHandle]->m_firstChild->m_handle, static_cast<quint32>(handle) );
    response = m_storage->writeData( handle, "xxx", 3, true, true );
    m_storage->writeData(handle, 0, 0, false, true);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QFile file("/tmp/mtptests/subdir2/D1/D2/f1");
    file.open( QIODevice::ReadOnly);
    char readBuf[4];
    readBuf[3] = '\0';
    int readBufLen = file.read( readBuf, 3 );
    QCOMPARE( file.size(), static_cast<long long>(3) );
    QCOMPARE( readBufLen, 3 );
    QCOMPARE( readBuf, "xxx\0" );
    file.close();
}

void FSStoragePlugin_test::testObjectHandlesCountAfterAddition()
{
    QCOMPARE( m_storage->m_objectHandlesMap.size(), m_storage->m_pathNamesMap.size() );
    QCOMPARE( m_storage->m_objectHandlesMap.size(), totalCount + 1 );
    quint32 noOfObjects;
    QVector<ObjHandle> objectHandles;

    MTPResponseCode response = m_storage->getObjectHandles( 0x0000, 0x00000000, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(totalCount) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, 0xFFFFFFFF, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(9) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(5) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir2")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(4) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1/subdir3")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(4) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, 100, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_InvalidParentObject ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(0) );
}

void FSStoragePlugin_test::testObjectHandlesAfterAddition()
{
    quint32 noOfObjects;
    QVector<ObjHandle> objectHandles;

    MTPResponseCode response = m_storage->getObjectHandles( 0x0000, 0x00000000, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), totalCount );
    QCOMPARE( objectHandles.contains(0), static_cast<bool>(false) );
    QCOMPARE( objectHandles.contains(1), static_cast<bool>(true) );
    QCOMPARE( objectHandles.contains(totalCount), static_cast<bool>(true) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1/subdir3")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), 4 );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), 5 );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, 100, objectHandles );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidParentObject); 
    QCOMPARE( objectHandles.size(), 0 );
}

void FSStoragePlugin_test::testObjectInfoAfterAddition()
{
    const MTPObjectInfo *objectInfo;

    MTPResponseCode response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("mtptests") );

    response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("subdir1") );

    response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir1/addfile")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("addfile") );

    response = m_storage->getObjectInfo( m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir2/D1/D2/f1")], objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectInfo->mtpFileName, QString("f1") );

    StorageItem* storageItem;

    storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/mtptests/subdir2/D1/D2/f1" ) );
    QCOMPARE( storageItem != 0, true );
    QCOMPARE( storageItem->m_path, QString("/tmp/mtptests/subdir2/D1/D2/f1") );

    response = m_storage->getObjectInfo( 100, objectInfo );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidObjectHandle); 
}

void FSStoragePlugin_test::testGetObjectPropertyValue()
{
    MTPResponseCode response;
    MtpObjPropDesc desc;
    desc.uPropCode = MTP_OBJ_PROP_Obj_File_Name;
    desc.uDataType = MTP_DATA_TYPE_STR;
    QList<MTPObjPropDescVal> propValList;
    propValList.append(MTPObjPropDescVal(&desc));
    response = m_storage->getObjectPropertyValue(1, propValList);
    QString filename = propValList[0].propVal.toString();
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( filename, QString("file1") );
}

void FSStoragePlugin_test::testSetObjectPropertyValue()
{
    MTPResponseCode response;
    QString filename = "file";
    QVariant value;
    value.setValue(filename);
    QList<MTPObjPropDescVal> propValList;
    MtpObjPropDesc desc;
    desc.uPropCode = MTP_OBJ_PROP_Obj_File_Name;
    desc.uDataType = MTP_DATA_TYPE_STR;
    propValList.append(MTPObjPropDescVal(&desc, value));
    response = m_storage->setObjectPropertyValue(1, propValList);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    response = m_storage->getObjectPropertyValue(1, propValList);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    filename = propValList[0].propVal.toString();
    QCOMPARE( filename, QString("file") );

    filename = "file1";
    propValList[0].propVal.setValue(filename);
    response = m_storage->setObjectPropertyValue(1, propValList);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    response = m_storage->getObjectPropertyValue(1, propValList);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    filename = propValList[0].propVal.toString();
    QCOMPARE( filename, QString("file1") );
}

void FSStoragePlugin_test::testSetReferences()
{
    MTPResponseCode response;
    QVector<ObjHandle> references;

    references.append(1);
    references.append(15);
    references.append(16);

    response = m_storage->setReferences( 100, references );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidObjectHandle); 

    response = m_storage->setReferences( m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/fileA"], references );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
}

void FSStoragePlugin_test::testGetReferences()
{
    QVector<ObjHandle> references;
    MTPResponseCode response;

    response = m_storage->getReferences( 100, references );
    QCOMPARE( response,(MTPResponseCode) MTP_RESP_InvalidObjectHandle); 

    response = m_storage->getReferences( m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/fileA"], references );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( references.size(), 3 );
    QCOMPARE( references[0], static_cast<unsigned int>(1) );
    QCOMPARE( references[1], static_cast<unsigned int>(15) );
    QCOMPARE( references[2], static_cast<unsigned int>(16) );
}

void FSStoragePlugin_test::testDeleteFile()
{
    MTPResponseCode response;

    response = m_storage->deleteItem( m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/file1"],  MTP_OBF_FORMAT_Undefined );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->deleteItem( m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/file2"],  MTP_OBF_FORMAT_Undefined );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->deleteItem( m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/file3"],  MTP_OBF_FORMAT_Undefined );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

}

void FSStoragePlugin_test::testDeleteDir()
{
    MTPResponseCode response;

    response = m_storage->deleteItem( m_storage->m_pathNamesMap["/tmp/mtptests/subdir1/subdir3"],  MTP_OBF_FORMAT_Undefined );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->deleteItem( m_storage->m_pathNamesMap["/tmp/mtptests/subdir1"],  MTP_OBF_FORMAT_Undefined );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    totalCount -= 9;
}

void FSStoragePlugin_test::testObjectHandlesCountAfterDeletion()
{
    QCOMPARE( m_storage->m_objectHandlesMap.size(), m_storage->m_pathNamesMap.size() );
    QCOMPARE( m_storage->m_objectHandlesMap.size(), totalCount );
    quint32 noOfObjects;
    QVector<ObjHandle> objectHandles;

    MTPResponseCode response = m_storage->getObjectHandles( 0x0000, 0x00000000, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(totalCount - 1) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, 0xFFFFFFFF, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(8) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, m_storage->m_pathNamesMap[QString("/tmp/mtptests/subdir2")], objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(4) );

    objectHandles.clear();
    response = m_storage->getObjectHandles( 0x0000, 100, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_InvalidParentObject ); 
    QCOMPARE( objectHandles.size(), static_cast<qint32>(0) );
}

void FSStoragePlugin_test::testObjectHandlesAfterDeletion()
{
    quint32 noOfObjects;
    QVector<ObjHandle> objectHandles;

    MTPResponseCode response = m_storage->getObjectHandles( 0x0000, 0x00000000, objectHandles );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( objectHandles.size(), totalCount -1 );
    QCOMPARE( objectHandles.contains(0), static_cast<bool>(false) );
    QCOMPARE( objectHandles.contains(1), static_cast<bool>(true) );
}

void FSStoragePlugin_test::testFileCopy()
{
    MTPResponseCode response;
    ObjHandle newHandle;
    QVector<ObjHandle> objectHandles;

    response = m_storage->copyObject( 1, m_storage->m_pathNamesMap["/tmp/mtptests/subdir2"], 0, newHandle );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->copyObject( 3, m_storage->m_pathNamesMap["/tmp/mtptests/subdir2"], 0, newHandle );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
}

void FSStoragePlugin_test::testDirCopy()
{
    MTPResponseCode response;
    ObjHandle parentHandle;
    ObjHandle handle;
    ObjHandle newHandle;
    quint32 storageId;
    MTPObjectInfo objectInfo;
    //memset(&objectInfo, 0 , sizeof(MTPObjectInfo));

    //add a nested dir to subdir2 : D1, D1->D2, D1->D2->f
    objectInfo.mtpParentObject = m_storage->m_pathNamesMap["/tmp/mtptests/subdir2"];
    objectInfo.mtpFileName = "D1";
    objectInfo.mtpObjectCompressedSize = 0;
    objectInfo.mtpObjectFormat = MTP_OBF_FORMAT_Association;
    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );

    objectInfo.mtpParentObject = handle;
    objectInfo.mtpFileName = "D2";
    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );

    objectInfo.mtpParentObject = handle;
    objectInfo.mtpFileName = "f1";
    objectInfo.mtpObjectFormat = 0;
    response = m_storage->addItem( parentHandle, handle, &objectInfo );
    response = m_storage->writeData( handle, "xxx", 3, true, true );
    m_storage->writeData(handle, 0, 0, false, true);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );

    // Copy dir D1 from subdir2 to mtptests
    response = m_storage->copyObject( m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/D1"], m_storage->m_pathNamesMap["/tmp/mtptests"], 0, newHandle );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
}

void FSStoragePlugin_test::testFileMove()
{
    MTPResponseCode response;
    
    response = m_storage->moveObject( m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/fileA"], m_storage->m_pathNamesMap["/tmp/mtptests"], 1 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
}

void FSStoragePlugin_test::testDirMove()
{
    MTPResponseCode response;

    response = m_storage->moveObject( m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/D1"], m_storage->m_pathNamesMap["/tmp/mtptests/D1"], 1 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
}

void FSStoragePlugin_test::testGetLargestObjectHandle()
{
    ObjHandle handle;
    m_storage->getLargestObjectHandle( handle );
    QCOMPARE( handle > 0, true );
}

void FSStoragePlugin_test::testGetLargestPuoid()
{
    MtpInt128 puoid;
    MtpInt128 zero(0);
    m_storage->getLargestPuoid( puoid );
    QCOMPARE( puoid.compare(zero) == 0, true );
}

void FSStoragePlugin_test::testTruncateItem()
{
    MTPResponseCode response;
    response = m_storage->truncateItem( m_storage->m_pathNamesMap["/tmp/mtptests/file3"], 0 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QFile file("/tmp/mtptests/file3");
    QCOMPARE( file.size(), static_cast<qint64>(0));
}

void FSStoragePlugin_test::testGetPath()
{
    MTPResponseCode response;
    QString path;
    response = m_storage->getPath( m_storage->m_pathNamesMap["/tmp/mtptests/file3"], path );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( path, QString("/tmp/mtptests/file3") );
}

void FSStoragePlugin_test::testGetObjectPropertyValueFromStorage()
{
    MTPResponseCode response;
    QVariant v;
    ObjHandle handle = m_storage->m_pathNamesMap["/tmp/mtptests/file3"];
    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Association_Desc, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toInt(), 0 );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Association_Type, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toInt(), 0 );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Parent_Obj, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toUInt(), m_storage->m_pathNamesMap["/tmp/mtptests/"] );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Obj_Size, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toULongLong(), static_cast<quint64>(0) );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_StorageID, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toInt(), 1 );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Obj_Format, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toUInt(), (unsigned int)MTP_OBF_FORMAT_Undefined  );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Protection_Status, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toInt(), 0 );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Allowed_Folder_Contents, v, MTP_DATA_TYPE_UNDEF );
    QVector<quint16> vec = v.value<QVector<quint16> >();
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( vec.size(), 0 );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Date_Created, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toString() != "", true );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Date_Added, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toString() != "", true );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Date_Modified, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toString() != "", true );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Obj_File_Name, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toString(), QString("file3") );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Rep_Sample_Format, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toUInt(), quint32(MTP_OBF_FORMAT_JFIF) );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Rep_Sample_Size, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toInt(), 1024 * 48 );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Rep_Sample_Height, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toInt(), 100 );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Rep_Sample_Width, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toInt(), 100 );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Video_FourCC_Codec, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Hidden, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toInt(), 0 );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Persistent_Unique_ObjId, v, MTP_DATA_TYPE_UNDEF );
    MtpInt128 puoid = v.value<MtpInt128>();
    MtpInt128 zero(0);
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( puoid.compare(zero) == 0, true );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Non_Consumable, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toInt(), 0 );
}

void FSStoragePlugin_test::testGetObjectPropertyValueFromTracker()
{
    MTPResponseCode response;
    QVariant v;
    ObjHandle handle = m_storage->m_pathNamesMap["/tmp/mtptests/file3"];

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Date_Created, v, MTP_DATA_TYPE_STR );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Name, v, MTP_DATA_TYPE_STR );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    //QCOMPARE( v.toString() == QString("file3") || v.toString() == QString("none"), true );

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Artist, v, MTP_DATA_TYPE_STR );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Width, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Height, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Duration, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    /*
    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Rating, v, MTP_DATA_TYPE_UINT16 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    */

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Track, v, MTP_DATA_TYPE_UINT16 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Genre, v, MTP_DATA_TYPE_STR );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Use_Count, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    /*
    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Original_Release_Date, v, MTP_DATA_TYPE_STR );
                                                             
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    */

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Album_Name, v, MTP_DATA_TYPE_STR );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    /*
    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Album_Artist, v, MTP_DATA_TYPE_STR );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    */

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Bitrate_Type, v, MTP_DATA_TYPE_UINT16 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Sample_Rate, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Nbr_Of_Channels, v, MTP_DATA_TYPE_UINT16 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    /*
    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Scan_Type, v, MTP_DATA_TYPE_UINT16 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    */

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Audio_WAVE_Codec, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Audio_BitRate, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Video_FourCC_Codec, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Video_BitRate, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Frames_Per_Thousand_Secs, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    /*
    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_KeyFrame_Distance, v, MTP_DATA_TYPE_UINT32 );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Encoding_Profile, v, MTP_DATA_TYPE_STR );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    */
}

void FSStoragePlugin_test::testSetObjectPropertyValueInTracker()
{
    MTPResponseCode response;
    quint32 uInt32 = 0;
    quint16 uInt16 = 0;
    QString none = "none";
    QString empty = "";
    ObjHandle handle = m_storage->m_pathNamesMap["/tmp/mtptests/file3"];
    QList<MTPObjPropDescVal> propValList;
    MTPObjPropDescVal val;
    MtpObjPropDesc desc;

    propValList.append(val);
    QVariant &v = propValList[0].propVal;

    propValList[0].propDesc = &desc;
    v.setValue(empty);
    desc.uPropCode = MTP_OBJ_PROP_Date_Created;
    desc.uDataType = MTP_DATA_TYPE_STR;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(none);
    desc.uPropCode = MTP_OBJ_PROP_Name;
    desc.uDataType = MTP_DATA_TYPE_STR;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    response = m_storage->getObjectPropertyValueFromTracker( handle,
                                                             MTP_OBJ_PROP_Name, v, MTP_DATA_TYPE_STR );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
    QCOMPARE( v.toString() == QString("file3") || v.toString() == QString("none"), true );

    v.setValue(none);
    desc.uPropCode = MTP_OBJ_PROP_Artist;
    desc.uDataType = MTP_DATA_TYPE_STR;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt32);
    desc.uPropCode = MTP_OBJ_PROP_Width;
    desc.uDataType = MTP_DATA_TYPE_STR;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt32);
    desc.uPropCode = MTP_OBJ_PROP_Height;
    desc.uDataType = MTP_DATA_TYPE_UINT32;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt32);
    desc.uPropCode = MTP_OBJ_PROP_Duration;
    desc.uDataType = MTP_DATA_TYPE_UINT32;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt16);
    desc.uPropCode = MTP_OBJ_PROP_Rating;
    desc.uDataType = MTP_DATA_TYPE_UINT16;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt16);
    desc.uPropCode = MTP_OBJ_PROP_Track;
    desc.uDataType = MTP_DATA_TYPE_UINT16;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(empty);
    desc.uPropCode = MTP_OBJ_PROP_Genre;
    desc.uDataType = MTP_DATA_TYPE_STR;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt32);
    desc.uPropCode = MTP_OBJ_PROP_Use_Count;
    desc.uDataType = MTP_DATA_TYPE_UINT32;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(empty);
    desc.uPropCode = MTP_OBJ_PROP_Original_Release_Date;
    desc.uDataType = MTP_DATA_TYPE_STR;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(none);
    desc.uPropCode = MTP_OBJ_PROP_Album_Name;
    desc.uDataType = MTP_DATA_TYPE_STR;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(none);
    desc.uPropCode = MTP_OBJ_PROP_Album_Artist;
    desc.uDataType = MTP_DATA_TYPE_STR;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt16);
    desc.uPropCode = MTP_OBJ_PROP_Bitrate_Type;
    desc.uDataType = MTP_DATA_TYPE_UINT16;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt32);
    desc.uPropCode = MTP_OBJ_PROP_Sample_Rate;
    desc.uDataType = MTP_DATA_TYPE_UINT32;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt16);
    desc.uPropCode = MTP_OBJ_PROP_Nbr_Of_Channels;
    desc.uDataType = MTP_DATA_TYPE_UINT16;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt16);
    desc.uPropCode = MTP_OBJ_PROP_Scan_Type;
    desc.uDataType = MTP_DATA_TYPE_UINT16;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt16);
    desc.uPropCode = MTP_OBJ_PROP_Audio_WAVE_Codec;
    desc.uDataType = MTP_DATA_TYPE_UINT32;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt32);
    desc.uPropCode = MTP_OBJ_PROP_Audio_BitRate;
    desc.uDataType = MTP_DATA_TYPE_UINT32;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt32);
    desc.uPropCode = MTP_OBJ_PROP_Video_FourCC_Codec;
    desc.uDataType = MTP_DATA_TYPE_UINT32;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt32);
    desc.uPropCode = MTP_OBJ_PROP_Video_BitRate;
    desc.uDataType = MTP_DATA_TYPE_UINT32;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt32);
    desc.uPropCode = MTP_OBJ_PROP_Frames_Per_Thousand_Secs;
    desc.uDataType = MTP_DATA_TYPE_UINT32;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(uInt32);
    desc.uPropCode = MTP_OBJ_PROP_KeyFrame_Distance;
    desc.uDataType = MTP_DATA_TYPE_UINT32;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 

    v.setValue(empty);
    desc.uPropCode = MTP_OBJ_PROP_Encoding_Profile;
    desc.uDataType = MTP_DATA_TYPE_STR;
    response = m_storage->setObjectPropertyValue( handle, propValList );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK ); 
}

void FSStoragePlugin_test::testGetInvalidObjectPropertyValueFromStorage()
{
    MTPResponseCode response;
    QVariant v;
    ObjHandle handle = m_storage->m_pathNamesMap["/tmp/mtptests/file3"];
    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             0x0000, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_ObjectProp_Not_Supported ); 
}

void FSStoragePlugin_test::testInotifyCreate()
{
    QEventLoop loop;
    quint32 getOutOfHere = 0;
    loop.processEvents();
    while( !m_storage->m_pathNamesMap["/tmp/mtptests/tmpfile"] && getOutOfHere < 20 )
    {
        system("touch /tmp/mtptests/tmpfile");
        loop.processEvents();
        ++getOutOfHere;
    }
    QCOMPARE( m_storage->m_pathNamesMap["/tmp/mtptests/tmpfile"] > 0, true );
}

void FSStoragePlugin_test::testInotifyModify()
{
    QEventLoop loop;
    quint32 getOutOfHere = 0;
    quint64 size = 0;
    loop.processEvents();
    const MTPObjectInfo *objInfo;
    system("echo \"tmp\" > /tmp/mtptests/tmpfile");
    while( size !=3 && getOutOfHere < 20 )
    {
        // Recalculate object Info
        m_storage->getObjectInfo(m_storage->m_pathNamesMap["/tmp/mtptests/tmpfile"], objInfo);
        size = objInfo->mtpObjectCompressedSize;
        loop.processEvents();
        ++getOutOfHere;
    }
    QCOMPARE( size, static_cast<quint64>(4) );
} 

void FSStoragePlugin_test::testInotifyMove()
{
    QEventLoop loop;
    quint32 getOutOfHere = 0;
    loop.processEvents();
    //system("mkdir /tmp/mtptests/tmpdir");
    //system("mv /tmp/mtptests/tmpfile /tmp/mtptests/tmpdir");
    system("mv /tmp/mtptests/tmpfile /tmp/mtptests/subdir2");
    StorageItem *storageItem = 0;
    while( !storageItem && getOutOfHere < 20 )
    {
        //storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/mtptests/tmpdir/tmpfile" ) );
        storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/mtptests/subdir2/tmpfile" ) );
        loop.processEvents();
        ++getOutOfHere;
    }
    QCOMPARE( storageItem != 0, true );
    //QCOMPARE( storageItem->m_parent->m_handle, m_storage->m_pathNamesMap["/tmp/mtptests/tmpdir"] );
    QCOMPARE( storageItem->m_parent->m_handle, m_storage->m_pathNamesMap["/tmp/mtptests/subdir2"] );
    // Fetch the object info once
    const MTPObjectInfo *objInfo;
    m_storage->getObjectInfo(m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/tmpfile"], objInfo);
} 

void FSStoragePlugin_test::testInotifyDelete()
{
    QEventLoop loop;
    quint32 getOutOfHere = 0;
    loop.processEvents();
    //StorageItem *storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/mtptests/tmpdir/tmpfile" ) );
    StorageItem *storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/mtptests/subdir2/tmpfile" ) );
    //system("rm /tmp/mtptests/tmpdir/tmpfile");
    system("rm /tmp/mtptests/subdir2/tmpfile");
    while( storageItem && getOutOfHere < 20 )
    {
        //storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/mtptests/tmpdir/tmpfile" ) );
        storageItem = static_cast<StorageItem*>( m_storage->findStorageItemByPath( "/tmp/mtptests/subdir2/tmpfile" ) );
        loop.processEvents();
        ++getOutOfHere;
    }
    QCOMPARE( storageItem == 0, true );
}

void FSStoragePlugin_test::testReadPlaylists()
{
    // Ensure that the tracker playlists created are present in our storage tree
    // as .pla files
    // Get handle to the playlists directory
    ObjHandle playlistsDirHandle = 0;
    playlistsDirHandle = m_storage->m_pathNamesMap["/tmp/mtptests/Playlists"];
    QCOMPARE(playlistsDirHandle != 0, true);

    // Get children of the playlists directory
    QVector<ObjHandle> playlists;
    MTPResponseCode response = m_storage->getObjectHandles(0x0000, playlistsDirHandle, playlists);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QCOMPARE(playlists.size(), 2);

    ObjHandle playlist1Handle = 0;
    ObjHandle playlist2Handle = 0;
    
    const MTPObjectInfo *objectInfo = 0;
    response = m_storage->getObjectInfo(playlists[0], objectInfo);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QCOMPARE((objectInfo->mtpFileName == "play1.pla" || objectInfo->mtpFileName == "play2.pla"), true);
    if(objectInfo->mtpFileName == "play1.pla")
    {
        playlist1Handle = playlists[0];
        playlist2Handle = playlists[1];
    }
    else
    {
        playlist1Handle = playlists[1];
        playlist2Handle = playlists[0];
    }

    response = m_storage->getObjectInfo(playlists[1], objectInfo);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QCOMPARE((objectInfo->mtpFileName == "play1.pla" || objectInfo->mtpFileName == "play2.pla"), true);

    QVector<ObjHandle> playlistEntries;
    // Test their references
    response = m_storage->getReferences(playlist1Handle, playlistEntries);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QCOMPARE(playlistEntries.size(), 2); 

    response = m_storage->getObjectInfo(playlistEntries[0], objectInfo);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QCOMPARE(objectInfo->mtpFileName, QString("song1.mp3"));
    
    response = m_storage->getObjectInfo(playlistEntries[1], objectInfo);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QCOMPARE(objectInfo->mtpFileName, QString("song2.mp3"));

    // Test their references
    response = m_storage->getReferences(playlist2Handle, playlistEntries);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QCOMPARE(playlistEntries.size(), 2); 

    response = m_storage->getObjectInfo(playlistEntries[0], objectInfo);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QCOMPARE(objectInfo->mtpFileName, QString("song3.mp3"));
    
    response = m_storage->getObjectInfo(playlistEntries[1], objectInfo);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QCOMPARE(objectInfo->mtpFileName, QString("song4.mp3"));
}

void FSStoragePlugin_test::testDeletePlaylists()
{
    // Delete one of the existing playlists
    ObjHandle handle = 0;
    handle = m_storage->m_pathNamesMap["/tmp/mtptests/Playlists/play1.pla"];
    QCOMPARE(handle != 0, true);

    MTPResponseCode response = m_storage->deleteItem(handle, MTP_OBF_FORMAT_Undefined);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);

    // Ensure that the playlist is deleted from tracker too
    QVector<QStringList> result = ::tracker()->rawSparqlQuery(QString("SELECT ?f WHERE {?f a nmm:Playlist}"));
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0][0], QString("urn:playlist:pl2"));

    // Delete the other one too
    handle = m_storage->m_pathNamesMap["/tmp/mtptests/Playlists/play2.pla"];
    QCOMPARE(handle != 0, true);

    response = m_storage->deleteItem(handle, MTP_OBF_FORMAT_Undefined);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    result.clear();
    // Ensure that the playlist is deleted from tracker too
    result = ::tracker()->rawSparqlQuery(QString("SELECT ?f WHERE {?f a nmm:Playlist}"));
    QCOMPARE(result.size(), 0);
}

void FSStoragePlugin_test::testCreatePlaylists()
{
    // Create a new abstract audio video playlist and assign references to it
    ObjHandle parentHandle = 0;
    ObjHandle newPlaylistHandle = 0;
    parentHandle = m_storage->m_pathNamesMap["/tmp/mtptests/Playlists"];
    QCOMPARE(parentHandle == 0, false);

    MTPObjectInfo objInfo;
    objInfo.mtpObjectFormat = MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist;
    objInfo.mtpParentObject = parentHandle;
    objInfo.mtpFileName = "MyPlaylist";

    MTPResponseCode response = m_storage->addItem(parentHandle, newPlaylistHandle, &objInfo);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QCOMPARE(newPlaylistHandle == 0, false);

    // Set references to all songs under Music into this playlist
    QVector<ObjHandle> allSongs;
    allSongs.append(m_storage->m_pathNamesMap["/tmp/mtptests/Music/song1.mp3"]);
    allSongs.append(m_storage->m_pathNamesMap["/tmp/mtptests/Music/song2.mp3"]);
    allSongs.append(m_storage->m_pathNamesMap["/tmp/mtptests/Music/song3.mp3"]);
    allSongs.append(m_storage->m_pathNamesMap["/tmp/mtptests/Music/song4.mp3"]);
    response = m_storage->setReferences(newPlaylistHandle, allSongs);
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
}

void FSStoragePlugin_test::testPlaylistsPersistence()
{
    // Make direct tracker queries to ensure that the playlist MyPlaylist has
    // been added.
    QString urn;
    QVector<QStringList> resultSet;
    QString query = QString("SELECT ?f WHERE {?f a nmm:Playlist}");
    resultSet = ::tracker()->rawSparqlQuery(query);
    QCOMPARE(resultSet.size(), 1);
    urn = resultSet[0][0];
    
    query = QString("SELECT ?f WHERE{<") + urn + QString("> nie:title ?f}");
    resultSet = ::tracker()->rawSparqlQuery(query);
    QCOMPARE(resultSet.size(), 1);
    QCOMPARE(resultSet[0][0], QString("MyPlaylist"));

    query = QString("SELECT ?f WHERE{<") + urn + QString("> nie:url ?f}");
    resultSet = ::tracker()->rawSparqlQuery(query);
    QCOMPARE(resultSet.size(), 1);
    QCOMPARE(resultSet[0][0], QString("file:///tmp/mtptests/Playlists/MyPlaylist"));

    // Query the entries now
    query = QString("SELECT ?f WHERE{<") + urn + QString("> nfo:hasMediaFileListEntry ?f}");
    QVector<QStringList> resultSetAll = ::tracker()->rawSparqlQuery(query);
    QCOMPARE(resultSetAll.size(), 4);

    // Query the entry content's uri to ensure that the playlist entries were
    // added correctly
    query = QString("SELECT ?f WHERE{<") + resultSetAll[0][0] + QString("> nfo:entryContent ?f}");
    resultSet = ::tracker()->rawSparqlQuery(query);
    QCOMPARE(resultSet.size(), 1);
    QCOMPARE(resultSet[0][0], QString("file:///tmp/mtptests/Music/song1.mp3"));

    query = QString("SELECT ?f WHERE{<") + resultSetAll[1][0] + QString("> nfo:entryContent ?f}");
    resultSet = ::tracker()->rawSparqlQuery(query);
    QCOMPARE(resultSet.size(), 1);
    QCOMPARE(resultSet[0][0], QString("file:///tmp/mtptests/Music/song2.mp3"));

    query = QString("SELECT ?f WHERE{<") + resultSetAll[2][0] + QString("> nfo:entryContent ?f}");
    resultSet = ::tracker()->rawSparqlQuery(query);
    QCOMPARE(resultSet.size(), 1);
    QCOMPARE(resultSet[0][0], QString("file:///tmp/mtptests/Music/song3.mp3"));

    query = QString("SELECT ?f WHERE{<") + resultSetAll[3][0] + QString("> nfo:entryContent ?f}");
    resultSet = ::tracker()->rawSparqlQuery(query);
    QCOMPARE(resultSet.size(), 1);
    QCOMPARE(resultSet[0][0], QString("file:///tmp/mtptests/Music/song4.mp3"));
}

void FSStoragePlugin_test::cleanupTestCase()
{
    delete m_storage;
    system("rm -rf ~/.mtp");
    system("rm -rf /tmp/mtptests");
    system("tracker-control -r");
    sleep(2);
    system("tracker-control -s");
    sleep(2);
}

QTEST_MAIN(FSStoragePlugin_test);

