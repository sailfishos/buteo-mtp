/*
* This file is part of libmeegomtp package
*
* Copyright (C) 2010 Nokia Corporation. All rights reserved.
*
* Contact: Santosh Puranik <santosh.puranik@nokia.com>
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

#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>
#include <QtAlgorithms>
#include <qglobal.h>

#include "mtpresponder.h"
#include "mtpcontainerwrapper.h"
#include "mtptxcontainer.h"
#include "mtprxcontainer.h"
#include "mtpevent.h"
#include "storagefactory.h"
#include "trace.h"
#include "deviceinfoprovider.h"
#include "mtptransporterusb.h"
#include "mtptransporterdummy.h"
#include "propertypod.h"
#include "objectpropertycache.h"
#include "mtpextensionmanager.h"

using namespace meegomtp1dot0;

// BUFFER_MAX_LEN is based on the max request size in ci13xxx_udc.c,
// which is four pages of 4k each
static const quint32 BUFFER_MAX_LEN = 4 * 4096;
MTPResponder* MTPResponder::m_instance = 0;

MTPResponder* MTPResponder::instance()
{
    MTP_FUNC_TRACE();
    if(0 == m_instance)
    {
        /* Do all the metatype registration here to ensure we have all the
         * needed symbols defined in libmtp-server.so rather than in some
         * storage plugin that doesn't have to be loaded all the time. */
        qRegisterMetaType<char>();
        qRegisterMetaType<MtpInt128>();
        qRegisterMetaType<MTPEventCode>();
        qRegisterMetaType<MtpEnumForm>();
        qRegisterMetaType<MtpRangeForm>();

        qRegisterMetaType<QVector<char> >();
        qRegisterMetaType<QVector<qint8> >();
        qRegisterMetaType<QVector<qint16> >();
        qRegisterMetaType<QVector<qint32> >();
        qRegisterMetaType<QVector<qint64> >();
        qRegisterMetaType<QVector<quint8> >();
        qRegisterMetaType<QVector<quint16> >();
        qRegisterMetaType<QVector<quint32> >();
        qRegisterMetaType<QVector<quint64> >();
        qRegisterMetaType<QVector<MtpInt128> >();

        m_instance = new MTPResponder();
    }
    return m_instance;
}

MTPResponder::MTPResponder(): m_storageServer(0),
    m_transporter(0),
    m_devInfoProvider(new DeviceInfoProvider),
    m_propertyPod(PropertyPod::instance(m_devInfoProvider, m_extensionManager)),
    m_propCache(ObjectPropertyCache::instance()),
    m_extensionManager(new MTPExtensionManager),
    m_copiedObjHandle(0),
    m_containerToBeResent(false),
    m_isLastPacket(false),
    m_state(RESPONDER_IDLE),
    m_prevState(RESPONDER_IDLE),
    m_objPropListInfo(0),
    m_sendObjectSequencePtr(0),
    m_transactionSequence(new MTPTransactionSequence)
{
    MTP_FUNC_TRACE();

    createCommandHandler();
}

bool MTPResponder::initTransport( TransportType transport )
{
    bool transportOk = true;
    if(USB == transport)
    {
        m_transporter = new MTPTransporterUSB();
        transportOk = m_transporter->activate();
        if( transportOk )
        {
            // Connect signals from the transporter
            QObject::connect(m_transporter, SIGNAL(dataReceived(quint8*, quint32, bool, bool)),
                             this, SLOT(receiveContainer(quint8*, quint32, bool, bool)));
            QObject::connect(m_transporter, SIGNAL(eventReceived()), this, SLOT(receiveEvent()));
            QObject::connect(m_transporter, SIGNAL(cleanup()), this, SLOT(closeSession()));
            QObject::connect(m_transporter, SIGNAL(fetchObjectSize(const quint8*, quint64*)),
                             this, SLOT(fetchObjectSize(const quint8*, quint64*)));
            QObject::connect(this, SIGNAL(deviceStatusOK()), m_transporter, SLOT(sendDeviceOK()));
            QObject::connect(this, SIGNAL(deviceStatusBusy()), m_transporter, SLOT(sendDeviceBusy()));
            QObject::connect(this, SIGNAL(deviceStatusTxCancelled()), m_transporter, SLOT(sendDeviceTxCancelled()));
            QObject::connect(m_transporter, SIGNAL(cancelTransaction()), this, SLOT(handleCancelTransaction()));
            QObject::connect(m_transporter, SIGNAL(deviceReset()), this, SLOT(handleDeviceReset()));
            QObject::connect(m_transporter, SIGNAL(suspendSignal()), this, SLOT(handleSuspend()));
            QObject::connect(m_transporter, SIGNAL(resumeSignal()), this, SLOT(handleResume()));
        }
    }
    else if (DUMMY == transport)
    {
        m_transporter = new MTPTransporterDummy();
    }
    emit deviceStatusOK();
    return transportOk;
}

bool MTPResponder::initStorages()
{
    m_storageServer = new StorageFactory();

    QObject::connect(m_storageServer, SIGNAL(checkTransportEvents(bool&)), this, SLOT(processTransportEvents(bool&)));

    //TODO What action to take if enumeration fails?
    QVector<quint32> failedStorageIds;
    bool result = m_storageServer->enumerateStorages( failedStorageIds );
    if(false == result)
    {
        MTP_LOG_CRITICAL("Failed to enumerate storages");
    }
    return result;
}

MTPResponder::~MTPResponder()
{
    MTP_FUNC_TRACE();

    if(m_storageServer)
    {
        delete m_storageServer;
        m_storageServer = 0;
    }

    if(m_transporter)
    {
        delete m_transporter;
        m_transporter = 0;
    }

    if(m_propertyPod)
    {
        PropertyPod::releaseInstance();
        m_propertyPod = 0;
    }

    if(m_devInfoProvider)
    {
        delete m_devInfoProvider;
        m_devInfoProvider = 0;
    }

    if( m_transactionSequence )
    {
        deleteStoredRequest();
        delete m_transactionSequence;
        m_transactionSequence = 0;
    }

    if(m_extensionManager)
    {
        delete m_extensionManager;
        m_extensionManager = 0;
    }

    if( m_sendObjectSequencePtr )
    {
        delete m_sendObjectSequencePtr;
        m_sendObjectSequencePtr = 0;
    }

    freeObjproplistInfo();

    m_instance = 0;
}

void MTPResponder::createCommandHandler()
{
    MTP_FUNC_TRACE();
    m_opCodeTable[MTP_OP_GetDeviceInfo] = &MTPResponder::getDeviceInfoReq;
    m_opCodeTable[MTP_OP_OpenSession] = &MTPResponder::openSessionReq;
    m_opCodeTable[MTP_OP_CloseSession] = &MTPResponder::closeSessionReq;
    m_opCodeTable[MTP_OP_GetStorageIDs] = &MTPResponder::getStorageIDReq;
    m_opCodeTable[MTP_OP_GetStorageInfo] = &MTPResponder::getStorageInfoReq;
    m_opCodeTable[MTP_OP_GetNumObjects] = &MTPResponder::getNumObjectsReq;
    m_opCodeTable[MTP_OP_GetObjectHandles] = &MTPResponder::getObjectHandlesReq;
    m_opCodeTable[MTP_OP_GetObjectInfo] = &MTPResponder::getObjectInfoReq;
    m_opCodeTable[MTP_OP_GetObject] = &MTPResponder::getObjectReq;
    m_opCodeTable[MTP_OP_GetThumb] = &MTPResponder::getThumbReq;
    m_opCodeTable[MTP_OP_DeleteObject] = &MTPResponder::deleteObjectReq;
    m_opCodeTable[MTP_OP_SendObjectInfo] = &MTPResponder::sendObjectInfoReq;
    m_opCodeTable[MTP_OP_SendObject] = &MTPResponder::sendObjectReq;
    m_opCodeTable[MTP_OP_GetPartialObject] = &MTPResponder::getPartialObjectReq;
    m_opCodeTable[MTP_OP_SetObjectProtection] = &MTPResponder::setObjectProtectionReq;
    m_opCodeTable[MTP_OP_GetDevicePropDesc] = &MTPResponder::getDevicePropDescReq;
    m_opCodeTable[MTP_OP_GetDevicePropValue] = &MTPResponder::getDevicePropValueReq;
    m_opCodeTable[MTP_OP_SetDevicePropValue] = &MTPResponder::setDevicePropValueReq;
    m_opCodeTable[MTP_OP_ResetDevicePropValue] = &MTPResponder::resetDevicePropValueReq;
    m_opCodeTable[MTP_OP_MoveObject] = &MTPResponder::moveObjectReq;
    m_opCodeTable[MTP_OP_CopyObject] = &MTPResponder::copyObjectReq;
    m_opCodeTable[MTP_OP_GetObjectPropsSupported] = &MTPResponder::getObjPropsSupportedReq;
    m_opCodeTable[MTP_OP_GetObjectPropDesc] = &MTPResponder::getObjPropDescReq;
    m_opCodeTable[MTP_OP_GetObjectPropValue] = &MTPResponder::getObjPropValueReq;
    m_opCodeTable[MTP_OP_SetObjectPropValue] = &MTPResponder::setObjPropValueReq;
    m_opCodeTable[MTP_OP_GetObjectPropList] = &MTPResponder::getObjectPropListReq;
    m_opCodeTable[MTP_OP_SetObjectPropList] = &MTPResponder::setObjectPropListReq;
    m_opCodeTable[MTP_OP_GetInterdependentPropDesc] = &MTPResponder::getInterdependentPropDescReq;
    m_opCodeTable[MTP_OP_SendObjectPropList] = &MTPResponder::sendObjectPropListReq;
    m_opCodeTable[MTP_OP_GetObjectReferences] = &MTPResponder::getObjReferencesReq;
    m_opCodeTable[MTP_OP_SetObjectReferences] = &MTPResponder::setObjReferencesReq;
    m_opCodeTable[MTP_OP_Skip] = &MTPResponder::skipReq;
}

//TODO This returns false now if a cancel txn was received. If we have other reasons because of which the
// container won't be sent, then apart from failure, we will also need a reason.
bool MTPResponder::sendContainer(MTPTxContainer &container, bool isLastPacket)
{
    MTP_FUNC_TRACE();

    MTP_LOG_INFO("Sending container of type:: " << QString("0x%1").arg(container.containerType(), 0, 16));
    MTP_LOG_INFO("Code:: " << QString("0x%1").arg(container.code(), 0, 16));

    if(MTP_CONTAINER_TYPE_RESPONSE == container.containerType() || MTP_CONTAINER_TYPE_DATA == container.containerType() ||
       MTP_CONTAINER_TYPE_EVENT == container.containerType() )
    {
        //m_transporter->disableRW();
        //QCoreApplication::processEvents();
        //m_transporter->enableRW();
        if( RESPONDER_TX_CANCEL == m_state && MTP_CONTAINER_TYPE_EVENT != container.containerType())
        {
            return false;
        }
        if( RESPONDER_SUSPEND == m_state )
        {
            MTP_LOG_WARNING("Received suspend while sending");
            if( MTP_CONTAINER_TYPE_EVENT != container.containerType() )
            {
                MTP_LOG_WARNING("Received suspend while sending data/response, wait for resume");
                m_containerToBeResent = true;
                m_resendBuffer = new quint8[container.bufferSize()];
                memcpy( m_resendBuffer, container.buffer(), container.bufferSize() );
                m_resendBufferSize = container.bufferSize();
                m_isLastPacket = isLastPacket;
            }
            return false;
        }
    }

    // send container
    if(MTP_CONTAINER_TYPE_EVENT == container.containerType())
    {
        m_transporter->sendEvent(container.buffer(), container.bufferSize(), isLastPacket);
    }
    else
    {
        m_transporter->sendData(container.buffer(), container.bufferSize(), isLastPacket);
    }
    if(MTP_CONTAINER_TYPE_RESPONSE == container.containerType())
    {
        // Restore state to IDLE to get ready to received the next operation
        emit deviceStatusOK();
        m_state = RESPONDER_IDLE;
        deleteStoredRequest();
    }
    return true;
}

