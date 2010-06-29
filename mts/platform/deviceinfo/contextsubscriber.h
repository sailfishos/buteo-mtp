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

#ifndef CONTEXTSUBSCRIBER_H
#define CONTEXTSUBSCRIBER_H

#include <QObject>


class ContextProperty;

/// This class makes use of context provider to fetch battery level.

/// This class subscribes to the context provider service to get notified
/// about changes in device properties like battery level, and also to fetch
/// their current values.
namespace meegomtp1dot0
{
class ContextSubscriber : public QObject
{
    Q_OBJECT

public:
    /// Constructor.
    /// \param parent QObject, if any.
    ContextSubscriber(QObject* parent = 0);

    /// Destructor.
    ~ContextSubscriber();

    /// Gets the device's current battery stength.
    /// \return the battery strength.
    /// \callgraph
    /// \callergraph
    quint8 batteryLevel();

signals:
    /// This signal is emitted when the battery strength changes by 10% or more.
    /// \param the new battery level.
    void batteryLevelChanged( const quint8& batteryLevel );

private slots:
    /// This slot handles the batteryLevelChanged signal.
    void onBatteryLevelChanged();

private:
    ContextProperty *m_propBatteryLevel; ///< The battery level property.
    quint8 m_batteryLevel; ///< The battery level.
};
}

#endif
