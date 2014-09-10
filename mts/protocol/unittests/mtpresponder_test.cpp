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

#include "mtpresponder_test.h"
#include "mtpresponder.h"
#include "storagefactory.h"
#include "mtptransporterdummy.h"
#include "mtptxcontainer.h"
#include "mtprxcontainer.h"
#include <limits>

using namespace meegomtp1dot0;

static void cleanDirs()
{
    QDir(QDir::homePath() + "/.local/mtp").removeRecursively();
}

void MTPResponder_test::copyAndSendContainer(MTPTxContainer *container)
{
    quint8 *buffer = new quint8[container->bufferSize()];
    memcpy(buffer, container->buffer(), container->bufferSize());
    m_responder->receiveContainer(buffer, container->bufferSize(), true, true);
    delete container;
}

void MTPResponder_test::initTestCase()
{
    m_transactionId = 0x00000000;
    m_storageId = 0x00000000;
    m_objectHandle = m_parentHandle = 0x00000000;
    m_responseCode = (MTPResponseCode)MTP_RESP_Undefined;
    cleanDirs();
    m_responder = new MTPResponder();
    bool ok;
    ok = m_responder->initTransport(DUMMY);
    QObject::connect( m_responder->m_transporter, SIGNAL(dataReceived(quint8*, quint32, bool, bool)),this, SLOT(processReceivedData(quint8*, quint32, bool, bool)) );

    /* Process events until storages are initialized. */
    m_responder->initStorages();
    QEventLoop eventLoop;
    connect(m_responder->m_storageServer, &StorageFactory::storageReady,
            &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
}

void MTPResponder_test::cleanupTestCase()
{
    delete m_responder;
    system("rm -rf /tmp/mtptests");
    cleanDirs();
}

quint32 MTPResponder_test::nextTransactionId()
{
    return ++m_transactionId == 0xFFFFFFFF ? m_transactionId = 0x00000001 : m_transactionId;
}

void MTPResponder_test::processReceivedData( quint8* data, quint32 len, bool isFirstPacket,
                                             bool isLastPacket )
{
    MTPRxContainer container(data, len);

    // Store response code.
    m_responseCode = container.code();

    // We need the response payload to test sendObject that succeeds sendObjectPropList/sendObjectInfo
    if( MTP_OP_SendObjectInfo == m_opcode || MTP_OP_SendObjectPropList == m_opcode )
    {
        container >> m_storageId;
        container >> m_parentHandle;
        container >> m_objectHandle;
        m_opcode = 0xFFFF;
    }
}

void MTPResponder_test::testCloseSessionBeforeOpen()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_CloseSession, 0x00000001);
    m_responder->m_transactionSequence->reqContainer = reinterpret_cast<MTPRxContainer*>(reqContainer);
    m_responder->closeSessionReq();
    // The slot MTPResponder_test::processReceivedData should have been called by now
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_SessionNotOpen );
}

void MTPResponder_test::testOpenSession()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_OpenSession, 0x00000001, sizeof(quint32));
    *reqContainer << (quint32)0x00000001;
    copyAndSendContainer(reqContainer);
    // The slot MTPResponder_test::processReceivedData should have been called by now
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testOpenSessionAgain()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_OpenSession, 0x00000001, sizeof(quint32));
    *reqContainer << (quint32)0x00000001;
    copyAndSendContainer(reqContainer);
    // The slot MTPResponder_test::processReceivedData should have been called by now
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_SessionAlreadyOpen );
}

void MTPResponder_test::testGetDeviceInfo()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetDeviceInfo, nextTransactionId());
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetStorageIDs()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetStorageIDs, nextTransactionId());
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetStorageInfo()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetStorageIDs, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)0x00010001;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetNumObjects()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetNumObjects, nextTransactionId(), 3 * sizeof(quint32));
    *reqContainer << (quint32)0x00010001 << (quint32)0x00000000 << (quint32)0x00000000;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetObjectHandles()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectHandles, nextTransactionId(), 3 * sizeof(quint32));
    *reqContainer << (quint32)0x00010001 << (quint32)0x00000000 << (quint32)0x00000000;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testSendObjectPropList()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_SendObjectPropList, nextTransactionId(), 5 * sizeof(quint32));
    *reqContainer << (quint32)0x00000000 << (quint32)0x00000000 << (quint32)MTP_OBF_FORMAT_Text << (quint32)0x00000000 << (quint32)0x00000005;
    copyAndSendContainer(reqContainer);

    quint8* objPropListData = 0;
    quint32 offset = 0;
    quint32 noOfElements = 1;
    ObjHandle handle = 0x00000000;
    quint16 propCode = MTP_OBJ_PROP_Obj_File_Name;
    quint16 datatype = MTP_DATA_TYPE_STR;
    QString value = "tmpfile";

    quint32 payloadLength = sizeof(quint32) + sizeof(ObjHandle) + ( 2 * sizeof(quint16) ) +
                           ( ( value.size() + 1 ) * 2 );
    MTPTxContainer *dataContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_DATA, MTP_OP_SendObjectPropList, m_transactionId, payloadLength);
    *dataContainer << noOfElements << handle << propCode << datatype << value;
    m_opcode = MTP_OP_SendObjectPropList;
    copyAndSendContainer(dataContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testSendObject()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_SendObject, nextTransactionId());
    copyAndSendContainer(reqContainer);

    char tmp[6] = "xxxxx";

    MTPTxContainer *dataContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_DATA, MTP_OP_SendObject, m_transactionId, 5);
    memcpy( dataContainer->payload(), tmp, 5 );
    dataContainer->seek(5);
    copyAndSendContainer(dataContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}