bool MTPResponder::sendResponse(MTPResponseCode code)
{
    MTP_FUNC_TRACE();

    if (0 == m_transactionSequence->reqContainer)
    {
        MTP_LOG_WARNING("Transaction gone, not sending response");
        return false;
    }

    quint16 transactionId = m_transactionSequence->reqContainer->transactionId();
    MTPTxContainer respContainer(MTP_CONTAINER_TYPE_RESPONSE, code, transactionId);
    bool sent = sendContainer(respContainer);
    if( false == sent )
    {
        MTP_LOG_CRITICAL("Could not send response");
    }
    return sent;
}

bool MTPResponder::sendResponse(MTPResponseCode code, quint32 param1)
{
    MTP_FUNC_TRACE();

    quint16 transactionId = m_transactionSequence->reqContainer->transactionId();
    MTPTxContainer respContainer(MTP_CONTAINER_TYPE_RESPONSE, code, transactionId, sizeof(param1));
    respContainer << param1;
    bool sent = sendContainer(respContainer);
    if( false == sent )
    {
        MTP_LOG_CRITICAL("Could not send response");
    }
    return sent;
}

void MTPResponder::receiveContainer(quint8* data, quint32 dataLen, bool isFirstPacket, bool isLastPacket)
{
    MTP_FUNC_TRACE();
    // TODO: Change this to handle event containers. Event containers can be
    // received in any state.
    switch(m_state)
    {
        case RESPONDER_IDLE:
        case RESPONDER_TX_CANCEL:
        case RESPONDER_SUSPEND:
            {
                m_state = RESPONDER_IDLE;
                // Delete any old request, just in case
                if(0 != m_transactionSequence->reqContainer)
                {
                    delete m_transactionSequence->reqContainer;
                    m_transactionSequence->reqContainer = 0;
                }
                // This must be a request container, request containers cannot
                // be segmented!
                if(isFirstPacket && isLastPacket)
                {
                    // Populate our internal request container
                    m_transactionSequence->reqContainer = new MTPRxContainer(data, dataLen);
                    // Check if the operation has a data phase
                    if(hasDataPhase(m_transactionSequence->reqContainer->code()))
                    {
                        // Change state to expect an (optional) data phase from the
                        // initiator. We need to let the data phase complete even if
                        // there is an error in the request phase, before entering
                        // the response phase.
                        m_state = RESPONDER_WAIT_DATA;
                    }
                    else
                    {
                        m_state = RESPONDER_WAIT_RESP;
                    }

                    // Handle the command phase
                    emit deviceStatusBusy();
                    commandHandler();
                }
                else
                {
                    m_state = RESPONDER_IDLE;
                    MTP_LOG_CRITICAL("Invalid container received! Expected command, received data");
                    m_transporter->reset();
                }
            }
            break;
        case RESPONDER_WAIT_DATA:
            {
                // If no valid request container, then exit...
                if(0 == m_transactionSequence->reqContainer)
                {
                    m_state = RESPONDER_IDLE;
                    MTP_LOG_CRITICAL("Received a data container before a request container!");
                    m_transporter->reset();
                    break;
                }
                if( isFirstPacket )
                {
                    emit deviceStatusBusy();
                }
                // This must be a data container
                dataHandler(data, dataLen, isFirstPacket, isLastPacket);
            }
            break;
        case RESPONDER_WAIT_RESP:
        default:
            MTP_LOG_CRITICAL("Container received in wrong state!" << m_state);
            m_state = RESPONDER_IDLE;
            m_transporter->reset();
            break;
    }
    //delete [] data;
}

void MTPResponder::commandHandler()
{
    MTP_FUNC_TRACE();

    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    QVector<quint32> params;

    bool waitForDataPhase = false;

    MTP_LOG_INFO("MTP OPERATION:::: " << QString("0x%1").arg(reqContainer->code(), 0, 16));

    reqContainer->params(params);

    foreach(quint32 param, params)
    {
        MTP_LOG_INFO("Param = " << QString("0x%1").arg(param, 0, 16));
    }

    // preset the response code - to be changed if the handler of the operation
    // detects an error in the operation phase
    m_transactionSequence->mtpResp = MTP_RESP_OK;

    if(m_opCodeTable.contains(reqContainer->code()))
    {
        (this->*(m_opCodeTable[reqContainer->code()]))();
    }
    else if (true == m_extensionManager->operationHasDataPhase(reqContainer->code(), waitForDataPhase))
    {
        // Operation handled by an extension
        if(false == waitForDataPhase)
        {
            // TODO: Check the return below? Can we assume that this will succeed as operationHasDataPhase has succeeded?
            //handleExtendedOperation();
            sendResponse(MTP_RESP_OperationNotSupported);
        }
    }
    else
    {
        // invalid opcode -- This response should be sent right away (no need to
        // wait on the data phase)
        sendResponse(MTP_RESP_OperationNotSupported);
    }
}

bool MTPResponder::hasDataPhase(MTPOperationCode code)
{
    bool ret = false;
    switch(code)
    {
        case MTP_OP_SendObjectInfo:
        case MTP_OP_SendObject:
        case MTP_OP_SetObjectPropList:
        case MTP_OP_SendObjectPropList:
        case MTP_OP_SetDevicePropValue:
        case MTP_OP_SetObjectPropValue:
        case MTP_OP_SetObjectReferences:
            ret = true;
            break;
        case MTP_OP_GetDeviceInfo:
        case MTP_OP_OpenSession:
        case MTP_OP_CloseSession:
        case MTP_OP_GetStorageIDs:
        case MTP_OP_GetStorageInfo:
        case MTP_OP_GetNumObjects:
        case MTP_OP_GetObjectHandles:
        case MTP_OP_GetObjectInfo:
        case MTP_OP_GetObject:
        case MTP_OP_GetThumb:
        case MTP_OP_DeleteObject:
        case MTP_OP_InitiateCapture:
        case MTP_OP_FormatStore:
        case MTP_OP_ResetDevice:
        case MTP_OP_SelfTest:
        case MTP_OP_SetObjectProtection:
        case MTP_OP_PowerDown:
        case MTP_OP_GetDevicePropDesc:
        case MTP_OP_GetDevicePropValue:
        case MTP_OP_ResetDevicePropValue:
        case MTP_OP_TerminateOpenCapture:
        case MTP_OP_MoveObject:
        case MTP_OP_CopyObject:
        case MTP_OP_GetPartialObject:
        case MTP_OP_InitiateOpenCapture:
        case MTP_OP_GetObjectPropsSupported:
        case MTP_OP_GetObjectPropDesc:
        case MTP_OP_GetObjectPropValue:
        case MTP_OP_GetObjectPropList:
        case MTP_OP_GetInterdependentPropDesc:
        case MTP_OP_GetObjectReferences:
        case MTP_OP_Skip:
            ret = false;
            break;
        default:
            // Try to check with the extension manager
            m_extensionManager->operationHasDataPhase(code, ret);
    }
    return ret;
}

void MTPResponder::dataHandler(quint8* data, quint32 dataLen, bool isFirstPacket, bool isLastPacket)
{
    MTP_FUNC_TRACE();
    MTPResponseCode respCode =  MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    if(MTP_OP_SendObject != m_transactionSequence->reqContainer->code())
    {
        if(isFirstPacket)
        {
            // Delete any old data container
            if(0 != m_transactionSequence->dataContainer)
            {
                delete m_transactionSequence->dataContainer;
                m_transactionSequence->dataContainer = 0;
            }
            // Create a new data container
            m_transactionSequence->dataContainer = new MTPRxContainer(data, dataLen);
        }
        else if(m_transactionSequence->dataContainer)
        {
            // Call append on the previously stored data container
            m_transactionSequence->dataContainer->append(data, dataLen);
        }
        if(false == isLastPacket)
        {
            // Return here and wait for the next segment in the data phase
            return;
        }
    }
    // check if an error was already detected in the operation request phase
    if(MTP_RESP_OK != m_transactionSequence->mtpResp)
    {
        respCode = m_transactionSequence->mtpResp;
    }
    // Match the transaction IDs from the request phase
    else if(m_transactionSequence->dataContainer && (m_transactionSequence->dataContainer->transactionId() != reqContainer->transactionId()))
    {
        respCode = MTP_RESP_InvalidTransID;
    }
        // check whether the opcode of the data container corresponds to the one of the operation phase
    else if(m_transactionSequence->dataContainer && (m_transactionSequence->dataContainer->code() != reqContainer->code()))
    {
        respCode = MTP_RESP_GeneralError;
    }

    if(MTP_RESP_OK == respCode)
    {
        switch(reqContainer->code())
        {
            // For operations with no I->R data phases, we should never get
            // here!
            case MTP_OP_SendObjectInfo:
                {
                    sendObjectInfoData();
                    return;
                }
            case MTP_OP_SendObject:
                {
                    sendObjectData(data, dataLen, isFirstPacket, isLastPacket);
                    return;
                }
            case MTP_OP_SetObjectPropList:
                {
                    setObjectPropListData();
                    return;
                }
            case MTP_OP_SendObjectPropList:
                {
                    sendObjectPropListData();
                    return;
                }
            case MTP_OP_SetDevicePropValue:
                {
                    setDevicePropValueData();
                    return;
                }
            case MTP_OP_SetObjectPropValue:
                {
                    setObjPropValueData();
                    return;
                }
            case MTP_OP_SetObjectReferences:
                {
                    setObjReferencesData();
                    return;
                }
            default:
                {
                    respCode = MTP_RESP_OperationNotSupported;
                    /*if(true == handleExtendedOperation())
                    {
                        // Response has been sent from the extended operation handler
                        return;
                    }
                    respCode = MTP_RESP_InvalidCodeFormat;*/
                    break;
                }
        }
    }
    // RESPONSE PHASE
    sendResponse(respCode);
}

bool MTPResponder::handleExtendedOperation()
{
    MTP_FUNC_TRACE();
    bool ret = false;
    if(m_transactionSequence && m_transactionSequence->reqContainer)
    {
        MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
        MTPRxContainer *dataContainer = m_transactionSequence->dataContainer;
        MtpRequest req;
        MtpResponse resp;

        req.opCode = reqContainer->code();
        reqContainer->params(req.params);
        if(0 != dataContainer)
        {
            req.data = dataContainer->payload();
            req.dataLen = (dataContainer->containerLength() - MTP_HEADER_SIZE);
        }
        if(true == (ret = m_extensionManager->handleOperation(req, resp)))
        {
            // Extension handled this operation successfully, send the data (if present)
            // and the response
            if(0 != resp.data && 0 < resp.dataLen)
            {
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), resp.dataLen);
                memcpy(dataContainer.payload(), resp.data, resp.dataLen);
                dataContainer.seek(resp.dataLen);
                bool sent = sendContainer(dataContainer);
                if( false == sent )
                {
                    MTP_LOG_CRITICAL("Could not send data");
                }
                delete[] resp.data;
            }
            // Send response
            MTPTxContainer respContainer(MTP_CONTAINER_TYPE_RESPONSE, resp.respCode, reqContainer->transactionId(), resp.params.size() * sizeof(MtpParam));
            for(int i = 0; i < resp.params.size(); i++)
            {
                respContainer << resp.params[i];
            }
            bool sent = sendContainer(respContainer);
            if( false == sent )
            {
                MTP_LOG_CRITICAL("Could not send response");
            }
        }
    }
    return ret;
}

void MTPResponder::eventHandler()
{
    MTP_FUNC_TRACE();
    // TODO: This needs to be implemented
#if 0
    // check what kind of event has been received
    switch()
    {
        case MTP_EV_CancelTransaction:
            // FIXME: Call correct handler here!!
            break;
        default:
            break;
    }
#endif
}

