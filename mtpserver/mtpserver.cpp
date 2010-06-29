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

#include <QtDebug>

#include <mts.h>
#include "trace.h"
#include "mtpserver.h"


using namespace meegomtp1dot0;

MtpServer::MtpServer(const QString& aPluginName,
        const Profile& aProfile, PluginCbInterface* aCbInterface) : ServerPlugin(aPluginName, aProfile, aCbInterface)
{
    MTP_LOG_INFO(__PRETTY_FUNCTION__);
}

MtpServer::~MtpServer()
{
    MTP_LOG_INFO(__PRETTY_FUNCTION__);
    Mts::destroyInstance();
}

bool MtpServer::init()
{
    MTP_LOG_INFO(__PRETTY_FUNCTION__);
    return true;
}

bool MtpServer::uninit()
{
    MTP_LOG_INFO(__PRETTY_FUNCTION__);
    return true;
}

bool MtpServer::startListen()
{
    bool ok = Mts::getInstance()->activate();
    return ok;
}

void MtpServer::stopListen()
{
    Mts::destroyInstance();
}

void MtpServer::connectivityStateChanged( Sync::ConnectivityType /*aType*/, bool /*aState*/ )
{
    MTP_LOG_INFO(__PRETTY_FUNCTION__);
}
