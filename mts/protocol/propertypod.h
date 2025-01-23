/*
* This file is part of libmeegomtp package
*
* Copyright (c) 2010 Nokia Corporation. All rights reserved.
* Copyright (c) 2021 - 2022 Jolla Ltd.
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

#ifndef PROPERTYPOD_H
#define PROPERTYPOD_H

#include "mtptypes.h"

namespace meegomtp1dot0 {
class MtpDeviceInfo;
class MTPExtensionManager;
}

/// \brief The PropertyPod class provides access to MTP object properties and their description
///
/// The PropertyPod class provides convenient access to MTP object properties and their descriptions.
/// It can also be used to determine if a property is supported by a particular object format.
/// It's methods provide read access to these values.
namespace meegomtp1dot0 {
class PropertyPod
{
public:
    /// Returns the instance to a PropertyPod object
    /// \param devInfoProvider [in] The pointer to the device info provider class. The PropertyPod class uses this to fetch some property descriptions
    /// \param extManager [in] Pointer to the extension manager class. PropertyPod uses this to fetch some extended device properties
    /// \return Returns a pointer to the instance of PropertyPod
    static PropertyPod *instance(MtpDeviceInfo *devInfoProvider, MTPExtensionManager *extManager);

    /// Releases the instance of the class
    static void releaseInstance();

    /// Returns a vector of all object properties supported for the format category of the object provided
    /// \param category [in] The category type of the object
    /// \param propsSupported [out] The supported set of object properties is returned in the vector
    /// \return Returns the result as an MTP response code.
    MTPResponseCode getObjectPropsSupportedByType(MTPObjectFormatCategory category,
                                                  QVector<MTPObjPropertyCode> &propsSupported);

    /// Returns a set of interdependent properties
    /// \param category [in] The category type
    /// \param propDesc [out] A vector containing the pointers to the property descriptions of the set of mutually dependent properties
    /// \return Returns the result as an MTP response code
    MTPResponseCode getInterdependentPropDesc(MTPObjectFormatCategory category, QVector<MtpObjPropDesc *> &propDesc);

    /// Use this to get the property description for the MTP object property
    /// \param category [in] The category type
    /// \param propCode [in] The MTP code for the object property
    /// \param propDesc [out] This will be populated with the object property description
    /// \return Returns the result as an MTP response code
    MTPResponseCode getObjectPropDesc(MTPObjectFormatCategory category, MTPObjPropertyCode propCode,
                                      const MtpObjPropDesc *&propDesc);

    /// Use this to get the property description for a MTP device property
    /// \param propCode [in] The MTP code for the object property
    /// \param propDesc [out] This will be populated with the object property description
    /// \return Returns the result as an MTP response code
    MTPResponseCode getDevicePropDesc(MTPDevPropertyCode propCode, MtpDevPropDesc **propDesc);

    /// Destructor
    ~PropertyPod();

private:
    PropertyPod(MtpDeviceInfo *devInfoProvider, MTPExtensionManager *extManager); ///< Private ctor

    PropertyPod(const PropertyPod &) {} ///< Disable copying

    static PropertyPod *m_instance; ///< Singleton instance

    MtpDeviceInfo *m_provider; ///< The device info provider

    MTPExtensionManager *m_extManager; ///< Extensions manager class

    static MtpObjPropDesc m_commonPropDesc[]; ///< Array of property descriptors common to all categories

    static MtpObjPropDesc m_imagePropDesc[]; ///< Array of property descriptors for image category

    static MtpObjPropDesc m_audioPropDesc[]; ///< Array of property descriptors for audio category

    static MtpObjPropDesc m_videoPropDesc[]; ///< Array of property descriptors for video category

    static MtpDevPropDesc m_devicePropDesc[]; ///< Array of descriptors for device properties

    QMap<MTPObjPropertyCode, MtpObjPropDesc *>
        m_objPropMapCommon; ///< Maps the object property code to the property description

    QMap<MTPObjPropertyCode, MtpObjPropDesc *>
        m_objPropMapImage; ///< Maps the object property code to the property description

    QMap<MTPObjPropertyCode, MtpObjPropDesc *>
        m_objPropMapAudio; ///< Maps the object property code to the property description

    QMap<MTPObjPropertyCode, MtpObjPropDesc *>
        m_objPropMapVideo; ///< Maps the object property code to the property description

    QMap<MTPDevPropertyCode, MtpDevPropDesc *> m_devPropMap; ///< Maps the device property code to the property description

    void populateEnumDesc(
        MtpObjPropDesc *desc,
        MTPObjectFormatCategory category); ///< Populates enum form flag into the property desc

    void populateTechObjPropDesc(
        MtpObjPropDesc *desc,
        MTPObjectFormatCategory category); ///< Populates property desc for audio and video types

    bool isTechObjProp(MTPObjPropertyCode code); ///< Returns true if the property code refers to a tech. object property
};
}
#endif
