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

#ifndef OBJECTPROPERTYCACHE_H
#define OBJECTPROPERTYCACHE_H

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QVariant>

#include "mtptypes.h"

/// \brief The ObjectPropertyCache class caches object property values per session
/// for objects in memory for faster access.
///
/// The ObjectPropertyCache class caches object property values for objects in memory for faster access.
/// This prevents going to storage plug-in's and in turn to the db's used by various storage plug-in's. Since
/// object handles are unique across storages, the cache is maintained by the responder: it caches object property
/// values per object. Values are cached first either when a setObjectPropList/Value is called or a getObjectPropList/Value
/// is called for an object already on the responder. Future access to these properties fetches the values from cache. The
/// cache is updated when a property's value is modified. This class is a singleton.
namespace meegomtp1dot0
{
class ObjectPropertyCache
{
    public:
        ObjectPropertyCache() {}

        /// Add/Modify a property-value pair for an object to the cache.
        /// \param handle [in] the object handle which needs to be added/modified
        /// \param propertyCode [in] object property code
        /// \param value [in] object property value
        void add( ObjHandle handle, MTPObjPropertyCode propertyCode, const QVariant &value );

        /// Add/Modify from an MTPObjectPropDesc structure
        /// \param handle [in] the object handle which needs to be added/modified
        /// \propDescVal [in] reference to a propDescVal structure
        void add( ObjHandle handle, const MTPObjPropDescVal &propDescVal );

        /// Add/Modify from a list of MTPObjectPropDesc structures
        /// \param handle [in] the object handle which needs to be added/modified
        /// \propDescValList [in] list of propDescVal structures
        void add( ObjHandle handle, QList<MTPObjPropDescVal> propDescValList );

        /// Remove a property-value pair for an object from the cache.
        /// If there's no code specified then the object itself is removed
        /// If this was the last property in the cache then the object itself is removed
        /// \param handle [in] the object handle
        /// \param propertyCode [in] object property code
        void remove( ObjHandle handle, MTPObjPropertyCode propertyCode = 0x0000 );

        /// Remove from cached based on a MTPObjPropDescVal
        /// If this was the only property in the cache then the object itself is removed
        /// \param handle [in] the object handle
        /// \propDescVal [in] reference to a propDescVal structure
        void remove( ObjHandle handle, const MTPObjPropDescVal &propDescVal );

        /// Remove a set of properties from cache based on a list of MTPObjPropDescVal
        /// If these were the last set of properties in the cache then the object itself is removed
        /// \param handle [in] the object handle which needs to be added/modified
        /// \propDescValList [in] list of propDescVal structures
        void remove( ObjHandle handle, QList<MTPObjPropDescVal> propDescValList );

        /// Get the value for an object given the handle and the object property code.
        /// \param handle [in] the object handle
        /// \param propertyCode [in] the object property code
        /// \param value [out] the object property code's value
        /// \return bool true if this property was cached and found, false otherwise
        bool get( ObjHandle handle, MTPObjPropertyCode propertyCode, QVariant &value );
        
        /// Get the value for an object given the handle and a reference to a MTPObjPropDescVal structure.
        /// The value is populated in the structure.
        /// \param handle [in] the object handle
        /// \param propDescVal [in/out] the object property code
        /// \param value [out] the object property code's value
        /// \return bool true if this property in the MTPObjPropDescVal structure was cached and found, false otherwise
        bool get( ObjHandle handle, MTPObjPropDescVal &propDescVal );

        /// Retrieves multiple property values of given object from the cache.
        ///
        /// The method fills in the property values into \c propDescValList.
        /// Properties for which values aren't found in the cache are moved into
        /// \c notFoundList, so both of these parameters must be provided.
        ///
        /// \param handle [in] the object handle.
        /// \param propDescValList [in, out] list of propDescVal structures
        ///                        whose values to retrieve.
        /// \param notFoundList [out] properties that weren't found are moved
        ///                     into this list.
        ///
        /// \return \c true if the cache was able to fill in all the properties
        ///         (i.e. notFoundList is empty), otherwise \c false.
        bool get(ObjHandle handle, QList<MTPObjPropDescVal> &propDescValList,
                QList<MTPObjPropDescVal> &notFoundList);

        /// clear everything in the cache
        void clear();

        ~ObjectPropertyCache();

    private:
        /// The cache!
        QHash<ObjHandle, QHash<MTPObjPropertyCode,QVariant> > m_propertyMap;
};
}
#endif
