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
#include "threadio.h"

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
        void processReceivedData();  // Helper function for handleDataReady()
        bool writeMtpDescriptors();  // configure the USB endpoints for functionfs
        bool writeMtpStrings();      // step 2 of functionfs configuration

        enum IOState{
            ACTIVE,
            EXCEPTION,
            SUSPENDED
        };

        enum ReaderBusyState {
            READER_FREE,
            READER_BUSY,
            READER_POSTPONED,
        };

        IOState                 m_ioState; ///< The current state of the IO device

        quint64                 m_containerReadLen; ///< The data, in bytes remaining to be received for a data packet
        int                     m_resetCount;   ///< Distinguishes new data from old

        int                     m_ctrlFd;
        int                     m_intrFd;
        int                     m_inFd;
        int                     m_outFd;

        ControlReaderThread     m_ctrl;         ///< Threaded IO for Control EP
        BulkReaderThread        m_bulkRead;     ///< Threaded Reader for Bulk Out EP
        ReaderBusyState         m_reader_busy;
        BulkWriterThread        m_bulkWrite;    ///< Threaded Writer for Bulk In EP
        bool                    m_writer_busy;
        InterruptWriterThread   m_intrWrite;    ///< Threaded Writer for Interrupt EP

    public Q_SLOTS:
        /// The usb transporter catches the device ok status signal from the responder and informs the driver about the same.
        void sendDeviceOK();

        /// The usb transporter catches the device busy status signal from the responder and informs the driver about the same.
        void sendDeviceBusy();

        /// The usb transporter catches the tx cancelled signal from the responder and informs the driver about the same.
        void sendDeviceTxCancelled();

    private Q_SLOTS:
        /// Close the Bulk and Interrupt devices
        void closeDevices();
        /// Open the Bulk and Interrupt devices
        void openDevices();

        /// Initialize the reader thread
        void startRead();
        /// Stop the reader thread
        void stopRead();

        // Handle incoming data from m_bulkRead
        void handleDataReady();

        /// Handle high priority requests from the underlying transport driver.
        void handleHighPriorityData();
};

}
#endif

