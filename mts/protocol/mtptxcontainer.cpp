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

#include "mtptxcontainer.h"
using namespace meegomtp1dot0;

MTPTxContainer::MTPTxContainer(MTPContainerType type, quint16 code, quint32 transactionID, quint32 bufferEstimate /*= 0*/)
    : MTPContainer()
{
    // Allocate buffer for header + playload estimate
    // Have to use malloc here, as we need to be able to realloc the buffer
    m_buffer = static_cast<quint8 *>(malloc(MTP_HEADER_SIZE + bufferEstimate));
    m_container = reinterpret_cast<MTPUSBContainer *>(m_buffer);
    // Populate the buffer header
    // Container length is set to 0 now, it needs to be populated with the
    // correct container length, when buffer() is called.
    putl32(&m_container->containerLength, 0);
    putl16(&m_container->containerType, type);
    putl16(&m_container->code, code);
    putl32(&m_container->transactionID, transactionID);
    m_offset = MTP_HEADER_SIZE;
    m_bufferCapacity = MTP_HEADER_SIZE + bufferEstimate;
    m_computeContainerLength = true;
}

MTPTxContainer::~MTPTxContainer()
{
    if (0 != m_buffer) {
        free(m_buffer);
        m_buffer = 0;
    }
}

void MTPTxContainer::setContainerLength(quint32 containerLength)
{
    putl32(&m_container->containerLength, containerLength);
    m_computeContainerLength = false;
}

void MTPTxContainer::resetContainerLength()
{
    m_computeContainerLength = true;
}

const quint8 *MTPTxContainer::buffer()
{
    // Populate the container length
    if (m_computeContainerLength) {
        if (m_extraLargeContainer) {
            putl32(&m_container->containerLength, 0XFFFFFFFF);
        } else {
            putl32(&m_container->containerLength, m_offset);
        }
    }
    return m_buffer;
}

MTPTxContainer &MTPTxContainer::operator<<(bool d)
{
    quint8 i = (d) ? 0x01 : 0x00;
    return operator<<(i);
}

MTPTxContainer &MTPTxContainer::operator<<(qint8 d)
{
    return operator<<((quint8) d);
}

