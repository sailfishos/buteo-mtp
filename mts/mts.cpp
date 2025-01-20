/*
* This file is part of libmeegomtp package
*
* Copyright (C) 2010 Nokia Corporation. All rights reserved.
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

#include "mts.h"
#include "mtpresponder.h"

using namespace meegomtp1dot0;

Mts *Mts::mts_instance = 0;
bool Mts::m_debugLogsEnabled = true;

Mts *Mts::getInstance()
{
    if (!mts_instance) {
        mts_instance =  new Mts;
    }
    return mts_instance;
}

Mts::Mts()
{
}

bool Mts::activate()
{
    m_MTPResponder = MTPResponder::instance();
    bool ok = m_MTPResponder->initTransport(USB);
    if (ok)
        ok = m_MTPResponder->initStorages();
    return ok;
}

bool Mts::deactivate()
{
    return true;
    //TODO
}

Mts::~Mts()
{
    delete m_MTPResponder;
}

void Mts::destroyInstance()
{
    delete mts_instance;
    mts_instance = nullptr;
}

void Mts::toggleDebugLogs()
{
    m_debugLogsEnabled = !m_debugLogsEnabled;
}

bool Mts::debugLogsEnabled()
{
    return m_debugLogsEnabled;
}

void Mts::suspend()
{
    m_MTPResponder->suspend();
}

void Mts::resume()
{
    m_MTPResponder->resume();
}
