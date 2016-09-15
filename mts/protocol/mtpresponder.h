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

#ifndef MTPRESPONDER_H
#define MTPRESPONDER_H

#include <QObject>
#include <QHash>
#include <QList>

#include "mtptypes.h"

namespace meegomtp1dot0
{
class StorageFactory;
class MTPTransporter;
class MTPResponder;
class DeviceInfo;
class PropertyPod;
class ObjectPropertyCache;
class MTPExtensionManager;
class MTPTxContainer;
class MTPRxContainer;
typedef void (MTPResponder::*MTPCommandHandler)();
}


/// \brief MTPResponder is the main class of the MTP stack.
///
/// MTPResponder is the main class of the MTP stack. The class acts as a responder to an MTP initiator.
/// The is a singleton, whose instance should be created by the MTP server plugin.
/// The MTPResponder class creates inturn, instances of 1) StorageServer, 2) StorageChangeDB, 3) StorageRefDB
/// 4) Transport Module, 5) Device Info Provider. MTPResponder currently only supports a single active MTP session.
/// This class also takes care of handling all MTP operations.
namespace meegomtp1dot0
{
class MTPResponder : public QObject
{
    Q_OBJECT
#ifdef UT_ON
    friend class MTPResponder_test;
#endif
    public:
        /// Destructor for MTPResponder
        ~MTPResponder();

        /// Returns the instance to the MTPResponder class
        /// An instance must be created by the application, and deleted before the application exits
        static MTPResponder* instance();
        
        /// Sends an MTP container over the transport layer
        /// Returns false is a cancel transaction was received, this container won't be sent then.
        /// \param container [in] The container to be sent
        bool sendContainer(MTPTxContainer &container, bool isLastPacket = true);
        
        /// Initiliazes the transport mechanism over which MTP traffic is exchange.
	/// Call this method after construction the responder.
	bool initTransport(TransportType transport);

        /// Initiliazes the storages.
	/// Call this method after construction the responder.
	bool initStorages();
  
        /// Suspends the MTP session at the transport layer
        void suspend();

        /// Resumes the suspended MTP session
        void resume();

    public slots:
        void dispatchEvent(MTPEventCode event, const QVector<quint32> &params);
        void onStorageReady();

    Q_SIGNALS:
        /// This signal is emitted by the mtpresponder to indicate that the responder is an OK state. 
	void deviceStatusOK();

        /// This signal is emitted by the mtpresponder to indicate that the responder is a busy state. 
	void deviceStatusBusy();

        /// This signal is emitted by the mtpresponder to indicate that a transaction was cancelled.
	void deviceStatusTxCancelled();

        /// This signal is emitted based on MTP_OP_OpenSession / MTP_OP_CloseSession handling
        void sessionOpenChanged(bool isOpen);

        /// Emitted when mtp command is received
        void commandPending();

        /// Emitted when mtp command has been processed
        void commandFinished();

    private Q_SLOTS:
        /// This slot acts as a callback to the transport layer, used to receive MTP containers
        /// \param data [in] The data received from the transport layer
        /// \param len [in] The length of the data received, in bytes
        /// \param isFirstPacket [in] If true, the packet is the first packet in a phase.
        /// \param isLastPacket [in] If true, the packet is the last packet in a phase.
        void receiveContainer(quint8* data, quint32 len, bool isFirstPacket, bool isLastPacket);
        
        /// This slot acts as a callback for handling MTP events from the transport layer
        void receiveEvent();
        
        /// This slot acts as a callback to handle close session from the transport layer
        void closeSession();

        /// This slot acts as a callback from the transporter to fetch the object size corresponding to a SendObject operation
        /// \param data [in] The data container (first packet). Used to determine the operation code and transaction ID for verification
        /// \param objSize [out] The object size for the SendObject operation is returned in this
        void fetchObjectSize(const quint8* data, quint64* objSize);

        /// This slot handles a cancel transaction request.
        void handleCancelTransaction();

        /// This slot handles a device reset request.
        void handleDeviceReset();

        /// This slot provides information about transport events ( cancel as of now ) to interested parties.
        void processTransportEvents( bool &txCancelled );

        void handleSuspend();
        void handleResume();

        void onDevicePropertyChanged(MTPDevPropertyCode property);

