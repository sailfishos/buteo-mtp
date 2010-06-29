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

#ifndef MTP_CONTAINER_H
#define MTP_CONTAINER_H

#include <QByteArray>
#include <QVector>
#include "mtptypes.h"

class QString;

/// \brief The MTPContainer class provides interfaces to serialize/de-serialize a byte stream as per the MTP specification
///
/// The protocol layer should use the methods exposed by this class to (de-)serilaize the dataset inside within the data phase
/// of an MTP operation. The class also provides utility inline functions to read/write data in little endian (regardless of the
/// processor architecture) off a byte stream. The size of it's internal buffer should be provided as an estimate in it's constructor,
/// or by using the setBufferSize() method. The class will automatically reallocate the data buffer as needed. When used as a serializer,
/// the class also allocates, automatically, extra memory to hold the MTP header.
namespace meegomtp1dot0
{
    class MTPContainer
    {
        protected:
            /// Constructor. This class serves as the base class for all other
            /// containers, and is not instantiable.
            MTPContainer();
            
            /// Destructor
            virtual ~MTPContainer();
        public:
            /// Returns the size (used number of bytes) of the buffer
            /// \return Returns the number of used bytes in the buffer (Inclusive of
            /// the payload)
            quint32 bufferSize() const;

            /// Returns the total number of bytes in the buffer
            /// \return Returns the total capacity of the buffer
            quint32 bufferCapacity() const;

            /// Gets the length of the container( including payload length )
            /// \return containerLength
            quint32 containerLength() const;

            /// Gets the type of the container.
            /// \return containerType
            quint16 containerType() const;

            /// Gets the code.
            /// \return code
            quint16 code() const;

            /// Gets the transaction id.
            /// \return transaction id.
            quint32 transactionId() const;

            /// Gets a pointer to the payload.
            /// \return payload pointer.
            quint8* payload() const;

            /// Returns the parameters for request containers
            /// \param params [out] The request parameters. The vector is cleared,
            /// so if the container is not a request container, the vector is an
            /// empty one. 
            void params(QVector<quint32> &params);

            /// Moves the internal offset forward/backward by the number of bytes provided.
            /// If there isn't enough capacity in the internal buffer, the offset
            /// remains unchanged. Useful if you wish to forward the offset after
            /// having manually altered the container buffer, or want to rewind to
            /// insert data at a specific offset in the container.
            /// \param pos [in] The number of bytes to forward the offset to.
            void seek(qint32 pos);

            /// Sets the container to be extra large (that is payload of > 4GB)
            /// \param isExtraLargeContainer [in] If true, the container is made
            /// extra large, else it is made a regular container.
            void setExtraLargeContainer(bool isExtraLargeContainer);

            /// Some static functions to get/set data in little-endian
            static quint8 getl8(const void *d); ///< Reads a byte at d in LE
            static quint16 getl16(const void *d); ///< Reads a 16 bit value at d in LE
            static quint32 getl32(const void *d); ///< Reads a 32 bit value at d in LE
            static quint64 getl64(const void *d); ///< Reads a 64 bit value at d in LE

            static void putl8(void *d, quint8 val); ///< Writes val at d in LE
            static void putl16(void *d, quint16 val); ///< Writes 2 byte val at d in LE
            static void putl32(void *d, quint32 val); ///< Writes 4 bytes val at d in LE
            static void putl64(void *d, quint64 val); ///< Writes 8 bytes val at d in LE

        protected:
            quint32 m_expectedLength; ///< from containerLength
            quint32 m_accumulatedLength; ///< length of payload received so far.
            quint8  *m_buffer; ///< This byte array holds the buffer for serialization/deserialization
            quint32 m_offset; ///< Offset into the internal buffer. The next serialization/deserialization will happen from this position
            quint32 m_bufferCapacity; ///< The total size of the internal buffer
            static const quint32 EXPANSION_STEP_SIZE = 512; ///< The expansion step size, in bytes, for the buffer
            bool m_extraLargeContainer; ///< Boolean to indicate if the container size is > 4GB

            /// Structure of the USB generic container.
            /// The structure conveniently maps an MTP container to
            /// a buffer in memory. The members of this structure represent
            /// a buffer in memory in LE. Hence you will note that the getl* and
            /// putl* functions are always used to access the members!
            struct MTPUSBContainer
            {
                quint32 containerLength;
                MTPContainerType containerType;
                quint16 code;
                quint32 transactionID;
                quint8 payload[1];
            }__attribute__((packed));

            MTPUSBContainer *m_container; ///< This internal container structure is populated for every received MTP container
        private:
            MTPContainer(const MTPContainer&){} ///< Private copy-constructor to disable copying
            ///< Private assignment operator to prevent duplication
            MTPContainer& operator=(const MTPContainer&)
            {
                return *this;
            }
    };
}
#endif

