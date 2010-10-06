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

#ifndef FSINOTIFY_H
#define FSINOTIFY_H

#include <QObject>
#include "sys/inotify.h"
class QSocketNotifier;

/// FSInotify is a wrapper class for the inotify library.

/// FSInotify notifies the filesystem storage plug-in about changes in the file system
/// so that the storage can take appropriate action.
namespace meegomtp1dot0
{
class FSInotify : public QObject
{
    Q_OBJECT

public:
    /// Constructor.
    /// \param mask [in] indicates what to watch for.
    FSInotify( uint32_t theMask = IN_MOVE | IN_CREATE | IN_DELETE | IN_CLOSE_WRITE );

    /// Desctructor.
    ~FSInotify();

    /// Adds a new watch to the file whose pathname is provided.
    /// \param pathName [in] the file's pathname.
    /// \return watch descriptor on success, -1 on failure.
    int addWatch( const QString &pathName ) const;

    /// Removes an added watch.
    /// \param wd [in] the watch to be removed.
    /// \return 0 on success -1 on failure.
    int removeWatch( const int &wd ) const;

public slots:
    /// This slot is for reading the inotify event, when one is generated.
    void inotifyEventSlot(int);

signals:
    /// This signal is emitted corresponding to an inotify event.
    /// \param event [in] the structure holding inotify event details.
    void inotifyEventSignal( struct inotify_event *event );

private:
    uint32_t m_mask; ///< indicates what to watch for on a file.
    QSocketNotifier *m_readSocket; ///< inotify events will be written to this socket.
};
}

#endif