    private:
        Q_DISABLE_COPY(MTPResponder)
        static MTPResponder*                            m_instance;         ///< Instance pointer
        QHash<MTPOperationCode, MTPCommandHandler>      m_opCodeTable;      ///< Hash table storing command handler functions
        StorageFactory*                                 m_storageServer;    ///< Pointer to the object storage server
        MTPTransporter*                                 m_transporter;      ///< Pointer to the transport layer
        DeviceInfo*                                     m_devInfoProvider;  ///< Pointer to the device info class
        PropertyPod*                                    m_propertyPod;      ///< Pointer to the MTP properties utility class
        MTPExtensionManager*                            m_extensionManager; ///< Pointer to the MTP extension manager class
        ObjHandle                                       m_copiedObjHandle;  ///< Stored in case the copied object needs to be deleted due to cancel tx
        bool                                            m_containerToBeResent;
        bool                                            m_isLastPacket;
        quint8                                          *m_resendBuffer;
        quint32                                         m_resendBufferSize;
        QByteArray                                      m_storageWaitData;  ///< holding area for data arriving during WAIT_STORAGE
        bool                                            m_storageWaitDataComplete;  ///< m_storageWaitData holds a whole container

        enum ResponderState
        {
            RESPONDER_IDLE = 0,                                             ///< Responder is idle, meaning it is ready to receive a new request
            RESPONDER_WAIT_DATA = 1,                                        ///< Responder has received a request, and is now waiting for the data phase
            RESPONDER_WAIT_RESP = 2,                                        ///< Responder is waiting for the response phase
            RESPONDER_TX_CANCEL = 3,                                        ///< A transaction got cancelled
            RESPONDER_SUSPEND = 4,                                          ///< A suspended session
            RESPONDER_WAIT_STORAGE = 5,                                     ///< Responder has received a request, but cannot handle it before storage is ready
        } m_state_accessor_only;                                            ///< Responder state

        const char *responderStateName(MTPResponder::ResponderState state);
        ResponderState getResponderState(void);
        void setResponderState(ResponderState state);


        ResponderState                                  m_prevState;

        struct ObjPropListInfo
        {
            quint32 noOfElements;                                           ///< Number of "elements" in the property list
            quint32 storageId;                                              ///< The storage ID sent in the sendObjectPropList operation
            quint64 objectSize;                                             ///< The object size, in bytes.
            quint64 objectCurrSize;                                         ///< The incremental current size, in bytes, of the object (for segmented sendObject)
            ObjHandle objectHandle;                                         ///< The handle of the object.
            ObjHandle parentHandle;                                         ///< The handle of the object's parent.
            MTPObjFormatCode objectFormatCode;                              ///< The object's MTP format code.
            struct ObjectPropList
            {
                ObjHandle objectHandle;                                     ///< The object handle for individual "elements"
                MTPObjPropertyCode objectPropCode;                          ///< The property code in the "element"
                MTPDataType datatype;                                       ///< The MTP datatype corresponding to the property code
                QVariant *value;                                            ///< The value of the property.
                
                ObjectPropList() : objectHandle(0), objectPropCode(0), datatype(0), value(0)
                {
                }
            }*objPropList;                                                  ///< An array of "elements" contained within a SendObjectPropList request
            
            ObjPropListInfo() : noOfElements(0), storageId(0), objectSize(0), objectCurrSize(0),
            objectHandle(0), parentHandle(0), objectFormatCode(MTP_OBF_FORMAT_Undefined), objPropList(0)
            {
            }
        }*m_objPropListInfo;                                                ///< This structure stores the information from SendObjectPropList for a future SendObject operation

        struct MTPSendObjectSequence
        {
            MTPObjectInfo *objInfo;                                         ///< Stores the object info (only valid for SendObjectInfo operation)
            ObjHandle objHandle;                                            ///< Stores the ObjectHandle associated to a SendObject operation
            quint32 sendObjBytesWritten;                                    ///< Bytes written to storage during SendObject data phase
            
            MTPSendObjectSequence(): objInfo(0), objHandle(0), sendObjBytesWritten(0)
            {
            }
        }*m_sendObjectSequencePtr;                                          ///< This structure stores information from a SendObjectInfo operation for a future SendObject operation

        struct MTPTransactionSequence
        {
            quint32                             mtpSessionId;               ///< Current MTP session ID
            MTPResponseCode                     mtpResp;                    ///< MTP response code for the current operation
            MTPRxContainer                        *reqContainer;              ///< The container that holds the request phase
            MTPRxContainer                        *dataContainer;             ///< The container that holds the data phase (I->R)
            
            MTPTransactionSequence() : mtpSessionId(0),
            mtpResp(MTP_RESP_OK), reqContainer(0), dataContainer(0)
            {
            }
        }*m_transactionSequence;                                            ///< Stores information related to the currently ongoing transaction

