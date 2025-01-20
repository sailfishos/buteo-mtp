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

#include <unistd.h>
#include "fsinotify.h"
#include <QSocketNotifier>

using namespace meegomtp1dot0;

/**************************************************
 * FSInotify::FSInotify
 *************************************************/
FSInotify::FSInotify( uint32_t mask) : m_mask(mask)
{
    m_readSocket = new QSocketNotifier( inotify_init(), QSocketNotifier::Read );
    if ( m_readSocket ) {
        QObject::connect( m_readSocket, SIGNAL(activated(int)), this, SLOT(inotifyEventSlot(int)) );
    }
}

/**************************************************
 * FSInotify::~FSInotify
 *************************************************/
FSInotify::~FSInotify()
{
    delete m_readSocket;
}

/**************************************************
 * int FSInotify::addWatch
 *************************************************/
int FSInotify::addWatch( const QString &pathName ) const
{
    if ( !m_readSocket ) {
        return -1;
    }
    QByteArray ba = pathName.toUtf8();
    return inotify_add_watch( m_readSocket->socket(), ba.constData(), m_mask );
}

/**************************************************
 * int FSInotify::removeWatch
 *************************************************/
int FSInotify::removeWatch( const int &wd ) const
{
    if ( !m_readSocket ) {
        return -1;
    }
    return inotify_rm_watch( m_readSocket->socket(), wd );
}

/**************************************************
 * void FSInotify::inotifyEventSlot
 *************************************************/
void FSInotify::inotifyEventSlot(int)
{
    char tmp[512], *ptr;
    int bytes_read = -1;

    bytes_read = read( m_readSocket->socket(), tmp, sizeof(tmp) );

    if (-1 == bytes_read) {
        return;
    }

    ptr = tmp;
    while (ptr < tmp + bytes_read) {
        struct inotify_event *event = (struct inotify_event *) ptr;
        // FIXME: would it be better to pass all events or one at a time?
        emit inotifyEventSignal( event );
        ptr += sizeof * event + event->len;
    }
}