void MTPResponder::getDeviceInfoReq()
{
    MTP_FUNC_TRACE();
    quint32 payloadLength = 0;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // Standard Version
    quint16 stdVer = m_devInfoProvider->standardVersion();
    // Vendor Extension ID
    quint32 vendorExtId = m_devInfoProvider->vendorExtension();
    // Vendor Extendion Version
    quint16 vendorExtVer = m_devInfoProvider->MTPVersion();
    // Functional Mode
    quint16 funcMode = m_devInfoProvider->functionalMode();
    // Operations Supported (array)
    QVector<quint16> opsSupported = m_devInfoProvider->MTPOperationsSupported();
    // Events Supported (array)
    QVector<quint16> evsSupported = m_devInfoProvider->MTPEventsSupported();
    // Device Properties Supported (array)
    QVector<quint16> propsSupported = m_devInfoProvider->MTPDevicePropertiesSupported();

    // Capture Formats (array)
    quint32 captureFormatsNumElem = 0;
    quint32 captureFormatsLen = 0;

    // Image Formats (array)
    QVector<quint16> imageFormats = m_devInfoProvider->supportedFormats();

    // Vendor Extension Description (string)
    QString vendorExtDesc = m_devInfoProvider->MTPExtension();

    // Manufacturer (string)
    QString manufacturer = m_devInfoProvider->manufacturer();

    // Model (string)
    QString model = m_devInfoProvider->model();

    // Device Version (string)
    QString devVersion = m_devInfoProvider->deviceVersion();

    // Serial Number (string)
    QString serialNbr = m_devInfoProvider->serialNo();

    payloadLength = sizeof(stdVer) +
        sizeof(vendorExtId) +
        sizeof(vendorExtVer) +
        sizeof(funcMode) +
        opsSupported.size() +    // For arrays, number of elements + total length
        opsSupported.size() * sizeof(quint16) +
        evsSupported.size() +
        evsSupported.size() * sizeof(quint16) +
        propsSupported.size() +
        propsSupported.size() * sizeof(quint16) +
        sizeof(captureFormatsNumElem) +
        captureFormatsLen +
        imageFormats.size() +
        imageFormats.size() * sizeof(quint16) +
        sizeof(quint8) +
        ( vendorExtDesc.length() + 1 ) * 2 +    // For strings, need to add 1 to accommodate the NULL terminator too
        sizeof(quint8) +
        ( manufacturer.length() + 1 ) * 2 +
        sizeof(quint8) +
        ( model.length() + 1 ) * 2 +
        sizeof(quint8) +
        ( devVersion.length() + 1 ) * 2 +
        sizeof(quint8) +
        ( serialNbr.length() + 1 ) * 2;

    // Create a container with the estimated buffer size
    MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);

    dataContainer << stdVer << vendorExtId << vendorExtVer
        << vendorExtDesc << funcMode;

    dataContainer << opsSupported;
    dataContainer << evsSupported;
    dataContainer << propsSupported;
    dataContainer << captureFormatsNumElem;
    dataContainer << imageFormats;

    dataContainer << manufacturer << model << devVersion
        << serialNbr;

    bool sent = sendContainer(dataContainer);
    if( false == sent )
    {
        MTP_LOG_CRITICAL("Could not send data");
    }

    if( true == sent )
    {
        sendResponse(MTP_RESP_OK);
    }
}

void MTPResponder::openSessionReq()
{
    MTP_FUNC_TRACE();
    QVector<quint32> params;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    reqContainer->params(params);

    if ((MTP_INITIAL_SESSION_ID == params[0]))
    {
        sendResponse(MTP_RESP_InvalidParameter);
    }
    else if(m_transactionSequence->mtpSessionId != MTP_INITIAL_SESSION_ID)
    {
        sendResponse(MTP_RESP_SessionAlreadyOpen, m_transactionSequence->mtpSessionId);
    }
    else
    {
        // save new session id
        m_transactionSequence->mtpSessionId = params[0];
        // TODO:: inform storage server that a new session has been opened
        sendResponse(MTP_RESP_OK);
    }
}

void MTPResponder::closeSessionReq()
{
    MTP_FUNC_TRACE();
    quint16 code =  MTP_RESP_OK;

    if(MTP_INITIAL_SESSION_ID == m_transactionSequence->mtpSessionId)
    {
         code = MTP_RESP_SessionNotOpen;
    }
    else
    {
         m_transactionSequence->mtpSessionId = MTP_INITIAL_SESSION_ID;

        if( m_sendObjectSequencePtr )
        {
            delete m_sendObjectSequencePtr;
            m_sendObjectSequencePtr = 0;
        }

        freeObjproplistInfo();

         // FIXME: Trigger the discarding of a file, which has been possibly created in StorageServer
    }
    sendResponse(code);
}

void MTPResponder::getStorageIDReq()
{
    MTP_FUNC_TRACE();
    quint16 code =  MTP_RESP_OK;
    QVector<quint32> storageIds;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // check if everything is ok, so that cmd can be processed
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    if (MTP_RESP_OK == code && !m_storageServer)
    {
        code = initStorages() ? code : MTP_RESP_StoreNotAvailable;
    }

    if (MTP_RESP_OK == code)
    {
        // if session ID and transaction ID are valid
        // check still whether the storageIDs can be obtained from StorageServer
        code = m_storageServer->storageIds(storageIds);
    }
    bool sent = true;
    // if all preconditions fulfilled start enter data phase
    if(MTP_RESP_OK == code)
    {
        // DATA PHASE
        quint32 payloadLength = storageIds.size() * sizeof(quint32) + sizeof(quint32);
        MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);
        dataContainer << storageIds;
        sent = sendContainer(dataContainer);
        if( false == sent )
        {
            MTP_LOG_CRITICAL("Could not send data");
        }
    }
    // RESPONSE PHASE
    if( true == sent )
    {
        sendResponse(code);
    }
}

void MTPResponder::getStorageInfoReq()
{
    MTP_FUNC_TRACE();
    quint32 payloadLength = 0;
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    bool sent = true;
    if( MTP_RESP_OK == code )
    {
        QVector<quint32> params;
        reqContainer->params(params);
        // check still if the StorageID is correct and the storage is OK
        quint32 storageId = params[0];
        code = m_storageServer->checkStorage( storageId );
        if( MTP_RESP_OK == code )
        {
            MTPStorageInfo storageInfo;

            // get info from storage server
            code = m_storageServer->storageInfo( storageId, storageInfo );

            if( MTP_RESP_OK == code )
            {
                // all information for StorageInfo dataset retrieved from Storage Server
                // determine total length of Storage Info dataset
                payloadLength = MTP_STORAGE_INFO_SIZE + ( ( storageInfo.storageDescription.size() + 1 ) * 2 ) +
                                ( ( storageInfo.volumeLabel.size() + 1 ) * 2 );

                // Create data container
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);

                dataContainer << storageInfo.storageType << storageInfo.filesystemType << storageInfo.accessCapability
                    << storageInfo.maxCapacity << storageInfo.freeSpace << storageInfo.freeSpaceInObjects
                    << storageInfo.storageDescription << storageInfo.volumeLabel;

                // Check the storage once more before entering the data phase
                // if the storage factory returns an error the data phase will be skipped
                code = m_storageServer->checkStorage( storageId );
                if( MTP_RESP_OK == code )
                {
                    sent = sendContainer(dataContainer);
                    if( false == sent )
                    {
                        MTP_LOG_CRITICAL("Could not send data");
                    }
                }
            }
        }
    }

    if( true == sent )
    {
        sendResponse(code);
    }
}

void MTPResponder::getNumObjectsReq()
{
    MTP_FUNC_TRACE();
    quint32 noOfObjects;
    QVector<ObjHandle> handles;
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);

    // check if everything is ok, so that cmd can be processed
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    if (MTP_RESP_OK == code && !m_storageServer)
    {
        code = initStorages() ? code : MTP_RESP_StoreNotAvailable;
    }

    // Check the validity of the operation params.
    if( MTP_RESP_OK == code )
    {
        // Check if the storage id sent is valid.
        if( 0xFFFFFFFF != params[0] )
        {
            code = m_storageServer->checkStorage( params[0] );
        }
        if( MTP_RESP_OK == code )
        {
            // Check if the object format, if provided, is valid
            QVector<quint16> formats = m_devInfoProvider->supportedFormats();
            if( params[1] && !formats.contains( params[1] ) )
            {
                code = MTP_RESP_Invalid_ObjectProp_Format;
            }
            // Check if the object handle, if provided is valid
            if( code == MTP_RESP_OK && 0x00000000 != params[2] && 0xFFFFFFFF != params[2] )
            {
                code = m_storageServer->checkHandle( params[2] );
            }
        }
    }

    if( MTP_RESP_OK == code )
    {
        // retrieve the number of objects from storage server
        code = m_storageServer->getObjectHandles(params[0], static_cast<MTPObjFormatCode>(params[1]), params[2], handles);
    }
    noOfObjects = handles.size() >= 0 ? handles.size() : 0;

    sendResponse(code, noOfObjects);
}

void MTPResponder::getObjectHandlesReq()
{
    MTP_FUNC_TRACE();

    quint32 payloadLength = 0;
    QVector<ObjHandle> handles;
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);

    // check if everything is ok, so that cmd can be processed
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    if (MTP_RESP_OK == code && !m_storageServer)
    {
        code = initStorages() ? code : MTP_RESP_StoreNotAvailable;
    }

    // Check the validity of the operation params.
    if( MTP_RESP_OK == code )
    {
        // Check if the storage id sent is valid.
        if( 0xFFFFFFFF != params[0] )
        {
            code = m_storageServer->checkStorage( params[0] );
        }
        if( MTP_RESP_OK == code )
        {
            // Check if the object format, if provided, is valid
            QVector<quint16> formats = m_devInfoProvider->supportedFormats();
            if( params[1] && !formats.contains( params[1] ) )
            {
                code = MTP_RESP_Invalid_ObjectProp_Format;
            }
            // Check if the object handle, if provided is valid
            if( code == MTP_RESP_OK && 0x00000000 != params[2] && 0xFFFFFFFF != params[2] )
            {
                code = m_storageServer->checkHandle( params[2] );
            }
        }
    }

    if( MTP_RESP_OK == code )
    {
        // retrieve the number of objects from storage server
        code = m_storageServer->getObjectHandles(params[0],
                static_cast<MTPObjFormatCode>(params[1]),
                params[2], handles);
    }
    bool sent = true;
    if( MTP_RESP_OK == code )
    {
        // At least one PTP client (iPhoto) only shows all pictures if
        // the handles are sorted. It's probably related to having parent
        // folders listed before the objects they contain.
        qSort(handles);
        // DATA PHASE
        payloadLength = ( handles.size() + 1 ) * sizeof(quint32);
        MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);
        dataContainer << handles;
        sent = sendContainer(dataContainer);
        if( false == sent )
        {
            MTP_LOG_CRITICAL("Could not send data");
        }
    }

    if( true == sent )
    {
        sendResponse(code);
    }
}

void MTPResponder::getObjectInfoReq()
{
    MTP_FUNC_TRACE();
    const MTPObjectInfo *objectInfo;
    quint32 payloadLength = 0;
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // check if everthing is ok, so that cmd can be processed
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    QVector<quint32> params;
    reqContainer->params(params);
    bool sent = true;
    if((MTP_RESP_OK == code) && (MTP_RESP_OK == (code = m_storageServer->getObjectInfo(params[0], objectInfo))))
    {
        payloadLength = sizeof(MTPObjectInfo);
        // consider the variable part(strings) in the ObjectInfo dataset
        // for the length of the payload
        payloadLength += ( objectInfo->mtpFileName.size() ? ( objectInfo->mtpFileName.size() + 1 ) * 2 : 0 );
        payloadLength += ( objectInfo->mtpCaptureDate.size() ? ( objectInfo->mtpCaptureDate.size() + 1 ) * 2 : 0 );
        payloadLength += ( objectInfo->mtpModificationDate.size() ? ( objectInfo->mtpModificationDate.size() + 1 ) * 2 : 0 );

        MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);

        dataContainer << *objectInfo;

        sent = sendContainer(dataContainer);
        if( false == sent )
        {
            MTP_LOG_CRITICAL("Could not send data");
        }
    }

    if( true == sent )
    {
        sendResponse(code);
    }
}

