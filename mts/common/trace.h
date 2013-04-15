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

#ifndef PRN_TRACE_H
#define PRN_TRACE_H
// Use logging macros available in sync-fw
#include <buteosyncfw/LogMacros.h>
#include "mts.h"

#define MTP_LOG_LEVEL_CRITICAL      1
#define MTP_LOG_LEVEL_WARNING       2
#define MTP_LOG_LEVEL_INFO          3
#define MTP_LOG_LEVEL_TRACE         4
#define MTP_LOG_LEVEL_TRACE_EXTRA   5

#ifndef MTP_LOG_LEVEL
#define MTP_LOG_LEVEL MTP_LOG_LEVEL_INFO
#endif

/*Critical logs always enabled, use selectively*/
#define MTP_LOG_CRITICAL(msg) LOG_CRITICAL(msg)

#if MTP_LOG_LEVEL >= MTP_LOG_LEVEL_WARNING
#define MTP_LOG_WARNING(msg) LOG_WARNING(msg)
#else
#define MTP_LOG_WARNING(msg)
#endif

#if MTP_LOG_LEVEL >= MTP_LOG_LEVEL_INFO
#define MTP_LOG_INFO(msg) \
    do \
    { \
        LOG_INFO(msg); \
    }while(0)
#else
#define MTP_LOG_INFO(msg)
#endif


/* Tracing macros should produce output only at the highest log level */

#if MTP_LOG_LEVEL < MTP_LOG_LEVEL_TRACE
#define MTP_LOG_TRACE(msg)
#else
#define MTP_LOG_TRACE(msg) \
    do \
    { \
            LOG_DEBUG(msg); \
    }while(0)
#endif

#if MTP_LOG_LEVEL < MTP_LOG_LEVEL_TRACE_EXTRA
#define MTP_HEX_TRACE(p,l)
#define MTP_LOG_TRACE_EXTRA(msg)
#define MTP_FUNC_TRACE()
#else
#define MTP_HEX_TRACE(p,l)
#include <QByteArray>
#define MTP_HEX_TRACE1(p,l)\
do {\
    QByteArray arr = QByteArray::fromRawData((const char*)(p), (l)); \
    MTP_LOG_TRACE(arr); \
} while (0)
#define MTP_LOG_TRACE_EXTRA(msg) MTP_LOG_TRACE(msg)
#define MTP_FUNC_TRACE() \
    do \
    { \
       LOG_TRACE_PLAIN(__PRETTY_FUNCTION__); \
    }while(0)
#endif

#endif /* PRN_TRACE_H */

/*  End of trace.h  */
