/*
* This file is part of libmeegomtp package
*
* Copyright (c) 2010 Nokia Corporation. All rights reserved.
* Copyright (c) 2016 - 2020 Jolla Ltd.
* Copyright (c) 2020 Open Mobile Platform LLC.
*
* Contact: Deepak Kodihalli <deepak.kodihalli@nokia.com>
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

#include "storageitem.h"
#include "trace.h"

using namespace meegomtp1dot0;

// Constructor.
StorageItem::StorageItem() :
    m_handle(0),
    m_path(""),
    m_wd(-1),
    m_objectInfo(0),
    m_parent(0),
    m_firstChild(0),
    m_nextSibling(0),
    m_puoid(MtpInt128(0)),
    m_eventsEnabled(false)
{
}

// Destructor
StorageItem::~StorageItem()
{
    if( m_objectInfo )
    {
        delete m_objectInfo;
        m_objectInfo = 0;
    }
}

void StorageItem::setEventsEnabled(bool enabled)
{
    if( m_eventsEnabled != enabled ) {
        m_eventsEnabled = enabled;
        if( m_eventsEnabled )
            MTP_LOG_INFO("events enabled for:" << m_path);
        else
            MTP_LOG_INFO("events disabled for:" << m_path);
    }
}

bool StorageItem::eventsAreEnabled(void) const
{
    return m_eventsEnabled;
}