// This handler does double duty for GetObject and GetPartialObject
void MTPResponder::getObjectReq()
{
    MTP_FUNC_TRACE();

    quint64 payloadLength = 0;
    quint32 startingOffset = 0;
    quint64 maxBufferSize = BUFFER_MAX_LEN;
    qint32 readLength = 0;
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;

    // check if everthing is ok, so that cmd can be processed
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if( MTP_RESP_OK == code )
    {
        reqContainer->params(params);
        const MTPObjectInfo *objectInfo;
        code = m_storageServer->getObjectInfo(params[0], objectInfo);
        // If the requested object is an association, then return an error code.
        if(MTP_OBF_FORMAT_Association == objectInfo->mtpObjectFormat)
        {
            code = MTP_RESP_InvalidObjectHandle;
        }
        payloadLength = objectInfo->mtpObjectCompressedSize;
        if( MTP_OP_GetPartialObject == reqContainer->code() )
        {
            startingOffset = params[1];
            // clamp payloadLength to remaining length of file
            payloadLength = startingOffset > payloadLength ? 0 : payloadLength - startingOffset;
            // clamp payloadLength to maximum length in request
            payloadLength = payloadLength > params[2] ? params[2] : payloadLength;
        }
    }

    bool sent = true;
    // enter data phase if precondition has been OK
    if( MTP_RESP_OK == code )
    {
        if( payloadLength > maxBufferSize )
        {
            // segmentation needed
            // set the segmentation info
            m_segmentedSender.totalDataLen = startingOffset + payloadLength;
            m_segmentedSender.payloadLen = BUFFER_MAX_LEN - MTP_HEADER_SIZE;
            m_segmentedSender.objHandle = params[0];
            m_segmentedSender.offset = startingOffset;
            m_segmentedSender.segmentationStarted = true;
            m_segmentedSender.sendResp = false;
            m_segmentedSender.headerSent = false;
            // start segmented sending of the object
            sendObjectSegmented();
            // response will be send from sendObjectSegmented
            return;
        }
        else
        {
            // no segmentation needed
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);

            // get the Object from the storage Server
            readLength = payloadLength;
            code = m_storageServer->readData(static_cast<ObjHandle&>(params[0]), reinterpret_cast<char*>(dataContainer.payload()),
                                             readLength, static_cast<quint32>(startingOffset));

            if( MTP_RESP_OK == code )
            {
                // Advance the container's offset
                dataContainer.seek(payloadLength);
                sent = sendContainer(dataContainer);
                if( false == sent )
                {
                    MTP_LOG_CRITICAL("Could not send data");
                }
            }
        }

    }

    if( true == sent )
    {
        if( MTP_OP_GetPartialObject == reqContainer->code() )
        {
            sendResponse(code, readLength);
        }
        else
        {
            sendResponse(code);
        }
    }
}

void MTPResponder::getThumbReq()
{
    MTP_FUNC_TRACE();
    bool sent = false;
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    code = preCheck(m_transactionSequence->mtpSessionId,
            reqContainer->transactionId());
    if( MTP_RESP_OK == code )
    {
        QVector<quint32> params;
        reqContainer->params( params );

        const MtpObjPropDesc *propDesc = 0;
        m_propertyPod->getObjectPropDesc( MTP_IMAGE_FORMAT,
                MTP_OBJ_PROP_Rep_Sample_Data, propDesc );

        QList<MTPObjPropDescVal> propValList;
        propValList.append( MTPObjPropDescVal ( propDesc ) );

        code = m_storageServer->getObjectPropertyValue( params[0], propValList );
        if( MTP_RESP_OK == code )
        {
            QVector<quint8> thumbnailData =
                    propValList[0].propVal.value<QVector<quint8> >();

            int payloadLength = thumbnailData.size();

            MTPTxContainer dataContainer( MTP_CONTAINER_TYPE_DATA,
                    reqContainer->code(), reqContainer->transactionId(),
                    payloadLength );

            memcpy( dataContainer.payload(), thumbnailData.constData(),
                    payloadLength );

            dataContainer.seek( payloadLength );
            sent = sendContainer( dataContainer );
            if( false == sent )
            {
                MTP_LOG_CRITICAL("Could not send thumbnail data");
            }
        }
    }

    if( true == sent )
    {
        sendResponse(code);
    }
}

void MTPResponder::deleteObjectReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if( MTP_RESP_OK == code )
    {
        QVector<quint32> params;
        reqContainer->params(params);
        code = m_storageServer->deleteItem(params[0], static_cast<MTPObjFormatCode>(params[1]));
        m_propCache->remove( params[0] );
    }
    sendResponse(code);
}

void MTPResponder::sendObjectInfoReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    m_transactionSequence->mtpResp = code;
    // DATA PHASE will follow
}

void MTPResponder::sendObjectReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // check whether the used IDs are OK
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if( MTP_RESP_OK == code )
    {
        // check still whether a SendObjectInfo dataset or an objectproplist dataset exists
        // TODO Why was the 0 != size check added? It will be 0 for abstract objects like playlists
        if( ( !m_sendObjectSequencePtr && !m_objPropListInfo )  /*|| (m_objPropListInfo && 0 == m_objPropListInfo->objectSize)*/ )
        {
            // Either there has been no SendObjectInfo taken place before
            // or an error occured in the SendObjectInfo, so that the
            // necessary ObjectInfo dataset does not exists */
            code = MTP_RESP_NoValidObjectInfo;
        }
    }
    m_transactionSequence->mtpResp = code;
}

void MTPResponder::getPartialObjectReq()
{
    MTP_FUNC_TRACE();
    getObjectReq();
}

void MTPResponder::setObjectProtectionReq()
{
    MTP_FUNC_TRACE();
    // FIXME: Implement this!!
    sendResponse(MTP_RESP_OperationNotSupported);
}

void MTPResponder::getDevicePropDescReq()
{
    MTP_FUNC_TRACE();

    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    // check if everything is ok, so that cmd can be processed
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    bool sent = true;
    //  enter data phase
    if( MTP_RESP_OK == code )
    {
        MtpDevPropDesc *propDesc = 0;
        quint32 payloadLength = sizeof(MtpDevPropDesc); // approximation
        QVector<quint32> params;
        reqContainer->params(params);
        code = m_propertyPod->getDevicePropDesc(params[0], &propDesc);
        if(MTP_RESP_OK == code && 0 != propDesc)
        {
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);
            dataContainer << *propDesc;
            sent = sendContainer(dataContainer);
            if( false == sent )
            {
                MTP_LOG_CRITICAL("Could not send data");
            }
        }
    }

    if( true == sent )
    {
        sendResponse(code);
    }
}

void MTPResponder::getDevicePropValueReq()
{
    MTP_FUNC_TRACE();

    quint32 payloadLength = sizeof(QVariant); // approximation
    MTPResponseCode code =  MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    bool sent = true;

    // check if everything is ok, so that cmd can be processed
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if( MTP_RESP_OK == code )
    {
        QVector<quint32> params;
        reqContainer->params(params);
        MtpDevPropDesc *propDesc = 0;
        code = m_propertyPod->getDevicePropDesc(params[0], &propDesc);

        if(MTP_RESP_OK == code && 0 != propDesc)
        {
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);
            dataContainer.serializeVariantByType(propDesc->uDataType, propDesc->currentValue);
            sent = sendContainer(dataContainer);
            if( false == sent )
            {
                MTP_LOG_CRITICAL("Could not send data");
            }
        }
    }

    if( true == sent )
    {
        sendResponse(code);
    }
}

void MTPResponder::setDevicePropValueReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode code =  MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    // check if everything is ok, so that cmd can be processed
    code = preCheck( m_transactionSequence->mtpSessionId, reqContainer->transactionId() );
    if( MTP_RESP_OK == code )
    {
        QVector<quint32> params;
        reqContainer->params(params);
        MTPDevPropertyCode propCode = static_cast<MTPDevPropertyCode>(params[0]);
        MtpDevPropDesc *propDesc = 0;

        code = m_propertyPod->getDevicePropDesc(propCode, &propDesc);
        if(MTP_RESP_OK != code || 0 == propDesc)
        {
            code = MTP_RESP_DevicePropNotSupported;
        }
    }
    // wait for the data phase to get the value to be set and to respond
    // in case that here in the request phase an error was detected, the
    // response with the error code is sent at the end of the data phase
    m_transactionSequence->mtpResp = code;
}

void MTPResponder::resetDevicePropValueReq()
{
    MTP_FUNC_TRACE();
    // FIXME: Implement this?
    sendResponse(MTP_RESP_OperationNotSupported);
}

void MTPResponder::moveObjectReq()
{
    MTP_FUNC_TRACE();
    quint16 code =  MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // Check if the session id and transaction id are valid before proceeding further
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    // Session and transaction id's are ok
    if(MTP_RESP_OK == code)
    {
        QVector<quint32> params;
        reqContainer->params(params);
        // Check if the object handle passed to us is valid and exists in the storage passed to us
        if( MTP_RESP_OK != ( code = m_storageServer->checkHandle(params[0]) ) )
        {
        }
        // Check if storage handle that's passed to us exists, is not full, and is writable
        else if( MTP_RESP_OK != ( code = m_storageServer->checkStorage(params[1]) ) )
        {
            // We came across a storage related error ;
        }
        // If the parent object handle is passed, check if that's valid
        else if( params[2] && MTP_RESP_OK != m_storageServer->checkHandle(params[2]) )
        {
            code = MTP_RESP_InvalidParentObject;
        }
        // Storage, object and parent handles are ok, proceed to copy the object
        else
        {
            code = m_storageServer->moveObject(params[0], params[2], params[1]);
            if( MTP_RESP_OK == code )
            {
                // Invalidate the parent handle property from the cache. The
                // other properties do not change
                m_propCache->remove( params[0], MTP_OBJ_PROP_Parent_Obj );
            }
        }
    }

    sendResponse(code);
}

void MTPResponder::copyObjectReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode code =  MTP_RESP_OK;
    ObjHandle retHandle = 0;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // Check if the session id and transaction id are valid before proceeding further
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    // Session and transaction id's are ok
    if(MTP_RESP_OK == code)
    {
        QVector<quint32> params;
        reqContainer->params(params);
        // Check if the object handle passed to us is valid and exists in the storage passed to us
        if( MTP_RESP_OK != ( code = m_storageServer->checkHandle(params[0]) ) )
        {
        }
        // Check if storage handle that's passed to us exists, is not full, and is writable
        else if( MTP_RESP_OK != ( code = m_storageServer->checkStorage(params[1]) ) )
        {
            // We cam across a storage related error ;
        }
        // If the parent object handle is passed, check if that's valid
        else if( params[2] && MTP_RESP_OK != m_storageServer->checkHandle(params[2]) )
        {
            code = MTP_RESP_InvalidParentObject;
        }
        // Storage, object and parent handles are ok, proceed to copy the object
        else
        {
            code = m_storageServer->copyObject(params[0], params[2], params[1], retHandle);
        }
    }

    if( RESPONDER_TX_CANCEL == m_state )
    {
        return;
    }

    // RESPONSE PHASE
    m_copiedObjHandle = retHandle;
    sendResponse(code, retHandle);
}

