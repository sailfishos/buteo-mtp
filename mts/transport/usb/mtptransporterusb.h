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

#ifndef MTPTTRANSPORTERUSB_H
#define MTPTTRANSPORTERUSB_H

#include "mtptransporter.h"
#include "mtpfsdriver.h"

class QSocketNotifier;

/// \brief The MTPTransporterUSB class implements the transport layer for USB over MTP.
///
/// The MTPTransporterUSB class implements the transport layer for MTP over USB protocol.
/// The class uses the MTP USB driver file /dev/ttyGS* to perform the actual I/O
namespace meegomtp1dot0
{
class MTPTransporterUSB : public MTPTransporter
{
    Q_OBJECT
    public:
        /// The MTPTransporterUSB constructor
        MTPTransporterUSB();

        /// The MTPTransporterUSB destructor
        ~MTPTransporterUSB();

        /// Sends data (an MTP data container) to the initiator. The function must be synchronous.
        /// \param data [in] The buffer of data to be sent. The buffer is assumed to be allocated by the caller, and will not be modified.
        /// \param len [in] The length of the data buffer in bytes.
        /// \param sendZeroPacket [in] If true, the transport layer must send a zero length MTP packet (in case where the data length was a multiple
        /// to the transport's max packet size).
        /// \return Returns true if write to the USB FD was a success, else false.
        bool sendData(const quint8* data, quint32 len, bool isLastPacket = true);

        /// Sends data (an MTP event container) to the initiator. The function must be synchronous.
        /// \param data [in] The buffer of data to be sent. The buffer is assumed to be allocated by the caller, and will not be modified.
        /// \param len [in] The length of the data buffer in bytes.
        /// \param sendZeroPacket [in] If true, the transport layer must send a zero length MTP packet (in case where the data length was a multiple
        /// to the transport's max packet size).
        /// \return Returns true if write to the USB FD was a success, else false.
        bool sendEvent(const quint8* data, quint32 len, bool isLastPacket = true);

        /// Make necessary connetions, connect signals, etc.
        bool activate();

        /// Deactivate the transport
        bool deactivate();

        /// Flush out all data
        bool flushData();

        /// Reset the transport to a default state.
        void reset();

        /// Enable the transport to read/write.
        void enableRW();

        /// Disable the transport from reading/writing.
        void disableRW();

        /// Suspend the transport channel
        void suspend();

        /// Resume the suspended channel
        void resume();

    private:

        /// Opens and sets up the USB driver file descriptor
        /// \param fileName [in] The path of the USB device driver file
        /// \return Returns the file descriptor (> 0) if success, else -1
        int openUsbFd(const char* fileName);

        /// Internal function to send data and event packets
        bool sendDataOrEvent(const quint8* data, quint32 dataLen, bool isEvent, bool sendZeroPacket);
        /// Internal function that does the actual data write to the USB FD
        /// \param data [in] The data to be written
        /// \param len [in] The length, in bytes, of the data buffer
        /// \return Returns true if data write succeeded, else false
        bool sendDataInternal(const quint8* data, quint32 len);

        /// Internal function that does the actual data write to the USB FD
        /// \param data [in] The data to be written
        /// \param len [in] The length, in bytes, of the data buffer
        /// \return Returns true if data write succeeded, else false
        bool sendEventInternal(const quint8* data, quint32 len);

        /// Retrieves the max data packet size for the USB driver
        /// \return Returns the max data size in bytes
        quint32 getMaxDataPacketSize();

        /// Retrieves the max event packet size for the USB driver
        /// \return Returns the max event size in bytes
        quint32 getMaxEventPacketSize();

        /// Private function to process data received from the USB driver
        void processReceivedData(quint8* data, quint32 dataLen);
    
        enum IOState{
            ACTIVE,
            EXCEPTION,
            SUSPENDED
        };
        
        const char*             MTP_DEVICE_PORT; ///< The path of the device driver file
        
        IOState                 m_ioState; ///< The current state of the IO device

        int                     m_usbFd; ///< The USB TTY file descriptor
        
        QSocketNotifier*        m_readSocket; ///< The socket notifier for data read/written to the USB fd

        QSocketNotifier*        m_excpSocket; ///< The socket notifier for exception, errors and hangups on the USB fd

        QSocketNotifier*        m_ctrlSocket;

        quint32                  m_maxDataSize; ///< The maximum size, in bytes, of the data packet size of the USB layer
        
        quint32                  m_maxEventSize; ///< The maximum size, in bytes, of the event packet size of the USB layer

        quint64                  m_containerReadLen; ///< The data, in bytes remaining to be received for a data packet

        MTPFSDriver              m_driver;
    public Q_SLOTS:
        /// The usb transporter catches the device ok status signal from the responder and informs the driver about the same.
        void sendDeviceOK();

        /// The usb transporter catches the device busy status signal from the responder and informs the driver about the same.
        void sendDeviceBusy();

        /// The usb transporter catches the tx cancelled signal from the responder and informs the driver about the same.
        void sendDeviceTxCancelled();

	/// This slot listens for usb ptp class requests from the driver.
        void handleHighPriorityData();

        void handleInFd(int);
        void handleOutFd(int);
        void handleIntrFd(int);

    private Q_SLOTS:

        // The slot handles incoming data on the USB fd that was alrady read
        void handleDataRead(char*, int);

        /// The slot handles incoming data on the USB fd
        void handleRead();

        /// This slot handles exceptions (considered hangups for now) on the USB fd
        void handleHangup();

};
}
#endif

