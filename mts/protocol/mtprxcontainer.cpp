/*
* This file is part of libmeegomtp package
*
* Copyright (c) 2010 Nokia Corporation. All rights reserved.
* Copyright (c) 2018 - 2020 Jolla Ltd.
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

#include "mtprxcontainer.h"
using namespace meegomtp1dot0;

MTPRxContainer::MTPRxContainer(const quint8 *buffer, quint32 len)
{
    // This will be deleted in the destructor
    m_offset = MTP_HEADER_SIZE;
    const MTPUSBContainer *containerTemp = reinterpret_cast<const MTPUSBContainer *>(buffer);
    m_expectedLength = getl32(&containerTemp->containerLength);
    m_bufferCapacity = m_expectedLength;
    m_accumulatedLength = len;
    m_buffer = static_cast<quint8 *>(malloc(m_expectedLength));
    memcpy(m_buffer, buffer, len);
    // Reassign the container structure to the newly allocated buffer
    m_container = reinterpret_cast<MTPUSBContainer *>(m_buffer);
    m_extraLargeContainer = (0xFFFFFFFF == m_expectedLength);
    // buffer can now be free'd by the caller
}

MTPRxContainer::~MTPRxContainer()
{
    if (0 != m_buffer) {
        free(m_buffer);
        m_buffer = 0;
    }
}

void MTPRxContainer::append(const quint8 *buffer, quint32 len)
{
    // Append incoming buffer to an existing buffer
    if ((0 != buffer)) {
        if (m_accumulatedLength + len <= m_expectedLength) {
            // Combine the new buffer with the old one, and delete the old one
            memcpy(m_buffer + m_accumulatedLength, buffer, len);
            m_accumulatedLength += len;
            // buffer can now be free'd by the caller
        }
    }
}

MTPRxContainer &MTPRxContainer::operator>>(bool &d)
{
    quint8 i = 0;
    operator>>(i);
    d = (0 == i) ? false : true;
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(qint8 &d)
{
    return operator>>((quint8 &)d);
}

MTPRxContainer &MTPRxContainer::operator>>(QVector<qint8> &d)
{
    quint32 sz = 0;
    operator>>(sz);
    d.resize(sz);
    deserialize(d.data(), sizeof(quint8), sz);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(qint16 &d)
{
    return operator>>((quint16 &)d);
}

MTPRxContainer &MTPRxContainer::operator>>(QVector<qint16> &d)
{
    quint32 sz = 0;
    operator>>(sz);
    d.resize(sz);
    deserialize(d.data(), sizeof(quint16), sz);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(qint32 &d)
{
    return operator>>((quint32 &)d);
}

MTPRxContainer &MTPRxContainer::operator>>(QVector<qint32> &d)
{
    quint32 sz = 0;
    operator>>(sz);
    d.resize(sz);
    deserialize(d.data(), sizeof(quint32), sz);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(qint64 &d)
{
    return operator>>((quint64 &)d);
}

MTPRxContainer &MTPRxContainer::operator>>(QVector<qint64> &d)
{
    quint32 sz = 0;
    operator>>(sz);
    d.resize(sz);
    deserialize(d.data(), sizeof(quint64), sz);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(quint8 &d)
{
    deserialize(&d, sizeof(d), 1);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(QVector<quint8> &d)
{
    quint32 sz = 0;
    operator>>(sz);
    d.resize(sz);
    deserialize(d.data(), sizeof(quint8), sz);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(quint16 &d)
{
    deserialize(&d, sizeof(d), 1);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(QVector<quint16> &d)
{
    quint32 sz = 0;
    operator>>(sz);
    d.resize(sz);
    deserialize(d.data(), sizeof(quint16), sz);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(quint32 &d)
{
    deserialize(&d, sizeof(d), 1);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(QVector<quint32> &d)
{
    quint32 sz = 0;
    operator>>(sz);
    d.resize(sz);
    deserialize(d.data(), sizeof(quint32), sz);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(quint64 &d)
{
    deserialize(&d, sizeof(d), 1);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(QVector<quint64> &d)
{
    quint32 sz = 0;
    operator>>(sz);
    d.resize(sz);
    deserialize(d.data(), sizeof(quint64), sz);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(MtpInt128 &d)
{
    deserialize(&d, sizeof(quint8), sizeof(d));
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(QVector<MtpInt128> &d)
{
    quint32 sz = 0;
    operator>>(sz);
    d.resize(sz);
    memcpy(static_cast<void *>(d.data()),
           static_cast<void *>(m_buffer + m_offset),
           sz * sizeof(MtpInt128));
    m_offset += sz * sizeof(MtpInt128);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(QString &d)
{
    // Determine the number of characters in the string
    quint8 numChars = 0;
    deserialize(&numChars, sizeof numChars, 1);
    if (numChars > 0) {
        quint16 utf16[numChars];
        deserialize(utf16, sizeof * utf16, numChars);
        d = QString::fromUtf16(utf16, numChars - 1);
    } else {
        d.truncate(0);
    }
    MTP_LOG_TRACE("string:" << d);
    return *this;
}

MTPRxContainer &MTPRxContainer::operator>>(MTPObjectInfo &objInfo)
{
    // This temporary quint32 is needed as the initiator sends us a 32 bit file
    // size, however, objInfo.mtpObjectCompressedSize is a quint64. Casting a
    // reference of quint64 to quint32 is not strict-aliasing safe.
    quint32 objectSize;

    *this >> objInfo.mtpStorageId >> objInfo.mtpObjectFormat >> objInfo.mtpProtectionStatus
          >> objectSize >> objInfo.mtpThumbFormat
          >> objInfo.mtpThumbCompressedSize >> objInfo.mtpThumbPixelWidth
          >> objInfo.mtpThumbPixelHeight >> objInfo.mtpImagePixelWidth
          >> objInfo.mtpImagePixelHeight >> objInfo.mtpImageBitDepth
          >> objInfo.mtpParentObject >> objInfo.mtpAssociationType >> objInfo.mtpAssociationDescription
          >> objInfo.mtpSequenceNumber >> objInfo.mtpFileName
          >> objInfo.mtpCaptureDate >> objInfo.mtpModificationDate
          >> objInfo.mtpKeywords;
    objInfo.mtpObjectCompressedSize = objectSize;
    return *this;
}

void MTPRxContainer::deserializeVariantByType(MTPDataType type, QVariant &d)
{
    switch (type) {
    case MTP_DATA_TYPE_INT8:
    case MTP_DATA_TYPE_UINT8: {
        quint8 val = 0;
        deserialize(&val, sizeof(val), 1);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_INT16:
    case MTP_DATA_TYPE_UINT16: {
        quint16 val = 0;
        deserialize(&val, sizeof(val), 1);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_INT32:
    case MTP_DATA_TYPE_UINT32: {
        quint32 val = 0;
        deserialize(&val, sizeof(val), 1);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_INT64:
    case MTP_DATA_TYPE_UINT64: {
        quint64 val = 0;
        deserialize(&val, sizeof(val), 1);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_INT128:
    case MTP_DATA_TYPE_UINT128: {
        MtpInt128 val;
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_AINT8: {
        QVector<qint8> val;
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_AUINT8: {
        QVector<quint8> val;
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_AINT16: {
        QVector<qint16> val;
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_AUINT16: {
        QVector<quint16> val;
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_AINT32: {
        QVector<qint32> val;
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_AUINT32: {
        QVector<quint32> val;
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_AINT64: {
        QVector<qint64> val;
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_AUINT64: {
        QVector<quint64> val;
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_AINT128:
    case MTP_DATA_TYPE_AUINT128: {
        QVector<MtpInt128> val;
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    case MTP_DATA_TYPE_STR: {
        QString val = d.value<QString>();
        operator>>(val);
        d = QVariant::fromValue(val);
    }
    break;
    default:
        break;
    }
}

void MTPRxContainer::deserialize(void *target, quint32 elementSize, quint32 numberOfElements)
{
#ifdef LITTLE_ENDIAN
    // A direct memcpy works for little endian machines
    memcpy(static_cast<quint8 *>(target), m_buffer + m_offset, numberOfElements * elementSize);
    m_offset += (numberOfElements * elementSize);
#else
    for (quint32 i = 0; i < numberOfElements; i++) {
        if (elementSize == sizeof(quint8)) {
            (static_cast<quint8 *>(target))[i] = getl8(m_buffer + m_offset);
            m_offset += sizeof(quint8);
        } else if (elementSize == sizeof(quint16)) {
            (static_cast<quint16 *>(target))[i] = getl16(m_buffer + m_offset);
            m_offset += sizeof(quint16);
        } else if (elementSize == sizeof(quint32)) {
            (static_cast<quint32 *>(target))[i] = getl32(m_buffer + m_offset);
            m_offset += sizeof(quint32);
        } else {
            (static_cast<quint64 *>(target))[i] = getl64(m_buffer + m_offset);
            m_offset += sizeof(quint64);
        }
    }
#endif
}

