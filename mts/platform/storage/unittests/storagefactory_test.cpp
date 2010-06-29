/*
 * This file is part of buteo-mtp package
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Sateesh Kavuri <sateesh.kavuri@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "storagefactory_test.h"
#include "storagefactory.h"
#include "mtptypes.h"

void StorageFactory_test::initTestCase()
{
    m_storageFactory = new StorageFactory;
}

void StorageFactory_test::testStorageIds()
{
    MTPResponseCode response;
    QVector<quint32> storageIds;
    response = m_storageFactory->storageIds( storageIds );
    QCOMPARE( response, MTP_RESP_OK );
    QCOMPARE( storageIds.size(), 1 );
    QCOMPARE( storageIds[0], static_cast<quint32>(1) );
}

void StorageFactory_test::testGetObjectHandles()
{
    MTPResponseCode response;
    QVector<ObjHandle> handles;
    quint32 noOfObjects = 0;
    response = m_storageFactory->getObjectHandles( 1, static_cast<MTPObjFormatCode>(0x00000000),
                                                  0, noOfObjects, handles );
    QCOMPARE( response, MTP_RESP_OK );
    QCOMPARE( noOfObjects, static_cast<quint32>(2) );
    QCOMPARE( handles.size(), 2 );
    for( int i = 0 ; i < handles.size(); i++ )
    {
        QString path;
        m_storageFactory->getPath( handles[i], path );
    }
}

void StorageFactory_test::testMaxCapacity()
{
    quint64 maxCapacity;
    MTPResponseCode response = m_storageFactory->maxCapacity( 1, maxCapacity );
    QCOMPARE( response, MTP_RESP_OK );
}

void StorageFactory_test::testFreeSpace()
{
    quint64 freeSpace;
    MTPResponseCode response = m_storageFactory->freeSpace( 1, freeSpace );
    QCOMPARE( response, MTP_RESP_OK );
}

void StorageFactory_test::testStorageDescription()
{
    QString description;
    MTPResponseCode response = m_storageFactory->storageDescription( 1, description );
    QCOMPARE( response, MTP_RESP_OK );
}

void StorageFactory_test::cleanupTestCase()
{
    delete m_storageFactory;
}

QTEST_APPLESS_MAIN(StorageFactory_test);
