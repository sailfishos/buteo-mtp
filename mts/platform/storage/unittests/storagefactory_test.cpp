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
#include "mtpresponder.h"
#include "mtptypes.h"

using namespace meegomtp1dot0;

static const quint32 STORAGE_ID = 0x10001;
static const quint32 INVALID_STORAGE_ID = 0xEEEEE;

void StorageFactory_test::initTestCase()
{
    MTPResponder::instance()->initTransport(DUMMY);
    m_storageFactory = new StorageFactory;

    QVector<quint32> failedStorages;
    QVERIFY(m_storageFactory->enumerateStorages(failedStorages));

    // Remember filesystem root of the first storage.
    QVector<ObjHandle> handles;
    QCOMPARE(m_storageFactory->getObjectHandles(STORAGE_ID, 0, 0xFFFFFFFF,
            handles), static_cast<MTPResponseCode>(MTP_RESP_OK));

    QVERIFY(!handles.isEmpty());

    QCOMPARE(m_storageFactory->getPath(handles.at(0), m_storageRoot),
            static_cast<MTPResponseCode>(MTP_RESP_OK));
    m_storageRoot.truncate(m_storageRoot.lastIndexOf("/") + 1);
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

void StorageFactory_test::testGetDevicePropValueAfterObjectInfoChanged()
{
    QString filePath(QStringLiteral("tmpFsModifyFile"));

    MTPObjectInfo objInfo;
    objInfo.mtpStorageId = 0x00010001;
    objInfo.mtpObjectCompressedSize = 0;
    objInfo.mtpObjectFormat = MTP_OBF_FORMAT_Undefined;
    objInfo.mtpFileName = filePath;
    objInfo.mtpParentObject = 0;

    filePath.prepend(m_storageRoot);
    QFile::remove(filePath);

    QEventLoop loop;
    while (loop.processEvents());

    quint32 storage = STORAGE_ID;
    ObjHandle handle;
    ObjHandle parentHandle;
    QCOMPARE(m_storageFactory->addItem(storage, parentHandle, handle, &objInfo),
            static_cast<MTPResponseCode>(MTP_RESP_OK));

    MtpObjPropDesc desc;
    desc.uPropCode = MTP_OBJ_PROP_Obj_Size;
    desc.uDataType = MTP_DATA_TYPE_UINT64;

    MTPObjPropDescVal value;
    value.propDesc = &desc;

    QList<MTPObjPropDescVal> values;
    values.append(value);

    QCOMPARE(m_storageFactory->getObjectPropertyValue(handle, values),
            static_cast<MTPResponseCode>(MTP_RESP_OK));

    QCOMPARE(values[0].propVal.value<quint64>(), static_cast<quint64>(0));

    QString filePathFromFactory;
    QCOMPARE(m_storageFactory->getPath(handle, filePathFromFactory),
            static_cast<MTPResponseCode>(MTP_RESP_OK));
    QCOMPARE(filePath, filePathFromFactory);

    const QString TEST_STRING("test_string");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(TEST_STRING.toLocal8Bit());
    file.close();

    while (loop.processEvents());

    QCOMPARE(m_storageFactory->getObjectPropertyValue(handle, values),
            static_cast<MTPResponseCode>(MTP_RESP_OK));

    QCOMPARE(values[0].propVal.value<quint64>(),
            static_cast<quint64>(TEST_STRING.size()));

    file.remove();
}

void StorageFactory_test::cleanupTestCase()
{
    delete m_storageFactory;
}

QTEST_MAIN(StorageFactory_test);
