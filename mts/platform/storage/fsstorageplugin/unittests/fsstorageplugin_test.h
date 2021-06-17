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

#ifndef FSSTORAGEPLUGIN_TEST_H
#define FSSTORAGEPLUGIN_TEST_H

#include <QtTest/QtTest>
#include <QObject>

namespace meegomtp1dot0
{
class StoragePlugin;
class FSStoragePlugin;
}

namespace meegomtp1dot0
{
class FSStoragePlugin_test : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testStorageCreation();
    void testDeleteAll();
    void testObjectHandlesCountAfterCreation();
    void testObjectHandlesAfterCreation();
    void testInvalidGetObjectHandle();
    void testGetObjectHandleByFormat();
    void testObjectInfoAfterCreation();
    void testFindByPath();
    void testObjectHandle();
    void testStorageInfo();
    void testWriteData();
    void testReadData();
    void testAddFile();
    void testAddDir();
    void testObjectHandlesCountAfterAddition();
    void testObjectHandlesAfterAddition();
    void testObjectInfoAfterAddition();
    void testGetObjectPropertyValue();
    void testSetObjectPropertyValue();
    void testGetChildPropertyValues();
    void testSetReferences();
    void testGetReferences();
    void testDeleteFile();
    void testDeleteDir();
    void testObjectHandlesCountAfterDeletion();
    void testObjectHandlesAfterDeletion();
    void testFileCopy();
    void testDirCopy();
    void testFileMove();
    void testFileMoveAcrossStorage();
    void testDirMove();
    void testDirMoveAcrossStorage();
    void testGetLargestPuoid();
    void testTruncateItem();
    void testGetPath();
    void testGetObjectPropertyValueFromStorage();
    void testGetInvalidObjectPropertyValueFromStorage();
    void testInotifyCreate();
    void testInotifyModify();
    void testInotifyMove();
    void testInotifyDelete();
    void testThumbnailer();
    void cleanupTestCase();

private:
    FSStoragePlugin *m_storage;

    void setupPlugin(StoragePlugin *plugin);
};
}
#endif

