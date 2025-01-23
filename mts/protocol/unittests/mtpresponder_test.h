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

#ifndef MTPRESPONDER_TEST_H
#define MTPRESPONDER_TEST_H

#include <QtTest/QtTest>
#include <QObject>
#include "mtptypes.h"

namespace meegomtp1dot0 {
class MTPResponder;
class MTPTransporterDummy;
class MTPTxContainer;
}

namespace meegomtp1dot0 {
class MTPResponder_test : public QObject
{
    Q_OBJECT

public slots:
    void processReceivedData(quint8 *data, quint32 len, bool, bool);

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testCloseSessionBeforeOpen();
    void testOpenSession();
    void testOpenSessionAgain();
    void testGetDeviceInfo();
    void testGetStorageIDs();
    void testGetStorageInfo();
    void testGetNumObjects();
    void testSendObjectPropList();
    void testSendObject();
    void testSendObjectInfo();
    void testSendObject2();
    void testGetObjectHandles();
    void testGetObjectInfo();
    void testGetObjectPropList();
    void testGetObject();
    void testGetObjectPropDesc();
    void testGetDevicePropDesc();
    void testGetDevicePropValue();
    void testSetDevicePropValue();
    void testGetObjectPropsSupported();
    void testGetObjectPropValue();
    void testSetObjectPropValue();
    void testSetObjectPropList();
    void testGetObjectReferences();
    void testSetObjectReferences();
    void testCopyObject();
    void testMoveObject();
    //void testGetThumb();
    //void testGetPartialObject();
    void testDeleteObject();
    void testCloseSession();

private:
    quint32 nextTransactionId();
    void copyAndSendContainer(MTPTxContainer *container);

    MTPResponder *m_responder;
    MTPTransporterDummy *m_transport;
    MTPResponseCode m_responseCode;
    quint32 m_transactionId;
    quint32 m_storageId;
    ObjHandle m_parentHandle, m_objectHandle;
    quint32 m_opcode;
    QList<quint8 *> m_tempBuffers;
};
}

#endif
