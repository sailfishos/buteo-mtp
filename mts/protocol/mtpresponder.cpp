/*
* This file is part of libmeegomtp package
*
* Copyright (c) 2010 Nokia Corporation. All rights reserved.
* Copyright (c) 2013 - 2022 Jolla Ltd.
* Copyright (c) 2020 Open Mobile Platform LLC.
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
#include "storagefactory.h"
#include "trace.h"
#include "deviceinfoprovider.h"
#include "mtptransporterusb.h"
#include "mtptransporterdummy.h"
#include "propertypod.h"
#include "objectpropertycache.h"
#include "mtpextensionmanager.h"

using namespace meegomtp1dot0;

/* Once we write configuration to control endpoint, we *must* be ready
 * to serve ptp/mtp commands - otherwise mtp initiator at the other end
 * sees responce delay proportional to amount of files that need to be
 * scanned before storages are ready.
 *
 * The correct fix would probably be to start with empty set of storages
 * and send notification events when file enumeration for each storage
 * finishes, but not all responders are able to cope with that and it
 * would also require major changes to buteo mtp side.
 *
 * As a workaround: Defer endpoint configuration until scanning of all
 * storages has been completed.
 */
#define DEFER_TRANSPORTER_ACTIVATION 1

// BUFFER_MAX_LEN is based on the max request size in ci13xxx_udc.c,
// which is four pages of 4k each
static const quint32 BUFFER_MAX_LEN = 4 * 4096;
MTPResponder *MTPResponder::m_instance = 0;

