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

#ifndef MTP_TXCONTAINER_H
#define MTP_TXCONTAINER_H

#include "mtpcontainer.h"

namespace meegomtp1dot0 {
class MTPTxContainer : public MTPContainer
{
public:
    /// Constructor; Constructs a container for serialization. Use this constructor when sending data to the initiator
    /// \param type [in] The type of the container to create
    /// \param code [in] The code portion of the container (For request and
    /// data containers, this is the operation code, and for response
    /// containers, this is the response code)
    /// \param transactionID [in] The transaction ID of the container
    /// \param bufferEstimate [in] The estimated initial size of the payload
    /// \param extraLargeContainer [in] If true, the container size is set to 0xFFFFFFFF
    /// buffer. Note that this is exclusive of the container header.
    MTPTxContainer(MTPContainerType type, quint16 code, quint32 transactionID, quint32 bufferEstimate = 0);

    /// Destructor
    ~MTPTxContainer();

    /// Returns a pointer to the internal buffer. This may be called, for example, once all data has been serialized,
    /// and the application wants to send the data over.
    /// \return A pointer (const) to the buffer.
    const quint8 *buffer();

    //************
    // Serializers
    //************
    /// Serializes a boolean as a 8-bit value
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(bool d);
    /// Serializes a 8-bit signed value
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(qint8 d);
    /// Serializes a 16-bit signed value
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(qint16 d);
    /// Serializes a 32-bit signed value
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(qint32 d);
    /// Serializes a 64-bit signed value
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(qint64 d);
    /// Serializes a 8-bit unsigned value
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(quint8 d);
    /// Serializes a 16-bit unsigned value
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(quint16 d);
    /// Serializes a 32-bit unsigned value
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(quint32 d);
    /// Serializes a 64-bit unsigned value
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(quint64 d);
    /// Serializes a 128-bit signed/unsigned value
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const MtpInt128 &d);
    /// Serializes an array of signed 8-bit values
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const QVector<qint8> &d);
    /// Serializes an array of signed 16-bit values
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const QVector<qint16> &d);
    /// Serializes an array of signed 32-bit values
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const QVector<qint32> &d);
    /// Serializes an array of signed 64-bit values
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const QVector<qint64> &d);
    /// Serializes an array of unsigned 8-bit values
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const QVector<quint8> &d);
    /// Serializes an array of unsigned 16-bit values
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const QVector<quint16> &d);
    /// Serializes an array of unsigned 32-bit values
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const QVector<quint32> &d);
    /// Serializes an array of unsigned 64-bit values
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const QVector<quint64> &d);
    /// Serializes an array of signed/unsigned 128-bit values
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const QVector<MtpInt128> &d);
    /// Serializes a string
    /// \param d [in] The string to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const QString &d);
    /// Serializes device property description dataset
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const MtpDevPropDesc &propDesc);
    /// Serializes object property description dataset
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const MtpObjPropDesc &propDesc);
    /// Serializes the MTP object info dataset
    /// \param d [in] The value to serialize
    /// \return Returns a self-reference
    MTPTxContainer &operator<<(const MTPObjectInfo &objInfo);
    /// Serializes a variant value by it's MTP type
    /// \param type [in] The MTP type of the value to be serialized
    /// \param d [in] The value to serialize
    void serializeVariantByType(MTPDataType type, const QVariant &d);
    ///< Provide a container length and prevent MTPTxContainer from determining the same
    void setContainerLength(quint32 containerLength);
    ///< Allow MTPTxContainer to determine container length ( the default )
    void resetContainerLength();

private:
    ///< Serializes into the internal buffer, elements of the given size and
    /// number
    void serialize(const void *source, quint32 elementSize, quint32 numberOfElements);
    ///< Helper function to serialize the form field for property
    /// descriptions
    void serializeFormField(MTPDataType type, MtpFormFlag formFlag, const QVariant &formField);
    ///< Expands the class's internal buffer by the required capacity
    void expandBuffer(quint32 requiredSpace);

    bool m_computeContainerLength; ///< if true, allow MTPTxContainer to determine container length ( the default )
};
}

#endif
