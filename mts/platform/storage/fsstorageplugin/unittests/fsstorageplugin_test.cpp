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

#include <unistd.h>
#include "fsstorageplugin_test.h"
#include "fsstorageplugin.h"
#include "storageitem.h"
#include <QImage>
#include <QPainter>
#include <QRadialGradient>
#include <QSignalSpy>


using namespace meegomtp1dot0;

int totalCount = 17;

static const int THUMBNAIL_WIDTH = 100;
static const int THUMBNAIL_HEIGHT = 100;

void FSStoragePlugin_test::initTestCase()
{
    QDir dir;
    QFile file( QDir::homePath() + "/.local/mtp/mtphandles");
    if( file.open( QIODevice::ReadOnly ) )
    {
        m_storage = new FSStoragePlugin( 1, MTP_STORAGE_TYPE_FixedRAM, "/tmp/mtptests", "media", "Phone Memory");
        dir = QDir( m_storage->m_storagePath );
        setupPlugin(m_storage);
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
        dir.remove("mtphandles");
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
}

void FSStoragePlugin_test::testStorageCreation()
{
    m_storage = new FSStoragePlugin( 1, MTP_STORAGE_TYPE_FixedRAM, "/tmp/mtptests", "media", "Phone Memory" );
    QDir dir( m_storage->m_storagePath );

    setupPlugin(m_storage);
    QVERIFY( m_storage->m_root != 0 );
    QCOMPARE( m_storage->m_root->m_handle, static_cast<unsigned int>(0) );
    QVERIFY( m_storage->m_root->m_parent == 0 );
    QVERIFY( m_storage->m_root->m_firstChild != 0 );
    QVERIFY( m_storage->m_root->m_nextSibling == 0 );
    QCOMPARE( m_storage->m_root->m_path, QString("/tmp/mtptests") );

    // Check whether child items are correctly linked to their parents.
    StorageItem *parentItem =
            m_storage->findStorageItemByPath("/tmp/mtptests/subdir1");
    StorageItem *childItem =
            m_storage->findStorageItemByPath("/tmp/mtptests/subdir1/file1");
    QVERIFY( parentItem && childItem );
    QCOMPARE( childItem->m_objectInfo->mtpParentObject, parentItem->m_handle );
}

void FSStoragePlugin_test::testDeleteAll()
{
    MTPResponseCode response = MTP_RESP_GeneralError;

    // The storageitem for the root should survive deletion, hence the
    // PartialDeletion and the remaining item count of 1
    response = m_storage->deleteItem( 0xFFFFFFFF,  MTP_OBF_FORMAT_Undefined );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_PartialDeletion );

    QCOMPARE( m_storage->m_objectHandlesMap.size(), 1 );
    QCOMPARE( m_storage->m_objectHandlesMap.size(), m_storage->m_pathNamesMap.size() );

    delete m_storage;
    system("rm -rf ~/.local/mtp");
    system("rm -rf /tmp/mtptests");
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

void FSStoragePlugin_test::testGetChildPropertyValues()
{
    MTPObjectInfo info;
    info.mtpFileName = "childDirectory";
    info.mtpObjectFormat = MTP_OBF_FORMAT_Association;

    ObjHandle unused;
    ObjHandle directoryHandle;

    QCOMPARE(m_storage->addItem(unused, directoryHandle, &info),
            (MTPResponseCode)MTP_RESP_OK);

    info.mtpObjectFormat = MTP_OBF_FORMAT_Text;
    info.mtpParentObject = directoryHandle;

    ObjHandle file1Handle;
    info.mtpFileName = "childFile1";
    QCOMPARE(m_storage->addItem(unused, file1Handle, &info),
            (MTPResponseCode)MTP_RESP_OK);

    ObjHandle file2Handle;
    info.mtpFileName = "childFile2";
    QCOMPARE(m_storage->addItem(unused, file2Handle, &info),
            (MTPResponseCode)MTP_RESP_OK);

    MtpObjPropDesc desc;
    desc.uPropCode = MTP_OBJ_PROP_Obj_File_Name;
    desc.uDataType = MTP_DATA_TYPE_STR;

    QList<MTPObjPropDescVal> propValList;
    propValList.append(MTPObjPropDescVal(&desc, QVariant()));

    propValList.last().propVal = "filename1";
    QCOMPARE(m_storage->setObjectPropertyValue(file1Handle, propValList),
            (MTPResponseCode)MTP_RESP_OK);

    propValList.last().propVal = "filename2";
    QCOMPARE(m_storage->setObjectPropertyValue(file2Handle, propValList),
            (MTPResponseCode)MTP_RESP_OK);

    // Some additional property we know we haven't set.
    MtpObjPropDesc descWeDontWantToBeReturned;
    descWeDontWantToBeReturned.uPropCode = MTP_OBJ_PROP_Genre;
    descWeDontWantToBeReturned.uDataType = MTP_DATA_TYPE_STR;

    QList<const MtpObjPropDesc *> properties;
    properties.append(&descWeDontWantToBeReturned);
    properties.append(&desc);

    QMap<ObjHandle, QList<QVariant> > values;
    QCOMPARE(m_storage->getChildPropertyValues(directoryHandle, properties,
            values), (MTPResponseCode)MTP_RESP_OK);

    QVERIFY(values.size() == 2);
    QVERIFY(values.contains(file1Handle) && values.contains(file2Handle));
    QVERIFY(values[file1Handle].size() == 2 && values[file2Handle].size() == 2);

    QVERIFY(!values[file1Handle][0].isValid());
    QCOMPARE(values[file1Handle][1].toString(), QString("filename1"));
    QVERIFY(!values[file2Handle][0].isValid());
    QCOMPARE(values[file2Handle][1].toString(), QString("filename2"));


    m_storage->deleteItem(directoryHandle, MTP_OBF_FORMAT_Undefined);
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

    response = m_storage->moveObject( m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/fileA"],
            m_storage->m_pathNamesMap["/tmp/mtptests"], m_storage );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
}

void FSStoragePlugin_test::testFileMoveAcrossStorage()
{
    QEventLoop loop;
    while ( loop.processEvents() );

    const QByteArray TEXT( "some text for comparison" );

    QDir dir( "/tmp/mtptests-second" );
    dir.removeRecursively();

    dir.mkpath("/tmp/mtptests-second");
    dir.mkdir( "dir1" );

    QFile file( "/tmp/mtptests/fileToMove" );
    file.remove();
    file.open(QFile::WriteOnly);
    file.write(TEXT);
    file.close();

    // Process all accumulated events.
    while ( loop.processEvents() );

    FSStoragePlugin secondStorage( 2, MTP_STORAGE_TYPE_FixedRAM,
            "/tmp/mtptests-second", "second", "Second Storage" );
    setupPlugin(&secondStorage);

    StorageItem *item = m_storage->findStorageItemByPath( "/tmp/mtptests/fileToMove" );

    ObjHandle originalHandle = item->m_handle;
    MTPObjectInfo originalInfo = *item->m_objectInfo;

    QCOMPARE( m_storage->moveObject( originalHandle,
            secondStorage.m_pathNamesMap["/tmp/mtptests-second/dir1"], &secondStorage ),
            (MTPResponseCode)MTP_RESP_OK);

    QVERIFY( !m_storage->checkHandle( originalHandle ) );

    StorageItem *movedItem =
            secondStorage.findStorageItemByPath( "/tmp/mtptests-second/dir1/fileToMove" );
    StorageItem *parentItem =
            secondStorage.findStorageItemByPath( "/tmp/mtptests-second/dir1" );

    QVERIFY( movedItem );
    QVERIFY( parentItem );
    QCOMPARE ( movedItem->m_handle, originalHandle );

    MTPObjectInfo *movedInfo = movedItem->m_objectInfo;
    QCOMPARE ( movedInfo->mtpStorageId, secondStorage.storageId() );
    QCOMPARE ( movedInfo->mtpParentObject, parentItem->m_handle );
    QCOMPARE ( movedInfo->mtpFileName, originalInfo.mtpFileName );

    QFile copiedFile( "/tmp/mtptests-second/dir1/fileToMove" );
    copiedFile.open( QFile::ReadOnly );
    QByteArray text( copiedFile.readAll() );
    QVERIFY( text == TEXT );
}

void FSStoragePlugin_test::testDirMove()
{
    MTPResponseCode response;

    response = m_storage->moveObject( m_storage->m_pathNamesMap["/tmp/mtptests/subdir2/D1"],
            m_storage->m_pathNamesMap["/tmp/mtptests/D1"], m_storage );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
}

void FSStoragePlugin_test::testDirMoveAcrossStorage()
{
    QEventLoop loop;

    QDir dir( "/tmp/mtptests-second" );
    dir.removeRecursively();
    dir.mkpath( "/tmp/mtptests-second" );
    dir.mkdir( "dir" );

    dir.cd( "/tmp/mtptests/d1" );
    dir.removeRecursively();
    dir.mkpath( "/tmp/mtptests/d1/d2" );
    QFile file( "/tmp/mtptests/d1/d2/f1" );
    file.open( QFile::WriteOnly );
    file.close();

    loop.processEvents();

    FSStoragePlugin secondStorage( 2, MTP_STORAGE_TYPE_FixedRAM,
            "/tmp/mtptests-second", "second", "Second Storage" );
    setupPlugin(&secondStorage);

    StorageItem *item;

    item = m_storage->findStorageItemByPath( "/tmp/mtptests/d1" );
    ObjHandle hOrigD1 = item->m_handle;
    MTPObjectInfo iOrigD1 = *item->m_objectInfo;

    item = m_storage->findStorageItemByPath( "/tmp/mtptests/d1/d2" );
    ObjHandle hOrigD2 = item->m_handle;
    MTPObjectInfo iOrigD2 = *item->m_objectInfo;

    item = m_storage->findStorageItemByPath( "/tmp/mtptests/d1/d2/f1" );
    ObjHandle hOrigF1 = item->m_handle;
    MTPObjectInfo iOrigF1 = *item->m_objectInfo;

    QCOMPARE( m_storage->moveObject( hOrigD1,
            secondStorage.m_pathNamesMap["/tmp/mtptests-second/dir"], &secondStorage ),
            (MTPResponseCode)MTP_RESP_OK);

    QVERIFY( !m_storage->checkHandle( hOrigD1 ) &&
             !m_storage->checkHandle( hOrigD2 ) &&
             !m_storage->checkHandle( hOrigF1 ) );

    StorageItem *parentItem =
            secondStorage.findStorageItemByPath( "/tmp/mtptests-second/dir" );

    StorageItem *movedD1 = secondStorage.m_objectHandlesMap[ hOrigD1 ];
    QCOMPARE ( movedD1->m_objectInfo->mtpParentObject, parentItem->m_handle );
    QCOMPARE ( movedD1->m_objectInfo->mtpFileName, iOrigD1.mtpFileName );

    StorageItem *movedD2 = secondStorage.m_objectHandlesMap[ hOrigD2 ];
    QCOMPARE ( movedD2->m_objectInfo->mtpParentObject, movedD1->m_handle );
    QCOMPARE ( movedD2->m_objectInfo->mtpFileName, iOrigD2.mtpFileName );

    StorageItem *movedF1 = secondStorage.m_objectHandlesMap[ hOrigF1 ];
    QCOMPARE ( movedF1->m_objectInfo->mtpParentObject, movedD2->m_handle );
    QCOMPARE ( movedF1->m_objectInfo->mtpFileName, iOrigF1.mtpFileName );
}

void FSStoragePlugin_test::testGetLargestPuoid()
{
    MtpInt128 puoid;
    MtpInt128 zero(0);
    m_storage->getLargestPuoid(puoid);
    QVERIFY(puoid == zero);
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
    QCOMPARE( v.toUInt(), m_storage->m_pathNamesMap["/tmp/mtptests"] );

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
    QCOMPARE( v.toInt(), THUMBNAIL_HEIGHT );

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Rep_Sample_Width, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( v.toInt(), THUMBNAIL_WIDTH );

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
    QCOMPARE(response, (MTPResponseCode)MTP_RESP_OK);
    QVERIFY(puoid == zero);

    response = m_storage->getObjectPropertyValueFromStorage( handle,
                                                             MTP_OBJ_PROP_Non_Consumable, v, MTP_DATA_TYPE_UNDEF );
    QCOMPARE( response, (MTPResponseCode)MTP_RESP_OK );
    QCOMPARE( v.toInt(), 0 );
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

    QDir dir;
    dir.mkpath( "/tmp/mtptests/inotifydir" );

    while( loop.processEvents() );

    QFile file( "/tmp/mtptests/inotifydir/tmpfile" );
    file.open( QFile::ReadWrite );
    file.close();

    while( loop.processEvents() );

    QVERIFY( m_storage->m_pathNamesMap.contains("/tmp/mtptests/inotifydir/tmpfile") );
}

void FSStoragePlugin_test::testInotifyModify()
{
    QEventLoop loop;
    loop.processEvents();

    QFile file( "/tmp/mtptests/tmpfile" );
    file.open( QFile::ReadWrite );
    file.close();

    loop.processEvents();

    StorageItem *item =
            m_storage->findStorageItemByPath("/tmp/mtptests/tmpfile");
    QVERIFY( item );
    QCOMPARE( item->m_objectInfo->mtpObjectCompressedSize,
              static_cast<quint64>(0) );

    const QString TEXT( "some text to be written into the file" );
    file.open( QFile::ReadWrite );
    file.write( TEXT.toUtf8() );
    file.close();

    loop.processEvents();

    QCOMPARE( item->m_objectInfo->mtpObjectCompressedSize,
              static_cast<quint64>(TEXT.size()) );
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

void FSStoragePlugin_test::testThumbnailer()
{
    // Create an image for the thumbnailer to work on
    QImage testpic(800, 600, QImage::Format_RGB32);
    QPainter painter(&testpic);
    QRadialGradient gradient(400, 400, 100, 400, 300);
    gradient.setColorAt(0, QColor::fromRgb(0x404040));
    gradient.setColorAt(1, QColor::fromRgb(0x00ffff));
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRect(testpic.rect());
    QVERIFY2(!testpic.isNull(), "testpic not valid");
    bool success = testpic.save("/tmp/mtptests/testpic.png");
    QVERIFY2(success, "testpic not saved");

    // Wait for the inotify handler to process it
    QEventLoop loop;
    ObjHandle handle = 0;
    int maxtries = 20;
    while (!handle && maxtries > 0)
    {
        loop.processEvents();
        handle = m_storage->m_pathNamesMap["/tmp/mtptests/testpic.png"];
        --maxtries;
    }
    QVERIFY2(handle != 0, "testpic not registered in storage");

    // Now ask for its thumbnail
    MTPResponseCode response;
    QVariant value;
    response = m_storage->getObjectPropertyValueFromStorage( handle,
        MTP_OBJ_PROP_Rep_Sample_Data, value, MTP_DATA_TYPE_UNDEF);
    QVERIFY(!value.isNull());
    QVector<quint8> data = value.value<QVector<quint8> >(); // sigh, why is this not a QByteArray

    if ( data.isEmpty() )
    {
        // Thumbnail was not generated synchronously, so wait for
        // it to be ready. Readiness should be signaled to the initiator
        // via an ObjectPropChanged event.
        QSignalSpy spy(m_storage, SIGNAL(eventGenerated(MTPEventCode, const QVector<quint32>&)));
        QList<QVariant> arguments;
        QVERIFY(spy.isValid());
        int maxsignals = 20; // prevent infinite loop if signals keep coming
        bool got_signal = false;
        while (!got_signal && maxsignals > 0)
        {
            bool emitted = spy.wait(1000); // wait 1 second for new signals
            QVERIFY2(emitted, "thumbnail-ready signal not received");
            while( spy.count() > 0 )
            {
                arguments = spy.takeFirst();
                if (arguments.at(0).value<MTPEventCode>() == MTP_EV_ObjectPropChanged)
                {
                    QVector<quint32> params = arguments.at(1).value<QVector<quint32> >();
                    QCOMPARE(params.count(), 2);
                    QCOMPARE(params[0], handle);
                    QCOMPARE(params[1], (unsigned) MTP_OBJ_PROP_Rep_Sample_Data);
                    got_signal = true;
                    break;
                }
            }
            --maxsignals;
        }
        QVERIFY2(got_signal, "thumbnail-ready signal not received");
        response = m_storage->getObjectPropertyValueFromStorage( handle,
            MTP_OBJ_PROP_Rep_Sample_Data, value, MTP_DATA_TYPE_UNDEF);
        QVERIFY(!value.isNull());
        data = value.value<QVector<quint8> >();
        // After getting the event, there must be a thumbnail
        QVERIFY(!data.isEmpty());
    }

    // Check that it's a valid image
    QImage thumbnail;
    // expect JPEG because other tests expect JFIF for Sample_Format
    thumbnail.loadFromData(data.data(), data.size(), "JPEG");
    QVERIFY(!thumbnail.isNull());
    QVERIFY(thumbnail.width() <= THUMBNAIL_WIDTH);
    QVERIFY(thumbnail.height() <= THUMBNAIL_HEIGHT);
}

void FSStoragePlugin_test::setupPlugin(StoragePlugin *plugin)
{
    QSignalSpy readySpy(plugin, SIGNAL(storagePluginReady(quint32)));
    plugin->enumerateStorage();
    QVERIFY(readySpy.wait());
}

void FSStoragePlugin_test::cleanupTestCase()
{
    delete m_storage;
    system("rm -rf ~/.local/mtp");
    system("rm -rf /tmp/mtptests");
    QDir( "/tmp/mtptests-second" ).removeRecursively();
}

QTEST_MAIN(FSStoragePlugin_test);
