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

#ifndef MTP_RXCONTAINER_H
#define MTP_RXCONTAINER_H

#include "mtpcontainer.h"

namespace meegomtp1dot0 {
class MTPRxContainer : public MTPContainer
{
public:

    /// Constructor; Use this constructor for de-serialization of data
    /// incoming from the initiator.
    /// \param buffer [in] The data buffer.
    /// \param len [in] The length of the data buffer, in bytes.
    MTPRxContainer(const quint8 *buffer, quint32 len);

    /// Destructor.
    ~MTPRxContainer();

    /// Appends buffer to the container (Combines segmented container packets)
    /// \param buffer [in] The new segment, to be appended to existing buffer
    /// \param len [in] The length of the new segment
    void append(const quint8 *buffer, quint32 len);

    //***************
    // De-Serializers
    //***************
    /// Deserializes a 8-bit value as a boolean
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(bool &d);
    /// Deserializes a 8-bit signed value
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(qint8 &d);
    /// Deserializes a 16-bit signed value
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(qint16 &d);
    /// Deserializes a 32-bit signed value
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(qint32 &d);
    /// Deserializes a 64-bit signed value
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(qint64 &d);
    /// Deserializes a 8-bit unsigned value
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(quint8 &d);
    /// Deserializes a 16-bit unsigned value
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(quint16 &d);
    /// Deserializes a 32-bit unsigned value
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(quint32 &d);
    /// Deserializes a 64-bit unsigned value
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(quint64 &d);
    /// Deserializes a 128-bit signed/unsigned value
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(MtpInt128 &d);
    /// Deserializes an array of 8-bit signed values
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(QVector<qint8> &d);
    /// Deserializes an array of 16-bit signed values
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(QVector<qint16> &d);
    /// Deserializes an array of 32-bit signed values
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(QVector<qint32> &d);
    /// Deserializes an array of 64-bit signed values
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(QVector<qint64> &d);
    /// Deserializes an array of 8-bit unsigned values
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(QVector<quint8> &d);
    /// Deserializes an array of 16-bit unsigned values
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(QVector<quint16> &d);
    /// Deserializes an array of 32-bit unsigned values
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(QVector<quint32> &d);
    /// Deserializes an array of 64-bit unsigned values
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(QVector<quint64> &d);
    /// Deserializes an array of 128-bit signed/unsigned values
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(QVector<MtpInt128> &d);
    /// Deserializes a string
    /// \param d [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(QString &d);
    /// Deserializes the MTP object info dataset
    /// \param objInfo [out] The data is deserialized into this
    /// \return Returns a self-reference
    MTPRxContainer &operator>>(MTPObjectInfo &objInfo);
    /// Deserializes a variant data by it's MTP type
    /// \param type [in] The MTP type of the data to be deserialized
    /// \param d [out] The data is deserialized into this
    void deserializeVariantByType(MTPDataType type, QVariant &d);

private:
    ///< Deserializes from the internal buffer, elements of the given size and
    /// number
    void deserialize(void *target, quint32 elementSize, quint32 numberOfElements);
};
}
#endif