MTPTxContainer &MTPTxContainer::operator<<(const QVector<qint8> &d)
{
    quint32 len = d.size();
    operator<<(len);
    serialize(d.data(), sizeof(quint8), len);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(qint16 d)
{
    return operator<<((quint16) d);
}

MTPTxContainer &MTPTxContainer::operator<<(const QVector<qint16> &d)
{
    quint32 len = d.size();
    operator<<(len);
    serialize(d.data(), sizeof(quint16), len);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(qint32 d)
{
    return operator<<((quint32) d);
}

MTPTxContainer &MTPTxContainer::operator<<(const QVector<qint32> &d)
{
    quint32 len = d.size();
    operator<<(len);
    serialize(d.data(), sizeof(quint32), len);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(qint64 d)
{
    return operator<<((quint64) d);
}

MTPTxContainer &MTPTxContainer::operator<<(const QVector<qint64> &d)
{
    quint32 len = d.size();
    operator<<(len);
    serialize(d.data(), sizeof(quint64), len);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(quint8 d)
{
    serialize(&d, sizeof(d), 1);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(const QVector<quint8> &d)
{
    quint32 len = d.size();
    operator<<(len);
    serialize(d.data(), sizeof(quint8), len);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(quint16 d)
{
    serialize(&d, sizeof(d), 1);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(const QVector<quint16> &d)
{
    quint32 len = d.size();
    operator<<(len);
    serialize(d.data(), sizeof(quint16), len);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(quint32 d)
{
    serialize(&d, sizeof(d), 1);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(const QVector<quint32> &d)
{
    quint32 len = d.size();
    operator<<(len);
    serialize(d.data(), sizeof(quint32), len);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(quint64 d)
{
    serialize(&d, sizeof(d), 1);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(const QVector<quint64> &d)
{
    quint32 len = d.size();
    operator<<(len);
    serialize(d.data(), sizeof(quint64), len);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(const MtpInt128 &d)
{
    serialize(&d, sizeof(quint8), sizeof(d));
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(const QVector<MtpInt128> &d)
{
    // Required size = no. of elements in the array + size of array data
    quint32 len = d.size();
    quint32 reqSize = sizeof(quint32) + (len * sizeof(MtpInt128));
    if (m_bufferCapacity < m_offset + reqSize) {
        expandBuffer(reqSize);
    }
    operator<<(len);
    memcpy(m_buffer + m_offset, d.data(), reqSize - sizeof(quint32));
    m_offset += (reqSize - sizeof(quint32));
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(const QString &d)
{
    QString str;

    const quint16 *dta = 0;
    const quint16 *end = 0;
    int len = 0;

    // Maximum possible string length is 255 (one character for the NULL terminator)
    // Due to UTF16 encoding, content size can be greater than string length
    // -> Shorten the string until encoded length does not exceed the limit.
    for (int cutoff = 254;; --cutoff) {
        str = d;
        str.truncate(cutoff);
        end = dta = str.utf16();
        while (*end)
            ++end;
        len = (int) (end - dta);
        if (len <= 254)
            break;
    }

    MTP_LOG_TRACE("string:" << str);

    quint8 stringChars = (len > 0) ? (len + 1) : 0;
    serialize(&stringChars, sizeof(quint8), 1);
    if (stringChars)
        serialize(dta, sizeof *dta, stringChars);

    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(const MtpDevPropDesc &propDesc)
{
    // Serialize the deice property description dataset
    *this << propDesc.uPropCode << propDesc.uDataType << propDesc.bGetSet;
    serializeVariantByType(propDesc.uDataType, propDesc.defValue);
    serializeVariantByType(propDesc.uDataType, propDesc.currentValue);
    // Serialize the form flag, followed by the form field
    *this << propDesc.formFlag;
    serializeFormField(propDesc.uDataType, propDesc.formFlag, propDesc.formField);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(const MtpObjPropDesc &propDesc)
{
    // Serialize the deice property description dataset
    *this << propDesc.uPropCode << propDesc.uDataType << propDesc.bGetSet;
    serializeVariantByType(propDesc.uDataType, propDesc.defValue);
    // Serialize the group code, form flag followed by the form field
    *this << propDesc.groupCode << propDesc.formFlag;
    serializeFormField(propDesc.uDataType, propDesc.formFlag, propDesc.formField);
    return *this;
}

MTPTxContainer &MTPTxContainer::operator<<(const MTPObjectInfo &objInfo)
{
    *this << objInfo.mtpStorageId << objInfo.mtpObjectFormat << objInfo.mtpProtectionStatus
          << static_cast<quint32>(objInfo.mtpObjectCompressedSize) << objInfo.mtpThumbFormat
          << objInfo.mtpThumbCompressedSize << objInfo.mtpThumbPixelWidth
          << objInfo.mtpThumbPixelHeight << objInfo.mtpImagePixelWidth
          << objInfo.mtpImagePixelHeight << objInfo.mtpImageBitDepth
          << objInfo.mtpParentObject << objInfo.mtpAssociationType << objInfo.mtpAssociationDescription
          << objInfo.mtpSequenceNumber << objInfo.mtpFileName
          << objInfo.mtpCaptureDate << objInfo.mtpModificationDate
          << objInfo.mtpKeywords;
    return *this;
}

void MTPTxContainer::serializeVariantByType(MTPDataType type, const QVariant &d)
{
    switch (type) {
    case MTP_DATA_TYPE_INT8:
    case MTP_DATA_TYPE_UINT8: {
        quint8 val = d.value<quint8>();
        serialize(&val, sizeof(val), 1);
    }
    break;
    case MTP_DATA_TYPE_INT16:
    case MTP_DATA_TYPE_UINT16: {
        quint16 val = d.value<quint16>();
        serialize(&val, sizeof(val), 1);
    }
    break;
    case MTP_DATA_TYPE_INT32:
    case MTP_DATA_TYPE_UINT32: {
        quint32 val = d.value<quint32>();
        serialize(&val, sizeof(val), 1);
    }
    break;
    case MTP_DATA_TYPE_INT64:
    case MTP_DATA_TYPE_UINT64: {
        quint64 val = d.value<quint64>();
        serialize(&val, sizeof(val), 1);
    }
    break;
    case MTP_DATA_TYPE_INT128:
    case MTP_DATA_TYPE_UINT128: {
        MtpInt128 val = d.value<MtpInt128>();
        operator<<(val);
    }
    break;
    case MTP_DATA_TYPE_AINT8: {
        QVector<qint8> val = d.value< QVector<qint8> >();
        operator<<(val);
    }
    break;
    case MTP_DATA_TYPE_AUINT8: {
        QVector<quint8> val = d.value< QVector<quint8> >();
        operator<<(val);
    }
    break;
    case MTP_DATA_TYPE_AINT16: {
        QVector<qint16> val = d.value< QVector<qint16> >();
        operator<<(val);
    }
    break;
    case MTP_DATA_TYPE_AUINT16: {
        QVector<quint16> val = d.value< QVector<quint16> >();
        operator<<(val);
    }
    break;
    case MTP_DATA_TYPE_AINT32: {
        QVector<qint32> val = d.value< QVector<qint32> >();
        operator<<(val);
    }
    break;
    case MTP_DATA_TYPE_AUINT32: {
        QVector<quint32> val = d.value< QVector<quint32> >();
        operator<<(val);
    }
    break;
    case MTP_DATA_TYPE_AINT64: {
        QVector<qint64> val = d.value< QVector<qint64> >();
        operator<<(val);
    }
    break;
    case MTP_DATA_TYPE_AUINT64: {
        QVector<quint64> val = d.value< QVector<quint64> >();
        operator<<(val);
    }
    break;
    case MTP_DATA_TYPE_AINT128:
    case MTP_DATA_TYPE_AUINT128: {
        QVector<MtpInt128> val = d.value< QVector<MtpInt128> >();
        operator<<(val);
    }
    break;
    case MTP_DATA_TYPE_STR: {
        QString val = d.value<QString>();
        operator<<(val);
    }
    break;
    default:
        break;
    }
}

void MTPTxContainer::serialize(const void *source, quint32 elementSize, quint32 numberOfElements)
{
    // Expand buffer if needed
    if (m_offset + (elementSize * numberOfElements) > m_bufferCapacity) {
        expandBuffer(m_offset + (elementSize * numberOfElements));
    }
#ifdef LITTLE_ENDIAN
    // A direct memcpy works for little endian machines
    memcpy(m_buffer + m_offset, (static_cast<const quint8 *>(source)), elementSize * numberOfElements);
    m_offset += (elementSize * numberOfElements);
#else
    for (quint32 i = 0; i < numberOfElements; i++) {
        if (elementSize == sizeof(quint8)) {
            putl8(m_buffer + m_offset, (static_cast<const quint8 *>(source))[i]);
            m_offset += sizeof(quint8);
        } else if (elementSize == sizeof(quint16)) {
            putl16(m_buffer + m_offset, (static_cast<const quint16 *>(source))[i]);
            m_offset += sizeof(quint16);
        } else if (elementSize == sizeof(quint32)) {
            putl32(m_buffer + m_offset, (static_cast<const quint32 *>(source))[i]);
            m_offset += sizeof(quint32);
        } else {
            putl64(m_buffer + m_offset, (static_cast<const quint64 *>(source))[i]);
            m_offset += sizeof(quint64);
        }
    }
#endif
}

void MTPTxContainer::serializeFormField(MTPDataType type, MtpFormFlag formFlag, const QVariant &formField)
{
    switch (formFlag) {
    case MTP_FORM_FLAG_NONE:
        break;
    case MTP_FORM_FLAG_RANGE: {
        MtpRangeForm range = formField.value<MtpRangeForm>();
        serializeVariantByType(type, range.minValue);
        serializeVariantByType(type, range.maxValue);
        serializeVariantByType(type, range.stepSize);
    }
    break;
    case MTP_FORM_FLAG_ENUM: {
        MtpEnumForm en = formField.value<MtpEnumForm>();
        *this << en.uTotal;
        for (int i = 0; i < en.values.size(); i++) {
            serializeVariantByType(type, en.values.at(i));
        }
    }
    break;
    case MTP_FORM_FLAG_DATE_TIME:
        break;
    case MTP_FORM_FLAG_FIXED_ARRAY: {
        quint16 len = formField.value<quint16>();
        *this << len;
    }
    break;
    case MTP_FORM_FLAG_REGEX: {
        QString regexp = formField.value<QString>();
        *this << regexp;
    }
    break;
    case MTP_FORM_FLAG_BYTE_ARRAY:
    case MTP_FORM_FLAG_LONG_STRING: {
        quint32 len = formField.value<quint32>();
        *this << len;
    }
    break;
    default:
        break;
    }
}

void MTPTxContainer::expandBuffer(quint32 requiredSpace)
{
    // TODO: The optimal expansion step size needs to be figured out, should it
    // be the page size??
    requiredSpace += EXPANSION_STEP_SIZE;
    m_buffer = static_cast<quint8 *>(realloc(m_buffer, m_bufferCapacity + requiredSpace));
    m_bufferCapacity += requiredSpace;
    m_container = reinterpret_cast<MTPUSBContainer *>(m_buffer);
}
