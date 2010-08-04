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

#ifndef MTPTRANSPORTER_H
#define MTPTRANSPORTER_H

#include <QObject>


namespace meegomtp1dot0
{
class MTPResponder;
}

/// \brief The MTPTransporter class defines the interface required for an MTP transport layer
///
/// The MTPTransporter class is an abstract class that acts as an interface definition for an MTP
/// transport layer. The MTPTransporter interface can be used by real transport providers. For example
/// USB, BT, IP as needed. The type of transport to create can be determined using a configuration parameter
/// while creating the MTPResponder object.
namespace meegomtp1dot0
{
class MTPTransporter : public QObject
{
    Q_OBJECT
    public:
        /// The MTPTransporter constructor
        MTPTransporter(){}

        /// The MTPTransporter destructor
        ~MTPTransporter(){}

        /// Sends data (an MTP data container) to the initiator. The function must be synchronous.
        /// \param data [in] The buffer of data to be sent. The buffer is assumed to be allocated by the caller, and will not be modified.
        /// \param len [in] The length of the data buffer in bytes.
        /// \param sendZeroPacket [in] If true, the transport layer must send a zero length MTP packet (in case where the data length was a multiple
        /// to the transport's max packet size). Transports like BT do not require an zero length packet, and hence can ignore this parameter.
        /// \return Must return true if send was a success, else false.
        virtual bool sendData(const quint8* data, quint32 len, bool sendZeroPacket = true) = 0;

        /// Sends data (an MTP event container) to the initiator. The function must be synchronous.
        /// \param data [in] The buffer of data to be sent. The buffer is assumed to be allocated by the caller, and will not be modified.
        /// \param len [in] The length of the data buffer in bytes.
        /// \param sendZeroPacket [in] If true, the transport layer must send a zero length MTP packet (in case where the data length was a multiple
        /// to the transport's max packet size). Transports like BT do not require an zero length packet, and hence can ignore this parameter.
        /// \return Must return true if send was a success, else false.
        virtual bool sendEvent(const quint8* data, quint32 len, bool sendZeroPacket = true) = 0;

        /// Make necessary connections - could be opening an fd, or opening a BT channel for eg.
        virtual bool activate() = 0;

        /// Deactivate the transport.
        virtual bool deactivate() = 0;

        /// Reset the transport to a default state.
        virtual void reset() = 0;

        /// Disable the transport from reading/writing.
        virtual void disableRW() = 0;

        /// Enable the transport to read/write.
        virtual void enableRW() = 0;

        /// Flush out all data
        virtual bool flushData() = 0;

    Q_SIGNALS:
        /// The transporter must emit this signal when data is received from the initiator
        /// \param data [in] The data received from the underlying transport
        /// \param len [in] The length, in bytes, of the received data
        /// \param isFirstPacket [in] true if this data packet is the first one in the container, i.e, starts with the header
        /// \param isLastPacket [out] true if this is the last packet of the container
        void dataReceived(quint8* data, quint32 len, bool isFirstPacket, bool isLastPacket);

        /// The transporter must emit this signal when event data is received from the initiator
        /// TODO: Decide on the type of parameter and it's meaning.
        void eventReceived(); 

        /// The transport layer must emit this signal when it determines that the connection with the underlying hardware has been
        /// severed. For example, when the USB cable is plugged out with a USB transport.
        void cleanup();

        /// This signal is to emitted by the transporter to determine the size of the object for SendObject operations (> 4GB case)
        /// \param data [in] The data container (for the data phase of the send object operation). Assumed that the length
        /// \param size [out] The responder would return the object size of the SendObject operation in this parameter.
        void fetchObjectSize(const quint8* data, quint64* size);

        /// This signals is emitted by the transporter so that the responder can perform the necessary cleanup for a cancelled transaction.
	/// \param transactionId [in] not sure what this will be used for.
	void cancelTransaction();

        /// This signals is emitted by the transporter so that the responder can perform a device reset to a an idle/default state. 
	void deviceReset();

        /// This signal is emitted by the transporter to indicate a suspend
	void suspend();

        /// This signal is emitted by the transporter to resume the suspended session
	void resume();

    public Q_SLOTS:
        /// A transporter uses this slot to catch the ok state signal from responder and transports it to the host if required.
	virtual void sendDeviceOK() = 0;

        /// A transporter uses this slot to catch the busy state signal from responder and transports it to the host if required.
	virtual void sendDeviceBusy() = 0;

        /// Handle high priority requests from the underlying transport driver.
        virtual void handleHighPriorityData() = 0;
};
}

#endif