void MTPResponder::getObjPropsSupportedReq()
{
    MTP_FUNC_TRACE();

    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    bool sent = true;

    // check if everthing is ok, so that cmd can be processed
    // Pass 0x00000001 as the session ID, as this operation does not need an active session
    if (MTP_RESP_OK != (code = preCheck(0x00000001, reqContainer->transactionId())))
    {
        code = MTP_RESP_InvalidTransID;
    }
    else
    {
        QVector<quint32> params;
        reqContainer->params(params);
        MTPObjectFormatCategory category = (MTPObjectFormatCategory)m_devInfoProvider->getFormatCodeCategory(params[0]);

        QVector<MTPObjPropertyCode> propsSupported;

        code = m_propertyPod->getObjectPropsSupportedByType(category, propsSupported);

        // enter data phase if precondition has been OK
        if(MTP_RESP_OK == code)
        {
            quint32 payloadLength = propsSupported.size() * sizeof(MTPObjPropertyCode) + sizeof(quint32);
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);
            dataContainer << propsSupported;
            sent = sendContainer(dataContainer);
            if( false == sent )
            {
                MTP_LOG_CRITICAL("Could not send data");
            }
        }
    }

    if( true == sent )
    {
        sendResponse(code);
    }
}

void MTPResponder::getObjPropDescReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    bool sent = true;

    // check if everthing is ok, so that cmd can be processed
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if(MTP_RESP_OK == code)
    {
        QVector<quint32> params;
        reqContainer->params(params);
        MTPObjPropertyCode propCode = static_cast<MTPObjPropertyCode>(params[0]);
        MTPObjFormatCode formatCode = static_cast<MTPObjFormatCode>(params[1]);
        MTPObjectFormatCategory category = (MTPObjectFormatCategory)m_devInfoProvider->getFormatCodeCategory(formatCode);
        if( MTP_UNSUPPORTED_FORMAT == category)
        {
            code = MTP_RESP_Invalid_ObjectProp_Format;
        }
        else
        {
            const MtpObjPropDesc* propDesc = 0;
            code = m_propertyPod->getObjectPropDesc(category, propCode, propDesc);
            if(MTP_RESP_OK == code)
            {
                // Data phase
                quint32 payloadLength = sizeof(MtpObjPropDesc); // approximation
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);
                dataContainer << *propDesc;
                sent = sendContainer(dataContainer);
                if( false == sent )
                {
                    MTP_LOG_CRITICAL("Could not send data");
                }
            }
        }
    }

    if( true == sent )
    {
        sendResponse(code);
    }
}

void MTPResponder::getObjPropValueReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    bool sent = true;

    // check if everthing is ok, so that cmd can be processed
    code =  preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if(MTP_RESP_OK == code)
    {
        QVector<quint32> params;
        reqContainer->params(params);
        MTPObjPropertyCode propCode = static_cast<MTPObjPropertyCode>(params[1]);
        ObjHandle handle = static_cast<ObjHandle>(params[0]);
        const MTPObjectInfo *objectInfo;
        MTPObjectFormatCategory category;
        const MtpObjPropDesc *propDesc = 0;
        code = m_storageServer->getObjectInfo( handle, objectInfo );
        if(MTP_RESP_OK == code)
        {
            category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(objectInfo->mtpObjectFormat));
            code = m_propertyPod->getObjectPropDesc(category, propCode, propDesc);
        }

        if(MTP_RESP_OK == code)
        {
            QList<MTPObjPropDescVal> propValList;

            propValList.append(MTPObjPropDescVal(propDesc));

            if( m_propCache->get( handle, propValList[0] ) )
            {
                code = MTP_RESP_OK;
            }
            else
            {
                code = m_storageServer->getObjectPropertyValue(handle, propValList);
                if( MTP_RESP_OK == code )
                {
                    m_propCache->add( handle, propValList[0] );
                }
            }
            if(MTP_RESP_ObjectProp_Not_Supported == code)
            {
                QString path;
                if(MTP_RESP_OK == m_storageServer->getPath(handle, path))
                {
                    // Try with an extension
                    m_extensionManager->getObjPropValue(path, propCode, propValList[0].propVal, code);
                }
            }
            // enter data phase if value has been retrieved
            if(MTP_RESP_OK == code)
            {
                // DATA PHASE
                quint32 payloadLength = sizeof(QVariant); // approximation
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);
                if(1 == propValList.size())
                {
                    dataContainer.serializeVariantByType(propDesc->uDataType, propValList[0].propVal);
                }
                else
                {
                    // Serialize empty variant
                    dataContainer.serializeVariantByType(propDesc->uDataType, QVariant());
                }
                sent = sendContainer(dataContainer);
                if( false == sent )
                {
                    MTP_LOG_CRITICAL("Could not send data");
                }
            }
        }
    }

    if( true == sent )
    {
        sendResponse(code);
    }
}

void MTPResponder::setObjPropValueReq()
{
    MTP_FUNC_TRACE();

    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    // check if everthing is ok, so that cmd can be processed
    m_transactionSequence->mtpResp = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if(MTP_RESP_OK == m_transactionSequence->mtpResp)
    {
        QVector<quint32> params;
        reqContainer->params(params);
        // if session ID and transaction ID are valid
        // check still whether the StorageItem can be accessed
        ObjHandle objHandle = static_cast<ObjHandle>(params[0]);
        const MTPObjectInfo *objInfo;
        if(MTP_RESP_OK != (m_transactionSequence->mtpResp = m_storageServer->getObjectInfo(objHandle, objInfo)))
        {
            return;
        }
        MTPObjFormatCode format = static_cast<MTPObjFormatCode>(objInfo->mtpObjectFormat);
        MTPObjectFormatCategory category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(format));
        MTPObjPropertyCode propCode = static_cast<MTPObjPropertyCode>(params[1]);
        const MtpObjPropDesc* propDesc = 0;

        if(MTP_RESP_OK == (m_transactionSequence->mtpResp = m_propertyPod->getObjectPropDesc(category, propCode, propDesc)))
        {
            if(false == propDesc->bGetSet)
            {
                m_transactionSequence->mtpResp = MTP_RESP_AccessDenied;
            }
        }
    }
}

void MTPResponder::getObjectPropListReq()
{
    MTP_FUNC_TRACE();
    ObjHandle objHandle = 0;
    MTPObjFormatCode format = MTP_OBF_FORMAT_Undefined;
    MTPObjPropertyCode propCode;
    quint32 depth = 0;
    MTPResponseCode resp = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);


    objHandle = static_cast<ObjHandle>(params[0]);
    format = static_cast<MTPObjFormatCode>(params[1]);
    propCode = static_cast<MTPObjPropertyCode>(params[2]);
    depth = (params[4]);
    bool sent = true;

    resp = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    if( MTP_RESP_OK == resp )
    {
        if((0 == objHandle) && (0 == depth))
        {
            // return an empty data set
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), sizeof(quint32));
            dataContainer << (quint32)0;
            sent = sendContainer(dataContainer);
            if( false == sent )
            {
                    MTP_LOG_CRITICAL("Could not send data");
            }
        }
        else if( (1 < depth) && (0xFFFFFFFF > depth) )
        {
            // we do not support depth values between 1 and 0xFFFFFFFF
            resp = MTP_RESP_Specification_By_Depth_Unsupported;
        }
        else if(0 == propCode)
        {
            // propCode of 0 means we need the group code, but we do not support that
            resp = MTP_RESP_Specification_By_Group_Unsupported;
        }
        else if((0 != format) && (MTP_UNSUPPORTED_FORMAT == static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(format))))
        {
            resp = MTP_RESP_InvalidCodeFormat;
        }
        else
        {
            QVector<ObjHandle> objHandles;
            quint32 numHandles = 0;
            quint32 numElements = 0;
            MTPObjFormatCode objFormat;
            MTPObjectFormatCategory category;
            quint32 storageID = 0xFFFFFFFF;
            const MTPObjectInfo *objInfo;
            ObjHandle currentObj = 0;

            if(0 == depth)
            {
                // The properties for this object are requested
                objHandles.append(objHandle);
            }
            else
            {
                ObjHandle tempHandle = objHandle;
                // The properties for the object's immediate children or for all objects
                if(0 == objHandle)
                {
                    // All objects under root
                    tempHandle = 0xFFFFFFFF;
                }
                else if(0xFFFFFFFF == objHandle)
                {
                    // All objects in (all?) storage
                    tempHandle = 0;
                }
                resp = m_storageServer->getObjectHandles(storageID, format, tempHandle, objHandles);
            }

            if(MTP_RESP_OK == resp)
            {
                // TODO: The buffer max len needs to be decided
                quint32 maxPayloadLen = BUFFER_MAX_LEN - MTP_HEADER_SIZE;
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), maxPayloadLen);

                // if there are no handles found an empty dataset will be sent
                numHandles = objHandles.size();
                MTP_LOG_TRACE( numHandles);
                dataContainer << numHandles; // Write the numHandles here for now, to advance the serializer's pointer
                // go through the list of found ObjectHandles
                for(quint32 i = 0; (i < numHandles && (MTP_RESP_OK == resp)); i++)
                {
                    currentObj = objHandles[i];

                    resp = m_storageServer->getObjectInfo(currentObj, objInfo);
                    if(MTP_RESP_OK == resp)
                    {
                        // find the format and the category of the object
                        objFormat = static_cast<MTPObjFormatCode>(objInfo->mtpObjectFormat);
                        category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(objFormat));

                        // FIXME: Investigate if the below force assignment to common format is really needed
                        if (category == MTP_UNSUPPORTED_FORMAT)
                        {
                            category = MTP_COMMON_FORMAT;
                        }

                        QList<MTPObjPropDescVal> propValList;
                        // check whether all or a certain ObjectProperty of the referenced Object is requested
                        if(0xFFFF == propCode)
                        {
                            const MtpObjPropDesc *propDesc = 0;
                            QVector<MTPObjPropertyCode> propsSupported;
                            int pos = 0;

                            resp = m_propertyPod->getObjectPropsSupportedByType(category, propsSupported);
                            for (int i = 0; ((i < propsSupported.size()) && (MTP_RESP_OK == resp)); i++)
                            {
                                resp = m_propertyPod->getObjectPropDesc(category, propsSupported[i], propDesc);
                                if(MTP_OBJ_PROP_Rep_Sample_Data != propDesc->uPropCode)
                                {
                                    propValList.append(MTPObjPropDescVal(propDesc));
                                    QList<MTPObjPropDescVal> currPropValList = propValList.mid(pos);
                                    MTPResponseCode code = m_storageServer->getObjectPropertyValue(currentObj,
                                                                                                   currPropValList,
                                                                                                   true,
                                                                                                   false);
                                    if( MTP_RESP_OK == code )
                                    {
                                        m_propCache->add( currentObj, currPropValList[0] );
                                    }
                                    ++pos;
                                }
                            }
                        }
                        else
                        {
                            const MtpObjPropDesc* propDesc = 0;
                            resp = m_propertyPod->getObjectPropDesc(category, propCode, propDesc);
                            propValList.append(MTPObjPropDescVal(propDesc));
                        }
                        if(MTP_RESP_OK == resp)
                        {
                            if(true == serializePropListQuantum(currentObj, propValList, dataContainer))
                            {
                                numElements += propValList.size();
                            }
                            else
                            {
                                resp = MTP_RESP_GeneralError;
                            }
                        }
                    }
                }
                if (MTP_RESP_OK == resp)
                {
                    // KLUDGE: We have to manually insert the number of elements at
                    // the start of the container payload. Is there a better way?
                    dataContainer.putl32(dataContainer.payload(), numElements);
                    sent = sendContainer(dataContainer);
                    if( false == sent )
                    {
                        MTP_LOG_CRITICAL("Could not send data");
                    }
                }
            }
            else//FIXME Is this needed?
            {
                // return an empty data set
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), sizeof(quint32));
                dataContainer << (quint32)0;
                sent = sendContainer(dataContainer);
                if( false == sent )
                {
                    MTP_LOG_CRITICAL("Could not send data");
                }
            }
        }
        if(MTP_RESP_InvalidObjectFormatCode == resp)//FIXME investigate
        {
                resp = MTP_RESP_InvalidCodeFormat;
        }
    }

    if( true == sent )
    {
        sendResponse(resp);
    }
}

