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

#ifndef XMLHANDLER_H
#define XMLHANDLER_H

#include <QXmlDefaultHandler>
#include <QVector>
#include <QHash>
#include <QString>
#include "deviceinfo.h"


/// This class parses the device info xml file and populates DeviceInfo class.

/// This class is a friend of DeviceInfo class. It parses deviceinfo.xml and populates the various
/// device properties in DeviceInfo. This class uses Qt's SAX based parser.
namespace meegomtp1dot0
{
class XMLHandler : public QXmlDefaultHandler
{
public:
    /// Constructor.
    /// \param d, pointer to DeviceInfo object.
    XMLHandler(DeviceInfo* d = 0);

    /// Destructor.
    ~XMLHandler();

    /// This callback will be called by Qt upon encountering the beginning of an element in the xml file.
    /// \param aNamespaceURI the xml namespace.
    /// \param aLocalName local name.
    /// \param aName element name.
    /// \param aAttributes element attributes.
    /// \return success of failure
    bool startElement(const QString& aNamespaceURI, const QString& aLocalName, const QString& aName, const QXmlAttributes& aAttributes);

    /// This callback will be called by Qt upon encountering the end of an element in the xml file.
    /// \param aNamespaceURI the xml namespace.
    /// \param aLocalName local name.
    /// \param aName element name.
    /// \return success of failure
    bool endElement(const QString& aNamespaceURI, const QString& aLocalName, const QString& aName);

    /// This callback will provide the value for a certain attribute.
    /// \param aStr the value for an element's attribute.
    /// \return success of failure
    bool characters(const QString& aStr);

private:
    quint8 m_state; ///< indicates the element that was just parsed in the xml file.
    DeviceInfo *m_devInfo; ///< An object of DeviceInfo class, this object will be populated.
    quint16 m_devpropcode; ///< Stores the currently parsed device property's code.
    ///The member state will be one of these.
    enum m_element{ DEFAULT, START, STDVERSION, MTPVNDEXTN, MTPVERSION, MTPEXTN, FNMODE, MANU,
              MODEL, DEVVER, SERIAL, OPCODE, EVCODE, DEVPROPCODE, DEVPROPVAL, CODEC,
              IMINWIDTH, IMAXWIDTH, IMINHEIGHT, IMAXHEIGHT,
              VMINWIDTH, VMAXWIDTH, VMINHEIGHT, VMAXHEIGHT,
                  VCHANNEL, VMINFPS, VMAXFPS, VSCANTYPE, VSAMPLERATE, VMINBR, VMAXBR, VAUDMINBR,
          VAUDMAXBR, VMINKFD, VMAXKFD, ACHANNEL, ASAMPLERATE, AMINBR, AMAXBR,
                  COMMONF, AUDIOF, IMAGEF, VIDEOF };
};
}

#endif
