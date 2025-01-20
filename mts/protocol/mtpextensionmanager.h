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

#ifndef MTP_EXTENSION_MANAGER_H
#define MTP_EXTENSION_MANAGER_H

#include <QList>
#include "mtptypes.h"

class QVariant;

namespace meegomtp1dot0 {
class MTPExtension;
}

/// \brief The MTPExtensionManager class serves as a common interface to handle extended operations
///
/// This class creates and manages MTP extension classes. The extension class handle MTP operations that
/// are extensions to the MTP 1.0 specification. In addition to handling operations, the class also provides
/// interfaces to get and set device and object properties that belong to the MTP extended specification.
/// All methods of this interface return a bool to indicate if the request was handled by one of the extension
/// classes (true), or none (false). The interface has no support for MTP events right now.
namespace meegomtp1dot0 {
class MTPExtensionManager
{
public:
    /// Constructor: Loads all available extensions
    MTPExtensionManager();

    /// Destructor: Unloads all available extensions
    ~MTPExtensionManager();

    /// Used to check if the MTP operation has also, a data phase. Using this, the caller can determine whether to
    /// wait on a data phase.
    /// \param opCode [in] The MTP operation code
    /// \param hasDataPhase [out] true is returned in this parameter if the operation has a data phase, false otherwise
    /// \return Returns true if the extension manager could find an extension that handled this operation, else returns false
    bool operationHasDataPhase(MTPOperationCode opCode, bool &hasDataPhase) const;

    /// Handles an MTP operation. The caller must fill in the request and data phase values into the first parameter
    /// and then call this function! This means, that if an operation has a data phase (I->R), then the caller must wait on
    /// the data phase before filling in the request. Similarly, the second parameter contains the response to the operation,
    /// the response data (R->I) will be filled along with the response parameters and response code.
    /// \param req [in] The operation request
    /// \param resp [out] The operation response
    /// \return Returns true if the extension manager could find an extension that handled this operation, else returns false
    bool handleOperation(const MtpRequest &req, MtpResponse &resp) const;

    /// Uses one of the extension classes to get the value of a device property
    /// \param propCode [in] The device property code
    /// \param val [out] The device property value is returned here
    /// \param respCode [out] The MTP response code is returned here. If
    /// there is no plugin to handle this, the code will remain unaltered.
    /// \return Returns true in case of success, else false
    bool getDevPropValue(MTPDevPropertyCode propCode, QVariant &val, MTPResponseCode &respCode) const;

    /// Uses one of the extension classes to set the value of a device property
    /// \param propCode [in] The device property code
    /// \param val [out] The device property value to be set
    /// \param respCode [out] The MTP response code is returned here. If
    /// there is no plugin to handle this, the code will remain unaltered.
    /// \return Returns true in case of success, else false
    bool setDevPropValue(MTPDevPropertyCode propCode, const QVariant &val, MTPResponseCode &respCode) const;

    /// Uses one of the extension classes to get the value of an object property
    /// \param path [in] The full path to the object
    /// \param propCode [in] The object property code
    /// \param val [out] The object property value is returned here
    /// \param respCode [out] The MTP response code is returned here. If
    /// there is no plugin to handle this, the code will remain unaltered.
    /// \return Returns true in case of success, else false
    bool getObjPropValue(const QString &path, MTPObjPropertyCode propCode, QVariant &val, MTPResponseCode &respCode) const;

    /// Uses one of the extension classes to set the value of an object property
    /// \param path [in] The full path to the object
    /// \param propCode [in] The object property code
    /// \param val [out] The object property value is returned here
    /// \param respCode [out] The MTP response code is returned here. If
    /// there is no plugin to handle this, the code will remain unaltered.
    /// \return Returns true in case of success, else false
    bool setObjPropValue(const QString &path, MTPObjPropertyCode propCode, const QVariant &val,
                         MTPResponseCode &respCode) const;
private:
    QList<MTPExtension *>  m_extensionList; ///< An internal list of MTPExtension classes
};
}
#endif //MTP_EXTENSION_MANAGER_H