        struct SendObjectSegment
        {
            quint64 totalDataLen;                                           ///< The total object size
            quint32 payloadLen;                                             ///< The length of the current segment
            quint32 offset;                                                 ///< Offset into the object (current segment)
            quint32 bytesSent;                                              ///< Bytes of the object transferred so far
            ObjHandle objHandle;                                            ///< The object handle
            bool segmentationStarted;                                       ///< Flag to indicate state of segmentation
            bool headerSent;                                                ///< Flag to indicate if the MTP header has been sent
            bool sendResp;                                                  ///< Flag to indicate if MTP response phase can begin
            
            SendObjectSegment() : totalDataLen(0), payloadLen(0), offset(0), bytesSent(0), 
            objHandle(0), segmentationStarted(false), headerSent(false), sendResp(0)
            {
            }
        }m_segmentedSender;                                                 ///< This structure holds data for segmented getObject operations

        /// Constructor for MTPResponder
        /// \param transport [in] The transport type to be used by the responder
        MTPResponder();

        /// Handles all MTP operations. Delegates the actual handling to the function retrieved from m_opCodeTable
        void commandHandler();

        /// Handles all MTP data phases. Performs operations on the received data and sends response back to the initiator
        /// \param data [in] Pointer to the incoming data
        /// \param dataLen [in] Length, in bytes, of the incoming data
        /// \param isFirstPacket [in] If true, this is the first packet of the phase
        /// \param isLastPacket [in] If true, this is the last packet of the phase
        void dataHandler(quint8* data, quint32 dataLen, bool isFirstPacket, bool isLastPacket);
        
        /// Handler MTP initiator events (Sent as MTP operations)
        void eventHandler();

        /// Handles Device info MTP operation (request phase)
        void getDeviceInfoReq();
        
        /// Handles OpenSession MTP operation (request phase)
        void openSessionReq();
         
        /// Handles CloseSession MTP operation (request phase)
        void closeSessionReq();
        
        /// Handles GetStorageID MTP operation (request phase)
        void getStorageIDReq();
        
        /// Handles GetStorageInfo MTP operation (request phase)
        /// \param numParams [in] The number of MTP opration parameters
        /// \param operationPtr [in] The pointer to the MTP container
        void getStorageInfoReq();
        
        /// Handles GetNumOfObjects MTP operation (request phase)
        void getNumObjectsReq();
        
        /// Handles Device info MTP operation (request phase)
        void getObjectHandlesReq();
        
        /// Handles Device info MTP operation (request phase)
        void getObjectInfoReq();
        
        /// Handles Device info MTP operation (request phase)
        void getObjectReq();
        
        /// Handles Device info MTP operation (request phase)
        void getThumbReq();
        
        /// Handles Device info MTP operation (request phase)
        void deleteObjectReq();
        
        /// Handles Device info MTP operation (request phase)
        void sendObjectInfoReq();
        
        /// Handles Device info MTP operation (request phase)
        void sendObjectReq();
        
        /// Handles Device info MTP operation (request phase)
        void getPartialObjectReq();
        
        /// Handles Device info MTP operation (request phase)
        void setObjectProtectionReq();
        
        /// Handles Device info MTP operation (request phase)
        void getDevicePropDescReq();
        
        /// Handles Device info MTP operation (request phase)
        void getDevicePropValueReq();
        
        /// Handles Device info MTP operation (request phase)
        void setDevicePropValueReq();
        
        /// Handles Device info MTP operation (request phase)
        void resetDevicePropValueReq();
        
        /// Handles Device info MTP operation (request phase)
        void moveObjectReq();
        
        /// Handles Device info MTP operation (request phase)
        void copyObjectReq();
        
        /// Handles Device info MTP operation (request phase)
        void getObjPropsSupportedReq();
        
        /// Handles Device info MTP operation (request phase)
        void getObjPropDescReq();
        
        /// Handles Device info MTP operation (request phase)
        void getObjPropValueReq();
        
        /// Handles Device info MTP operation (request phase)
        void setObjPropValueReq();
        
        /// Handles Device info MTP operation (request phase)
        void getObjectPropListReq();
        
        /// Handles Device info MTP operation (request phase)
        void setObjectPropListReq();
        
        /// Handles Device info MTP operation (request phase)
        void getInterdependentPropDescReq();
        
        /// Handles Device info MTP operation (request phase)
        void sendObjectPropListReq();
        
        /// Handles Device info MTP operation (request phase)
        void getObjReferencesReq();
        