void MTPResponder_test::testSendObjectInfo()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_SendObjectInfo, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << (quint32)0x00010001 << (quint32)0xFFFFFFFF;
    copyAndSendContainer(reqContainer);
    MTPObjectInfo objInfo;
    objInfo.mtpStorageId = 0x00010001;
    objInfo.mtpObjectCompressedSize = 5;
    objInfo.mtpThumbCompressedSize = 0;
    objInfo.mtpThumbPixelWidth = 0;
    objInfo.mtpThumbPixelHeight = 0;
    objInfo.mtpImagePixelWidth = 0;
    objInfo.mtpImagePixelHeight = 0;
    objInfo.mtpImageBitDepth = 0;
    objInfo.mtpParentObject = 0;
    objInfo.mtpAssociationDescription = 0;
    objInfo.mtpSequenceNumber = 0;
    objInfo.mtpObjectFormat = MTP_OBF_FORMAT_Text;
    objInfo.mtpProtectionStatus = 0;
    objInfo.mtpThumbFormat = 0;
    objInfo.mtpAssociationType = 0;
    objInfo.mtpFileName = "tmpfile1";
    objInfo.mtpCaptureDate = "20090101T230000";
    objInfo.mtpModificationDate = "20090101T230000";

    quint32 payloadLength = sizeof(MTPObjectInfo); // approximation

    MTPTxContainer *dataContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_DATA, MTP_OP_SendObjectInfo, m_transactionId, payloadLength);
    *dataContainer << objInfo;
    m_opcode = MTP_OP_SendObjectInfo;
    copyAndSendContainer(dataContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testSendObject2()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_SendObject, nextTransactionId());
    copyAndSendContainer(reqContainer);

    char tmp[6] = "yyyyy";

    MTPTxContainer *dataContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_DATA, MTP_OP_SendObject, m_transactionId, 5);
    memcpy( dataContainer->payload(), tmp, 5 );
    dataContainer->seek(5);
    copyAndSendContainer(dataContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetObjectInfo()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectInfo, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetObjectPropList()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropList, nextTransactionId(), 5 * sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle << (quint32)0x00000000 << (quint32)0xFFFFFFFF << (quint32)0x00000000 << (quint32)0x00000000;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetObject()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObject, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetObjectPropDesc()
{
    MTPTxContainer *reqContainer = 0;

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_StorageID << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Obj_Format << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Protection_Status << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Obj_Size << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Obj_File_Name << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Date_Created << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Date_Modified << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Parent_Obj << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Persistent_Unique_ObjId << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Name << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Non_Consumable << MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Width << MTP_OBF_FORMAT_EXIF_JPEG;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Height << MTP_OBF_FORMAT_EXIF_JPEG;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Rep_Sample_Width << MTP_OBF_FORMAT_EXIF_JPEG;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Rep_Sample_Height << MTP_OBF_FORMAT_EXIF_JPEG;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Rep_Sample_Format << MTP_OBF_FORMAT_EXIF_JPEG;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Rep_Sample_Data << MTP_OBF_FORMAT_EXIF_JPEG;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Artist << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Album_Name << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Track << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Genre << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Use_Count << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Duration << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Bitrate_Type << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Sample_Rate << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Nbr_Of_Channels << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Audio_WAVE_Codec << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Audio_BitRate << MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Sample_Rate << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Video_FourCC_Codec << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Video_BitRate << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Frames_Per_Thousand_Secs << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Genre << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Nbr_Of_Channels << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Audio_WAVE_Codec << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Audio_BitRate << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Width << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Height << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Artist << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Album_Name << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Use_Count << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Duration << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropDesc, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << MTP_OBJ_PROP_Track << MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetDevicePropDesc()
{
    MTPTxContainer *reqContainer = 0;
    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetDevicePropDesc, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_DEV_PROPERTY_Device_Friendly_Name;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetDevicePropDesc, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_DEV_PROPERTY_Synchronization_Partner;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetDevicePropDesc, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_DEV_PROPERTY_Perceived_Device_Type;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetDevicePropDesc, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_DEV_PROPERTY_DeviceIcon;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetDevicePropValue()
{
    MTPTxContainer *reqContainer = 0;

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetDevicePropValue, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_DEV_PROPERTY_Device_Friendly_Name;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetDevicePropValue, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_DEV_PROPERTY_Synchronization_Partner;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetDevicePropValue, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_DEV_PROPERTY_Perceived_Device_Type;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetDevicePropValue, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_DEV_PROPERTY_DeviceIcon;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testSetDevicePropValue()
{
    MTPTxContainer *reqContainer = 0;
    MTPTxContainer *dataContainer = 0;

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_SetDevicePropValue, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_DEV_PROPERTY_Device_Friendly_Name;
    copyAndSendContainer(reqContainer);

    quint32 payloadLength = 0;
    QString value = "IAmYourFriend";
    payloadLength = ((value.length() + 1) * sizeof(quint16)) + sizeof(quint8);
    dataContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_DATA, MTP_OP_SetDevicePropValue, m_transactionId, payloadLength);
    *dataContainer << value;
    copyAndSendContainer(dataContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_SetDevicePropValue, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_DEV_PROPERTY_Synchronization_Partner;
    copyAndSendContainer(reqContainer);

    payloadLength = 0;
    value = "Unit Test";
    payloadLength = ((value.length() + 1) * sizeof(quint16)) + sizeof(quint8);
    dataContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_DATA, MTP_OP_SetDevicePropValue, m_transactionId, payloadLength);
    *dataContainer << value;
    copyAndSendContainer(dataContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetObjectPropsSupported()
{
    MTPTxContainer *reqContainer = 0;

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropsSupported, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_OBF_FORMAT_MP3;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropsSupported, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_OBF_FORMAT_EXIF_JPEG;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropsSupported, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_OBF_FORMAT_AVI;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );

    reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropsSupported, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)MTP_OBF_FORMAT_Text;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetObjectPropValue()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectPropValue, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle << (quint32)MTP_OBJ_PROP_Obj_Size;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testSetObjectPropValue()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_SetObjectPropValue, nextTransactionId(), 2 * sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle << (quint32)MTP_OBJ_PROP_Obj_File_Name;
    copyAndSendContainer(reqContainer);

    quint32 offset = 0;
    QString value = "newname";
    quint32 payloadLength = ((value.length() + 1) * sizeof(quint16)) + sizeof(quint8);
    MTPTxContainer *dataContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_DATA, MTP_OP_SetObjectPropValue, m_transactionId, payloadLength);
    *dataContainer << value;
    copyAndSendContainer(dataContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testSetObjectPropList()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_SetObjectPropList, nextTransactionId());
    copyAndSendContainer(reqContainer);

    quint32 noOfElements = 1;
    ObjHandle handle = m_objectHandle;
    quint16 propCode = MTP_OBJ_PROP_Obj_File_Name;
    quint16 datatype = MTP_DATA_TYPE_STR;
    QString value = "newname2";

    quint32 payloadLength = sizeof(quint32) + sizeof(ObjHandle) + ( 2 * sizeof(quint16) ) +
                           ( ( value.size() + 1 ) * 2 ) + sizeof(quint8);


    MTPTxContainer *dataContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_DATA, MTP_OP_SetObjectPropList, m_transactionId, payloadLength);
    *dataContainer << noOfElements << handle << propCode << datatype << value;
    copyAndSendContainer(dataContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testGetObjectReferences()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetObjectReferences, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testSetObjectReferences()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_SetObjectReferences, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle;
    copyAndSendContainer(reqContainer);
    QVector<ObjHandle> theRefs;
    theRefs.append(std::numeric_limits<ObjHandle>::max());
    quint32 payloadLength = theRefs.size() * sizeof(quint32) + sizeof(quint32);
    MTPTxContainer *dataContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_DATA, MTP_OP_SetObjectReferences, m_transactionId, payloadLength);
    *dataContainer << theRefs;
    copyAndSendContainer(dataContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_Invalid_ObjectReference );
}

void MTPResponder_test::testCopyObject()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_CopyObject, nextTransactionId(), 3 * sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle << (quint32)m_storageId << (quint32)0x00000000;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testMoveObject()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_MoveObject, nextTransactionId(), 3 * sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle << (quint32)m_storageId << (quint32)m_parentHandle;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_InvalidParentObject );
}

#if 0
// The below operations aren't implemnted.
void MTPResponder_test::testGetThumb()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetThumb, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_InvalidObjectFormatCode );
}

void MTPResponder_test::testGetPartialObject()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_GetPartialObject, nextTransactionId(), 3 * sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle << (quint32)0x00000001 << (quint32)0x00000001;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}
#endif

void MTPResponder_test::testDeleteObject()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_DeleteObject, nextTransactionId(), sizeof(quint32));
    *reqContainer << (quint32)m_objectHandle;
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

void MTPResponder_test::testCloseSession()
{
    MTPTxContainer *reqContainer = new MTPTxContainer(MTP_CONTAINER_TYPE_COMMAND, MTP_OP_CloseSession, nextTransactionId());
    copyAndSendContainer(reqContainer);
    QCOMPARE( m_responseCode, (MTPResponseCode)MTP_RESP_OK );
}

QTEST_MAIN(MTPResponder_test);