void MTPResponder::setObjectPropListReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode respCode =  MTP_RESP_OK;

    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    // check if everthing is ok, so that cmd can be processed
    respCode = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    // Store the code
    m_transactionSequence->mtpResp = respCode;
    // Wait for the data phase
}

void MTPResponder::getInterdependentPropDescReq()
{
    MTP_FUNC_TRACE();
    // FIXME: Implement this!
    sendResponse(MTP_RESP_OperationNotSupported);
}

void MTPResponder::sendObjectPropListReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode* respCode =  &m_transactionSequence->mtpResp;
    quint32 storageID = 0x00000000;
    quint64 objectSize = 0x00000000;
    ObjHandle parentHandle = 0x00000000;
    MTPObjFormatCode format = MTP_OBF_FORMAT_Undefined;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // check if everthing is ok, so that cmd can be processed
    *respCode = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if(MTP_RESP_OK == *respCode)
    {
        QVector<quint32> params;
        reqContainer->params(params);
        // If storage id is provided, check if it's valid
        storageID = params[0];
        if(storageID)
        {
            *respCode = m_storageServer->checkStorage(storageID);
        }

        if(MTP_RESP_OK == *respCode)
        {
            // If the parent object is provided, check that
            parentHandle = params[1];
            if((0 != parentHandle) && (0xFFFFFFFF != parentHandle))
            {
                *respCode = m_storageServer->checkHandle(parentHandle);
            }
            if(MTP_RESP_OK == *respCode)
            {
                // Fetch object format code and category
                format = static_cast<MTPObjFormatCode>(params[2]);
                // Get the object size
                quint64 msb = params[3];
                quint64 lsb = params[4];

                // Indicates that object is >= 4GB, we don't support this
                if( msb )
                {
                   *respCode = MTP_RESP_Object_Too_Large;
                }
                else
                {
                    // If there's an existing object prop list info, free that
                    freeObjproplistInfo();

                    objectSize = (msb << (sizeof(quint32) * 8)) | (lsb);

                    // Populate whatever we can now, rest will be populated in the data phase
                    m_objPropListInfo = new ObjPropListInfo;
                    m_objPropListInfo->storageId = storageID;
                    m_objPropListInfo->parentHandle = parentHandle;
                    m_objPropListInfo->objectSize = objectSize;
                    m_objPropListInfo->objectFormatCode = format;
                }
            }
        }
    }
}

void MTPResponder::getObjReferencesReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode respCode =  MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    bool sent = true;

    // check if everthing is ok, so that cmd can be processed
    respCode = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if(MTP_RESP_OK == respCode)
    {
        QVector<quint32> params;
        reqContainer->params(params);
        QVector<ObjHandle> objReferences;
        respCode = m_storageServer->getReferences(params[0], objReferences);
        // enter data phase if precondition has been OK
        if(MTP_RESP_OK == respCode)
        {
            quint32 payloadLength = objReferences.size() * sizeof(ObjHandle) + sizeof(quint32);
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(), payloadLength);
            dataContainer << objReferences;
            sent = sendContainer(dataContainer);
            if( false == sent )
            {
                MTP_LOG_CRITICAL("Could not send data");
            }
        }
    }

    if( true == sent )
    {
        sendResponse(respCode);
    }
}

void MTPResponder::setObjReferencesReq()
{
    MTP_FUNC_TRACE();
    quint16 respCode =  MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // check if everthing is ok, so that cmd can be processed
    respCode = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    m_transactionSequence->mtpResp = respCode;
}

void MTPResponder::skipReq()
{
    MTP_FUNC_TRACE();
    quint16 respCode =  MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // check if everthing is ok, so that cmd can be processed
    respCode = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if(MTP_RESP_OK == respCode)
    {
        QVector<quint32> params;
        reqContainer->params(params);
    }

    sendResponse(respCode);
}

void MTPResponder::sendObjectInfoData()
{
    MTP_FUNC_TRACE();

    MTPObjectInfo objectInfo;
    MTPResponseCode response =  m_transactionSequence->mtpResp;
    quint32 responseParams[3];
    MTPRxContainer *recvContainer = m_transactionSequence->dataContainer;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    if(MTP_RESP_OK == m_transactionSequence->mtpResp)
    {
        // If there's a objectproplist dataset existing, delete that
        freeObjproplistInfo();
        m_sendObjectSequencePtr = new MTPSendObjectSequence();

        reqContainer->params(params);

        // De-serialize the object info
        *recvContainer >> objectInfo;

        // Indicates that object is >= 4GB, we don't support this
        if( 0xFFFFFFFF == objectInfo.mtpObjectCompressedSize )
        {
            response = MTP_RESP_Object_Too_Large;
        }
        else
        {
            // get the requested StorageId
            responseParams[0] = params[0];
            // get the requested Parent ObjectHandle
            responseParams[1] = params[1];
            // some clients (Nautilus 3.4.2) send an info with blank parent
            objectInfo.mtpParentObject = params[1];
            // trigger creation of a file in Storage Server
            response = m_storageServer->addItem(responseParams[0], responseParams[1], responseParams[2],
                                                &objectInfo);
        }

        // creation of file was successful store information which are needed later temporarly
        if( MTP_RESP_OK == response )
        {
            // save the ObjectInfo temporarly
            m_sendObjectSequencePtr->objInfo = new MTPObjectInfo;
            *(m_sendObjectSequencePtr->objInfo) = objectInfo;
            // save the ObjectHandle temporarly
            m_sendObjectSequencePtr->objHandle = responseParams[2];
        }
        else
        {
            delete m_sendObjectSequencePtr;
            m_sendObjectSequencePtr = 0;
            // return parameters shall not be used
            memset(&responseParams[0],0, sizeof(responseParams));
        }
    }

    // create and send container for response
    MTPTxContainer respContainer(MTP_CONTAINER_TYPE_RESPONSE, response, reqContainer->transactionId(), sizeof(responseParams));
    if( MTP_RESP_OK == response )
    {
        respContainer << responseParams[0] << responseParams[1] << responseParams[2];
    }
    bool sent = sendContainer(respContainer);
    if( false == sent )
    {
        MTP_LOG_CRITICAL("Could not send response");
    }
}

void MTPResponder::sendObjectData(quint8* data, quint32 dataLen, bool isFirstPacket, bool isLastPacket)
{
    MTP_FUNC_TRACE();

    quint32 objectLen = 0;
    MTPResponseCode code =  MTP_RESP_OK;
    MTPContainerWrapper container(data);
    ObjHandle handle = 0;

    // error if no ObjectInfo or objectproplist dataset has been sent before
    if( !m_objPropListInfo && ( !m_sendObjectSequencePtr || !m_sendObjectSequencePtr->objInfo ) )
    {
        code = MTP_RESP_NoValidObjectInfo;
    }
    else
    {
        quint8 *writeBuffer = 0;
        if(m_sendObjectSequencePtr)
        {
            handle = m_sendObjectSequencePtr->objHandle;
        }
        else if( m_objPropListInfo )
        {
            handle = m_objPropListInfo->objectHandle;
        }

        // assume that the whole segment will be written
        objectLen = dataLen;

        if (isFirstPacket)
        {
            // the start segment includes the container header, which should not be written
            // set the pointer of the data to be written, on the payload
            writeBuffer = container.payload();
            //subtract header length
            objectLen -= MTP_HEADER_SIZE;
         }
         else
         {
             // if it is not the start, just take the receive data an length
             writeBuffer = data;
         }

         // write data
         code = m_storageServer->writeData( handle, reinterpret_cast<char*>(writeBuffer), objectLen, isFirstPacket, isLastPacket);
         if( MTP_RESP_OK == code )
         {
             code = sendObjectCheck(handle, objectLen, isLastPacket, code);
         }
    }

    if( MTP_RESP_Undefined != code )
    {
        // Delete the stored sendObjectInfo information
        if( m_sendObjectSequencePtr )
        {
            if( m_sendObjectSequencePtr->objInfo )
            {
                delete m_sendObjectSequencePtr->objInfo;
                m_sendObjectSequencePtr->objInfo = 0;
            }
            delete m_sendObjectSequencePtr;
            m_sendObjectSequencePtr = 0;
        }
        // Apply object prop list if one was sent thru SendObjectPropList
        if(MTP_RESP_OK == code && m_objPropListInfo)
        {
            MTPObjectFormatCategory category = m_devInfoProvider->getFormatCodeCategory(m_objPropListInfo->objectFormatCode);
            QList<MTPObjPropDescVal> propValList;
            const MtpObjPropDesc *propDesc = 0;
            for( quint32 i = 0 ; i < m_objPropListInfo->noOfElements; i++)
            {
                ObjPropListInfo::ObjectPropList &propList =
                        m_objPropListInfo->objPropList[i];

                switch(propList.objectPropCode)
                {
                    case MTP_OBJ_PROP_Obj_File_Name:
                        // This has already been set
                        continue;
                    case MTP_OBJ_PROP_Name:
                    {
                        // Some initiators send media file properties with Name
                        // equal to Object File Name, even though the tags
                        // embedded in the file may contain a value more
                        // suitable for display in a player as a song or movie
                        // title than a mere filename.
                        //
                        // Thus, when we encounter a filename in the Name
                        // property, we don't store the value and let Tracker
                        // try extracting the name from the file's embedded
                        // metadata.
                        const MTPObjectInfo *info;
                        if( m_storageServer->getObjectInfo( handle, info ) != MTP_RESP_OK )
                        {
                            break;
                        }

                        if( propList.value->toString() == info->mtpFileName )
                        {
                            continue;
                        }
                        break;
                    }
                    default:
                        break;
                }

                // Get the object prop desc for this property
                if(MTP_RESP_OK == m_propertyPod->getObjectPropDesc(category, propList.objectPropCode, propDesc))
                {
                    propValList.append(MTPObjPropDescVal(propDesc, *propList.value));
                }
            }
            m_storageServer->setObjectPropertyValue(handle, propValList, true);
            m_propCache->add( handle, propValList );
            m_propCache->setAllProps( handle );
        }
        // Trigger close file in the storage server... ignore return here
        m_storageServer->writeData( handle, 0, 0, false, true);
        // create and send container for response
        sendResponse(code);
        // This is moved here intentionally : in case a cancel tx is received before sending the response above
        // we would need the handle to delete the object.
        freeObjproplistInfo();
    } // no else, wait for the next data packet...
}

void MTPResponder::setObjectPropListData()
{
    MTP_FUNC_TRACE();
    MTPResponseCode respCode = MTP_RESP_OK;
    quint32 numObjects = 0;
    ObjHandle objHandle = 0;
    MTPObjPropertyCode propCode;
    MTPDataType datatype = MTP_DATA_TYPE_UNDEF;
    MTPObjFormatCode format;
    MTPObjectFormatCategory category;
    const MtpObjPropDesc* propDesc = 0;
    quint32 i = 0;
    MTPRxContainer *recvContainer = m_transactionSequence->dataContainer;

    // Deserialize the no. of elements for which we need to set object property values
    *recvContainer >> numObjects;

    // Proceed to set object property values for noOfElements objects
    for(; ((i < numObjects) && (MTP_RESP_OK == respCode)); i++)
    {
        // First, get the object handle
        *recvContainer >> objHandle;
        // Check if this is a valid object handle
        const MTPObjectInfo *objInfo;
        respCode = m_storageServer->getObjectInfo(objHandle, objInfo);

        if(MTP_RESP_OK == respCode)
        {
            // Next, get the object property code
            *recvContainer >> propCode;

            // Check if the object property code is valid and that it can be "set"
            format = static_cast<MTPObjFormatCode>(objInfo->mtpObjectFormat);
            category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(format));
            respCode = m_propertyPod->getObjectPropDesc(category, propCode, propDesc);
            if(MTP_RESP_OK == respCode)
            {
                respCode = propDesc->bGetSet ? MTP_RESP_OK : MTP_RESP_AccessDenied;
                if(MTP_RESP_OK == respCode)
                {
                    // Next, get this object property's datatype
                    *recvContainer >> datatype;
                    QList<MTPObjPropDescVal> propValList;
                    propValList.append(MTPObjPropDescVal(propDesc));
                    recvContainer->deserializeVariantByType(datatype, propValList[0].propVal);
                    // Set the object property value
                    respCode = m_storageServer->setObjectPropertyValue(objHandle, propValList);
                    m_propCache->add( objHandle, propValList );
                }
            }
        }
    }
    if(MTP_RESP_OK != respCode)
    {
        // Set the response parameter to the 0 based index of the property that caused the problem
        sendResponse(respCode, i - 1);
    }
    else
    {
        sendResponse(MTP_RESP_OK);
    }
}

