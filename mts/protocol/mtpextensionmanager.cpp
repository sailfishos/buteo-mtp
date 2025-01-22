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

#include "mtpextensionmanager.h"
#include "mtpextension.h"
#include "mtptypes.h"

using namespace meegomtp1dot0;

MTPExtensionManager::MTPExtensionManager()
{
}

MTPExtensionManager::~MTPExtensionManager()
{
    qDeleteAll(m_extensionList);
    m_extensionList.clear();
}

bool MTPExtensionManager::operationHasDataPhase(MTPOperationCode opCode, bool &hasDataPhase) const
{
    foreach (MTPExtension *extension, m_extensionList) {
        if (extension->operationHasDataPhase(opCode, hasDataPhase)) {
            return true;
        }
    }
    return false;
}

bool MTPExtensionManager::handleOperation(const MtpRequest &req, MtpResponse &resp) const
{
    foreach (MTPExtension *extension, m_extensionList) {
        if (extension->handleOperation(req, resp)) {
            return true;
        }
    }
    return false;
}

bool MTPExtensionManager::getDevPropValue(MTPDevPropertyCode propCode, QVariant &val, MTPResponseCode &respCode) const
{
    foreach (MTPExtension *extension, m_extensionList) {
        if (extension->getDevPropValue(propCode, val, respCode)) {
            return true;
        }
    }
    return false;
}

bool MTPExtensionManager::setDevPropValue(MTPDevPropertyCode propCode, const QVariant &val,
                                          MTPResponseCode &respCode) const
{
    foreach (MTPExtension *extension, m_extensionList) {
        if (extension->setDevPropValue(propCode, val, respCode)) {
            return true;
        }
    }
    return false;
}

bool MTPExtensionManager::getObjPropValue(const QString &path, MTPObjPropertyCode propCode, QVariant &val,
                                          MTPResponseCode &respCode) const
{
    foreach (MTPExtension *extension, m_extensionList) {
        if (extension->getObjPropValue(path, propCode, val, respCode)) {
            return true;
        }
    }
    return false;
}

bool MTPExtensionManager::setObjPropValue(const QString &path, MTPObjPropertyCode propCode, const QVariant &val,
                                          MTPResponseCode &respCode) const
{
    foreach (MTPExtension *extension, m_extensionList) {
        if (extension->setObjPropValue(path, propCode, val, respCode)) {
            return true;
        }
    }
    return false;
}

