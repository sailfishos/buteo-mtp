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

using namespace meegomtp1dot0;

static const quint32 STORAGE_ID = 0x10001;
static const quint32 INVALID_STORAGE_ID = 0xEEEEE;

void StorageFactory_test::initTestCase()
{
    m_storageFactory = new StorageFactory;
}

void StorageFactory_test::testStorageIds()
{
    QVector<quint32> storageIds;

    QCOMPARE(m_storageFactory->storageIds(storageIds),
            static_cast<MTPResponseCode>(MTP_RESP_OK));

    QVERIFY(storageIds.size() > 0);

    qSort(storageIds);
    QCOMPARE(storageIds.at(0), STORAGE_ID);
}

void StorageFactory_test::testGetObjectHandles()
{
    QVector<ObjHandle> handles;

    // Try invalid storage ID first.
    QCOMPARE(m_storageFactory->getObjectHandles(INVALID_STORAGE_ID,
            static_cast<MTPObjFormatCode>(0x00000000), 0, handles),
            static_cast<MTPResponseCode>(MTP_RESP_InvalidStorageID));

    handles.clear();

    QCOMPARE(m_storageFactory->getObjectHandles(STORAGE_ID,
            static_cast<MTPObjFormatCode>(0x00000000), 0, handles),
            static_cast<MTPResponseCode>(MTP_RESP_OK));

    foreach (ObjHandle handle, handles) {
        QString path;
        QCOMPARE(m_storageFactory->getPath(handle, path),
                static_cast<MTPResponseCode>(MTP_RESP_OK));
    }
}

void StorageFactory_test::cleanupTestCase()
{
    delete m_storageFactory;
}

QTEST_APPLESS_MAIN(StorageFactory_test);