void MTPResponder::sendObjectPropListData()
{
    MTP_FUNC_TRACE();
    quint32 respParam[4] = {0,0,0,0};
    quint32 respSize = 0;
    quint16 respCode =  MTP_RESP_OK;
    MTPObjectInfo objInfo;
    MTPObjectFormatCategory category = MTP_UNSUPPORTED_FORMAT;
    MTPObjPropertyCode propCode;
    const MtpObjPropDesc* propDesc = 0;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    MTPRxContainer *recvContainer = m_transactionSequence->dataContainer;
    bool sent = true;

    if(0 == m_objPropListInfo || MTP_RESP_OK != m_transactionSequence->mtpResp)
    {
        respCode = 0 == m_objPropListInfo ? MTP_RESP_GeneralError : m_transactionSequence->mtpResp;
        sendResponse(respCode);
        return;
    }

    // Deserialize the no. of elements for which we need to set object property values
    *recvContainer >> m_objPropListInfo->noOfElements;

    // Allocate memory for the the number of elements obj property list item
    m_objPropListInfo->objPropList = new ObjPropListInfo::ObjectPropList[m_objPropListInfo->noOfElements];

    for(quint32 i = 0; i < m_objPropListInfo->noOfElements; i++ )
    {
        m_objPropListInfo->objPropList[i].value = 0;
        // First, get the object handle
        *recvContainer >> m_objPropListInfo->objPropList[i].objectHandle;
        // Check if the object handle is 0x00000000
        if(0 != m_objPropListInfo->objPropList[i].objectHandle)
        {
            respCode = MTP_RESP_Invalid_Dataset;
            respParam[3] = i;
            respSize = 4 * sizeof(quint32);
            break;
        }

        // Next, get the object property code
        *recvContainer >> m_objPropListInfo->objPropList[i].objectPropCode;

        category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(m_objPropListInfo->objectFormatCode));
        propCode = static_cast<MTPObjPropertyCode>(m_objPropListInfo->objPropList[i].objectPropCode);
        // Check if the object property code is valid and that it can be "get"
        if(MTP_RESP_OK != m_propertyPod->getObjectPropDesc(category, propCode, propDesc))
        {
            respCode = MTP_RESP_Invalid_Dataset;
            respParam[3] = i;
            respSize = 4 * sizeof(quint32);
            break;
        }

        // Next, get this object property's datatype
        *recvContainer >> m_objPropListInfo->objPropList[i].datatype;

        m_objPropListInfo->objPropList[i].value = new QVariant();
        recvContainer->deserializeVariantByType(m_objPropListInfo->objPropList[i].datatype, *m_objPropListInfo->objPropList[i].value);

        // If this property is the filename, trigger object creation in the file system now
        if( MTP_OBJ_PROP_Obj_File_Name == m_objPropListInfo->objPropList[i].objectPropCode)
        {
            objInfo.mtpStorageId = m_objPropListInfo->storageId;
            objInfo.mtpObjectCompressedSize = m_objPropListInfo->objectSize;
            objInfo.mtpParentObject = m_objPropListInfo->parentHandle;
            objInfo.mtpObjectFormat = m_objPropListInfo->objectFormatCode;
            objInfo.mtpFileName = m_objPropListInfo->objPropList[i].value->value<QString>();
            respCode = m_storageServer->addItem(m_objPropListInfo->storageId, m_objPropListInfo->parentHandle, respParam[2], &objInfo);
            if(MTP_RESP_OK == respCode)
            {
                m_objPropListInfo->objectHandle = respParam[2];
                respParam[0] = m_objPropListInfo->storageId;
                respParam[1] = m_objPropListInfo->parentHandle;
                respSize = 3 * sizeof(quint32);
            }
            else
            {
                respParam[3] = i;
                respSize = 4 * sizeof(quint32);
                break;
            }
        }
    }
    // create and send container for response
    MTPTxContainer respContainer(MTP_CONTAINER_TYPE_RESPONSE, respCode, reqContainer->transactionId(), respSize);
    for(quint32 i = 0; i < (respSize/sizeof(quint32)); i++)
    {
        respContainer << respParam[i];
    }
    sent = sendContainer(respContainer);
    if( false == sent )
    {
        MTP_LOG_CRITICAL("Could not send response");
    }
}

void MTPResponder::setDevicePropValueData()
{
    MTP_FUNC_TRACE();

    quint16 response =  MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);
    MTPDevPropertyCode propCode = params[0];
    MTPRxContainer *recvContainer = m_transactionSequence->dataContainer;

    switch(propCode)
    {
        // set the value if the property is supported
        case MTP_DEV_PROPERTY_Device_Friendly_Name:
            {
                QString name;
                *recvContainer >> name;
                m_devInfoProvider->setDeviceFriendlyName(name);
            }
            break;
        case MTP_DEV_PROPERTY_Synchronization_Partner:
            {
                QString partner;
                *recvContainer >> partner;
                m_devInfoProvider->setSyncPartner(partner);
            }
            break;
        case MTP_DEV_PROPERTY_Volume:
            {
                qint32 vol = 0;
                *recvContainer >> vol;
                // TODO: implement this
            }
            break;
        default:
            {
                // Call extension manager's method to set device property value
            }
            break;
    }

    sendResponse(response);
}

void MTPResponder::setObjPropValueData()
{
    MTP_FUNC_TRACE();
    MTPResponseCode respCode = m_transactionSequence->mtpResp;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // If there was some problem in the request phase...
    if (MTP_RESP_OK == respCode)
    {
        QVector<quint32> params;
        reqContainer->params(params);
        // Check if the object property code is valid and that it can be "set"
        ObjHandle objHandle = params[0];
        const MTPObjectInfo *objInfo;
        if(MTP_RESP_OK == (respCode = m_storageServer->getObjectInfo(objHandle, objInfo)))
        {
            MTPObjFormatCode format = static_cast<MTPObjFormatCode>(objInfo->mtpObjectFormat);
            MTPObjectFormatCategory category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(format));
            MTPObjPropertyCode propCode = static_cast<MTPObjPropertyCode>(params[1]);
            const MtpObjPropDesc* propDesc = 0;

            if(MTP_RESP_OK == (respCode = m_propertyPod->getObjectPropDesc(category, propCode, propDesc)))
            {
                // take the data from the container to set the ObjectProperty
                MTPRxContainer *recvContainer = m_transactionSequence->dataContainer;
                QList<MTPObjPropDescVal> propValList;

                propValList.append(MTPObjPropDescVal(propDesc));
                recvContainer->deserializeVariantByType(propDesc->uDataType, propValList[0].propVal);
                respCode = m_storageServer->setObjectPropertyValue(objHandle, propValList);
                m_propCache->add( objHandle, propValList[0] );
                if(MTP_RESP_ObjectProp_Not_Supported == respCode)
                {
                    QString path;
                    if(MTP_RESP_OK == m_storageServer->getPath(objHandle, path))
                    {
                        // Try with an extension
                        m_extensionManager->setObjPropValue(path, propCode, propValList[0].propVal, respCode);
                    }
                }
            }
        }
    }
    sendResponse(respCode);
}

void MTPResponder::setObjReferencesData()
{
    MTP_FUNC_TRACE();
    QVector<quint32> references;
    quint16 respCode =  MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);
    MTPRxContainer *recvContainer = m_transactionSequence->dataContainer;

    // De-serialize the references array
    *recvContainer >> references;
    respCode = m_storageServer->setReferences(params[0], references);

    sendResponse(respCode);
}

void MTPResponder::deleteStoredRequest()
{
    MTP_FUNC_TRACE();
    // deallocate memory of stored old request if still existing
    if(0 != m_transactionSequence->dataContainer)
    {
        delete m_transactionSequence->dataContainer;
        m_transactionSequence->dataContainer = 0;
    }
    if(0 != m_transactionSequence->reqContainer)
    {
        delete m_transactionSequence->reqContainer;
        m_transactionSequence->reqContainer = 0;
    }
}

MTPResponseCode MTPResponder::preCheck(quint32 sessionID, quint32 transactionID)
{
    MTP_FUNC_TRACE();
    if( MTP_INITIAL_SESSION_ID == sessionID)
    {
        return MTP_RESP_SessionNotOpen;
    }
    else if ( transactionID == 0x00000000 || transactionID == 0xFFFFFFFF )
    {
        return MTP_RESP_InvalidTransID;
    }
    return MTP_RESP_OK;
}

void MTPResponder::freeObjproplistInfo()
{
    MTP_FUNC_TRACE();
    if(m_objPropListInfo)
    {
        for(quint32 i = 0; i < m_objPropListInfo->noOfElements; i++)
        {
            if( m_objPropListInfo->objPropList[i].value )
            {
                delete m_objPropListInfo->objPropList[i].value;
            }
        }
        if(m_objPropListInfo->objPropList)
        {
            delete[] m_objPropListInfo->objPropList;
        }
        delete m_objPropListInfo;
        m_objPropListInfo = 0;
    }
}

MTPResponseCode MTPResponder::sendObjectCheck( ObjHandle handle, const quint32 dataLen, bool isLastPacket, MTPResponseCode code )
{
    MTP_FUNC_TRACE();
    MTPResponseCode response = MTP_RESP_OK;
    if( MTP_RESP_OK == code && m_objPropListInfo )
    {
        // Update object size
        m_objPropListInfo->objectCurrSize += dataLen;
        if(m_objPropListInfo->objectSize > m_objPropListInfo->objectCurrSize)
        {
            if(isLastPacket)
            {
                // Some data packets are missing
                m_storageServer->truncateItem( handle, 0 );
                response =  MTP_RESP_IncompleteTransfer;
            }
            else
            {
                // Prepare to receive the next data packet
                response = MTP_RESP_Undefined;
            }
        }
    }
    else if( m_sendObjectSequencePtr )
    {
        // SendObjectInfo case
        quint32 bytesWritten = m_sendObjectSequencePtr->sendObjBytesWritten;
        // update the total amount of written bytes */
        m_sendObjectSequencePtr->sendObjBytesWritten = bytesWritten + dataLen;

        // check if all parts of the object have been received
        if( m_sendObjectSequencePtr->objInfo->mtpObjectCompressedSize >
            m_sendObjectSequencePtr->sendObjBytesWritten )
        {
            // check if there is an error in the container/data phase
            if( isLastPacket )
            {
                // although not all bytes of the Object were sent, the container is indicated
                // to be the last. Initiate the disposal of the data that has been written so far,
                // so that the Initiator may resend the Object with a new SendObject operation.
                //writing will start then from the beginning of the file
                m_storageServer->truncateItem( handle, 0 );
                // end the whole operation by responding with an error code
                response =  MTP_RESP_IncompleteTransfer;
            }
            else
            {
                // leave here to receive the next part
                response = MTP_RESP_Undefined;
            }
        }
    }
    return response;
}

