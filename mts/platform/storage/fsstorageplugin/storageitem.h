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

#ifndef STORAGEITEM_H
#define STORAGEITEM_H

#include "mtptypes.h"
#include <QString>

namespace meegomtp1dot0
{
class StorageItem
{
#ifdef UT_ON
    friend class FSStoragePlugin_test;
#endif
    friend class FSStoragePlugin;

public:
    /// Constructor
    StorageItem();

    /// Constructor
    ~StorageItem();

     // Allow read access to item path
    const QString &path(void) const {
        return m_path;
    }

private:
    ObjHandle m_handle; ///< the item's handle
    QString m_path; ///< the pathname by which this item is identified in the storage.
    int m_wd; ///< The item's iNotify watch descriptor. This will be -1 for non-directories
    MTPObjectInfo *m_objectInfo; ///< the objectinfo dataset for this item.
    StorageItem *m_parent; ///< this item's parent.
    StorageItem *m_firstChild; ///< this item's first child.
    StorageItem *m_nextSibling; ///< this item's first sibling.
    MtpInt128 m_puoid;
};
}

#endif
