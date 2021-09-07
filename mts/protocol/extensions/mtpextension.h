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

#ifndef MTP_EXTENSION_H
#define MTP_EXTENSION_H

#include "mtptypes.h"

/// \brief The MTPExtension class server as an interface to write MTP extensions
///
/// The MTPExtesnion class contains interfaces needed to implement an MTP extension. The interface
/// methods all return a bool (true if the extension was able to handle the command, false otherwise)
namespace meegomtp1dot0 {
class MTPExtension
{
public:

    /// Use to determine if the MTP operation has a data phase
    /// \param opCode [in] The MTP operation code
    /// \param hasDataPhase [out] true is returned here if the operation has data phase, false otherwise
    /// \return Returns true if the operation is supported by the extension.
    virtual bool operationHasDataPhase(MTPOperationCode opCode, bool &hasDataPhase) = 0;

    /// Process the operation and return the response. Return status as true/false.
    /// \param req [in] The MTP resquest (and data from data phase)
    /// \param resp [out] The function returns the reponse (and data) in this parameter
    /// \return Returns true if the operation is supported by the extension
    virtual bool handleOperation(const MtpRequest &req, MtpResponse &resp) = 0;

    /// Use this to get the value of a device property
    /// \param code [in] The MTP device property code
    /// \param val [out] The device property value is returned in this parameter
    /// \return Returns true if the device property is supported by the extension
    virtual bool getDevPropValue(MTPDevPropertyCode code, QVariant &val, MTPResponseCode &respCode) = 0;

    /// Use this to set the value of a device property
    /// \param code [in] The MTP device property code
    /// \param val [in] The device property value
    /// \return Returns true if the device property is supported by the extension
    virtual bool setDevPropValue(MTPDevPropertyCode code, const QVariant &val, MTPResponseCode &respCode) = 0;

    /// Use this to get the value of an object property
    /// \param path [in] The path to the MTP object
    /// \param code [in] The MTP object property code
    /// \param val [out] The device property value is returned in this parameter
    /// \param respCode [out] The MTP response code is returned here.
    /// \return Returns true if the object property is supported by the extension
    virtual bool getObjPropValue(const QString &path, MTPObjPropertyCode code, QVariant &val,
                                 MTPResponseCode &respCode) = 0;

    /// Use this to set the value of an object property
    /// \param path [in] The path to the MTP object
    /// \param code [in] The MTP object property code
    /// \param val [in] The device property value
    /// \return Returns true if the object property is supported by the extension
    virtual bool setObjPropValue(const QString &path, MTPObjPropertyCode code, const QVariant &val,
                                 MTPResponseCode &respCode) = 0;

    virtual ~MTPExtension() {}
};
}
#endif //MTP_EXTENSION_H