MTPResponder *MTPResponder::instance()
{
    MTP_FUNC_TRACE();
    if (0 == m_instance) {
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
    m_extensionManager(new MTPExtensionManager),
    m_copiedObjHandle(0),
    m_containerToBeResent(false),
    m_isLastPacket(false),
    m_storageWaitDataComplete(false),
    m_state_accessor_only(RESPONDER_IDLE),
    m_prevState(RESPONDER_IDLE),
    m_handler_idle_timer(0),
    m_objPropListInfo(0),
    m_sendObjectSequencePtr(0),
    m_editObjectSequencePtr(nullptr),
    m_transactionSequence(new MTPTransactionSequence)
{
    MTP_FUNC_TRACE();

    // command handler idle timer
    m_handler_idle_timer = new QTimer(this);
    m_handler_idle_timer->setInterval(100);
    m_handler_idle_timer->setSingleShot(true);

    connect(m_handler_idle_timer, &QTimer::timeout,
            this, &MTPResponder::onIdleTimeout);

    createCommandHandler();

    connect(m_devInfoProvider, &MtpDeviceInfo::devicePropertyChanged,
            this, &MTPResponder::onDevicePropertyChanged);
}

bool MTPResponder::initTransport( TransportType transport )
{
    bool transportOk = true;
    if (USB == transport) {
        m_transporter = new MTPTransporterUSB();
#if DEFER_TRANSPORTER_ACTIVATION
        MTP_LOG_INFO("Deferring transporter activate");
        transportOk = true;
#else
        transportOk = m_transporter->activate();
#endif
        if ( transportOk ) {
            // Connect signals to the transporter
            QObject::connect(this, SIGNAL(sessionOpenChanged(bool)),
                             m_transporter, SLOT(sessionOpenChanged(bool)));

            // Connect signals from the transporter
            QObject::connect(m_transporter, SIGNAL(dataReceived(quint8 *, quint32, bool, bool)),
                             this, SLOT(receiveContainer(quint8 *, quint32, bool, bool)));
            QObject::connect(m_transporter, SIGNAL(eventReceived()), this, SLOT(receiveEvent()));
            QObject::connect(m_transporter, SIGNAL(cleanup()), this, SLOT(closeSession()));
            QObject::connect(m_transporter, SIGNAL(fetchObjectSize(const quint8 *, quint64 *)),
                             this, SLOT(fetchObjectSize(const quint8 *, quint64 *)));
            QObject::connect(this, SIGNAL(deviceStatusOK()), m_transporter, SLOT(sendDeviceOK()));
            QObject::connect(this, SIGNAL(deviceStatusBusy()), m_transporter, SLOT(sendDeviceBusy()));
            QObject::connect(this, SIGNAL(deviceStatusTxCancelled()), m_transporter, SLOT(sendDeviceTxCancelled()));
            QObject::connect(m_transporter, SIGNAL(cancelTransaction()), this, SLOT(handleCancelTransaction()));
            QObject::connect(m_transporter, SIGNAL(deviceReset()), this, SLOT(handleDeviceReset()));
            QObject::connect(m_transporter, SIGNAL(suspendSignal()), this, SLOT(handleSuspend()));
            QObject::connect(m_transporter, SIGNAL(resumeSignal()), this, SLOT(handleResume()));
        }
    } else if (DUMMY == transport) {
        m_transporter = new MTPTransporterDummy();
    }
    emit deviceStatusOK();
    return transportOk;
}

bool MTPResponder::initStorages()
{
    m_storageServer = new StorageFactory();

    connect(m_storageServer, &StorageFactory::checkTransportEvents,
            this, &MTPResponder::processTransportEvents);
    connect(m_storageServer, &StorageFactory::storageReady,
            this, &MTPResponder::onStorageReady);

    // Inform storage server that a new session has been opened/closed
    connect(this, &MTPResponder::sessionOpenChanged,
            m_storageServer, &StorageFactory::sessionOpenChanged);

    /* Fixme: New style connect can't be used as responder
     *        has pointer to transporter base class while
     *        the slots are in derived usb/dummy classes. */
    connect(m_storageServer, SIGNAL(storageReady()),
            m_transporter, SLOT(onStorageReady()));

    QVector<quint32> failedStorageIds;
    bool result = m_storageServer->enumerateStorages(failedStorageIds);
    if (!result) {
        //TODO What action to take if enumeration fails?
        MTP_LOG_CRITICAL("Failed to enumerate storages");
        foreach (quint32 storageId, failedStorageIds) {
            MTP_LOG_CRITICAL("Failed storage:" << QString("0x%1").arg(storageId, 0, 16));
        }
    }
    return result;
}

MTPResponder::~MTPResponder()
{
    MTP_FUNC_TRACE();

    if (m_storageServer) {
        delete m_storageServer;
        m_storageServer = 0;
    }

    if (m_transporter) {
        delete m_transporter;
        m_transporter = 0;
    }

    if (m_propertyPod) {
        PropertyPod::releaseInstance();
        m_propertyPod = 0;
    }

    if (m_devInfoProvider) {
        delete m_devInfoProvider;
        m_devInfoProvider = 0;
    }

    if ( m_transactionSequence ) {
        deleteStoredRequest();
        delete m_transactionSequence;
        m_transactionSequence = 0;
    }

    if (m_extensionManager) {
        delete m_extensionManager;
        m_extensionManager = 0;
    }

    if ( m_sendObjectSequencePtr ) {
        delete m_sendObjectSequencePtr;
        m_sendObjectSequencePtr = 0;
    }

    delete m_editObjectSequencePtr;
    m_editObjectSequencePtr = nullptr;

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
    m_opCodeTable[MTP_OP_ANDROID_GetPartialObject64] = &MTPResponder::getPartialObject64Req;
    m_opCodeTable[MTP_OP_ANDROID_SendPartialObject64] = &MTPResponder::sendPartialObject64Req;
    m_opCodeTable[MTP_OP_ANDROID_TruncateObject64] = &MTPResponder::truncateObject64Req;
    m_opCodeTable[MTP_OP_ANDROID_BeginEditObject] = &MTPResponder::beginEditObjectReq;
    m_opCodeTable[MTP_OP_ANDROID_EndEditObject] = &MTPResponder::endEditObjectReq;
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

    int type = container.containerType();
    int code = container.code();
    quint32 size = container.containerLength();

    if ( type == MTP_CONTAINER_TYPE_RESPONSE && code != MTP_RESP_OK ) {
        MTP_LOG_WARNING(mtp_container_type_repr(type) << mtp_code_repr(code) << size << isLastPacket);
    } else {
        MTP_LOG_INFO(mtp_container_type_repr(type) << mtp_code_repr(code) << size << isLastPacket);
    }

    if ( !m_transporter ) {
        MTP_LOG_WARNING("Transporter not set; ignoring container");
        return false;
    }

    if (MTP_CONTAINER_TYPE_RESPONSE == container.containerType() || MTP_CONTAINER_TYPE_DATA == container.containerType() ||
            MTP_CONTAINER_TYPE_EVENT == container.containerType() ) {
        //m_transporter->disableRW();
        //QCoreApplication::processEvents();
        //m_transporter->enableRW();
        if ( RESPONDER_TX_CANCEL == getResponderState() && MTP_CONTAINER_TYPE_EVENT != container.containerType()) {
            return false;
        }
        if ( RESPONDER_SUSPEND == getResponderState() ) {
            MTP_LOG_WARNING("Received suspend while sending");
            if ( MTP_CONTAINER_TYPE_EVENT != container.containerType() ) {
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
    if (MTP_CONTAINER_TYPE_EVENT == container.containerType()) {
        m_transporter->sendEvent(container.buffer(), container.bufferSize(), isLastPacket);
    } else {
        if (MTP_CONTAINER_TYPE_RESPONSE == container.containerType()) {
            /* Data sending is done asynchronously from other thread. From protocol
             * point of view the host can send the next request when that send finishes,
             * not when buteo-mtp has managed to set some internal state -> we have
             * a race to set the m_state to RESPONDER_IDLE ... as a workaround do
             * the state transition before starting the transfer. */
            setResponderState(RESPONDER_IDLE);
        }
        m_transporter->sendData(container.buffer(), container.bufferSize(), isLastPacket);
    }
    if (MTP_CONTAINER_TYPE_RESPONSE == container.containerType()) {
        // Restore state to IDLE to get ready to received the next operation
        emit deviceStatusOK();
        deleteStoredRequest();
    }
    return true;
}

bool MTPResponder::sendResponse(MTPResponseCode code)
{
    MTP_FUNC_TRACE();

    if (0 == m_transactionSequence->reqContainer) {
        MTP_LOG_WARNING("Transaction gone, not sending response");
        return false;
    }

    quint16 transactionId = m_transactionSequence->reqContainer->transactionId();
    MTPTxContainer respContainer(MTP_CONTAINER_TYPE_RESPONSE, code, transactionId);
    bool sent = sendContainer(respContainer);
    if ( false == sent ) {
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
    if ( false == sent ) {
        MTP_LOG_CRITICAL("Could not send response");
    }
    return sent;
}

void MTPResponder::receiveContainer(quint8 *data, quint32 dataLen, bool isFirstPacket, bool isLastPacket)
{
    MTP_FUNC_TRACE();
    // TODO: Change this to handle event containers. Event containers can be
    // received in any state.
    switch (getResponderState()) {
    case RESPONDER_IDLE:
    case RESPONDER_TX_CANCEL:
    case RESPONDER_SUSPEND: {
        setResponderState(RESPONDER_IDLE);
        // Delete any old request, just in case
        if (0 != m_transactionSequence->reqContainer) {
            delete m_transactionSequence->reqContainer;
            m_transactionSequence->reqContainer = 0;
        }
        // This must be a request container, request containers cannot
        // be segmented!
        if (isFirstPacket && isLastPacket) {
            // Populate our internal request container
            m_transactionSequence->reqContainer = new MTPRxContainer(data, dataLen);
            // Check if the operation has a data phase
            if (hasDataPhase(m_transactionSequence->reqContainer->code())) {
                // Change state to expect an (optional) data phase from the
                // initiator. We need to let the data phase complete even if
                // there is an error in the request phase, before entering
                // the response phase.
                setResponderState(RESPONDER_WAIT_DATA);
            } else {
                setResponderState(RESPONDER_WAIT_RESP);
            }

            // Handle the command phase
            emit deviceStatusBusy();
            commandHandler();
        } else {
            setResponderState(RESPONDER_IDLE);
            MTP_LOG_CRITICAL("Invalid container received! Expected command, received data");
            m_transporter->reset();
        }
    }
    break;
    case RESPONDER_WAIT_DATA: {
        // If no valid request container, then exit...
        if (0 == m_transactionSequence->reqContainer) {
            setResponderState(RESPONDER_IDLE);
            MTP_LOG_CRITICAL("Received a data container before a request container!");
            m_transporter->reset();
            break;
        }
        if ( isFirstPacket ) {
            emit deviceStatusBusy();
        }
        // This must be a data container
        dataHandler(data, dataLen, isFirstPacket, isLastPacket);
    }
    break;
    case RESPONDER_WAIT_STORAGE:
        if (isFirstPacket) {
            if (!m_storageWaitData.isEmpty()) {
                // Initiator apparently didn't wait for response
                setResponderState(RESPONDER_IDLE);
                MTP_LOG_CRITICAL("Received more than one container while waiting for storage");
                m_transporter->reset();
                break;
            }
        }
        m_storageWaitData.append((char *) data, dataLen);
        m_storageWaitDataComplete = isLastPacket;
        break;
    case RESPONDER_WAIT_RESP:
    default:
        MTP_LOG_CRITICAL("Container received in wrong state!" << getResponderState());
        setResponderState(RESPONDER_IDLE);
        m_transporter->reset();
        break;
    }
}

void MTPResponder::onStorageReady(void)
{
    MTP_FUNC_TRACE();

    MTP_LOG_INFO("Storage ready");

#if DEFER_TRANSPORTER_ACTIVATION
    if (!m_transporter->activate())
        MTP_LOG_CRITICAL("Transporter activate failed");
    else
        MTP_LOG_INFO("Transporter activated");
#endif

    // Retry the last command, if one was waiting
    if (getResponderState() == RESPONDER_WAIT_STORAGE) {
        if (hasDataPhase(m_transactionSequence->reqContainer->code()))
            setResponderState(RESPONDER_WAIT_DATA);
        else
            setResponderState(RESPONDER_WAIT_RESP);
        MTP_LOG_INFO("Retrying operation");
        commandHandler();

        // Replay any data that arrived while waiting for storageReady
        if (!m_storageWaitData.isEmpty()) {
            MTP_LOG_INFO("Replaying data," << m_storageWaitData.size() << "bytes");
            receiveContainer((quint8 *) m_storageWaitData.data(),
                             m_storageWaitData.size(), true, m_storageWaitDataComplete);
        }
        m_storageWaitData.clear();
        m_storageWaitDataComplete = false;
    }
}

void MTPResponder::commandHandler()
{
    MTP_FUNC_TRACE();

    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    QVector<quint32> params;

    bool waitForDataPhase = false;

    reqContainer->params(params);

    int type = reqContainer->containerType();
    int code = reqContainer->code();

    quint32 ObjectHandle = 0;
    switch ( code ) {
    case MTP_OP_GetObjectInfo:
    case MTP_OP_GetObject:
    case MTP_OP_GetThumb:
    case MTP_OP_DeleteObject:
    case MTP_OP_GetPartialObject:
    case MTP_OP_SetObjectProtection:
    case MTP_OP_MoveObject:
    case MTP_OP_CopyObject:
    case MTP_OP_GetObjectPropValue:
    case MTP_OP_SetObjectPropValue:
    case MTP_OP_GetObjectPropList:
    case MTP_OP_GetObjectReferences:
    case MTP_OP_SetObjectReferences:
    case MTP_OP_ANDROID_GetPartialObject64:
    case MTP_OP_ANDROID_SendPartialObject64:
    case MTP_OP_ANDROID_TruncateObject64:
    case MTP_OP_ANDROID_BeginEditObject:
    case MTP_OP_ANDROID_EndEditObject:
        ObjectHandle = params[0];
        break;

    case MTP_OP_SendObjectInfo:     // parent object
    case MTP_OP_SendObjectPropList: // parent object
        ObjectHandle = params[1];
        break;

    case MTP_OP_GetNumObjects:      // top object
    case MTP_OP_GetObjectHandles:   // top object
        ObjectHandle = params[2];
        break;

    default:
        break;
    }
    QString ObjectPath("n/a");
    if ( ObjectHandle != 0x00000000 && ObjectHandle != 0xffffffff )
        m_storageServer->getPath(ObjectHandle, ObjectPath);

    MTP_LOG_INFO(mtp_container_type_repr(type)
                 << mtp_code_repr(code)
                 << ObjectPath);

    // preset the response code - to be changed if the handler of the operation
    // detects an error in the operation phase
    m_transactionSequence->mtpResp = MTP_RESP_OK;

    if (!m_storageServer->storageIsReady()) {
        if (needsStorageReady(reqContainer->code())) {
            MTP_LOG_INFO("Will wait for storageReady");
            setResponderState(RESPONDER_WAIT_STORAGE);
            m_storageWaitData.clear();
            m_storageWaitDataComplete = false;
            return; // onStorageReady will call again later
        } else {
            MTP_LOG_INFO("Storage not yet ready but operation is safe, continuing");
        }
    }

    if (m_opCodeTable.contains(reqContainer->code())) {
        (this->*(m_opCodeTable[reqContainer->code()]))();
    } else if (true == m_extensionManager->operationHasDataPhase(reqContainer->code(), waitForDataPhase)) {
        // Operation handled by an extension
        if (false == waitForDataPhase) {
            // TODO: Check the return below? Can we assume that this will succeed as operationHasDataPhase has succeeded?
            //handleExtendedOperation();
            sendResponse(MTP_RESP_OperationNotSupported);
        }
    } else {
        // invalid opcode -- This response should be sent right away (no need to
        // wait on the data phase)
        sendResponse(MTP_RESP_OperationNotSupported);
    }
}

bool MTPResponder::hasDataPhase(MTPOperationCode code)
{
    bool ret = false;
    switch (code) {
    case MTP_OP_SendObjectInfo:
    case MTP_OP_SendObject:
    case MTP_OP_SetObjectPropList:
    case MTP_OP_SendObjectPropList:
    case MTP_OP_SetDevicePropValue:
    case MTP_OP_SetObjectPropValue:
    case MTP_OP_SetObjectReferences:
    case MTP_OP_ANDROID_SendPartialObject64:
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
    case MTP_OP_ANDROID_GetPartialObject64:
    case MTP_OP_ANDROID_TruncateObject64:
    case MTP_OP_ANDROID_BeginEditObject:
    case MTP_OP_ANDROID_EndEditObject:
        ret = false;
        break;
    default:
        // Try to check with the extension manager
        m_extensionManager->operationHasDataPhase(code, ret);
    }
    return ret;
}

void MTPResponder::dataHandler(quint8 *data, quint32 dataLen, bool isFirstPacket, bool isLastPacket)
{
    MTP_FUNC_TRACE();
    // Cached responce status inherited from command phase
    MTPResponseCode respCode = m_transactionSequence->mtpResp;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    MTP_LOG_INFO("dataLen:" << dataLen << "isFirstPacket:" << isFirstPacket << "isLastPacket:" << isLastPacket <<
                 "on entry:" << mtp_code_repr(m_transactionSequence->mtpResp));

    /* Except for potentially huge file content transfers, data packets
     * are concatenated into a full container block before processing.
     */
    switch ( reqContainer->code() ) {
    case MTP_OP_SendObject:
    case MTP_OP_ANDROID_SendPartialObject64:
        // Each packet is passed through to file system as they come
        delete m_transactionSequence->dataContainer;
        m_transactionSequence->dataContainer = nullptr;
        break;
    default:
        // Packets are concatenated to form full data before handling
        if (isFirstPacket) {
            // Reset data container
            delete m_transactionSequence->dataContainer;
            m_transactionSequence->dataContainer = new MTPRxContainer(data, dataLen);
        } else if (m_transactionSequence->dataContainer) {
            // Call append on the previously stored data container
            m_transactionSequence->dataContainer->append(data, dataLen);
        }
        if ( !isLastPacket ) {
            // Return here and wait for the next segment in the data phase
            return;
        }
        break;
    }

    /* When applicable, sanity check container block */
    if ( respCode == MTP_RESP_OK && m_transactionSequence->dataContainer ) {
        // Match the transaction IDs from the request phase
        if ( m_transactionSequence->dataContainer->transactionId() != reqContainer->transactionId() ) {
            respCode = MTP_RESP_InvalidTransID;
        }
        // check whether the opcode of the data container corresponds to the one of the operation phase
        else if (m_transactionSequence->dataContainer->code() != reqContainer->code() ) {
            respCode = MTP_RESP_GeneralError;
        }
    }

    /* Command specific actions */
    if ( respCode == MTP_RESP_OK ) {
        switch (reqContainer->code()) {
        // For operations with no I->R data phases, we should never get
        // here!
        case MTP_OP_SendObjectInfo: {
            sendObjectInfoData();
            return;
        }
        case MTP_OP_SendObject: {
            sendObjectData(data, dataLen, isFirstPacket, isLastPacket);
            return;
        }
        case MTP_OP_ANDROID_SendPartialObject64: {
            respCode = sendPartialObject64Data(data, dataLen, isFirstPacket, isLastPacket);
            break;
        }
        case MTP_OP_SetObjectPropList: {
            setObjectPropListData();
            return;
        }
        case MTP_OP_SendObjectPropList: {
            sendObjectPropListData();
            return;
        }
        case MTP_OP_SetDevicePropValue: {
            setDevicePropValueData();
            return;
        }
        case MTP_OP_SetObjectPropValue: {
            setObjPropValueData();
            return;
        }
        case MTP_OP_SetObjectReferences: {
            setObjReferencesData();
            return;
        }
        default: {
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

    // Update cached status
    m_transactionSequence->mtpResp = respCode;

    MTP_LOG_INFO("dataLen:" << dataLen << "isFirstPacket:" << isFirstPacket << "isLastPacket:" << isLastPacket <<
                 "on leave:" << mtp_code_repr(m_transactionSequence->mtpResp));

    // RESPONSE PHASE
    if ( isLastPacket )
        sendResponse(respCode);
}

bool MTPResponder::handleExtendedOperation()
{
    MTP_FUNC_TRACE();
    bool ret = false;
    if (m_transactionSequence && m_transactionSequence->reqContainer) {
        MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
        MTPRxContainer *dataContainer = m_transactionSequence->dataContainer;
        MtpRequest req;
        MtpResponse resp;

        req.opCode = reqContainer->code();
        reqContainer->params(req.params);
        if (0 != dataContainer) {
            req.data = dataContainer->payload();
            req.dataLen = (dataContainer->containerLength() - MTP_HEADER_SIZE);
        }
        if (true == (ret = m_extensionManager->handleOperation(req, resp))) {
            // Extension handled this operation successfully, send the data (if present)
            // and the response
            if (0 != resp.data && 0 < resp.dataLen) {
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                             resp.dataLen);
                memcpy(dataContainer.payload(), resp.data, resp.dataLen);
                dataContainer.seek(resp.dataLen);
                bool sent = sendContainer(dataContainer);
                if ( false == sent ) {
                    MTP_LOG_CRITICAL("Could not send data");
                }
                delete[] resp.data;
            }
            // Send response
            MTPTxContainer respContainer(MTP_CONTAINER_TYPE_RESPONSE, resp.respCode, reqContainer->transactionId(),
                                         resp.params.size() * sizeof(MtpParam));
            for (int i = 0; i < resp.params.size(); i++) {
                respContainer << resp.params[i];
            }
            bool sent = sendContainer(respContainer);
            if ( false == sent ) {
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
    switch () {
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
    MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                 payloadLength);

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
    if ( false == sent ) {
        MTP_LOG_CRITICAL("Could not send data");
    }

    if ( true == sent ) {
        sendResponse(MTP_RESP_OK);
    }
}

void MTPResponder::openSessionReq()
{
    MTP_FUNC_TRACE();
    QVector<quint32> params;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    reqContainer->params(params);

    if ((MTP_INITIAL_SESSION_ID == params[0])) {
        sendResponse(MTP_RESP_InvalidParameter);
    } else if (m_transactionSequence->mtpSessionId != MTP_INITIAL_SESSION_ID) {
        sendResponse(MTP_RESP_SessionAlreadyOpen, m_transactionSequence->mtpSessionId);
    } else {
        // save new session id
        m_transactionSequence->mtpSessionId = params[0];

        sendResponse(MTP_RESP_OK);

        // Enable event sending etc
        emit sessionOpenChanged(true);
    }
}

void MTPResponder::closeSessionReq()
{
    MTP_FUNC_TRACE();
    quint16 code =  MTP_RESP_OK;

    if (MTP_INITIAL_SESSION_ID == m_transactionSequence->mtpSessionId) {
        code = MTP_RESP_SessionNotOpen;
    } else {
        m_transactionSequence->mtpSessionId = MTP_INITIAL_SESSION_ID;

        if ( m_sendObjectSequencePtr ) {
            delete m_sendObjectSequencePtr;
            m_sendObjectSequencePtr = 0;
        }

        freeObjproplistInfo();

        // FIXME: Trigger the discarding of a file, which has been possibly created in StorageServer

        // Disable event sending etc
        emit sessionOpenChanged(false);
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

    if (MTP_RESP_OK == code) {
        // if session ID and transaction ID are valid
        // check still whether the storageIDs can be obtained from StorageServer
        code = m_storageServer->storageIds(storageIds);
    }
    bool sent = true;
    // if all preconditions fulfilled start enter data phase
    if (MTP_RESP_OK == code) {
        // DATA PHASE
        quint32 payloadLength = storageIds.size() * sizeof(quint32) + sizeof(quint32);
        MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                     payloadLength);
        dataContainer << storageIds;
        sent = sendContainer(dataContainer);
        if ( false == sent ) {
            MTP_LOG_CRITICAL("Could not send data");
        }
    }
    // RESPONSE PHASE
    if ( true == sent ) {
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
    if ( MTP_RESP_OK == code ) {
        QVector<quint32> params;
        reqContainer->params(params);
        // check still if the StorageID is correct and the storage is OK
        quint32 storageId = params[0];
        code = m_storageServer->checkStorage( storageId );
        if ( MTP_RESP_OK == code ) {
            MTPStorageInfo storageInfo;

            // get info from storage server
            code = m_storageServer->storageInfo( storageId, storageInfo );

            if ( MTP_RESP_OK == code ) {
                // all information for StorageInfo dataset retrieved from Storage Server
                // determine total length of Storage Info dataset
                payloadLength = MTP_STORAGE_INFO_SIZE + ( ( storageInfo.storageDescription.size() + 1 ) * 2 ) +
                                ( ( storageInfo.volumeLabel.size() + 1 ) * 2 );

                // Create data container
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                             payloadLength);

                dataContainer << storageInfo.storageType << storageInfo.filesystemType << storageInfo.accessCapability
                              << storageInfo.maxCapacity << storageInfo.freeSpace << storageInfo.freeSpaceInObjects
                              << storageInfo.storageDescription << storageInfo.volumeLabel;

                // Check the storage once more before entering the data phase
                // if the storage factory returns an error the data phase will be skipped
                code = m_storageServer->checkStorage( storageId );
                if ( MTP_RESP_OK == code ) {
                    sent = sendContainer(dataContainer);
                    if ( false == sent ) {
                        MTP_LOG_CRITICAL("Could not send data");
                    }
                }
            }
        }
    }

    if ( true == sent ) {
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

    // Check the validity of the operation params.
    if ( MTP_RESP_OK == code ) {
        // Check if the storage id sent is valid.
        if ( 0xFFFFFFFF != params[0] ) {
            code = m_storageServer->checkStorage( params[0] );
        }
        if ( MTP_RESP_OK == code ) {
            // Check if the object format, if provided, is valid
            QVector<quint16> formats = m_devInfoProvider->supportedFormats();
            if ( params[1] && !formats.contains( params[1] ) ) {
                code = MTP_RESP_Invalid_ObjectProp_Format;
            }
            // Check if the object handle, if provided is valid
            if ( code == MTP_RESP_OK && 0x00000000 != params[2] && 0xFFFFFFFF != params[2] ) {
                code = m_storageServer->checkHandle( params[2] );
            }
        }
    }

    if ( MTP_RESP_OK == code ) {
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

    // Check the validity of the operation params.
    if ( MTP_RESP_OK == code ) {
        // Check if the storage id sent is valid.
        if ( 0xFFFFFFFF != params[0] ) {
            code = m_storageServer->checkStorage( params[0] );
        }
        if ( MTP_RESP_OK == code ) {
            // Check if the object format, if provided, is valid
            QVector<quint16> formats = m_devInfoProvider->supportedFormats();
            if ( params[1] && !formats.contains( params[1] ) ) {
                code = MTP_RESP_Invalid_ObjectProp_Format;
            }
            // Check if the object handle, if provided is valid
            if ( code == MTP_RESP_OK && 0x00000000 != params[2] && 0xFFFFFFFF != params[2] ) {
                code = m_storageServer->checkHandle( params[2] );
            }
        }
    }

    if ( MTP_RESP_OK == code ) {
        // retrieve the number of objects from storage server
        code = m_storageServer->getObjectHandles(params[0],
                                                 static_cast<MTPObjFormatCode>(params[1]),
                                                 params[2], handles);
    }
    bool sent = true;
    if ( MTP_RESP_OK == code ) {
        // At least one PTP client (iPhoto) only shows all pictures if
        // the handles are sorted. It's probably related to having parent
        // folders listed before the objects they contain.
        qSort(handles);
        MTP_LOG_INFO("handle count:" << handles.size());
        // DATA PHASE
        payloadLength = ( handles.size() + 1 ) * sizeof(quint32);
        MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                     payloadLength);
        dataContainer << handles;
        sent = sendContainer(dataContainer);
        if ( false == sent ) {
            MTP_LOG_CRITICAL("Could not send data");
        }
    }

    if ( true == sent ) {
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
    if ((MTP_RESP_OK == code) && (MTP_RESP_OK == (code = m_storageServer->getObjectInfo(params[0], objectInfo)))) {
        payloadLength = sizeof(MTPObjectInfo);
        // consider the variable part(strings) in the ObjectInfo dataset
        // for the length of the payload
        payloadLength += ( objectInfo->mtpFileName.size() ? ( objectInfo->mtpFileName.size() + 1 ) * 2 : 0 );
        payloadLength += ( objectInfo->mtpCaptureDate.size() ? ( objectInfo->mtpCaptureDate.size() + 1 ) * 2 : 0 );
        payloadLength += ( objectInfo->mtpModificationDate.size() ? ( objectInfo->mtpModificationDate.size() + 1 ) * 2 : 0 );

        MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                     payloadLength);

        dataContainer << *objectInfo;

        sent = sendContainer(dataContainer);
        if ( false == sent ) {
            MTP_LOG_CRITICAL("Could not send data");
        }
    }

    if ( true == sent ) {
        sendResponse(code);
    }
}

// Logic common for getObjectReq(), getPartialObjectReq() and getPartialObject64Req()
void MTPResponder::getObjectCommon(quint32 handle, quint64 offs, quint64 size)
{
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;
    bool sendReply = true;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    MTP_LOG_INFO("request - handle:" << handle << "offs:" << offs << "size:" << size);

    // check if everthing is ok, so that cmd can be processed
    if ( code == MTP_RESP_OK )
        code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    const MTPObjectInfo *objectInfo = nullptr;

    if ( code == MTP_RESP_OK )
        code = m_storageServer->getObjectInfo(handle, objectInfo);

    if ( code == MTP_RESP_OK ) {
        quint64 tail = offs + size;

        if ( objectInfo->mtpObjectFormat == MTP_OBF_FORMAT_Association ) {
            MTP_LOG_WARNING("handle:" << handle << "is not a regular file");
            code = MTP_RESP_InvalidObjectHandle;
        } else if ( offs > objectInfo->mtpObjectCompressedSize ) {
            MTP_LOG_WARNING("handle:" << handle << "read past file end");
            code = MTP_RESP_InvalidParameter;
        } else if ( tail < offs ) {
            MTP_LOG_WARNING("handle:" << handle << "read span overflow");
            code = MTP_RESP_InvalidParameter;
        } else {
            // Clip read extent to what is expected to be available
            offs = qMin(offs, objectInfo->mtpObjectCompressedSize);
            tail = qMin(tail, objectInfo->mtpObjectCompressedSize);
            size = tail - offs;

            MTP_LOG_INFO("transmit - handle:" << handle << "offs:" << offs << "size:" << size);

            // Start data phase - response will be sent from sendObjectSegmented()
            sendReply = false;
            MTP_LOG_INFO("handle:" << handle << "start segmented data phase");
            m_segmentedSender.objHandle    = handle;
            m_segmentedSender.offsetNow    = offs;
            m_segmentedSender.offsetEnd    = tail;
            sendObjectSegmented();
        }
    }

    if ( sendReply )
        sendResponse(code);
}


void MTPResponder::getObjectReq()
{
    MTP_FUNC_TRACE();
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);
    quint32 handle = params[0];
    quint64 offs   = 0;
    quint64 size   = MTP_MAX_CONTENT_SIZE;
    getObjectCommon(handle, offs, size);
}

void MTPResponder::getThumbReq()
{
    MTP_FUNC_TRACE();
    bool sent = false;
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    code = preCheck(m_transactionSequence->mtpSessionId,
                    reqContainer->transactionId());
    if ( MTP_RESP_OK == code ) {
        QVector<quint32> params;
        reqContainer->params( params );

        const MtpObjPropDesc *propDesc = 0;
        m_propertyPod->getObjectPropDesc( MTP_IMAGE_FORMAT,
                                          MTP_OBJ_PROP_Rep_Sample_Data, propDesc );

        QList<MTPObjPropDescVal> propValList;
        propValList.append( MTPObjPropDescVal ( propDesc ) );

        code = m_storageServer->getObjectPropertyValue( params[0], propValList );
        if ( MTP_RESP_OK == code ) {
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
            if ( false == sent ) {
                MTP_LOG_CRITICAL("Could not send thumbnail data");
            }
        }
    }

    if ( true == sent ) {
        sendResponse(code);
    }
}

void MTPResponder::deleteObjectReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if ( MTP_RESP_OK == code ) {
        QVector<quint32> params;
        reqContainer->params(params);
        code = m_storageServer->deleteItem(params[0], static_cast<MTPObjFormatCode>(params[1]));
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
    // -> MTPResponder::dataHandler()
    //    -> sendObjectInfoData()
}

void MTPResponder::sendObjectReq()
{
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // check whether the used IDs are OK
    code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if ( MTP_RESP_OK == code ) {
        // check still whether a SendObjectInfo dataset or an objectproplist dataset exists
        // TODO Why was the 0 != size check added? It will be 0 for abstract objects like playlists
        if ( ( !m_sendObjectSequencePtr
                && !m_objPropListInfo )  /*|| (m_objPropListInfo && 0 == m_objPropListInfo->objectSize)*/ ) {
            // Either there has been no SendObjectInfo taken place before
            // or an error occured in the SendObjectInfo, so that the
            // necessary ObjectInfo dataset does not exists */
            code = MTP_RESP_NoValidObjectInfo;
        }
    }
    m_transactionSequence->mtpResp = code;
    // DATA PHASE will follow
    // -> MTPResponder::dataHandler()
    //    -> sendObjectData()
}

void MTPResponder::getPartialObjectReq()
{
    MTP_FUNC_TRACE();
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);
    quint32 handle = params[0];
    quint64 offs   = params[1];
    quint64 size   = params[2];
    getObjectCommon(handle, offs, size);
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
    if ( MTP_RESP_OK == code ) {
        MtpDevPropDesc *propDesc = 0;
        quint32 payloadLength = sizeof(MtpDevPropDesc); // approximation
        QVector<quint32> params;
        reqContainer->params(params);
        code = m_propertyPod->getDevicePropDesc(params[0], &propDesc);
        if (MTP_RESP_OK == code && 0 != propDesc) {
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                         payloadLength);
            dataContainer << *propDesc;
            sent = sendContainer(dataContainer);
            if ( false == sent ) {
                MTP_LOG_CRITICAL("Could not send data");
            }
        }
    }

    if ( true == sent ) {
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
    if ( MTP_RESP_OK == code ) {
        QVector<quint32> params;
        reqContainer->params(params);
        MtpDevPropDesc *propDesc = 0;
        code = m_propertyPod->getDevicePropDesc(params[0], &propDesc);

        if (MTP_RESP_OK == code && 0 != propDesc) {
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                         payloadLength);
            dataContainer.serializeVariantByType(propDesc->uDataType, propDesc->currentValue);
            sent = sendContainer(dataContainer);
            if ( false == sent ) {
                MTP_LOG_CRITICAL("Could not send data");
            }
        }
    }

    if ( true == sent ) {
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
    if ( MTP_RESP_OK == code ) {
        QVector<quint32> params;
        reqContainer->params(params);
        MTPDevPropertyCode propCode = static_cast<MTPDevPropertyCode>(params[0]);
        MtpDevPropDesc *propDesc = 0;

        code = m_propertyPod->getDevicePropDesc(propCode, &propDesc);
        if (MTP_RESP_OK != code || 0 == propDesc) {
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
    if (MTP_RESP_OK == code) {
        QVector<quint32> params;
        reqContainer->params(params);
        // Check if the object handle passed to us is valid and exists in the storage passed to us
        if ( MTP_RESP_OK != ( code = m_storageServer->checkHandle(params[0]) ) ) {
        }
        // Check if storage handle that's passed to us exists, is not full, and is writable
        else if ( MTP_RESP_OK != ( code = m_storageServer->checkStorage(params[1]) ) ) {
            // We came across a storage related error ;
        }
        // If the parent object handle is passed, check if that's valid
        else if ( params[2] && MTP_RESP_OK != m_storageServer->checkHandle(params[2]) ) {
            code = MTP_RESP_InvalidParentObject;
        }
        // Storage, object and parent handles are ok, proceed to copy the object
        else {
            code = m_storageServer->moveObject(params[0], params[2], params[1]);
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
    if (MTP_RESP_OK == code) {
        QVector<quint32> params;
        reqContainer->params(params);
        // Check if the object handle passed to us is valid and exists in the storage passed to us
        if ( MTP_RESP_OK != ( code = m_storageServer->checkHandle(params[0]) ) ) {
        }
        // Check if storage handle that's passed to us exists, is not full, and is writable
        else if ( MTP_RESP_OK != ( code = m_storageServer->checkStorage(params[1]) ) ) {
            // We cam across a storage related error ;
        }
        // If the parent object handle is passed, check if that's valid
        else if ( params[2] && MTP_RESP_OK != m_storageServer->checkHandle(params[2]) ) {
            code = MTP_RESP_InvalidParentObject;
        }
        // Storage, object and parent handles are ok, proceed to copy the object
        else {
            code = m_storageServer->copyObject(params[0], params[2], params[1], retHandle);
        }
    }

    if ( RESPONDER_TX_CANCEL == getResponderState() ) {
        return;
    }

    // RESPONSE PHASE
    m_copiedObjHandle = retHandle;
    sendResponse(code, retHandle);
}

void MTPResponder::getPartialObject64Req()
{
    /* Extension: android.com 1.0
     * Operation: getPartialObject64Req(handle, offsLo, offsHi, size)
     */
    MTP_FUNC_TRACE();
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);
    quint32 handle = params[0];
    quint32 offsLo = params[1];
    quint32 offsHi = params[2];
    quint32 size   = params[3];
    quint64 offs   = offsHi;
    offs <<= 32;
    offs |= offsLo;
    getObjectCommon(handle, offs, size);
}

void MTPResponder::sendPartialObject64Req()
{
    /* Extension: android.com 1.0
     * Operation: sendPartialObject64Req(handle, offsLo, offsHi, size)
     */
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;

    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);
    quint32 handle = params[0];
    quint32 offsLo = params[1];
    quint32 offsHi = params[2];
    quint32 size   = params[3];
    quint64 offs   = offsHi;
    offs <<= 32;
    offs |= offsLo;

    MTP_LOG_INFO("handle:" << handle << "offs:" << offs << "size:" << size);

    // check if everthing is ok, so that cmd can be processed
    if ( code == MTP_RESP_OK )
        code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    /* Must be in edit object mode */
    if ( code == MTP_RESP_OK ) {
        /* Check that appropriate beginEditObjectReq() has been made */
        if ( !m_editObjectSequencePtr )
            code = MTP_RESP_GeneralError;
        else if ( m_editObjectSequencePtr->objHandle != handle )
            code = MTP_RESP_InvalidObjectHandle;
    }

    /* Adjust write position used by data phase */
    if ( code == MTP_RESP_OK )
        m_editObjectSequencePtr->writeOffset = offs;

    /* Data phase must be processed even if it is not going
     * to be accepted, store result from request phase for
     * later use.
     */
    m_transactionSequence->mtpResp = code;

    // DATA PHASE will follow
    // -> MTPResponder::dataHandler()
    //    -> sendPartialObject64Data()
}

MTPResponseCode MTPResponder::sendPartialObject64Data(const quint8 *data, quint32 dataLen, bool isFirstPacket,
                                                      bool isLastPacket)
{
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;

    MTP_LOG_INFO("dataLen:" << dataLen << "isFirstPacket:" << isFirstPacket << "isLastPacket:" << isLastPacket);

    /* Must be in edit object mode */
    if ( code == MTP_RESP_OK ) {
        if ( !m_editObjectSequencePtr )
            code = MTP_RESP_GeneralError;
    }

    /* Omit header bits included in the 1st packet */
    if ( code == MTP_RESP_OK && isFirstPacket ) {
        if ( dataLen < MTP_HEADER_SIZE )
            code = MTP_RESP_GeneralError;
        else
            dataLen -= MTP_HEADER_SIZE, data += MTP_HEADER_SIZE;
    }

    /* Write content to file */
    if ( code == MTP_RESP_OK ) {
        code = m_storageServer->writePartialData(m_editObjectSequencePtr->objHandle,
                                                 m_editObjectSequencePtr->writeOffset,
                                                 data, dataLen,
                                                 isFirstPacket, isLastPacket);
        m_editObjectSequencePtr->writeOffset += dataLen;
    }

    return code;
}

void MTPResponder::truncateObject64Req()
{
    /* Extension: android.com 1.0
     * Operation: truncateObject64Req(handle, offsLo, offsHi)
     */
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;

    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);
    quint32 handle = params[0];
    quint32 offsLo = params[1];
    quint32 offsHi = params[2];
    quint64 offs   = offsHi;
    offs <<= 32;
    offs |= offsLo;

    MTP_LOG_INFO("handle:" << handle << "offs:" << offs);

    // check if everthing is ok, so that cmd can be processed
    if ( code == MTP_RESP_OK )
        code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    /* Must be in edit object mode */
    if ( code == MTP_RESP_OK ) {
        if ( !m_editObjectSequencePtr )
            code =  MTP_RESP_GeneralError;
        else if ( m_editObjectSequencePtr->objHandle != handle )
            code = MTP_RESP_InvalidObjectHandle;
    }

    /* Truncate file */
    if ( code == MTP_RESP_OK )
        code = m_storageServer->truncateItem(handle, offs);

    sendResponse(code);
}

void MTPResponder::beginEditObjectReq()
{
    /* Extension: android.com 1.0
     * Operation: beginEditObjectReq(handle)
     */
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;

    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);
    quint32 handle = params[0];

    MTP_LOG_INFO("handle:" << handle);

    // check if everthing is ok, so that cmd can be processed
    if ( code == MTP_RESP_OK )
        code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    if ( code == MTP_RESP_OK )
        code = m_storageServer->checkHandle(handle);

    /* Enter edit mode */
    if ( code == MTP_RESP_OK ) {
        // Note: Any ongoing non-terminated edit object session is abandoned
        delete m_editObjectSequencePtr;
        m_editObjectSequencePtr = new MTPEditObjectSequence;
        m_editObjectSequencePtr->objHandle = handle;

        // temporarily disable object changed notifications
        m_storageServer->setEventsEnabled(handle, false);
    }

    sendResponse(code);
}

void MTPResponder::endEditObjectReq()
{
    /* Extension: android.com 1.0
     * Operation: endEditObjectReq(handle)
     */
    MTP_FUNC_TRACE();
    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    QVector<quint32> params;
    reqContainer->params(params);
    quint32 handle = params[0];

    MTP_LOG_INFO("handle:" << handle);

    // check if everthing is ok, so that cmd can be processed
    if ( code == MTP_RESP_OK )
        code = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());

    if ( code == MTP_RESP_OK ) {
        if ( !m_editObjectSequencePtr )
            code =  MTP_RESP_GeneralError;
        else if ( m_editObjectSequencePtr->objHandle != handle )
            code = MTP_RESP_InvalidObjectHandle;
    }

    /* Leave edit mode */
    if ( code == MTP_RESP_OK ) {
        // flush cached property values for the object
        m_storageServer->flushCachedObjectPropertyValues(handle);

        // enable object changed notifications again
        m_storageServer->setEventsEnabled(handle, true);

        delete m_editObjectSequencePtr;
        m_editObjectSequencePtr = nullptr;
    }

    sendResponse(code);
}

void MTPResponder::getObjPropsSupportedReq()
{
    MTP_FUNC_TRACE();

    MTPResponseCode code = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    bool sent = true;

    // check if everthing is ok, so that cmd can be processed
    // Pass 0x00000001 as the session ID, as this operation does not need an active session
    if (MTP_RESP_OK != (code = preCheck(0x00000001, reqContainer->transactionId()))) {
        code = MTP_RESP_InvalidTransID;
    } else {
        QVector<quint32> params;
        reqContainer->params(params);
        MTPObjectFormatCategory category = (MTPObjectFormatCategory)m_devInfoProvider->getFormatCodeCategory(params[0]);

        QVector<MTPObjPropertyCode> propsSupported;

        code = m_propertyPod->getObjectPropsSupportedByType(category, propsSupported);

        // enter data phase if precondition has been OK
        if (MTP_RESP_OK == code) {
            quint32 payloadLength = propsSupported.size() * sizeof(MTPObjPropertyCode) + sizeof(quint32);
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                         payloadLength);
            dataContainer << propsSupported;
            sent = sendContainer(dataContainer);
            if ( false == sent ) {
                MTP_LOG_CRITICAL("Could not send data");
            }
        }
    }

    if ( true == sent ) {
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
    if (MTP_RESP_OK == code) {
        QVector<quint32> params;
        reqContainer->params(params);
        MTPObjPropertyCode propCode = static_cast<MTPObjPropertyCode>(params[0]);
        MTPObjFormatCode formatCode = static_cast<MTPObjFormatCode>(params[1]);
        MTPObjectFormatCategory category = (MTPObjectFormatCategory)m_devInfoProvider->getFormatCodeCategory(formatCode);

        MTP_LOG_INFO(mtp_code_repr(propCode)
                     << mtp_code_repr(formatCode)
                     << mtp_format_category_repr(category));

        if ( MTP_UNSUPPORTED_FORMAT == category) {
            code = MTP_RESP_Invalid_ObjectProp_Format;
        } else {
            const MtpObjPropDesc *propDesc = 0;
            code = m_propertyPod->getObjectPropDesc(category, propCode, propDesc);
            if (MTP_RESP_OK == code) {
                // Data phase
                quint32 payloadLength = sizeof(MtpObjPropDesc); // approximation
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                             payloadLength);
                dataContainer << *propDesc;
                sent = sendContainer(dataContainer);
                if ( false == sent ) {
                    MTP_LOG_CRITICAL("Could not send data");
                }
            }
        }
    }

    if ( true == sent ) {
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
    if (MTP_RESP_OK == code) {
        QVector<quint32> params;
        reqContainer->params(params);
        MTPObjPropertyCode propCode = static_cast<MTPObjPropertyCode>(params[1]);
        ObjHandle handle = static_cast<ObjHandle>(params[0]);
        const MTPObjectInfo *objectInfo;
        MTPObjectFormatCategory category;
        const MtpObjPropDesc *propDesc = 0;
        code = m_storageServer->getObjectInfo( handle, objectInfo );
        if (MTP_RESP_OK == code) {
            category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(objectInfo->mtpObjectFormat));
            code = m_propertyPod->getObjectPropDesc(category, propCode, propDesc);
        }

        if (MTP_RESP_OK == code) {
            QList<MTPObjPropDescVal> propValList;

            propValList.append(MTPObjPropDescVal(propDesc));

            code = m_storageServer->getObjectPropertyValue(handle, propValList);
            if (MTP_RESP_ObjectProp_Not_Supported == code) {
                QString path;
                if (MTP_RESP_OK == m_storageServer->getPath(handle, path)) {
                    // Try with an extension
                    m_extensionManager->getObjPropValue(path, propCode, propValList[0].propVal, code);
                }
            }
            // enter data phase if value has been retrieved
            if (MTP_RESP_OK == code) {
                // DATA PHASE
                quint32 payloadLength = sizeof(QVariant); // approximation
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                             payloadLength);
                if (1 == propValList.size()) {
                    dataContainer.serializeVariantByType(propDesc->uDataType, propValList[0].propVal);
                } else {
                    // Serialize empty variant
                    dataContainer.serializeVariantByType(propDesc->uDataType, QVariant());
                }
                sent = sendContainer(dataContainer);
                if ( false == sent ) {
                    MTP_LOG_CRITICAL("Could not send data");
                }
            }
        }
    }

    if ( true == sent ) {
        sendResponse(code);
    }
}

void MTPResponder::setObjPropValueReq()
{
    MTP_FUNC_TRACE();

    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    // check if everthing is ok, so that cmd can be processed
    m_transactionSequence->mtpResp = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if (MTP_RESP_OK == m_transactionSequence->mtpResp) {
        QVector<quint32> params;
        reqContainer->params(params);
        // if session ID and transaction ID are valid
        // check still whether the StorageItem can be accessed
        ObjHandle objHandle = static_cast<ObjHandle>(params[0]);
        const MTPObjectInfo *objInfo;
        if (MTP_RESP_OK != (m_transactionSequence->mtpResp = m_storageServer->getObjectInfo(objHandle, objInfo))) {
            return;
        }
        MTPObjFormatCode format = static_cast<MTPObjFormatCode>(objInfo->mtpObjectFormat);
        MTPObjectFormatCategory category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(
                                                                                    format));
        MTPObjPropertyCode propCode = static_cast<MTPObjPropertyCode>(params[1]);
        const MtpObjPropDesc *propDesc = 0;

        if (MTP_RESP_OK == (m_transactionSequence->mtpResp = m_propertyPod->getObjectPropDesc(category, propCode, propDesc))) {
            if (false == propDesc->bGetSet) {
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

    if ( MTP_RESP_OK == resp ) {
        if ((0 == objHandle) && (0 == depth)) {
            // return an empty data set
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                         sizeof(quint32));
            dataContainer << (quint32)0;
            sent = sendContainer(dataContainer);
            if ( false == sent ) {
                MTP_LOG_CRITICAL("Could not send data");
            }
        } else if ( (1 < depth) && (0xFFFFFFFF > depth) ) {
            // we do not support depth values between 1 and 0xFFFFFFFF
            resp = MTP_RESP_Specification_By_Depth_Unsupported;
        } else if (0 == propCode) {
            // propCode of 0 means we need the group code, but we do not support that
            resp = MTP_RESP_Specification_By_Group_Unsupported;
        } else if ((0 != format)
                   && (MTP_UNSUPPORTED_FORMAT == static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(format)))) {
            resp = MTP_RESP_InvalidCodeFormat;
        } else {
            QVector<ObjHandle> objHandles;
            quint32 numHandles = 0;
            quint32 numElements = 0;
            MTPObjFormatCode objFormat;
            MTPObjectFormatCategory category;
            quint32 storageID = 0xFFFFFFFF;
            const MTPObjectInfo *objInfo;
            ObjHandle currentObj = 0;

            if (0 == depth) {
                // The properties for this object are requested
                objHandles.append(objHandle);
            } else {
                ObjHandle tempHandle = objHandle;
                // The properties for the object's immediate children or for all objects
                if (0 == objHandle) {
                    // All objects under root
                    tempHandle = 0xFFFFFFFF;
                } else if (0xFFFFFFFF == objHandle) {
                    // All objects in (all?) storage
                    tempHandle = 0;
                }
                resp = m_storageServer->getObjectHandles(storageID, format, tempHandle, objHandles);
            }

            if (MTP_RESP_OK == resp) {
                // TODO: The buffer max len needs to be decided
                quint32 maxPayloadLen = BUFFER_MAX_LEN - MTP_HEADER_SIZE;
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                             maxPayloadLen);

                // if there are no handles found an empty dataset will be sent
                numHandles = objHandles.size();
                MTP_LOG_TRACE( numHandles);
                dataContainer << numHandles; // Write the numHandles here for now, to advance the serializer's pointer
                // go through the list of found ObjectHandles
                for (quint32 i = 0; (i < numHandles && (MTP_RESP_OK == resp)); i++) {
                    currentObj = objHandles[i];

                    resp = m_storageServer->getObjectInfo(currentObj, objInfo);
                    if (MTP_RESP_OK == resp) {
                        // find the format and the category of the object
                        objFormat = static_cast<MTPObjFormatCode>(objInfo->mtpObjectFormat);
                        category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(objFormat));

                        // FIXME: Investigate if the below force assignment to common format is really needed
                        if (category == MTP_UNSUPPORTED_FORMAT) {
                            category = MTP_COMMON_FORMAT;
                        }

                        QList<MTPObjPropDescVal> propValList;
                        // check whether all or a certain ObjectProperty of the referenced Object is requested
                        if (0xFFFF == propCode) {
                            const MtpObjPropDesc *propDesc = 0;
                            QVector<MTPObjPropertyCode> propsSupported;

                            resp = m_propertyPod->getObjectPropsSupportedByType(category, propsSupported);
                            for (int i = 0; ((i < propsSupported.size()) && (MTP_RESP_OK == resp)); i++) {
                                resp = m_propertyPod->getObjectPropDesc(category, propsSupported[i], propDesc);
                                if (MTP_OBJ_PROP_Rep_Sample_Data != propDesc->uPropCode) {
                                    propValList.append(MTPObjPropDescVal(propDesc));
                                }
                            }
                        } else {
                            const MtpObjPropDesc *propDesc = 0;
                            resp = m_propertyPod->getObjectPropDesc(category, propCode, propDesc);
                            propValList.append(MTPObjPropDescVal(propDesc));
                        }
                        if (MTP_RESP_OK == resp) {
                            resp = m_storageServer->getObjectPropertyValue(currentObj, propValList);
                            if (resp == MTP_RESP_OK) {
                                numElements += serializePropList(currentObj, propValList, dataContainer);
                            }
                        }
                    }
                }
                if (MTP_RESP_OK == resp) {
                    MTP_LOG_INFO("element count:" << numElements);
                    // KLUDGE: We have to manually insert the number of elements at
                    // the start of the container payload. Is there a better way?
                    dataContainer.putl32(dataContainer.payload(), numElements);
                    sent = sendContainer(dataContainer);
                    if ( false == sent ) {
                        MTP_LOG_CRITICAL("Could not send data");
                    }
                }
            } else { //FIXME Is this needed?
                // return an empty data set
                MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                             sizeof(quint32));
                dataContainer << (quint32)0;
                sent = sendContainer(dataContainer);
                if ( false == sent ) {
                    MTP_LOG_CRITICAL("Could not send data");
                }
            }
        }
        if (MTP_RESP_InvalidObjectFormatCode == resp) { //FIXME investigate
            resp = MTP_RESP_InvalidCodeFormat;
        }
    }

    if ( true == sent ) {
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
    MTPResponseCode *respCode =  &m_transactionSequence->mtpResp;
    quint32 storageID = 0x00000000;
    quint64 objectSize = 0x00000000;
    ObjHandle parentHandle = 0x00000000;
    MTPObjFormatCode format = MTP_OBF_FORMAT_Undefined;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;

    // check if everthing is ok, so that cmd can be processed
    *respCode = preCheck(m_transactionSequence->mtpSessionId, reqContainer->transactionId());
    if (MTP_RESP_OK == *respCode) {
        QVector<quint32> params;
        reqContainer->params(params);
        // If storage id is provided, check if it's valid
        storageID = params[0];
        if (storageID) {
            *respCode = m_storageServer->checkStorage(storageID);
        }

        if (MTP_RESP_OK == *respCode) {
            // If the parent object is provided, check that
            parentHandle = params[1];
            if ((0 != parentHandle) && (0xFFFFFFFF != parentHandle)) {
                *respCode = m_storageServer->checkHandle(parentHandle);
            }
            if (MTP_RESP_OK == *respCode) {
                // Fetch object format code and category
                format = static_cast<MTPObjFormatCode>(params[2]);
                // Get the object size
                quint64 msb = params[3];
                quint64 lsb = params[4];

                // Indicates that object is >= 4GB, we don't support this
                if ( msb ) {
                    *respCode = MTP_RESP_Object_Too_Large;
                } else {
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
    if (MTP_RESP_OK == respCode) {
        QVector<quint32> params;
        reqContainer->params(params);
        QVector<ObjHandle> objReferences;
        respCode = m_storageServer->getReferences(params[0], objReferences);
        // enter data phase if precondition has been OK
        if (MTP_RESP_OK == respCode) {
            quint32 payloadLength = objReferences.size() * sizeof(ObjHandle) + sizeof(quint32);
            MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, reqContainer->code(), reqContainer->transactionId(),
                                         payloadLength);
            dataContainer << objReferences;
            sent = sendContainer(dataContainer);
            if ( false == sent ) {
                MTP_LOG_CRITICAL("Could not send data");
            }
        }
    }

    if ( true == sent ) {
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
    if (MTP_RESP_OK == respCode) {
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
    if (MTP_RESP_OK == m_transactionSequence->mtpResp) {
        // If there's a objectproplist dataset existing, delete that
        freeObjproplistInfo();
        m_sendObjectSequencePtr = new MTPSendObjectSequence();

        reqContainer->params(params);

        // De-serialize the object info
        *recvContainer >> objectInfo;

        // Indicates that object is >= 4GB, we don't support this
        if ( 0xFFFFFFFF == objectInfo.mtpObjectCompressedSize ) {
            response = MTP_RESP_Object_Too_Large;
        } else {
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
        if ( MTP_RESP_OK == response ) {
            // save the ObjectInfo temporarly
            m_sendObjectSequencePtr->objInfo = new MTPObjectInfo;
            *(m_sendObjectSequencePtr->objInfo) = objectInfo;
            // save the ObjectHandle temporarly
            m_sendObjectSequencePtr->objHandle = responseParams[2];
        } else {
            delete m_sendObjectSequencePtr;
            m_sendObjectSequencePtr = 0;
            // return parameters shall not be used
            memset(&responseParams[0], 0, sizeof(responseParams));
        }
    }

    // create and send container for response
    MTPTxContainer respContainer(MTP_CONTAINER_TYPE_RESPONSE, response, reqContainer->transactionId(),
                                 sizeof(responseParams));
    if ( MTP_RESP_OK == response ) {
        respContainer << responseParams[0] << responseParams[1] << responseParams[2];
    }
    bool sent = sendContainer(respContainer);
    if ( false == sent ) {
        MTP_LOG_CRITICAL("Could not send response");
    }
}

void MTPResponder::sendObjectData(quint8 *data, quint32 dataLen, bool isFirstPacket, bool isLastPacket)
{
    MTP_FUNC_TRACE();

    quint32 objectLen = 0;
    MTPResponseCode code =  MTP_RESP_OK;
    MTPContainerWrapper container(data);
    ObjHandle handle = 0;

    // error if no ObjectInfo or objectproplist dataset has been sent before
    if ( !m_objPropListInfo && ( !m_sendObjectSequencePtr || !m_sendObjectSequencePtr->objInfo ) ) {
        code = MTP_RESP_NoValidObjectInfo;
    } else {
        quint8 *writeBuffer = 0;
        if (m_sendObjectSequencePtr) {
            handle = m_sendObjectSequencePtr->objHandle;
        } else if ( m_objPropListInfo ) {
            handle = m_objPropListInfo->objectHandle;
        }

        // assume that the whole segment will be written
        objectLen = dataLen;

        if (isFirstPacket) {
            // the start segment includes the container header, which should not be written
            // set the pointer of the data to be written, on the payload
            writeBuffer = container.payload();
            //subtract header length
            objectLen -= MTP_HEADER_SIZE;
        } else {
            // if it is not the start, just take the receive data an length
            writeBuffer = data;
        }

        // write data
        code = m_storageServer->writeData( handle, reinterpret_cast<char *>(writeBuffer), objectLen, isFirstPacket,
                                           isLastPacket);
        if ( MTP_RESP_OK == code ) {
            code = sendObjectCheck(handle, objectLen, isLastPacket, code);
        }
    }

    if ( MTP_RESP_Undefined != code ) {
        // Delete the stored sendObjectInfo information
        if ( m_sendObjectSequencePtr ) {
            if ( m_sendObjectSequencePtr->objInfo ) {
                delete m_sendObjectSequencePtr->objInfo;
                m_sendObjectSequencePtr->objInfo = 0;
            }
            delete m_sendObjectSequencePtr;
            m_sendObjectSequencePtr = 0;
        }
        // Apply object prop list if one was sent thru SendObjectPropList
        if (MTP_RESP_OK == code && m_objPropListInfo) {
            MTPObjectFormatCategory category = m_devInfoProvider->getFormatCodeCategory(m_objPropListInfo->objectFormatCode);
            QList<MTPObjPropDescVal> propValList;
            const MtpObjPropDesc *propDesc = 0;
            for ( quint32 i = 0 ; i < m_objPropListInfo->noOfElements; i++) {
                ObjPropListInfo::ObjectPropList &propList =
                    m_objPropListInfo->objPropList[i];

                switch (propList.objectPropCode) {
                case MTP_OBJ_PROP_Obj_File_Name:
                    // This has already been set
                    continue;
                case MTP_OBJ_PROP_Name: {
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
                    if ( m_storageServer->getObjectInfo( handle, info ) != MTP_RESP_OK ) {
                        break;
                    }

                    if ( propList.value->toString() == info->mtpFileName ) {
                        continue;
                    }
                    break;
                }
                default:
                    break;
                }

                // Get the object prop desc for this property
                if (MTP_RESP_OK == m_propertyPod->getObjectPropDesc(category, propList.objectPropCode, propDesc)) {
                    propValList.append(MTPObjPropDescVal(propDesc, *propList.value));
                }
            }
            m_storageServer->setObjectPropertyValue(handle, propValList, true);
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
    const MtpObjPropDesc *propDesc = 0;
    quint32 i = 0;
    MTPRxContainer *recvContainer = m_transactionSequence->dataContainer;

    // Deserialize the no. of elements for which we need to set object property values
    *recvContainer >> numObjects;

    // Proceed to set object property values for noOfElements objects
    for (; ((i < numObjects) && (MTP_RESP_OK == respCode)); i++) {
        // First, get the object handle
        *recvContainer >> objHandle;
        // Check if this is a valid object handle
        const MTPObjectInfo *objInfo;
        respCode = m_storageServer->getObjectInfo(objHandle, objInfo);

        if (MTP_RESP_OK == respCode) {
            // Next, get the object property code
            *recvContainer >> propCode;

            // Check if the object property code is valid and that it can be "set"
            format = static_cast<MTPObjFormatCode>(objInfo->mtpObjectFormat);
            category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(format));
            respCode = m_propertyPod->getObjectPropDesc(category, propCode, propDesc);
            if (MTP_RESP_OK == respCode) {
                respCode = propDesc->bGetSet ? MTP_RESP_OK : MTP_RESP_AccessDenied;
                if (MTP_RESP_OK == respCode) {
                    // Next, get this object property's datatype
                    *recvContainer >> datatype;
                    QList<MTPObjPropDescVal> propValList;
                    propValList.append(MTPObjPropDescVal(propDesc));
                    recvContainer->deserializeVariantByType(datatype, propValList[0].propVal);
                    // Set the object property value
                    respCode = m_storageServer->setObjectPropertyValue(objHandle, propValList);
                }
            }
        }
    }
    if (MTP_RESP_OK != respCode) {
        // Set the response parameter to the 0 based index of the property that caused the problem
        sendResponse(respCode, i - 1);
    } else {
        sendResponse(MTP_RESP_OK);
    }
}

void MTPResponder::sendObjectPropListData()
{
    MTP_FUNC_TRACE();
    quint32 respParam[4] = {0, 0, 0, 0};
    quint32 respSize = 0;
    quint16 respCode =  MTP_RESP_OK;
    MTPObjectFormatCategory category = MTP_UNSUPPORTED_FORMAT;
    MTPObjPropertyCode propCode;
    const MtpObjPropDesc *propDesc = 0;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    MTPRxContainer *recvContainer = m_transactionSequence->dataContainer;
    bool sent = true;

    if (0 == m_objPropListInfo || MTP_RESP_OK != m_transactionSequence->mtpResp) {
        respCode = 0 == m_objPropListInfo ? MTP_RESP_GeneralError : m_transactionSequence->mtpResp;
        sendResponse(respCode);
        return;
    }

    // Deserialize the no. of elements for which we need to set object property values
    *recvContainer >> m_objPropListInfo->noOfElements;

    // Allocate memory for the the number of elements obj property list item
    m_objPropListInfo->objPropList = new ObjPropListInfo::ObjectPropList[m_objPropListInfo->noOfElements];

    // Scan propertylist
    QString fileNameValue;
    int fileNameIndex = -1;
    for (quint32 i = 0; i < m_objPropListInfo->noOfElements; i++ ) {
        m_objPropListInfo->objPropList[i].value = 0;
        // First, get the object handle
        *recvContainer >> m_objPropListInfo->objPropList[i].objectHandle;
        // Check if the object handle is 0x00000000
        if (0 != m_objPropListInfo->objPropList[i].objectHandle) {
            respCode = MTP_RESP_Invalid_Dataset;
            respParam[3] = i;
            respSize = 4 * sizeof(quint32);
            break;
        }

        // Next, get the object property code
        *recvContainer >> m_objPropListInfo->objPropList[i].objectPropCode;

        category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(
                                                            m_objPropListInfo->objectFormatCode));
        propCode = static_cast<MTPObjPropertyCode>(m_objPropListInfo->objPropList[i].objectPropCode);
        // Check if the object property code is valid and that it can be "get"
        if (MTP_RESP_OK != m_propertyPod->getObjectPropDesc(category, propCode, propDesc)) {
            respCode = MTP_RESP_Invalid_Dataset;
            respParam[3] = i;
            respSize = 4 * sizeof(quint32);
            break;
        }

        // Next, get this object property's datatype
        *recvContainer >> m_objPropListInfo->objPropList[i].datatype;

        m_objPropListInfo->objPropList[i].value = new QVariant();
        recvContainer->deserializeVariantByType(m_objPropListInfo->objPropList[i].datatype,
                                                *m_objPropListInfo->objPropList[i].value);

        // If this property is the filename, trigger object creation in the file system now
        if ( MTP_OBJ_PROP_Obj_File_Name == m_objPropListInfo->objPropList[i].objectPropCode) {
            fileNameValue = m_objPropListInfo->objPropList[i].value->value<QString>();
            fileNameIndex = i;
        }
    }

    if ( respCode != MTP_RESP_OK ) {
        // Property list scanning failed
    } else if ( fileNameIndex == -1 ) {
        // Property list did not contain filename
        respCode = MTP_RESP_Invalid_Dataset;
    } else {
        if ( fileNameValue.isEmpty() ) {
            // Invalid filename was specified
            respCode = MTP_RESP_Invalid_Dataset;
        } else {
            // Fill in object info
            MTPObjectInfo objInfo;
            objInfo.mtpStorageId = m_objPropListInfo->storageId;
            objInfo.mtpObjectCompressedSize = m_objPropListInfo->objectSize;
            objInfo.mtpParentObject = m_objPropListInfo->parentHandle;
            objInfo.mtpObjectFormat = m_objPropListInfo->objectFormatCode;
            objInfo.mtpFileName = fileNameValue;
            // Get object handle
            respCode = m_storageServer->addItem(m_objPropListInfo->storageId,
                                                m_objPropListInfo->parentHandle,
                                                m_objPropListInfo->objectHandle,
                                                &objInfo);
        }

        if ( respCode != MTP_RESP_OK ) {
            respParam[3] = fileNameIndex;
            respSize = 4 * sizeof(quint32);
        } else {
            respParam[0] = m_objPropListInfo->storageId;
            respParam[1] = m_objPropListInfo->parentHandle;
            respParam[2] = m_objPropListInfo->objectHandle;
            respSize = 3 * sizeof(quint32);
        }
    }
    // create and send container for response
    MTPTxContainer respContainer(MTP_CONTAINER_TYPE_RESPONSE, respCode, reqContainer->transactionId(), respSize);
    for (quint32 i = 0; i < (respSize / sizeof(quint32)); i++) {
        respContainer << respParam[i];
    }
    sent = sendContainer(respContainer);
    if ( false == sent ) {
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

    switch (propCode) {
    // set the value if the property is supported
    case MTP_DEV_PROPERTY_Device_Friendly_Name: {
        QString name;
        *recvContainer >> name;
        m_devInfoProvider->setDeviceFriendlyName(name);
    }
    break;
    case MTP_DEV_PROPERTY_Synchronization_Partner: {
        QString partner;
        *recvContainer >> partner;
        m_devInfoProvider->setSyncPartner(partner);
    }
    break;
    case MTP_DEV_PROPERTY_Volume: {
        qint32 vol = 0;
        *recvContainer >> vol;
        // TODO: implement this
    }
    break;
    default: {
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
    if (MTP_RESP_OK == respCode) {
        QVector<quint32> params;
        reqContainer->params(params);
        // Check if the object property code is valid and that it can be "set"
        ObjHandle objHandle = params[0];
        const MTPObjectInfo *objInfo;
        if (MTP_RESP_OK == (respCode = m_storageServer->getObjectInfo(objHandle, objInfo))) {
            MTPObjFormatCode format = static_cast<MTPObjFormatCode>(objInfo->mtpObjectFormat);
            MTPObjectFormatCategory category = static_cast<MTPObjectFormatCategory>(m_devInfoProvider->getFormatCodeCategory(
                                                                                        format));
            MTPObjPropertyCode propCode = static_cast<MTPObjPropertyCode>(params[1]);
            const MtpObjPropDesc *propDesc = 0;

            if (MTP_RESP_OK == (respCode = m_propertyPod->getObjectPropDesc(category, propCode, propDesc))) {
                // take the data from the container to set the ObjectProperty
                MTPRxContainer *recvContainer = m_transactionSequence->dataContainer;
                QList<MTPObjPropDescVal> propValList;

                propValList.append(MTPObjPropDescVal(propDesc));
                recvContainer->deserializeVariantByType(propDesc->uDataType, propValList[0].propVal);
                respCode = m_storageServer->setObjectPropertyValue(objHandle, propValList);
                if (MTP_RESP_ObjectProp_Not_Supported == respCode) {
                    QString path;
                    if (MTP_RESP_OK == m_storageServer->getPath(objHandle, path)) {
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
    if (0 != m_transactionSequence->dataContainer) {
        delete m_transactionSequence->dataContainer;
        m_transactionSequence->dataContainer = 0;
    }
    if (0 != m_transactionSequence->reqContainer) {
        delete m_transactionSequence->reqContainer;
        m_transactionSequence->reqContainer = 0;
    }
}

MTPResponseCode MTPResponder::preCheck(quint32 sessionID, quint32 transactionID)
{
    MTP_FUNC_TRACE();
    if ( MTP_INITIAL_SESSION_ID == sessionID) {
        return MTP_RESP_SessionNotOpen;
    } else if ( transactionID == 0x00000000 || transactionID == 0xFFFFFFFF ) {
        return MTP_RESP_InvalidTransID;
    }
    return MTP_RESP_OK;
}

void MTPResponder::freeObjproplistInfo()
{
    MTP_FUNC_TRACE();
    if (m_objPropListInfo) {
        for (quint32 i = 0; i < m_objPropListInfo->noOfElements; i++) {
            if ( m_objPropListInfo->objPropList[i].value ) {
                delete m_objPropListInfo->objPropList[i].value;
            }
        }
        if (m_objPropListInfo->objPropList) {
            delete[] m_objPropListInfo->objPropList;
        }
        delete m_objPropListInfo;
        m_objPropListInfo = 0;
    }
}

MTPResponseCode MTPResponder::sendObjectCheck( ObjHandle handle, const quint32 dataLen, bool isLastPacket,
                                               MTPResponseCode code )
{
    MTP_FUNC_TRACE();
    MTPResponseCode response = MTP_RESP_OK;
    if ( MTP_RESP_OK == code && m_objPropListInfo ) {
        // Update object size
        m_objPropListInfo->objectCurrSize += dataLen;
        if (m_objPropListInfo->objectSize > m_objPropListInfo->objectCurrSize) {
            if (isLastPacket) {
                // Some data packets are missing
                m_storageServer->truncateItem( handle, 0 );
                response =  MTP_RESP_IncompleteTransfer;
            } else {
                // Prepare to receive the next data packet
                response = MTP_RESP_Undefined;
            }
        }
    } else if ( m_sendObjectSequencePtr ) {
        // SendObjectInfo case
        quint32 bytesWritten = m_sendObjectSequencePtr->sendObjBytesWritten;
        // update the total amount of written bytes */
        m_sendObjectSequencePtr->sendObjBytesWritten = bytesWritten + dataLen;

        // check if all parts of the object have been received
        if ( m_sendObjectSequencePtr->objInfo->mtpObjectCompressedSize >
                m_sendObjectSequencePtr->sendObjBytesWritten ) {
            // check if there is an error in the container/data phase
            if ( isLastPacket ) {
                // although not all bytes of the Object were sent, the container is indicated
                // to be the last. Initiate the disposal of the data that has been written so far,
                // so that the Initiator may resend the Object with a new SendObject operation.
                //writing will start then from the beginning of the file
                m_storageServer->truncateItem( handle, 0 );
                // end the whole operation by responding with an error code
                response =  MTP_RESP_IncompleteTransfer;
            } else {
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
    setResponderState(RESPONDER_IDLE);
    if ( m_sendObjectSequencePtr ) {
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

void MTPResponder::fetchObjectSize(const quint8 *data, quint64 *objectSize)
{
    MTP_FUNC_TRACE();
    *objectSize = 0;
    if (data) {
        //If this is a data phase, the operation is SendObject
        //check if we still have a valid ObjectPropListInfo, if so return the object size.
        MTPContainerWrapper tempContainer(const_cast<quint8 *>(data));
        if (MTP_CONTAINER_TYPE_DATA == tempContainer.containerType() &&
                MTP_OP_SendObject == tempContainer.code()) {
            if (m_objPropListInfo) {
                *objectSize = m_objPropListInfo->objectSize;
            }
        }
    }
}

void MTPResponder::handleSuspend()
{
    MTP_LOG_WARNING("Received suspend");
    m_prevState = getResponderState();
    setResponderState(RESPONDER_SUSPEND);
}

void MTPResponder::handleResume()
{
    MTP_LOG_WARNING("Received resume");
    setResponderState(m_prevState);
    if ( m_containerToBeResent ) {
        m_containerToBeResent = false;
        //m_transporter->disableRW();
        //QCoreApplication::processEvents();
        //m_transporter->enableRW();
        if ( RESPONDER_TX_CANCEL != getResponderState() ) {
            MTP_LOG_WARNING("Resume sending");
            m_transporter->sendData( m_resendBuffer, m_resendBufferSize, m_isLastPacket );
        }
        delete[] m_resendBuffer;
    }
}

void MTPResponder::onDevicePropertyChanged(MTPDevPropertyCode property)
{
    dispatchEvent(MTP_EV_DevicePropChanged, QVector<quint32>() << property);
}

void MTPResponder::handleCancelTransaction()
{
    if ( !m_transactionSequence->reqContainer ) {
        emit deviceStatusOK();
        MTP_LOG_CRITICAL("Received Cancel Transaction while in idle state : do nothing");
        //Nothing to do
        return;
    }

    MTP_LOG_CRITICAL("Received Cancel Transaction for operation " << QString("0x%1").arg(
                         m_transactionSequence->reqContainer->code(), 0, 16 ));

    setResponderState(RESPONDER_TX_CANCEL);
    switch ( m_transactionSequence->reqContainer->code() ) {
    // Host initiated cancel for host to device data transfer
    case MTP_OP_SendObject:
    case MTP_OP_SendObjectPropList:
    case MTP_OP_SendObjectInfo: {
        ObjHandle handle = 0x00000000;
        // A SendObjectPropListInfo was used.
        if ( m_objPropListInfo ) {
            handle = m_objPropListInfo->objectHandle;
        }
        // A SendObjectInfo was used.
        else if ( m_sendObjectSequencePtr ) {
            handle = m_sendObjectSequencePtr->objHandle;
        }
        if ( 0x00000000 == handle ) {
            MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer: No object to cancel the host to device data transfer for");
        } else {
            MTPResponseCode response = m_storageServer->deleteItem( handle, MTP_OBF_FORMAT_Undefined );
            if ( MTP_RESP_OK != response ) {
                MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer: Object could not be deleted " << response);
            } else {
                MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer: host to device data transfer cancelled");
            }
        }

        if ( MTP_OP_SendObject == m_transactionSequence->reqContainer->code() && handle ) {
            if ( m_objPropListInfo ) {
                if ( m_objPropListInfo->objectSize > m_objPropListInfo->objectCurrSize ) {
                    MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer before data xfer was completed");
                } else {
                    MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer after data xfer was completed");
                }
            } else if ( m_sendObjectSequencePtr ) {
                if ( m_sendObjectSequencePtr->objInfo->mtpObjectCompressedSize > m_sendObjectSequencePtr->sendObjBytesWritten ) {
                    MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer before data xfer was completed");
                } else {
                    MTP_LOG_CRITICAL("Received Cancel Transaction for host to device data xfer after data xfer was completed");
                }
            }
        }

        if ( m_objPropListInfo ) {
            freeObjproplistInfo();
        } else if ( m_sendObjectSequencePtr ) {
            delete m_sendObjectSequencePtr;
            m_sendObjectSequencePtr = 0;
        }
        break;
    }

    default: {
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

quint32 MTPResponder::serializePropList(ObjHandle currentObj,
                                        QList<MTPObjPropDescVal> &propValList, MTPTxContainer &dataContainer)
{
    MTP_FUNC_TRACE();

    quint32 serializedCount = 0;

    for (QList<MTPObjPropDescVal>::const_iterator i = propValList.constBegin(); i != propValList.constEnd(); ++i) {
        if (!i->propVal.isValid()) {
            continue;
        }

        const MtpObjPropDesc *propDesc = i->propDesc;

        MTP_LOG_INFO("object:" << currentObj << "prop:" << mtp_code_repr(propDesc->uPropCode) << "type:" << mtp_data_type_repr(
                         propDesc->uDataType) << "data:" << i->propVal);

        dataContainer << currentObj << propDesc->uPropCode << propDesc->uDataType;
        dataContainer.serializeVariantByType(propDesc->uDataType, i->propVal);

        ++serializedCount;
    }

    return serializedCount;
}

void MTPResponder::sendObjectSegmented()
{
    MTP_FUNC_TRACE();

    MTPResponseCode respCode = MTP_RESP_OK;
    MTPRxContainer *reqContainer = m_transactionSequence->reqContainer;
    MTPOperationCode opCode = reqContainer->code();
    bool headerSent = false;
    bool contentSent = false;
    quint64 bytesSent = 0;
    quint8 *buffer = nullptr;

    quint64 remainingLength = m_segmentedSender.offsetEnd - m_segmentedSender.offsetNow;

    // Send ptp header + initial part of file content
    if ( respCode == MTP_RESP_OK && !headerSent ) {
        // Calculate amount of data to send in initial frame
        quint32 contentLength = BUFFER_MAX_LEN - MTP_HEADER_SIZE;
        if ( remainingLength < contentLength )
            contentLength = quint32(remainingLength);

        // Setup PTP header
        MTPTxContainer dataContainer(MTP_CONTAINER_TYPE_DATA, opCode, reqContainer->transactionId(), contentLength);
        dataContainer.setContainerLength(remainingLength > MTP_MAX_CONTENT_SIZE
                                         ? 0xFFFFFFFF
                                         : quint32(MTP_HEADER_SIZE + remainingLength));

        // Read file content
        respCode = m_storageServer->readData(m_segmentedSender.objHandle,
                                             reinterpret_cast<char *>(dataContainer.payload()),
                                             contentLength,
                                             m_segmentedSender.offsetNow);
        if ( respCode == MTP_RESP_OK ) {
            // Update container content length and send it
            dataContainer.seek(contentLength);
            if ( !sendContainer(dataContainer, (contentLength == remainingLength)) ) {
                MTP_LOG_CRITICAL("Could not send header");
            } else {
                bytesSent += contentLength;
                remainingLength -= contentLength;
                m_segmentedSender.offsetNow += contentLength;
                headerSent = true;
                contentSent = (remainingLength == 0);
            }
        }
    }

    while ( respCode == MTP_RESP_OK && headerSent && !contentSent ) {
        // Allocate buffer
        if ( !buffer )
            buffer = new quint8[BUFFER_MAX_LEN];

        // Calculate amount of data to send in continuation frame
        quint32 contentLength = BUFFER_MAX_LEN;
        if ( remainingLength < contentLength )
            contentLength = quint32(remainingLength);

        // Read file content
        respCode = m_storageServer->readData(m_segmentedSender.objHandle,
                                             (char *)buffer,
                                             contentLength,
                                             m_segmentedSender.offsetNow);
        if ( respCode == MTP_RESP_OK ) {
            // Send raw data
            if ( !m_transporter->sendData(buffer, contentLength, (contentLength == remainingLength)) ) {
                MTP_LOG_CRITICAL("Could not send content");
                break;
            }
            bytesSent += contentLength;
            remainingLength -= contentLength;
            m_segmentedSender.offsetNow += contentLength;
            contentSent = (remainingLength == 0);
        }

    }

    /* Initiator expects to receive a valid container.
     *
     * On happy path all of data container was transmitted
     * and we can continue by sending a reply container.
     *
     * In case we failed to send any parts of data container,
     * an error reply container can be sent instead.
     *
     * But if parts of data container were sent, initiator
     * and responder are out of sync -> abandon command
     * and hope that initiator handles the situation somehow
     * e.g. via timeout.
     */
    if ( headerSent && !contentSent ) {
        MTP_LOG_CRITICAL("Could not finish data phase");
    } else {
        switch ( opCode ) {
        case MTP_OP_GetPartialObject:
            sendResponse(respCode, bytesSent > MTP_MAX_CONTENT_SIZE ? 0xFFFFFFFF : quint32(bytesSent));
            break;
        default:
            sendResponse(respCode);
            break;
        }
    }
    delete[] buffer;
}

void MTPResponder::processTransportEvents( bool &txCancelled )
{
    m_transporter->disableRW();
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents();
    m_transporter->enableRW();

    txCancelled = RESPONDER_TX_CANCEL == getResponderState();

    if ( RESPONDER_TX_CANCEL == getResponderState() ) {
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

void MTPResponder::dispatchEvent(MTPEventCode event, const QVector<quint32> &params)
{
    bool    filteringAllowed = true;
    quint32 ObjectHandle = 0;
    switch ( event ) {
    case MTP_EV_ObjectAdded:
        filteringAllowed = false;
    // FALLTHRU
    case MTP_EV_ObjectRemoved:
    case MTP_EV_ObjectInfoChanged:
    case MTP_EV_ObjectPropChanged:
        ObjectHandle = params[0];
        break;
    default:
        break;
    }

    bool EventsEnabled(true);
    QString ObjectPath("n/a");
    if ( ObjectHandle != 0x00000000 && ObjectHandle != 0xffffffff ) {
        /* NB: Unit tests can be executed in abnormal setup without storage server */
        if (m_storageServer) {
            m_storageServer->getPath(ObjectHandle, ObjectPath);
            m_storageServer->getEventsEnabled(ObjectHandle, EventsEnabled);
        }
    }

    if ( filteringAllowed && !EventsEnabled ) {
        MTP_LOG_TRACE(mtp_code_repr(event) << ObjectPath << "[skipped]");
        return;
    }

    QString args;
    foreach (quint32 param, params) {
        char hex[16];
        snprintf(hex, sizeof hex, "0x%x", param);
        if ( !args.isEmpty() )
            args.append(" ");
        args.append(hex);
    }

    MTP_LOG_INFO(mtp_code_repr(event) << ObjectPath << args);

    if ( !m_transporter ) {
        MTP_LOG_WARNING("Transporter not set; event ignored");
        return;
    }

    MTPTxContainer container(MTP_CONTAINER_TYPE_EVENT, event,
                             MTP_NO_TRANSACTION_ID, params.size() * sizeof (quint32));
    foreach (quint32 param, params) {
        container << param;
    }

    if (!sendContainer(container)) {
        MTP_LOG_CRITICAL("Couldn't dispatch event" << event);
    }
}

bool MTPResponder::needsStorageReady(MTPOperationCode code)
{
    switch (code) {
    case MTP_OP_GetDeviceInfo:
    case MTP_OP_OpenSession:
    case MTP_OP_CloseSession:
    case MTP_OP_GetStorageIDs:
    case MTP_OP_GetStorageInfo:
    case MTP_OP_GetDevicePropDesc:
    case MTP_OP_GetDevicePropValue:
    case MTP_OP_SetDevicePropValue:
    case MTP_OP_ResetDevicePropValue:
        return false;
    default:
        return true;
    }
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

const char *MTPResponder::responderStateName(MTPResponder::ResponderState state)
{
    const char *name = "RESPONDER_<unknown>";
    switch (state) {
    case RESPONDER_IDLE:
        name = "RESPONDER_IDLE";
        break;
    case RESPONDER_WAIT_DATA:
        name = "RESPONDER_WAIT_DATA";
        break;
    case RESPONDER_WAIT_RESP:
        name = "RESPONDER_WAIT_RESP";
        break;
    case RESPONDER_TX_CANCEL:
        name = "RESPONDER_TX_CANCEL";
        break;
    case RESPONDER_SUSPEND:
        name = "RESPONDER_SUSPEND";
        break;
    case RESPONDER_WAIT_STORAGE:
        name = "RESPONDER_WAIT_STORAGE";
        break;
    default:
        break;
    }
    return name;
}

MTPResponder::ResponderState MTPResponder::getResponderState(void)
{
    return m_state_accessor_only;
}

void MTPResponder::onIdleTimeout()
{
    MTP_LOG_INFO("command sequence ended");
    emit commandIdle();
}

void MTPResponder::setResponderState(MTPResponder::ResponderState state)
{
    MTPResponder::ResponderState prev = m_state_accessor_only;

    if ( prev != state ) {
        m_state_accessor_only = state;

        MTP_LOG_INFO("state:" << responderStateName(prev)
                     << "->" << responderStateName(state));

        bool wasBusy = (prev  != RESPONDER_IDLE);
        bool isBusy  = (state != RESPONDER_IDLE);

        if ( wasBusy != isBusy ) {
            if ( isBusy ) {
                m_handler_idle_timer->stop();
                emit commandPending();
            } else {
                emit commandFinished();
                m_handler_idle_timer->start();
            }
        }
    }
}

/* Helper functions for making human readable diagnostic logging */
extern "C"
{
    const char *mtp_format_category_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_UNSUPPORTED_FORMAT:
            res = "UNSUPPORTED_FORMAT";
            break;
        case MTP_AUDIO_FORMAT:
            res = "AUDIO_FORMAT";
            break;
        case MTP_VIDEO_FORMAT:
            res = "VIDEO_FORMAT";
            break;
        case MTP_IMAGE_FORMAT:
            res = "IMAGE_FORMAT";
            break;
        case MTP_COMMON_FORMAT:
            res = "COMMON_FORMAT";
            break;
        }
        return res;
    }
    const char *mtp_file_system_type_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_FILE_SYSTEM_TYPE_Undefined:
            res = "Undefined";
            break;
        case MTP_FILE_SYSTEM_TYPE_GenFlat:
            res = "GenFlat";
            break;
        case MTP_FILE_SYSTEM_TYPE_GenHier:
            res = "GenHier";
            break;
        case MTP_FILE_SYSTEM_TYPE_DCF:
            res = "DCF";
            break;
        }
        return res;
    }
    const char *mtp_association_type_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_ASSOCIATION_TYPE_Undefined:
            res = "Undefined";
            break;
        case MTP_ASSOCIATION_TYPE_GenFolder:
            res = "GenFolder";
            break;
        case MTP_ASSOCIATION_TYPE_Album:
            res = "Album";
            break;
        case MTP_ASSOCIATION_TYPE_TimeSeq:
            res = "TimeSeq";
            break;
        case MTP_ASSOCIATION_TYPE_HorizontalPanoramic:
            res = "HorizontalPanoramic";
            break;
        case MTP_ASSOCIATION_TYPE_VerticalPanoramic:
            res = "VerticalPanoramic";
            break;
        case MTP_ASSOCIATION_TYPE_2DPanoramic:
            res = "2DPanoramic";
            break;
        case MTP_ASSOCIATION_TYPE_AncillaryData:
            res = "AncillaryData";
            break;
        }
        return res;
    }
    const char *mtp_storage_access_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_STORAGE_ACCESS_ReadWrite:
            res = "ReadWrite";
            break;
        case MTP_STORAGE_ACCESS_ReadOnly_NoDel:
            res = "ReadOnly_NoDel";
            break;
        case MTP_STORAGE_ACCESS_ReadOnly_Del:
            res = "ReadOnly_Del";
            break;
        }
        return res;
    }
    const char *mtp_container_type_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_CONTAINER_TYPE_UNDEFINED:
            res = "UNDEFINED";
            break;
        case MTP_CONTAINER_TYPE_COMMAND:
            res = "COMMAND";
            break;
        case MTP_CONTAINER_TYPE_DATA:
            res = "DATA";
            break;
        case MTP_CONTAINER_TYPE_RESPONSE:
            res = "RESPONSE";
            break;
        case MTP_CONTAINER_TYPE_EVENT:
            res = "EVENT";
            break;
        }
        return res;
    }
    const char *mtp_obj_prop_form_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_OBJ_PROP_FORM_None:
            res = "None";
            break;
        case MTP_OBJ_PROP_FORM_Range:
            res = "Range";
            break;
        case MTP_OBJ_PROP_FORM_Enumeration:
            res = "Enumeration";
            break;
        case MTP_OBJ_PROP_FORM_DateTime:
            res = "DateTime";
            break;
        case MTP_OBJ_PROP_FORM_Fixed_length_Array:
            res = "Fixed_length_Array";
            break;
        case MTP_OBJ_PROP_FORM_Regular_Expression:
            res = "Regular_Expression";
            break;
        case MTP_OBJ_PROP_FORM_ByteArray:
            res = "ByteArray";
            break;
        case MTP_OBJ_PROP_FORM_LongString:
            res = "LongString";
            break;
        }
        return res;
    }
    const char *mtp_storage_type_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_STORAGE_TYPE_Undefined:
            res = "Undefined";
            break;
        case MTP_STORAGE_TYPE_FixedROM:
            res = "FixedROM";
            break;
        case MTP_STORAGE_TYPE_RemovableROM:
            res = "RemovableROM";
            break;
        case MTP_STORAGE_TYPE_FixedRAM:
            res = "FixedRAM";
            break;
        case MTP_STORAGE_TYPE_RemovableRAM:
            res = "RemovableRAM";
            break;
        }
        return res;
    }
    const char *mtp_bitrate_type_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_BITRATE_TYPE_UNUSED:
            res = "UNUSED";
            break;
        case MTP_BITRATE_TYPE_DISCRETE:
            res = "DISCRETE";
            break;
        case MTP_BITRATE_TYPE_VARIABLE:
            res = "VARIABLE";
            break;
        case MTP_BITRATE_TYPE_FREE:
            res = "FREE";
            break;
        }
        return res;
    }
    const char *mtp_protection_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_PROTECTION_NoProtection:
            res = "NoProtection";
            break;
        case MTP_PROTECTION_ReadOnly:
            res = "ReadOnly";
            break;
        case MTP_PROTECTION_ReadOnlyData:
            res = "ReadOnlyData";
            break;
        case MTP_PROTECTION_NonTransferrableData:
            res = "NonTransferrableData";
            break;
        }
        return res;
    }
    const char *mtp_form_flag_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_FORM_FLAG_NONE:
            res = "NONE";
            break;
        case MTP_FORM_FLAG_RANGE:
            res = "RANGE";
            break;
        case MTP_FORM_FLAG_ENUM:
            res = "ENUM";
            break;
        case MTP_FORM_FLAG_DATE_TIME:
            res = "DATE_TIME";
            break;
        case MTP_FORM_FLAG_FIXED_ARRAY:
            res = "FIXED_ARRAY";
            break;
        case MTP_FORM_FLAG_REGEX:
            res = "REGEX";
            break;
        case MTP_FORM_FLAG_BYTE_ARRAY:
            res = "BYTE_ARRAY";
            break;
        case MTP_FORM_FLAG_LONG_STRING:
            res = "LONG_STRING";
            break;
        }
        return res;
    }
    const char *mtp_data_type_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_DATA_TYPE_UNDEF:
            res = "UNDEF";
            break;
        case MTP_DATA_TYPE_INT8:
            res = "INT8";
            break;
        case MTP_DATA_TYPE_UINT8:
            res = "UINT8";
            break;
        case MTP_DATA_TYPE_INT16:
            res = "INT16";
            break;
        case MTP_DATA_TYPE_UINT16:
            res = "UINT16";
            break;
        case MTP_DATA_TYPE_INT32:
            res = "INT32";
            break;
        case MTP_DATA_TYPE_UINT32:
            res = "UINT32";
            break;
        case MTP_DATA_TYPE_INT64:
            res = "INT64";
            break;
        case MTP_DATA_TYPE_UINT64:
            res = "UINT64";
            break;
        case MTP_DATA_TYPE_INT128:
            res = "INT128";
            break;
        case MTP_DATA_TYPE_UINT128:
            res = "UINT128";
            break;
        case MTP_DATA_TYPE_AINT8:
            res = "AINT8";
            break;
        case MTP_DATA_TYPE_AUINT8:
            res = "AUINT8";
            break;
        case MTP_DATA_TYPE_AINT16:
            res = "AINT16";
            break;
        case MTP_DATA_TYPE_AUINT16:
            res = "AUINT16";
            break;
        case MTP_DATA_TYPE_AINT32:
            res = "AINT32";
            break;
        case MTP_DATA_TYPE_AUINT32:
            res = "AUINT32";
            break;
        case MTP_DATA_TYPE_AINT64:
            res = "AINT64";
            break;
        case MTP_DATA_TYPE_AUINT64:
            res = "AUINT64";
            break;
        case MTP_DATA_TYPE_AINT128:
            res = "AINT128";
            break;
        case MTP_DATA_TYPE_AUINT128:
            res = "AUINT128";
            break;
        case MTP_DATA_TYPE_STR:
            res = "STR";
            break;
        }
        return res;
    }
    const char *mtp_ch_conf_repr(int val)
    {
        const char *res = "<unknown>";
        switch (val) {
        case MTP_CH_CONF_UNUSED:
            res = "UNUSED";
            break;
        case MTP_CH_CONF_MONO:
            res = "MONO";
            break;
        case MTP_CH_CONF_STEREO:
            res = "STEREO";
            break;
        case MTP_CH_CONF_2_1_CH:
            res = "2_1_CH";
            break;
        case MTP_CH_CONF_3_CH:
            res = "3_CH";
            break;
        case MTP_CH_CONF_3_1_CH:
            res = "3_1_CH";
            break;
        case MTP_CH_CONF_4_CH:
            res = "4_CH";
            break;
        case MTP_CH_CONF_4_1_CH:
            res = "4_1_CH";
            break;
        case MTP_CH_CONF_5_CH:
            res = "5_CH";
            break;
        case MTP_CH_CONF_5_1_CH:
            res = "5_1_CH";
            break;
        case MTP_CH_CONF_6_CH:
            res = "6_CH";
            break;
        case MTP_CH_CONF_6_1_CH:
            res = "6_1_CH";
            break;
        case MTP_CH_CONF_7_CH:
            res = "7_CH";
            break;
        case MTP_CH_CONF_7_1_CH:
            res = "7_1_CH";
            break;
        case MTP_CH_CONF_8_CH:
            res = "8_CH";
            break;
        case MTP_CH_CONF_8_1_CH:
            res = "8_1_CH";
            break;
        case MTP_CH_CONF_9_CH:
            res = "9_CH";
            break;
        case MTP_CH_CONF_9_1_CH:
            res = "9_1_CH";
            break;
        case MTP_CH_CONF_5_2_CH:
            res = "5_2_CH";
            break;
        case MTP_CH_CONF_6_2_CH:
            res = "6_2_CH";
            break;
        case MTP_CH_CONF_7_2_CH:
            res = "7_2_CH";
            break;
        case MTP_CH_CONF_8_2_CH:
            res = "8_2_CH";
            break;
        }
        return res;
    }
    const char *mtp_code_repr(int val)
    {
        const char *res = 0;
        switch (val) {
        case MTP_OP_GetDeviceInfo:
            res = "OP_GetDeviceInfo";
            break;
        case MTP_OP_OpenSession:
            res = "OP_OpenSession";
            break;
        case MTP_OP_CloseSession:
            res = "OP_CloseSession";
            break;
        case MTP_OP_GetStorageIDs:
            res = "OP_GetStorageIDs";
            break;
        case MTP_OP_GetStorageInfo:
            res = "OP_GetStorageInfo";
            break;
        case MTP_OP_GetNumObjects:
            res = "OP_GetNumObjects";
            break;
        case MTP_OP_GetObjectHandles:
            res = "OP_GetObjectHandles";
            break;
        case MTP_OP_GetObjectInfo:
            res = "OP_GetObjectInfo";
            break;
        case MTP_OP_GetObject:
            res = "OP_GetObject";
            break;
        case MTP_OP_GetThumb:
            res = "OP_GetThumb";
            break;
        case MTP_OP_DeleteObject:
            res = "OP_DeleteObject";
            break;
        case MTP_OP_SendObjectInfo:
            res = "OP_SendObjectInfo";
            break;
        case MTP_OP_SendObject:
            res = "OP_SendObject";
            break;
        case MTP_OP_InitiateCapture:
            res = "OP_InitiateCapture";
            break;
        case MTP_OP_FormatStore:
            res = "OP_FormatStore";
            break;
        case MTP_OP_ResetDevice:
            res = "OP_ResetDevice";
            break;
        case MTP_OP_SelfTest:
            res = "OP_SelfTest";
            break;
        case MTP_OP_SetObjectProtection:
            res = "OP_SetObjectProtection";
            break;
        case MTP_OP_PowerDown:
            res = "OP_PowerDown";
            break;
        case MTP_OP_GetDevicePropDesc:
            res = "OP_GetDevicePropDesc";
            break;
        case MTP_OP_GetDevicePropValue:
            res = "OP_GetDevicePropValue";
            break;
        case MTP_OP_SetDevicePropValue:
            res = "OP_SetDevicePropValue";
            break;
        case MTP_OP_ResetDevicePropValue:
            res = "OP_ResetDevicePropValue";
            break;
        case MTP_OP_TerminateOpenCapture:
            res = "OP_TerminateOpenCapture";
            break;
        case MTP_OP_MoveObject:
            res = "OP_MoveObject";
            break;
        case MTP_OP_CopyObject:
            res = "OP_CopyObject";
            break;
        case MTP_OP_GetPartialObject:
            res = "OP_GetPartialObject";
            break;
        case MTP_OP_InitiateOpenCapture:
            res = "OP_InitiateOpenCapture";
            break;
        case MTP_RESP_Undefined:
            res = "RESP_Undefined";
            break;
        case MTP_RESP_OK:
            res = "RESP_OK";
            break;
        case MTP_RESP_GeneralError:
            res = "RESP_GeneralError";
            break;
        case MTP_RESP_SessionNotOpen:
            res = "RESP_SessionNotOpen";
            break;
        case MTP_RESP_InvalidTransID:
            res = "RESP_InvalidTransID";
            break;
        case MTP_RESP_OperationNotSupported:
            res = "RESP_OperationNotSupported";
            break;
        case MTP_RESP_ParameterNotSupported:
            res = "RESP_ParameterNotSupported";
            break;
        case MTP_RESP_IncompleteTransfer:
            res = "RESP_IncompleteTransfer";
            break;
        case MTP_RESP_InvalidStorageID:
            res = "RESP_InvalidStorageID";
            break;
        case MTP_RESP_InvalidObjectHandle:
            res = "RESP_InvalidObjectHandle";
            break;
        case MTP_RESP_DevicePropNotSupported:
            res = "RESP_DevicePropNotSupported";
            break;
        case MTP_RESP_InvalidObjectFormatCode:
            res = "RESP_InvalidObjectFormatCode";
            break;
        case MTP_RESP_StoreFull:
            res = "RESP_StoreFull";
            break;
        case MTP_RESP_ObjectWriteProtected:
            res = "RESP_ObjectWriteProtected";
            break;
        case MTP_RESP_StoreReadOnly:
            res = "RESP_StoreReadOnly";
            break;
        case MTP_RESP_AccessDenied:
            res = "RESP_AccessDenied";
            break;
        case MTP_RESP_NoThumbnailPresent:
            res = "RESP_NoThumbnailPresent";
            break;
        case MTP_RESP_SelfTestFailed:
            res = "RESP_SelfTestFailed";
            break;
        case MTP_RESP_PartialDeletion:
            res = "RESP_PartialDeletion";
            break;
        case MTP_RESP_StoreNotAvailable:
            res = "RESP_StoreNotAvailable";
            break;
        case MTP_RESP_SpecByFormatUnsupported:
            res = "RESP_SpecByFormatUnsupported";
            break;
        case MTP_RESP_NoValidObjectInfo:
            res = "RESP_NoValidObjectInfo";
            break;
        case MTP_RESP_InvalidCodeFormat:
            res = "RESP_InvalidCodeFormat";
            break;
        case MTP_RESP_UnknowVendorCode:
            res = "RESP_UnknowVendorCode";
            break;
        case MTP_RESP_CaptureAlreadyTerminated:
            res = "RESP_CaptureAlreadyTerminated";
            break;
        case MTP_RESP_DeviceBusy:
            res = "RESP_DeviceBusy";
            break;
        case MTP_RESP_InvalidParentObject:
            res = "RESP_InvalidParentObject";
            break;
        case MTP_RESP_InvalidDevicePropFormat:
            res = "RESP_InvalidDevicePropFormat";
            break;
        case MTP_RESP_InvalidDevicePropValue:
            res = "RESP_InvalidDevicePropValue";
            break;
        case MTP_RESP_InvalidParameter:
            res = "RESP_InvalidParameter";
            break;
        case MTP_RESP_SessionAlreadyOpen:
            res = "RESP_SessionAlreadyOpen";
            break;
        case MTP_RESP_TransactionCancelled:
            res = "RESP_TransactionCancelled";
            break;
        case MTP_RESP_SpecificationOfDestinationUnsupported:
            res = "RESP_SpecificationOfDestinationUnsupported";
            break;
        case MTP_OBF_FORMAT_Undefined:
            res = "OBF_FORMAT_Undefined";
            break;
        case MTP_OBF_FORMAT_Association:
            res = "OBF_FORMAT_Association";
            break;
        case MTP_OBF_FORMAT_Script:
            res = "OBF_FORMAT_Script";
            break;
        case MTP_OBF_FORMAT_Executable:
            res = "OBF_FORMAT_Executable";
            break;
        case MTP_OBF_FORMAT_Text:
            res = "OBF_FORMAT_Text";
            break;
        case MTP_OBF_FORMAT_HTML:
            res = "OBF_FORMAT_HTML";
            break;
        case MTP_OBF_FORMAT_DPOF:
            res = "OBF_FORMAT_DPOF";
            break;
        case MTP_OBF_FORMAT_AIFF:
            res = "OBF_FORMAT_AIFF";
            break;
        case MTP_OBF_FORMAT_WAV:
            res = "OBF_FORMAT_WAV";
            break;
        case MTP_OBF_FORMAT_MP3:
            res = "OBF_FORMAT_MP3";
            break;
        case MTP_OBF_FORMAT_AVI:
            res = "OBF_FORMAT_AVI";
            break;
        case MTP_OBF_FORMAT_MPEG:
            res = "OBF_FORMAT_MPEG";
            break;
        case MTP_OBF_FORMAT_ASF:
            res = "OBF_FORMAT_ASF";
            break;
        case MTP_OBF_FORMAT_Unknown_Image_Object:
            res = "OBF_FORMAT_Unknown_Image_Object";
            break;
        case MTP_OBF_FORMAT_EXIF_JPEG:
            res = "OBF_FORMAT_EXIF_JPEG";
            break;
        case MTP_OBF_FORMAT_TIFF_EP:
            res = "OBF_FORMAT_TIFF_EP";
            break;
        case MTP_OBF_FORMAT_FlashPix:
            res = "OBF_FORMAT_FlashPix";
            break;
        case MTP_OBF_FORMAT_BMP:
            res = "OBF_FORMAT_BMP";
            break;
        case MTP_OBF_FORMAT_CIFF:
            res = "OBF_FORMAT_CIFF";
            break;
        case MTP_OBF_FORMAT_GIF:
            res = "OBF_FORMAT_GIF";
            break;
        case MTP_OBF_FORMAT_JFIF:
            res = "OBF_FORMAT_JFIF";
            break;
        case MTP_OBF_FORMAT_PCD:
            res = "OBF_FORMAT_PCD";
            break;
        case MTP_OBF_FORMAT_PICT:
            res = "OBF_FORMAT_PICT";
            break;
        case MTP_OBF_FORMAT_PNG:
            res = "OBF_FORMAT_PNG";
            break;
        case MTP_OBF_FORMAT_TIFF:
            res = "OBF_FORMAT_TIFF";
            break;
        case MTP_OBF_FORMAT_TIFF_IT:
            res = "OBF_FORMAT_TIFF_IT";
            break;
        case MTP_OBF_FORMAT_JP2:
            res = "OBF_FORMAT_JP2";
            break;
        case MTP_OBF_FORMAT_JPX:
            res = "OBF_FORMAT_JPX";
            break;
        case MTP_OBF_FORMAT_M4A:
            res = "OBF_FORMAT_M4A";
            break;
        case MTP_EV_Undefined:
            res = "EV_Undefined";
            break;
        case MTP_EV_CancelTransaction:
            res = "EV_CancelTransaction";
            break;
        case MTP_EV_ObjectAdded:
            res = "EV_ObjectAdded";
            break;
        case MTP_EV_ObjectRemoved:
            res = "EV_ObjectRemoved";
            break;
        case MTP_EV_StoreAdded:
            res = "EV_StoreAdded";
            break;
        case MTP_EV_StoreRemoved:
            res = "EV_StoreRemoved";
            break;
        case MTP_EV_DevicePropChanged:
            res = "EV_DevicePropChanged";
            break;
        case MTP_EV_ObjectInfoChanged:
            res = "EV_ObjectInfoChanged";
            break;
        case MTP_EV_DeviceInfoChanged:
            res = "EV_DeviceInfoChanged";
            break;
        case MTP_EV_RequestObjectTransfer:
            res = "EV_RequestObjectTransfer";
            break;
        case MTP_EV_StoreFull:
            res = "EV_StoreFull";
            break;
        case MTP_EV_DeviceReset:
            res = "EV_DeviceReset";
            break;
        case MTP_EV_StorageInfoChanged:
            res = "EV_StorageInfoChanged";
            break;
        case MTP_EV_CaptureComplete:
            res = "EV_CaptureComplete";
            break;
        case MTP_EV_UnreportedStatus:
            res = "EV_UnreportedStatus";
            break;
        case MTP_DEV_PROPERTY_Undefined:
            res = "DEV_PROPERTY_Undefined";
            break;
        case MTP_DEV_PROPERTY_BatteryLevel:
            res = "DEV_PROPERTY_BatteryLevel";
            break;
        case MTP_DEV_PROPERTY_FunctionalMode:
            res = "DEV_PROPERTY_FunctionalMode";
            break;
        case MTP_DEV_PROPERTY_ImageSize:
            res = "DEV_PROPERTY_ImageSize";
            break;
        case MTP_DEV_PROPERTY_CompressionSetting:
            res = "DEV_PROPERTY_CompressionSetting";
            break;
        case MTP_DEV_PROPERTY_WhiteBalance:
            res = "DEV_PROPERTY_WhiteBalance";
            break;
        case MTP_DEV_PROPERTY_RGB_Gain:
            res = "DEV_PROPERTY_RGB_Gain";
            break;
        case MTP_DEV_PROPERTY_F_Number:
            res = "DEV_PROPERTY_F_Number";
            break;
        case MTP_DEV_PROPERTY_FocalLength:
            res = "DEV_PROPERTY_FocalLength";
            break;
        case MTP_DEV_PROPERTY_FocusDistance:
            res = "DEV_PROPERTY_FocusDistance";
            break;
        case MTP_DEV_PROPERTY_FocusMod:
            res = "DEV_PROPERTY_FocusMod";
            break;
        case MTP_DEV_PROPERTY_ExposureMeteringMode:
            res = "DEV_PROPERTY_ExposureMeteringMode";
            break;
        case MTP_DEV_PROPERTY_FlashMode:
            res = "DEV_PROPERTY_FlashMode";
            break;
        case MTP_DEV_PROPERTY_ExposureTime:
            res = "DEV_PROPERTY_ExposureTime";
            break;
        case MTP_DEV_PROPERTY_ExposureProgramMode:
            res = "DEV_PROPERTY_ExposureProgramMode";
            break;
        case MTP_DEV_PROPERTY_ExposureInde:
            res = "DEV_PROPERTY_ExposureInde";
            break;
        case MTP_DEV_PROPERTY_ExposureBiasCompensation:
            res = "DEV_PROPERTY_ExposureBiasCompensation";
            break;
        case MTP_DEV_PROPERTY_DateTime:
            res = "DEV_PROPERTY_DateTime";
            break;
        case MTP_DEV_PROPERTY_CaptureDelay:
            res = "DEV_PROPERTY_CaptureDelay";
            break;
        case MTP_DEV_PROPERTY_StillCaptureMode:
            res = "DEV_PROPERTY_StillCaptureMode";
            break;
        case MTP_DEV_PROPERTY_Contrast:
            res = "DEV_PROPERTY_Contrast";
            break;
        case MTP_DEV_PROPERTY_Sharpness:
            res = "DEV_PROPERTY_Sharpness";
            break;
        case MTP_DEV_PROPERTY_DigitalZoom:
            res = "DEV_PROPERTY_DigitalZoom";
            break;
        case MTP_DEV_PROPERTY_EffectMode:
            res = "DEV_PROPERTY_EffectMode";
            break;
        case MTP_DEV_PROPERTY_BurstNumber:
            res = "DEV_PROPERTY_BurstNumber";
            break;
        case MTP_DEV_PROPERTY_BurstInterval:
            res = "DEV_PROPERTY_BurstInterval";
            break;
        case MTP_DEV_PROPERTY_TimelapseNumber:
            res = "DEV_PROPERTY_TimelapseNumber";
            break;
        case MTP_DEV_PROPERTY_TimelapseInterval:
            res = "DEV_PROPERTY_TimelapseInterval";
            break;
        case MTP_DEV_PROPERTY_FocusMeteringMode:
            res = "DEV_PROPERTY_FocusMeteringMode";
            break;
        case MTP_DEV_PROPERTY_UploadURL:
            res = "DEV_PROPERTY_UploadURL";
            break;
        case MTP_DEV_PROPERTY_Artist:
            res = "DEV_PROPERTY_Artist";
            break;
        case MTP_DEV_PROPERTY_CopyrightInfo:
            res = "DEV_PROPERTY_CopyrightInfo";
            break;
        case MTP_OP_ANDROID_GetPartialObject64:
            res = "OP_ANDROID_GetPartialObject64";
            break;
        case MTP_OP_ANDROID_SendPartialObject64:
            res = "OP_ANDROID_SendPartialObject64";
            break;
        case MTP_OP_ANDROID_TruncateObject64:
            res = "OP_ANDROID_TruncateObject64";
            break;
        case MTP_OP_ANDROID_BeginEditObject:
            res = "OP_ANDROID_BeginEditObject";
            break;
        case MTP_OP_ANDROID_EndEditObject:
            res = "OP_ANDROID_EndEditObject";
            break;
        case MTP_OP_GetObjectPropsSupported:
            res = "OP_GetObjectPropsSupported";
            break;
        case MTP_OP_GetObjectPropDesc:
            res = "OP_GetObjectPropDesc";
            break;
        case MTP_OP_GetObjectPropValue:
            res = "OP_GetObjectPropValue";
            break;
        case MTP_OP_SetObjectPropValue:
            res = "OP_SetObjectPropValue";
            break;
        case MTP_OP_GetObjectPropList:
            res = "OP_GetObjectPropList";
            break;
        case MTP_OP_SetObjectPropList:
            res = "OP_SetObjectPropList";
            break;
        case MTP_OP_GetInterdependentPropDesc:
            res = "OP_GetInterdependentPropDesc";
            break;
        case MTP_OP_SendObjectPropList:
            res = "OP_SendObjectPropList";
            break;
        case MTP_OP_GetObjectReferences:
            res = "OP_GetObjectReferences";
            break;
        case MTP_OP_SetObjectReferences:
            res = "OP_SetObjectReferences";
            break;
        case MTP_OP_Skip:
            res = "OP_Skip";
            break;
        case MTP_RESP_Invalid_ObjectPropCode:
            res = "RESP_Invalid_ObjectPropCode";
            break;
        case MTP_RESP_Invalid_ObjectProp_Format:
            res = "RESP_Invalid_ObjectProp_Format";
            break;
        case MTP_RESP_Invalid_ObjectProp_Value:
            res = "RESP_Invalid_ObjectProp_Value";
            break;
        case MTP_RESP_Invalid_ObjectReference:
            res = "RESP_Invalid_ObjectReference";
            break;
        case MTP_RESP_Invalid_Dataset:
            res = "RESP_Invalid_Dataset";
            break;
        case MTP_RESP_Specification_By_Group_Unsupported:
            res = "RESP_Specification_By_Group_Unsupported";
            break;
        case MTP_RESP_Specification_By_Depth_Unsupported:
            res = "RESP_Specification_By_Depth_Unsupported";
            break;
        case MTP_RESP_Object_Too_Large:
            res = "RESP_Object_Too_Large";
            break;
        case MTP_RESP_ObjectProp_Not_Supported:
            res = "RESP_ObjectProp_Not_Supported";
            break;
        case MTP_OBF_FORMAT_Undefined_Firmware:
            res = "OBF_FORMAT_Undefined_Firmware";
            break;
        case MTP_OBF_FORMAT_Windows_Image_Format:
            res = "OBF_FORMAT_Windows_Image_Format";
            break;
        case MTP_OBF_FORMAT_Undefined_Audio:
            res = "OBF_FORMAT_Undefined_Audio";
            break;
        case MTP_OBF_FORMAT_WMA:
            res = "OBF_FORMAT_WMA";
            break;
        case MTP_OBF_FORMAT_OGG:
            res = "OBF_FORMAT_OGG";
            break;
        case MTP_OBF_FORMAT_AAC:
            res = "OBF_FORMAT_AAC";
            break;
        case MTP_OBF_FORMAT_Audible:
            res = "OBF_FORMAT_Audible";
            break;
        case MTP_OBF_FORMAT_FLAC:
            res = "OBF_FORMAT_FLAC";
            break;
        case MTP_OBF_FORMAT_Undefined_Video:
            res = "OBF_FORMAT_Undefined_Video";
            break;
        case MTP_OBF_FORMAT_WMV:
            res = "OBF_FORMAT_WMV";
            break;
        case MTP_OBF_FORMAT_MP4_Container:
            res = "OBF_FORMAT_MP4_Container";
            break;
        case MTP_OBF_FORMAT_3GP_Container:
            res = "OBF_FORMAT_3GP_Container";
            break;
        case MTP_OBF_FORMAT_Undefined_Collection:
            res = "OBF_FORMAT_Undefined_Collection";
            break;
        case MTP_OBF_FORMAT_Abstract_Multimedia_Album:
            res = "OBF_FORMAT_Abstract_Multimedia_Album";
            break;
        case MTP_OBF_FORMAT_Abstract_Image_Album:
            res = "OBF_FORMAT_Abstract_Image_Album";
            break;
        case MTP_OBF_FORMAT_Abstract_Audio_Album:
            res = "OBF_FORMAT_Abstract_Audio_Album";
            break;
        case MTP_OBF_FORMAT_Abstract_Video_Album:
            res = "OBF_FORMAT_Abstract_Video_Album";
            break;
        case MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist:
            res = "OBF_FORMAT_Abstract_Audio_Video_Playlist";
            break;
        case MTP_OBF_FORMAT_Abstract_Contact_Group:
            res = "OBF_FORMAT_Abstract_Contact_Group";
            break;
        case MTP_OBF_FORMAT_Abstract_Message_Folder:
            res = "OBF_FORMAT_Abstract_Message_Folder";
            break;
        case MTP_OBF_FORMAT_Abstract_Chaptered_Production:
            res = "OBF_FORMAT_Abstract_Chaptered_Production";
            break;
        case MTP_OBF_FORMAT_Abstract_Audio_Playlist:
            res = "OBF_FORMAT_Abstract_Audio_Playlist";
            break;
        case MTP_OBF_FORMAT_Abstract_Video_Playlist:
            res = "OBF_FORMAT_Abstract_Video_Playlist";
            break;
        case MTP_OBF_FORMAT_WPL_Playlist:
            res = "OBF_FORMAT_WPL_Playlist";
            break;
        case MTP_OBF_FORMAT_M3U_Playlist:
            res = "OBF_FORMAT_M3U_Playlist";
            break;
        case MTP_OBF_FORMAT_MPL_Playlist:
            res = "OBF_FORMAT_MPL_Playlist";
            break;
        case MTP_OBF_FORMAT_ASX_Playlist:
            res = "OBF_FORMAT_ASX_Playlist";
            break;
        case MTP_OBF_FORMAT_PLS_Playlist:
            res = "OBF_FORMAT_PLS_Playlist";
            break;
        case MTP_OBF_FORMAT_Undefined_Document:
            res = "OBF_FORMAT_Undefined_Document";
            break;
        case MTP_OBF_FORMAT_Abstract_Document:
            res = "OBF_FORMAT_Abstract_Document";
            break;
        case MTP_OBF_FORMAT_Undefined_Message:
            res = "OBF_FORMAT_Undefined_Message";
            break;
        case MTP_OBF_FORMAT_Abstract_Message:
            res = "OBF_FORMAT_Abstract_Message";
            break;
        case MTP_OBF_FORMAT_Undefined_Contact:
            res = "OBF_FORMAT_Undefined_Contact";
            break;
        case MTP_OBF_FORMAT_Abstract_Contact:
            res = "OBF_FORMAT_Abstract_Contact";
            break;
        case MTP_OBF_FORMAT_vCard2:
            res = "OBF_FORMAT_vCard2";
            break;
        case MTP_OBF_FORMAT_vCard3:
            res = "OBF_FORMAT_vCard3";
            break;
        case MTP_OBF_FORMAT_Undefined_Calendar_Item:
            res = "OBF_FORMAT_Undefined_Calendar_Item";
            break;
        case MTP_OBF_FORMAT_Abstract_Calendar_Item:
            res = "OBF_FORMAT_Abstract_Calendar_Item";
            break;
        case MTP_OBF_FORMAT_vCal1:
            res = "OBF_FORMAT_vCal1";
            break;
        case MTP_OBF_FORMAT_vCal2:
            res = "OBF_FORMAT_vCal2";
            break;
        case MTP_OBF_FORMAT_Undefined_Windows_Executable:
            res = "OBF_FORMAT_Undefined_Windows_Executable";
            break;
        case MTP_OBF_FORMAT_WBMP:
            res = "OBF_FORMAT_WBMP";
            break;
        case MTP_OBF_FORMAT_JPEG_XR:
            res = "OBF_FORMAT_JPEG_XR";
            break;
        case MTP_OBF_FORMAT_QCELP:
            res = "OBF_FORMAT_QCELP";
            break;
        case MTP_OBF_FORMAT_AMR:
            res = "OBF_FORMAT_AMR";
            break;
        case MTP_OBF_FORMAT_MP2:
            res = "OBF_FORMAT_MP2";
            break;
        case MTP_OBF_FORMAT_3G2:
            res = "OBF_FORMAT_3G2";
            break;
        case MTP_OBF_FORMAT_AVCHD:
            res = "OBF_FORMAT_AVCHD";
            break;
        case MTP_OBF_FORMAT_ATSC_TS:
            res = "OBF_FORMAT_ATSC_TS";
            break;
        case MTP_OBF_FORMAT_DVB_TS:
            res = "OBF_FORMAT_DVB_TS";
            break;
        case MTP_OBF_FORMAT_Abstract_Mediacast:
            res = "OBF_FORMAT_Abstract_Mediacast";
            break;
        case MTP_OBF_FORMAT_XML_Document:
            res = "OBF_FORMAT_XML_Document";
            break;
        case MTP_OBF_FORMAT_Microsoft_Word_Document:
            res = "OBF_FORMAT_Microsoft_Word_Document";
            break;
        case MTP_OBF_FORMAT_MHT_Compiled_HTML_Document:
            res = "OBF_FORMAT_MHT_Compiled_HTML_Document";
            break;
        case MTP_OBF_FORMAT_Microsoft_Excel_spreadsheet:
            res = "OBF_FORMAT_Microsoft_Excel_spreadsheet";
            break;
        case MTP_OBF_FORMAT_Microsoft_Powerpoint_presentation:
            res = "OBF_FORMAT_Microsoft_Powerpoint_presentation";
            break;
        case MTP_OBF_FORMAT_Undefined_Bookmark:
            res = "OBF_FORMAT_Undefined_Bookmark";
            break;
        case MTP_OBF_FORMAT_Abstract_Bookmark:
            res = "OBF_FORMAT_Abstract_Bookmark";
            break;
        case MTP_OBF_FORMAT_Undefined_Appointment:
            res = "OBF_FORMAT_Undefined_Appointment";
            break;
        case MTP_OBF_FORMAT_Abstract_Appointment:
            res = "OBF_FORMAT_Abstract_Appointment";
            break;
        case MTP_OBF_FORMAT_vCalendar_1_0:
            res = "OBF_FORMAT_vCalendar_1_0";
            break;
        case MTP_OBF_FORMAT_Undefined_Task:
            res = "OBF_FORMAT_Undefined_Task";
            break;
        case MTP_OBF_FORMAT_Abstract_Task:
            res = "OBF_FORMAT_Abstract_Task";
            break;
        case MTP_OBF_FORMAT_iCalendar:
            res = "OBF_FORMAT_iCalendar";
            break;
        case MTP_OBF_FORMAT_Undefined_Note:
            res = "OBF_FORMAT_Undefined_Note";
            break;
        case MTP_OBF_FORMAT_Abstract_Note:
            res = "OBF_FORMAT_Abstract_Note";
            break;
        case MTP_EV_ObjectPropChanged:
            res = "EV_ObjectPropChanged";
            break;
        case MTP_EV_ObjectPropDescChanged:
            res = "EV_ObjectPropDescChanged";
            break;
        case MTP_DEV_PROPERTY_Synchronization_Partner:
            res = "DEV_PROPERTY_Synchronization_Partner";
            break;
        case MTP_DEV_PROPERTY_Device_Friendly_Name:
            res = "DEV_PROPERTY_Device_Friendly_Name";
            break;
        case MTP_DEV_PROPERTY_Volume:
            res = "DEV_PROPERTY_Volume";
            break;
        case MTP_DEV_PROPERTY_Supported_Formats_Ordered:
            res = "DEV_PROPERTY_Supported_Formats_Ordered";
            break;
        case MTP_DEV_PROPERTY_DeviceIcon:
            res = "DEV_PROPERTY_DeviceIcon";
            break;
        case MTP_DEV_PROPERTY_Perceived_Device_Type:
            res = "DEV_PROPERTY_Perceived_Device_Type";
            break;
        case MTP_OBJ_PROP_Purchase_Album:
            res = "OBJ_PROP_Purchase_Album";
            break;
        case MTP_OBJ_PROP_StorageID:
            res = "OBJ_PROP_StorageID";
            break;
        case MTP_OBJ_PROP_Obj_Format:
            res = "OBJ_PROP_Obj_Format";
            break;
        case MTP_OBJ_PROP_Protection_Status:
            res = "OBJ_PROP_Protection_Status";
            break;
        case MTP_OBJ_PROP_Obj_Size:
            res = "OBJ_PROP_Obj_Size";
            break;
        case MTP_OBJ_PROP_Association_Type:
            res = "OBJ_PROP_Association_Type";
            break;
        case MTP_OBJ_PROP_Association_Desc:
            res = "OBJ_PROP_Association_Desc";
            break;
        case MTP_OBJ_PROP_Obj_File_Name:
            res = "OBJ_PROP_Obj_File_Name";
            break;
        case MTP_OBJ_PROP_Date_Created:
            res = "OBJ_PROP_Date_Created";
            break;
        case MTP_OBJ_PROP_Date_Modified:
            res = "OBJ_PROP_Date_Modified";
            break;
        case MTP_OBJ_PROP_Keywords:
            res = "OBJ_PROP_Keywords";
            break;
        case MTP_OBJ_PROP_Parent_Obj:
            res = "OBJ_PROP_Parent_Obj";
            break;
        case MTP_OBJ_PROP_Allowed_Folder_Contents:
            res = "OBJ_PROP_Allowed_Folder_Contents";
            break;
        case MTP_OBJ_PROP_Hidden:
            res = "OBJ_PROP_Hidden";
            break;
        case MTP_OBJ_PROP_Sys_Obj:
            res = "OBJ_PROP_Sys_Obj";
            break;
        case MTP_OBJ_PROP_Persistent_Unique_ObjId:
            res = "OBJ_PROP_Persistent_Unique_ObjId";
            break;
        case MTP_OBJ_PROP_SyncID:
            res = "OBJ_PROP_SyncID";
            break;
        case MTP_OBJ_PROP_Property_Bag:
            res = "OBJ_PROP_Property_Bag";
            break;
        case MTP_OBJ_PROP_Name:
            res = "OBJ_PROP_Name";
            break;
        case MTP_OBJ_PROP_Created_By:
            res = "OBJ_PROP_Created_By";
            break;
        case MTP_OBJ_PROP_Artist:
            res = "OBJ_PROP_Artist";
            break;
        case MTP_OBJ_PROP_Date_Authored:
            res = "OBJ_PROP_Date_Authored";
            break;
        case MTP_OBJ_PROP_Description:
            res = "OBJ_PROP_Description";
            break;
        case MTP_OBJ_PROP_URL_Reference:
            res = "OBJ_PROP_URL_Reference";
            break;
        case MTP_OBJ_PROP_Language_Locale:
            res = "OBJ_PROP_Language_Locale";
            break;
        case MTP_OBJ_PROP_Copyright_Information:
            res = "OBJ_PROP_Copyright_Information";
            break;
        case MTP_OBJ_PROP_Source:
            res = "OBJ_PROP_Source";
            break;
        case MTP_OBJ_PROP_Origin_Location:
            res = "OBJ_PROP_Origin_Location";
            break;
        case MTP_OBJ_PROP_Date_Added:
            res = "OBJ_PROP_Date_Added";
            break;
        case MTP_OBJ_PROP_Non_Consumable:
            res = "OBJ_PROP_Non_Consumable";
            break;
        case MTP_OBJ_PROP_Corrupt_Unplayable:
            res = "OBJ_PROP_Corrupt_Unplayable";
            break;
        case MTP_OBJ_PROP_Rep_Sample_Format:
            res = "OBJ_PROP_Rep_Sample_Format";
            break;
        case MTP_OBJ_PROP_Rep_Sample_Size:
            res = "OBJ_PROP_Rep_Sample_Size";
            break;
        case MTP_OBJ_PROP_Rep_Sample_Height:
            res = "OBJ_PROP_Rep_Sample_Height";
            break;
        case MTP_OBJ_PROP_Rep_Sample_Width:
            res = "OBJ_PROP_Rep_Sample_Width";
            break;
        case MTP_OBJ_PROP_Rep_Sample_Duration:
            res = "OBJ_PROP_Rep_Sample_Duration";
            break;
        case MTP_OBJ_PROP_Rep_Sample_Data:
            res = "OBJ_PROP_Rep_Sample_Data";
            break;
        case MTP_OBJ_PROP_Width:
            res = "OBJ_PROP_Width";
            break;
        case MTP_OBJ_PROP_Height:
            res = "OBJ_PROP_Height";
            break;
        case MTP_OBJ_PROP_Duration:
            res = "OBJ_PROP_Duration";
            break;
        case MTP_OBJ_PROP_Rating:
            res = "OBJ_PROP_Rating";
            break;
        case MTP_OBJ_PROP_Track:
            res = "OBJ_PROP_Track";
            break;
        case MTP_OBJ_PROP_Genre:
            res = "OBJ_PROP_Genre";
            break;
        case MTP_OBJ_PROP_Credits:
            res = "OBJ_PROP_Credits";
            break;
        case MTP_OBJ_PROP_Lyrics:
            res = "OBJ_PROP_Lyrics";
            break;
        case MTP_OBJ_PROP_Subscription_Content_ID:
            res = "OBJ_PROP_Subscription_Content_ID";
            break;
        case MTP_OBJ_PROP_Produced_By:
            res = "OBJ_PROP_Produced_By";
            break;
        case MTP_OBJ_PROP_Use_Count:
            res = "OBJ_PROP_Use_Count";
            break;
        case MTP_OBJ_PROP_Skip_Count:
            res = "OBJ_PROP_Skip_Count";
            break;
        case MTP_OBJ_PROP_Last_Accessed:
            res = "OBJ_PROP_Last_Accessed";
            break;
        case MTP_OBJ_PROP_Parental_Rating:
            res = "OBJ_PROP_Parental_Rating";
            break;
        case MTP_OBJ_PROP_Meta_Genre:
            res = "OBJ_PROP_Meta_Genre";
            break;
        case MTP_OBJ_PROP_Composer:
            res = "OBJ_PROP_Composer";
            break;
        case MTP_OBJ_PROP_Effective_Rating:
            res = "OBJ_PROP_Effective_Rating";
            break;
        case MTP_OBJ_PROP_Subtitle:
            res = "OBJ_PROP_Subtitle";
            break;
        case MTP_OBJ_PROP_Original_Release_Date:
            res = "OBJ_PROP_Original_Release_Date";
            break;
        case MTP_OBJ_PROP_Album_Name:
            res = "OBJ_PROP_Album_Name";
            break;
        case MTP_OBJ_PROP_Album_Artist:
            res = "OBJ_PROP_Album_Artist";
            break;
        case MTP_OBJ_PROP_Mood:
            res = "OBJ_PROP_Mood";
            break;
        case MTP_OBJ_PROP_DRM_Status:
            res = "OBJ_PROP_DRM_Status";
            break;
        case MTP_OBJ_PROP_Sub_Description:
            res = "OBJ_PROP_Sub_Description";
            break;
        case MTP_OBJ_PROP_Is_Cropped:
            res = "OBJ_PROP_Is_Cropped";
            break;
        case MTP_OBJ_PROP_Is_Colour_Corrected:
            res = "OBJ_PROP_Is_Colour_Corrected";
            break;
        case MTP_OBJ_PROP_Exposure_Time:
            res = "OBJ_PROP_Exposure_Time";
            break;
        case MTP_OBJ_PROP_Exposure_Index:
            res = "OBJ_PROP_Exposure_Index";
            break;
        case MTP_OBJ_PROP_Display_Name:
            res = "OBJ_PROP_Display_Name";
            break;
        case MTP_OBJ_PROP_Body_Text:
            res = "OBJ_PROP_Body_Text";
            break;
        case MTP_OBJ_PROP_Subject:
            res = "OBJ_PROP_Subject";
            break;
        case MTP_OBJ_PROP_Priority:
            res = "OBJ_PROP_Priority";
            break;
        case MTP_OBJ_PROP_Given_Name:
            res = "OBJ_PROP_Given_Name";
            break;
        case MTP_OBJ_PROP_Middle_Names:
            res = "OBJ_PROP_Middle_Names";
            break;
        case MTP_OBJ_PROP_Family_Name:
            res = "OBJ_PROP_Family_Name";
            break;
        case MTP_OBJ_PROP_Prefix:
            res = "OBJ_PROP_Prefix";
            break;
        case MTP_OBJ_PROP_Suffix:
            res = "OBJ_PROP_Suffix";
            break;
        case MTP_OBJ_PROP_Phonetic_Given_Name:
            res = "OBJ_PROP_Phonetic_Given_Name";
            break;
        case MTP_OBJ_PROP_Phonetic_Family_Name:
            res = "OBJ_PROP_Phonetic_Family_Name";
            break;
        case MTP_OBJ_PROP_Email_Primary:
            res = "OBJ_PROP_Email_Primary";
            break;
        case MTP_OBJ_PROP_Email_Personal1:
            res = "OBJ_PROP_Email_Personal1";
            break;
        case MTP_OBJ_PROP_Email_Personal2:
            res = "OBJ_PROP_Email_Personal2";
            break;
        case MTP_OBJ_PROP_Email_Business1:
            res = "OBJ_PROP_Email_Business1";
            break;
        case MTP_OBJ_PROP_Email_Business2:
            res = "OBJ_PROP_Email_Business2";
            break;
        case MTP_OBJ_PROP_Email_Others:
            res = "OBJ_PROP_Email_Others";
            break;
        case MTP_OBJ_PROP_Phone_Nbr_Primary:
            res = "OBJ_PROP_Phone_Nbr_Primary";
            break;
        case MTP_OBJ_PROP_Phone_Nbr_Personal:
            res = "OBJ_PROP_Phone_Nbr_Personal";
            break;
        case MTP_OBJ_PROP_Phone_Nbr_Personal2:
            res = "OBJ_PROP_Phone_Nbr_Personal2";
            break;
        case MTP_OBJ_PROP_Phone_Nbr_Business:
            res = "OBJ_PROP_Phone_Nbr_Business";
            break;
        case MTP_OBJ_PROP_Phone_Nbr_Business2:
            res = "OBJ_PROP_Phone_Nbr_Business2";
            break;
        case MTP_OBJ_PROP_Phone_Nbr_Mobile:
            res = "OBJ_PROP_Phone_Nbr_Mobile";
            break;
        case MTP_OBJ_PROP_Phone_Nbr_Mobile2:
            res = "OBJ_PROP_Phone_Nbr_Mobile2";
            break;
        case MTP_OBJ_PROP_Fax_Nbr_Primary:
            res = "OBJ_PROP_Fax_Nbr_Primary";
            break;
        case MTP_OBJ_PROP_Fax_Nbr_Personal:
            res = "OBJ_PROP_Fax_Nbr_Personal";
            break;
        case MTP_OBJ_PROP_Fax_Nbr_Business:
            res = "OBJ_PROP_Fax_Nbr_Business";
            break;
        case MTP_OBJ_PROP_Pager_Nbr:
            res = "OBJ_PROP_Pager_Nbr";
            break;
        case MTP_OBJ_PROP_Phone_Nbr_Others:
            res = "OBJ_PROP_Phone_Nbr_Others";
            break;
        case MTP_OBJ_PROP_Primary_Web_Addr:
            res = "OBJ_PROP_Primary_Web_Addr";
            break;
        case MTP_OBJ_PROP_Personal_Web_Addr:
            res = "OBJ_PROP_Personal_Web_Addr";
            break;
        case MTP_OBJ_PROP_Business_Web_Addr:
            res = "OBJ_PROP_Business_Web_Addr";
            break;
        case MTP_OBJ_PROP_Instant_Messenger_Addr:
            res = "OBJ_PROP_Instant_Messenger_Addr";
            break;
        case MTP_OBJ_PROP_Instant_Messenger_Addr2:
            res = "OBJ_PROP_Instant_Messenger_Addr2";
            break;
        case MTP_OBJ_PROP_Instant_Messenger_Addr3:
            res = "OBJ_PROP_Instant_Messenger_Addr3";
            break;
        case MTP_OBJ_PROP_Post_Addr_Personal_Full:
            res = "OBJ_PROP_Post_Addr_Personal_Full";
            break;
        case MTP_OBJ_PROP_Post_Addr_Personal_Line1:
            res = "OBJ_PROP_Post_Addr_Personal_Line1";
            break;
        case MTP_OBJ_PROP_Post_Addr_Personal_Line2:
            res = "OBJ_PROP_Post_Addr_Personal_Line2";
            break;
        case MTP_OBJ_PROP_Post_Addr_Personal_City:
            res = "OBJ_PROP_Post_Addr_Personal_City";
            break;
        case MTP_OBJ_PROP_Post_Addr_Personal_Region:
            res = "OBJ_PROP_Post_Addr_Personal_Region";
            break;
        case MTP_OBJ_PROP_Post_Addr_Personal_Post_Code:
            res = "OBJ_PROP_Post_Addr_Personal_Post_Code";
            break;
        case MTP_OBJ_PROP_Post_Addr_Personal_Country:
            res = "OBJ_PROP_Post_Addr_Personal_Country";
            break;
        case MTP_OBJ_PROP_Post_Addr_Business_Full:
            res = "OBJ_PROP_Post_Addr_Business_Full";
            break;
        case MTP_OBJ_PROP_Post_Addr_Business_Line1:
            res = "OBJ_PROP_Post_Addr_Business_Line1";
            break;
        case MTP_OBJ_PROP_Post_Addr_Business_Line2:
            res = "OBJ_PROP_Post_Addr_Business_Line2";
            break;
        case MTP_OBJ_PROP_Post_Addr_Business_City:
            res = "OBJ_PROP_Post_Addr_Business_City";
            break;
        case MTP_OBJ_PROP_Post_Addr_Business_Region:
            res = "OBJ_PROP_Post_Addr_Business_Region";
            break;
        case MTP_OBJ_PROP_Post_Addr_Business_Post_Code:
            res = "OBJ_PROP_Post_Addr_Business_Post_Code";
            break;
        case MTP_OBJ_PROP_Post_Addr_Business_Country:
            res = "OBJ_PROP_Post_Addr_Business_Country";
            break;
        case MTP_OBJ_PROP_Post_Addr_Other_Full:
            res = "OBJ_PROP_Post_Addr_Other_Full";
            break;
        case MTP_OBJ_PROP_Post_Addr_Other_Line1:
            res = "OBJ_PROP_Post_Addr_Other_Line1";
            break;
        case MTP_OBJ_PROP_Post_Addr_Other_Line2:
            res = "OBJ_PROP_Post_Addr_Other_Line2";
            break;
        case MTP_OBJ_PROP_Post_Addr_Other_City:
            res = "OBJ_PROP_Post_Addr_Other_City";
            break;
        case MTP_OBJ_PROP_Post_Addr_Other_Region:
            res = "OBJ_PROP_Post_Addr_Other_Region";
            break;
        case MTP_OBJ_PROP_Post_Addr_Other_Post_Code:
            res = "OBJ_PROP_Post_Addr_Other_Post_Code";
            break;
        case MTP_OBJ_PROP_Post_Addr_Other_Country:
            res = "OBJ_PROP_Post_Addr_Other_Country";
            break;
        case MTP_OBJ_PROP_Organization_Name:
            res = "OBJ_PROP_Organization_Name";
            break;
        case MTP_OBJ_PROP_Phonetic_Organization_Name:
            res = "OBJ_PROP_Phonetic_Organization_Name";
            break;
        case MTP_OBJ_PROP_Role:
            res = "OBJ_PROP_Role";
            break;
        case MTP_OBJ_PROP_Birthdate:
            res = "OBJ_PROP_Birthdate";
            break;
        case MTP_OBJ_PROP_Message_To:
            res = "OBJ_PROP_Message_To";
            break;
        case MTP_OBJ_PROP_Message_CC:
            res = "OBJ_PROP_Message_CC";
            break;
        case MTP_OBJ_PROP_Message_BCC:
            res = "OBJ_PROP_Message_BCC";
            break;
        case MTP_OBJ_PROP_Message_Read:
            res = "OBJ_PROP_Message_Read";
            break;
        case MTP_OBJ_PROP_Message_Received_Time:
            res = "OBJ_PROP_Message_Received_Time";
            break;
        case MTP_OBJ_PROP_Message_Sender:
            res = "OBJ_PROP_Message_Sender";
            break;
        case MTP_OBJ_PROP_Activity_Begin_Time:
            res = "OBJ_PROP_Activity_Begin_Time";
            break;
        case MTP_OBJ_PROP_Activity_End_Time:
            res = "OBJ_PROP_Activity_End_Time";
            break;
        case MTP_OBJ_PROP_Activity_Location:
            res = "OBJ_PROP_Activity_Location";
            break;
        case MTP_OBJ_PROP_Activity_Required_Attendees:
            res = "OBJ_PROP_Activity_Required_Attendees";
            break;
        case MTP_OBJ_PROP_Activity_Optional_Attendees:
            res = "OBJ_PROP_Activity_Optional_Attendees";
            break;
        case MTP_OBJ_PROP_Activity_Resources:
            res = "OBJ_PROP_Activity_Resources";
            break;
        case MTP_OBJ_PROP_Activity_Accepted:
            res = "OBJ_PROP_Activity_Accepted";
            break;
        case MTP_OBJ_PROP_Activity_Tentative:
            res = "OBJ_PROP_Activity_Tentative";
            break;
        case MTP_OBJ_PROP_Activity_Declined:
            res = "OBJ_PROP_Activity_Declined";
            break;
        case MTP_OBJ_PROP_Activity_Reminder_Time:
            res = "OBJ_PROP_Activity_Reminder_Time";
            break;
        case MTP_OBJ_PROP_Activity_Owner:
            res = "OBJ_PROP_Activity_Owner";
            break;
        case MTP_OBJ_PROP_Activity_Status:
            res = "OBJ_PROP_Activity_Status";
            break;
        case MTP_OBJ_PROP_Media_GUID:
            res = "OBJ_PROP_Media_GUID";
            break;
        case MTP_OBJ_PROP_Total_BitRate:
            res = "OBJ_PROP_Total_BitRate";
            break;
        case MTP_OBJ_PROP_Bitrate_Type:
            res = "OBJ_PROP_Bitrate_Type";
            break;
        case MTP_OBJ_PROP_Sample_Rate:
            res = "OBJ_PROP_Sample_Rate";
            break;
        case MTP_OBJ_PROP_Nbr_Of_Channels:
            res = "OBJ_PROP_Nbr_Of_Channels";
            break;
        case MTP_OBJ_PROP_Audio_BitDepth:
            res = "OBJ_PROP_Audio_BitDepth";
            break;
        case MTP_OBJ_PROP_Scan_Type:
            res = "OBJ_PROP_Scan_Type";
            break;
        case MTP_OBJ_PROP_Audio_WAVE_Codec:
            res = "OBJ_PROP_Audio_WAVE_Codec";
            break;
        case MTP_OBJ_PROP_Audio_BitRate:
            res = "OBJ_PROP_Audio_BitRate";
            break;
        case MTP_OBJ_PROP_Video_FourCC_Codec:
            res = "OBJ_PROP_Video_FourCC_Codec";
            break;
        case MTP_OBJ_PROP_Video_BitRate:
            res = "OBJ_PROP_Video_BitRate";
            break;
        case MTP_OBJ_PROP_Frames_Per_Thousand_Secs:
            res = "OBJ_PROP_Frames_Per_Thousand_Secs";
            break;
        case MTP_OBJ_PROP_KeyFrame_Distance:
            res = "OBJ_PROP_KeyFrame_Distance";
            break;
        case MTP_OBJ_PROP_Buffer_Size:
            res = "OBJ_PROP_Buffer_Size";
            break;
        case MTP_OBJ_PROP_Encoding_Quality:
            res = "OBJ_PROP_Encoding_Quality";
            break;
        case MTP_OBJ_PROP_Encoding_Profile:
            res = "OBJ_PROP_Encoding_Profile";
            break;
        }

        if ( !res ) {
            static char buf[32];
            snprintf(buf, sizeof buf, "<unknown_%04x>", val);
            res = buf;
        }

        return res;
    }
};
