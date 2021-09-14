/*
 * This file is a part of buteo-mtp package.
 *
 * Copyright (C) 2010 Nokia Corporation. All rights reserved.
 *
 * Contact: Deepak Kodihalli <deepak.kodihalli@nokia.com>
 *
 * Copyright (C) 2014 Jolla Ltd.
 *
 * Contact: Jakub Adam <jakub.adam@jollamobile.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer. Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution. Neither the name of Jolla Ltd. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "storageplugin.h"
#include "trace.h"

using namespace meegomtp1dot0;

const quint32 MAX_READ_LEN = 64 * 1024;

MTPResponseCode StoragePlugin::copyData(StoragePlugin *sourceStorage,
                                        ObjHandle source, StoragePlugin *destinationStorage,
                                        ObjHandle destination)
{
    if ( !sourceStorage->checkHandle(source) ||
            !destinationStorage->checkHandle(destination) ) {
        return MTP_RESP_InvalidObjectHandle;
    }

    const MTPObjectInfo *sourceInfo;
    MTPResponseCode result = sourceStorage->getObjectInfo( source, sourceInfo );
    if (result != MTP_RESP_OK ) {
        return result;
    }

    quint32 readOffset = 0;
    quint32 remainingLen = sourceInfo->mtpObjectCompressedSize;
    qint32 readLen = MAX_READ_LEN;
    char readBuffer[MAX_READ_LEN];
    bool txCancelled = false;

    while ( remainingLen && result == MTP_RESP_OK ) {
        readLen = remainingLen >= MAX_READ_LEN ? MAX_READ_LEN : remainingLen;
        result = sourceStorage->readData( source, readBuffer, readLen, readOffset );

        emit sourceStorage->checkTransportEvents( txCancelled );
        if ( txCancelled ) {
            MTP_LOG_WARNING("CopyObject cancelled, aborting file copy...");
            result = destinationStorage->deleteItem( destination,
                                                     MTP_OBF_FORMAT_Undefined );
            return MTP_RESP_GeneralError;
        }

        if ( result == MTP_RESP_OK ) {
            remainingLen -= readLen;
            result = destinationStorage->writeData( destination, readBuffer,
                                                    readLen, readOffset == 0, false );
            readOffset += readLen;
            if ( !remainingLen ) {
                result = destinationStorage->writeData( destination, 0, 0,
                                                        false, true );
            }
        }
    }

    return result;
}
