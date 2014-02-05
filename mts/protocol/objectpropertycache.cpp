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

#include "objectpropertycache.h"
#include "trace.h"

using namespace meegomtp1dot0;

const MTPObjPropertyCode ALLPROPS = 0xFFFF;

ObjectPropertyCache* ObjectPropertyCache::m_instance = 0;

ObjectPropertyCache* ObjectPropertyCache::instance()
{
    MTP_FUNC_TRACE();

    if( !m_instance )
    {
        m_instance = new ObjectPropertyCache;
    }
    return m_instance;
}

void ObjectPropertyCache::destroyInstance()
{
    MTP_FUNC_TRACE();

    if( m_instance )
    {
        delete m_instance;
        m_instance = 0;
    }
}

void ObjectPropertyCache::add( ObjHandle handle, MTPObjPropertyCode propertyCode, const QVariant &value )
{
    MTP_FUNC_TRACE();

    //MTP_LOG_WARNING("Property code " << propertyCode << " with value " << value.toString() << " added/updated to cache for object handle " << handle);
    m_propertyMap[handle].insert( propertyCode,value );
}

void ObjectPropertyCache::add( ObjHandle handle, const MTPObjPropDescVal &propDescVal )
{
    MTP_FUNC_TRACE();

    add( handle, propDescVal.propDesc->uPropCode, propDescVal.propVal );
}

void ObjectPropertyCache::add( ObjHandle handle, QList<MTPObjPropDescVal> propDescValList )
{
    MTP_FUNC_TRACE();

    for( QList<MTPObjPropDescVal>::const_iterator itr = propDescValList.constBegin();
         itr != propDescValList.constEnd(); ++itr )
    {
        add( handle, *itr );
    }
}

void ObjectPropertyCache::remove( ObjHandle handle, MTPObjPropertyCode propertyCode )
{
    MTP_FUNC_TRACE();

    //MTP_LOG_WARNING("Property code " << propertyCode << " removed from cache for object handle " << handle);
    m_propertyMap[handle].remove( propertyCode );
    if( m_propertyMap[handle].empty() || 0x0000 == propertyCode )
    {
        m_propertyMap.remove( handle );
    }
}

void ObjectPropertyCache::remove( ObjHandle handle, const MTPObjPropDescVal &propDescVal )
{
    MTP_FUNC_TRACE();

    remove( handle, propDescVal.propDesc->uPropCode );
}

void ObjectPropertyCache::remove( ObjHandle handle, QList<MTPObjPropDescVal> propDescValList )
{
    MTP_FUNC_TRACE();

    for( QList<MTPObjPropDescVal>::const_iterator itr = propDescValList.constBegin();
         itr != propDescValList.constEnd(); ++itr )
    {
        remove( handle, *itr );
    }
}

bool ObjectPropertyCache::get( ObjHandle handle, MTPObjPropertyCode propertyCode, QVariant &value )
{
    MTP_FUNC_TRACE();

    bool found = false;
    if( m_propertyMap.contains( handle ) && m_propertyMap[handle].contains(propertyCode) )
    {
        value = m_propertyMap[handle].value( propertyCode );
        //MTP_LOG_WARNING("Property code " << propertyCode << " with value " << value.toString() << " fetched from cache for object handle " << handle);
        found = true;
    }
    else
    {
        //MTP_LOG_WARNING("Property code " << propertyCode << " not found in cache " << " for object handle " << handle);
    }
    return found;
}

bool ObjectPropertyCache::get( ObjHandle handle, MTPObjPropDescVal &propDescVal )
{
    MTP_FUNC_TRACE();

    return get( handle, propDescVal.propDesc->uPropCode, propDescVal.propVal );
}

bool ObjectPropertyCache::get( ObjHandle handle, QList<MTPObjPropDescVal> &propDescValList,  QList<MTPObjPropDescVal> &notFoundList )
{
    MTP_FUNC_TRACE();

    for( QList<MTPObjPropDescVal>::iterator itr = propDescValList.begin();
         itr != propDescValList.end(); )
    {
        if( !get( handle, *itr ) )
        {
            notFoundList.append( *itr );
            itr = propDescValList.erase( itr );
        }
        else
        {
            ++itr;
        }
    }
    return containsAllProps( handle );
}

void ObjectPropertyCache::clear()
{
    MTP_FUNC_TRACE();

    m_propertyMap.clear();
}

bool ObjectPropertyCache::containsAllProps( ObjHandle handle )
{
    return m_propertyMap[handle].contains( ALLPROPS );
}

void ObjectPropertyCache::setAllProps( ObjHandle handle )
{
    m_propertyMap[handle].insert( ALLPROPS, 0 );
}
