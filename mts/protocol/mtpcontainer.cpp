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

#include <QString>
#include <QVector>
#include "mtpcontainer.h"

using namespace meegomtp1dot0;

MTPContainer::MTPContainer()
    : m_expectedLength(0)
    , m_accumulatedLength(0)
    , m_buffer(0)
    , m_offset(MTP_HEADER_SIZE)
    , m_bufferCapacity(0)
    , m_extraLargeContainer(false)
    , m_container(0)
{
}

MTPContainer::~MTPContainer()
{
    // No cleanup needed as of now
}

quint32 MTPContainer::bufferSize() const
{
    return m_offset;
}

quint32 MTPContainer::bufferCapacity() const
{
    return m_bufferCapacity;
}

quint32 MTPContainer::containerLength() const
{
    return getl32(&m_container->containerLength);
}

quint16 MTPContainer::containerType() const
{
    return getl16(&m_container->containerType);
}

quint16 MTPContainer::code() const
{
    return getl16(&m_container->code);
}

quint32 MTPContainer::transactionId() const
{
    return getl32(&m_container->transactionID);
}

quint8 *MTPContainer::payload() const
{
    return &m_container->payload[0];
}

void MTPContainer::params(QVector<quint32> &params)
{
    params.clear();
    params.fill(0, 5);
    if (MTP_CONTAINER_TYPE_COMMAND == containerType()) {
        // Determine the number of parameters
        quint32 numParams = (m_bufferCapacity - MTP_HEADER_SIZE) / sizeof(quint32);
        quint8 *d = payload();
        if (0 != d) {
            for (quint32 i = 0; i < numParams; i++) {
                params[i] = getl32(d + (i * sizeof(quint32)));
            }
        }
    }
}

void MTPContainer::seek(qint32 pos)
{
    if ((pos > 0) && (m_offset + pos <= m_bufferCapacity)) {
        m_offset += pos;
    } else if ((pos < 0) && (m_offset + pos > MTP_HEADER_SIZE)) {
        m_offset += pos;
    }
}

void MTPContainer::setExtraLargeContainer(bool isExtraLargeContainer)
{
    m_extraLargeContainer = isExtraLargeContainer;
}

quint8 MTPContainer::getl8(const void *d)
{
    return *(quint8 *) d;
}

quint16 MTPContainer::getl16(const void *d)
{
#ifdef LITTLE_ENDIAN
    return *(quint16 *) d;
#else
    return ((quint16) (getl8((quint8 *) d + 1)) << 8) | ((quint16) (getl8(d)));
#endif
}

quint32 MTPContainer::getl32(const void *d)
{
#ifdef LITTLE_ENDIAN
    return *(quint32 *) d;
#else
    return ((quint32) (getl16((quint8 *) d + 2)) << 16) | ((quint32) (getl16(d)));
#endif
}

quint64 MTPContainer::getl64(const void *d)
{
#ifdef LITTLE_ENDIAN
    return *(quint64 *) d;
#else
    return ((quint64) (getl32((quint8 *) d + 4)) << 32) | ((quint64) (getl32(d)));
#endif
}

void MTPContainer::putl8(void *d, quint8 val)
{
    *(quint8 *) d = val;
}

void MTPContainer::putl16(void *d, quint16 val)
{
#ifdef LITTLE_ENDIAN
    *(quint16 *) d = val;
#else
    putl8(d, (quint8) val);
    putl8(((quint8 *) d + 1), (quint8) val >> 8);
#endif
}

void MTPContainer::putl32(void *d, quint32 val)
{
#ifdef LITTLE_ENDIAN
    *(quint32 *) d = val;
#else
    putl16(d, (quint16) val);
    putl16(((quint8 *) d + 2), (quint8) val >> 16);
#endif
}

void MTPContainer::putl64(void *d, quint64 val)
{
#ifdef LITTLE_ENDIAN
    *(quint64 *) d = val;
#else
    putl32(d, (quint32) val);
    putl32(((quint8 *) d + 4), (quint32) val >> 32);
#endif
}