void MTPResponder::closeSession()
{
    MTP_FUNC_TRACE();
    // FIXME: Decide on the cleanup needed here
    m_transactionSequence->mtpSessionId = MTP_INITIAL_SESSION_ID;
    deleteStoredRequest();
    m_state = RESPONDER_IDLE;
    if( m_sendObjectSequencePtr )
    {
        delete m_sendObjectSequencePtr;
        m_sendObjectSequencePtr = 0;
    }
    freeObjproplistInfo();
}

void MTPResponder::receiveEvent()
{
    MTP_FUNC_TRACE();
    // TODO: No events from the initiator are handled yet. This is still pending
}

void MTPResponder::fetchObjectSize(const quint8* data, quint64* objectSize)
{
    MTP_FUNC_TRACE();
    *objectSize = 0;
    if(data)
    {
        //If this is a data phase, the operation is SendObject
        //check if we still have a valid ObjectPropListInfo, if so return the object size.
        MTPContainerWrapper tempContainer(const_cast<quint8*>(data));
        if(MTP_CONTAINER_TYPE_DATA == tempContainer.containerType() &&
            MTP_OP_SendObject == tempContainer.code())
        {
            if(m_objPropListInfo)
            {
                *objectSize = m_objPropListInfo->objectSize;
            }
        }
    }
}

void MTPResponder::handleSuspend()
{
    MTP_LOG_WARNING("Received suspend");
    m_prevState = m_state;
    m_state = RESPONDER_SUSPEND;
}

void MTPResponder::handleResume()
{
    MTP_LOG_WARNING("Received resume");
    m_state = m_prevState;
    if( m_containerToBeResent )
    {
        m_containerToBeResent = false;
        //m_transporter->disableRW();
        //QCoreApplication::processEvents();
        //m_transporter->enableRW();
        if( RESPONDER_TX_CANCEL != m_state )
        {
            MTP_LOG_WARNING("Resume sending");
            m_transporter->sendData( m_resendBuffer, m_resendBufferSize, m_isLastPacket );
        }
        delete[] m_resendBuffer;
    }
}

void MTPResponder::handleCancelTransaction()
{
    if( !m_transactionSequence->reqContainer )
    {
        emit deviceStatusOK();
        MTP_LOG_CRITICAL("Received Cancel Transaction while in idle state : do nothing");
        //Nothing to do
        return;
    }

    MTP_LOG_CRITICAL("Received Cancel Transaction for operation " << QString("0x%1").arg( m_transactionSequence->reqContainer->code(), 0 , 16 ));

    m_state = RESPONDER_TX_CANCEL;
    switch( m_transactionSequence->reqContainer->code() )
    {
        // Host initiated cancel for host to device data transfer
        case MTP_OP_SendObject:
        case MTP_OP_SendObjectPropList:
        case MTP_OP_SendObjectInfo:
        {
            ObjHandle handle = 0x00000000;
            // A SendObjectPropListInfo was used.
            if( m_objPropListInfo )
            {
                handle = m_objPropListInfo->objectHandle;
            }
            // A SendObjectInfo was used.
            else if( m_sendObjectSequencePtr )
            {
                handle = m_sendObjectSequencePtr->objHandle;
            }
            if( 0x00000000 == handle )
            {
                MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer: No object to cancel the host to device data transfer for");
            }
            else
            {
                MTPResponseCode response = m_storageServer->deleteItem( handle, MTP_OBF_FORMAT_Undefined );
                if( MTP_RESP_OK != response )
                {
                    MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer: Object could not be deleted " << response);
                }
                else
                {
                    MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer: host to device data transfer cancelled");
                }
            }

            if( MTP_OP_SendObject == m_transactionSequence->reqContainer->code() && handle )
            {
                if( m_objPropListInfo )
                {
                    if( m_objPropListInfo->objectSize > m_objPropListInfo->objectCurrSize )
                    {
                        MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer before data xfer was completed");
                    }
                    else
                    {
                        MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer after data xfer was completed");
                    }
                }
                else if( m_sendObjectSequencePtr )
                {
                    if( m_sendObjectSequencePtr->objInfo->mtpObjectCompressedSize > m_sendObjectSequencePtr->sendObjBytesWritten )
                    {
                        MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer before data xfer was completed");
                    }
                    else
                    {
                        MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer after data xfer was completed");
                    }
                }
            }

            if( m_objPropListInfo )
            {
                freeObjproplistInfo();
            }
            else if( m_sendObjectSequencePtr )
            {
                delete m_sendObjectSequencePtr;
                m_sendObjectSequencePtr = 0;
            }
            break;
        }

        default:
        {
            MTP_LOG_CRITICAL("Ready for next transaction");
            break;
        }
    }

    deleteStoredRequest();
    emit deviceStatusOK();
}

void MTPResponder::handleDeviceReset()
{
    closeSession();
    emit deviceStatusOK();
}

bool MTPResponder::serializePropListQuantum(ObjHandle currentObj, QList<MTPObjPropDescVal> &propValList, MTPTxContainer &dataContainer)
{
    MTP_FUNC_TRACE();

    QList<MTPObjPropDescVal> notFoundList;
    bool allProps = !(propValList.count() == 1);
    // Ignore below return value
    if( allProps )
    {
        if( m_propCache->get( currentObj, propValList, notFoundList ) )
        {
            ;//Do Nothing
        }
        // Not cached yet
        else
        {
            MTP_LOG_INFO("All properties not cached yet, fetch from tracker");
            MTPResponseCode code = m_storageServer->getObjectPropertyValue(currentObj, notFoundList, false, true);
            if( MTP_RESP_OK == code )
            {
                m_propCache->add( currentObj, notFoundList );
                m_propCache->setAllProps( currentObj );
                propValList += notFoundList;
            }
        }
    }
    else
    {
        if( m_propCache->get( currentObj, propValList[0] ) )
        {
            ;//Do Nothing
        }
        // Not cached yet
        else
        {
            MTP_LOG_INFO("Property not cached yet, fetch from storage/tracker");
            MTPResponseCode code = m_storageServer->getObjectPropertyValue(currentObj, propValList);
            if( MTP_RESP_OK == code )
            {
                m_propCache->add( currentObj, propValList[0] );
            }
        }
    }
    for(QList<MTPObjPropDescVal>::const_iterator i = propValList.constBegin(); i != propValList.constEnd(); ++i)
    {
        const MtpObjPropDesc *propDesc = i->propDesc;
        dataContainer << currentObj << propDesc->uPropCode << propDesc->uDataType;
        dataContainer.serializeVariantByType(propDesc->uDataType, i->propVal);
    }
    //FIXME when do we return false
    return true;
}

void MTPResponder::sendObjectSegmented()
{
    MTP_FUNC_TRACE();

    quint32 segPayloadLength = 0;
    quint32 segDataOffset = 0;
    quint8 *segPtr = 0;
    MTPResponseCode respCode = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    MTPOperationCode opCode = reqContainer->code();
    bool sent = true;

    segDataOffset = m_segmentedSender.offset;
    segPayloadLength = m_segmentedSender.payloadLen;

    // start segmentation
    while(segDataOffset < m_segmentedSender.totalDataLen)
    {
        if(reqContainer != m_transactionSequence->reqContainer)
        {
            // Transaction was canceled or session was closed
            return;
        }
        if(m_segmentedSender.headerSent == false)
        {
            // This the first segment, thus it needs to have the MTP container header
            bool extraLargeContainer = ((m_segmentedSender.totalDataLen + MTP_HEADER_SIZE) > 0xFFFFFFFF);
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, opCode, reqContainer->transactionId(), segPayloadLength);
            dataContainer.setContainerLength(extraLargeContainer ? 0xFFFFFFFF : m_segmentedSender.totalDataLen + MTP_HEADER_SIZE);
            qint32 bytesRead = segPayloadLength;

            respCode = m_storageServer->readData(m_segmentedSender.objHandle,
                    reinterpret_cast<char*>(dataContainer.payload()), bytesRead,
                    segDataOffset);

            if(MTP_RESP_OK != respCode)
            {
                m_segmentedSender.sendResp = true;
                break;
            }

            m_segmentedSender.bytesSent += segPayloadLength;
            // Advance the container's offset
            dataContainer.seek(bytesRead);
            sent = sendContainer(dataContainer,false);
            if( false == sent )
            {
                MTP_LOG_CRITICAL("Could not send data");
                return;
            }
            m_segmentedSender.headerSent = true;
        }
        else
        {
            qint32 bytesRead = segPayloadLength;
            // The subsequent segments have no header
            segPtr = new quint8[segPayloadLength];
            respCode = m_storageServer->readData(m_segmentedSender.objHandle, (char*)segPtr, bytesRead, segDataOffset);
            if(MTP_RESP_OK != respCode)
            {
                m_segmentedSender.sendResp = true;
                delete[] (segPtr);
                break;
            }
            m_segmentedSender.bytesSent += segPayloadLength;
            // Directly call the transport method here
            m_transporter->sendData(segPtr, segPayloadLength, (m_segmentedSender.totalDataLen - segDataOffset) <= BUFFER_MAX_LEN);
            delete[] (segPtr);
        }
        // Prepare for the next segment to be sent
        segDataOffset += segPayloadLength;
        if ((m_segmentedSender.totalDataLen - segDataOffset) > BUFFER_MAX_LEN )
        {
            segPayloadLength = BUFFER_MAX_LEN;
        }
        else
        {
            segPayloadLength = m_segmentedSender.totalDataLen - segDataOffset;
            m_segmentedSender.sendResp = true;
        }

        m_segmentedSender.payloadLen = segPayloadLength;
        m_segmentedSender.offset = segDataOffset;

    }

    if(true == m_segmentedSender.sendResp)
    {
        if( true == sent )
        {
            if(MTP_OP_GetPartialObject == opCode)
            {
                sendResponse(respCode, m_segmentedSender.bytesSent);
            }
            else
            {
                sendResponse(respCode);
            }
        }
        // Segmented sending is done
        m_segmentedSender.segmentationStarted = false;
    }
}

void MTPResponder::processTransportEvents( bool &txCancelled )
{
    m_transporter->disableRW();
    QCoreApplication::processEvents();
    m_transporter->enableRW();

    txCancelled = RESPONDER_TX_CANCEL == m_state;

    if( RESPONDER_TX_CANCEL == m_state )
    {
        MTP_LOG_WARNING("Received a request to process transport events - processed a cancel");
    }
}

void MTPResponder::suspend()
{
    m_transporter->suspend();
}

void MTPResponder::resume()
{
    m_transporter->resume();
}

#if 0
// This was added as a workaround for the QMetaType bug in QT (NB #169065)
// However, the workaround for it is now also in sync-fw. So this unregistartion
// must not be done here. Even if it has to be done, it must be in the dll
// destructor routine, and not here

void MTPResponder::unregisterMetaTypes()
{
    QMetaType::unregisterType("MtpEnumForm");
    QMetaType::unregisterType("MtpRangeForm");
    QMetaType::unregisterType("MtpInt128");
    QMetaType::unregisterType("MTPEventCode");
    QMetaType::unregisterType("char");
    QMetaType::unregisterType("QVector<char>");
    QMetaType::unregisterType("QVector<qint8>");
    QMetaType::unregisterType("QVector<qint16>");
    QMetaType::unregisterType("QVector<qint32>");
    QMetaType::unregisterType("QVector<qint64>");
    QMetaType::unregisterType("QVector<quint8>");
    QMetaType::unregisterType("QVector<quint16>");
    QMetaType::unregisterType("QVector<quint32>");
    QMetaType::unregisterType("QVector<quint64>");
    QMetaType::unregisterType("QVector<MtpInt128>");
}
#endif

void MTPResponder::invalidatePropertyCache(ObjHandle objHandle, MTPObjPropertyCode propCode)
{
    m_propCache->remove(objHandle, propCode);
}