        /// Handles Device info MTP operation (request phase)
        void setObjReferencesReq();
        
        /// Handles Device info MTP operation (request phase)
        void skipReq();
        
        /// Handles SendObjectInfo MTP operation (data pahase)
        /// \param recvContainer
        void sendObjectInfoData();
        
        /// Handles SendObject MTP operation (data phase)
        /// \param data [in] The object data (or the container segment data)
        /// \param dataLen [in] The length of the data, in bytes
        /// \param isFirstPacket [in] true if this is the first segment in the
        /// data phase
        /// \param isLastPacket [in] true if this is the last segment in the
        /// data phase 
        void sendObjectData(quint8* data, quint32 dataLen, bool isFirstPacket, bool isLastPacket);
        
        /// Handles SendObjectInfo MTP operation (data pahase)
        /// \param recvContainer
        void setObjectPropListData();
        
        /// Handles SendObjectInfo MTP operation (data pahase)
        /// \param recvContainer
        void sendObjectPropListData();
        
        /// Handles SendObjectInfo MTP operation (data pahase)
        /// \param recvContainer
        void setDevicePropValueData();
        
        /// Handles SendObjectInfo MTP operation (data pahase)
        /// \param recvContainer
        void setObjPropValueData();
        
        /// Handles SendObjectInfo MTP operation (data pahase)
        /// \param recvContainer
        void setObjReferencesData();

        /// Populates the hash map with the right command handlers
        void createCommandHandler();

        /// Deletes the stored transaction data from the request phase (to be called in case of error, or completion of a command)
        void deleteStoredRequest();

        /// Performs preliminary checks before processing a command
        MTPResponseCode preCheck(quint32 sessionID, quint32 transactionID);

        /// Use this to free the property info list
        void freeObjproplistInfo();
        
        /// Checks segments of the sendObject data phase
        MTPResponseCode sendObjectCheck(ObjHandle handle, const quint32 dataLen, bool isLastPacket, MTPResponseCode code);

        /// Serializes a list of property values into an MTP container.
        ///
        /// Each value is converted into an element quadruple as per MTP 1.1
        /// specification E.2.1.1 ObjectPropList Dataset Table. The method skips
        /// any invalid QVariant values (their presence in the result set
        /// usually means the object was queried for that property but doesn't
        /// have it defined).
        ///
        /// \param handle [in] handle of an object the properties belong to.
        /// \param propValList [in] list of property descriptions and values.
        /// \param dataContainer [out] container into which the data will be
        ///                      serialized.
        ///
        /// \return the number of serialized properties, i.e. excluding invalid
        /// QVariants.
        quint32 serializePropList(ObjHandle handle,
                QList<MTPObjPropDescVal> &propValList, MTPTxContainer &dataContainer);

        /// Sends a large data packet in segments of max data packet size
        void sendObjectSegmented();

        /// Constructs and sends a standard MTP response container
        /// It uses the transaction id from m_transactionSequence->reqContainer
        bool sendResponse(MTPResponseCode code);
        bool sendResponse(MTPResponseCode code, quint32 param1);

        /// Handles extended MTP operations. data and dataLen must be 0 if the operation has no data phase
        bool handleExtendedOperation();

        /// Returns true if the operation has an I->R data phase
        bool hasDataPhase(MTPOperationCode code);

        /// Returns true if the operation needs to access m_storageServer
        bool needsStorageReady(MTPOperationCode code);

#if 0
        /// Unregister all the types we have registered with QMetaType. We noticed that not doing so can result in crashes:
        /// Once these types are registered for the first time, next time onwards Qt has cached the info like pointers to
        /// constructors and destructors for these types, but if the mtp plug-in is unloaded, the cached info is no longer valid
        /// when the plug-in is loaded the next time!! Nasty!!! So unregister these types.
        void unregisterMetaTypes();
#endif
};
}

/* Helper functions for making human readable diagnostic logging */
extern "C"
{
    const char *mtp_format_category_repr(int val);
    const char *mtp_file_system_type_repr(int val);
    const char *mtp_association_type_repr(int val);
    const char *mtp_storage_access_repr(int val);
    const char *mtp_container_type_repr(int val);
    const char *mtp_obj_prop_form_repr(int val);
    const char *mtp_storage_type_repr(int val);
    const char *mtp_bitrate_type_repr(int val);
    const char *mtp_protection_repr(int val);
    const char *mtp_form_flag_repr(int val);
    const char *mtp_data_type_repr(int val);
    const char *mtp_ch_conf_repr(int val);
    const char *mtp_code_repr(int val);
};
#endif

