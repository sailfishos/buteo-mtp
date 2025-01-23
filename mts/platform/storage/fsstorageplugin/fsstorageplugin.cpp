/*
* This file is part of libmeegomtp package
*
* Copyright (c) 2010 Nokia Corporation. All rights reserved.
* Copyright (c) 2013 - 2020 Jolla Ltd.
* Copyright (c) 2020 Open Mobile Platform LLC.
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

#include "fsstorageplugin.h"
#include "fsinotify.h"
#include "storageitem.h"
#include "thumbnailer.h"
#include "trace.h"
#include "../../../protocol/mtpresponder.h"

#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QVariant>
#include <QFileInfo>
#include <QDateTime>
#include <QMetaObject>
#include <QLocale>

#ifndef UT_ON
#include <blkid.h>
#include <libmount.h>
#endif

using namespace meegomtp1dot0;

const quint32 THUMB_MAX_SIZE = (1024 * 48); /* max thumbnailsize */
// Default width and height for thumbnails
const quint32 THUMB_WIDTH = 100;
const quint32 THUMB_HEIGHT = 100;

static quint32 fourcc_wmv3 = 0x574D5633;
static const QString FILENAMES_FILTER_REGEX("[<>:\\\"\\/\\\\\\|\\?\\*\\x0000-\\x001F]");

/* ========================================================================= *
 * Timestamp helpers
 * ========================================================================= */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

static int to_digit(int ch)
{
    int digit = -1;
    switch (ch) {
    case '0' ... '9':
        digit = ch - '0';
        break;
    default:
        break;
    }
    return digit;
}

static bool get_number(const char **ppos, int wdt, int *pvalue)
{
    bool taken = false;
    int value = 0;
    const char *pos = *ppos;

    for (; wdt > 0; --wdt) {
        int digit = to_digit(*pos);
        if (digit < 0 || digit > 9)
            break;
        value *= 10;
        value += digit;
        pos += 1;
    }

    if (wdt == 0) {
        *ppos = pos;
        *pvalue = value;
        taken = true;
    }

    return taken;
}

static bool get_constant(const char **ppos, int chr)
{
    bool taken = false;
    const char *pos = *ppos;

    if (*pos == chr) {
        pos += 1;
        *ppos = pos;
        taken = true;
    }

    return taken;
}

static time_t datetime_to_time_t(const char *dt)
{
    /* Support all allowed formats
     * "yyyymmddThhmmss[.s]"
     * "yyyymmddThhmmss[.s]Z"
     * "yyyymmddThhmmss[.s]+/-hhmm"
     */

    time_t t = -1;

    struct tm tm;

    memset(&tm, 0, sizeof tm);
    tm.tm_wday = -1;
    tm.tm_yday = -1;
    tm.tm_isdst = -1;

    bool east = true;
    int off_h = 0;
    int off_m = 0;
    int off_s = 0;

    const char *pos = dt;

    /* Parse year-mon-day & reject obviously bogus values */
    if (!get_number(&pos, 4, &tm.tm_year))
        goto EXIT;

    if (tm.tm_year < 1900)
        goto EXIT;

    if (!get_number(&pos, 2, &tm.tm_mon))
        goto EXIT;

    if (tm.tm_mon < 1 || tm.tm_mon > 12)
        goto EXIT;

    if (!get_number(&pos, 2, &tm.tm_mday))
        goto EXIT;

    if (tm.tm_mday < 1 || tm.tm_mday > 31)
        goto EXIT;

    /* Parse hour-minute-second[.decisecond] */
    if (!get_constant(&pos, 'T'))
        goto EXIT;

    if (!get_number(&pos, 2, &tm.tm_hour))
        goto EXIT;

    if (tm.tm_hour < 0 || tm.tm_hour > 23)
        goto EXIT;

    if (!get_number(&pos, 2, &tm.tm_min))
        goto EXIT;

    if (tm.tm_min < 0 || tm.tm_min > 59)
        goto EXIT;

    if (!get_number(&pos, 2, &tm.tm_sec))
        goto EXIT;

    if (tm.tm_sec < 0 || tm.tm_sec > 59)
        goto EXIT;

    if (get_constant(&pos, '.')) {
        /* Formatting of the fraction is checked, but
         * otherwise it is ignored. */
        int dsec = 0;

        if (!get_number(&pos, 1, &dsec))
            goto EXIT;

        if (dsec < 0 || dsec > 9)
            goto EXIT;
    }

    /* Adjust to expected base */
    tm.tm_mon -= 1;
    tm.tm_year -= 1900;

    /* The rest depends on time zone */

    switch (*pos) {
    case 0x0:
        /* Local time with unspecified tz */
        t = mktime(&tm);
        break;

    case 'Z':
        /* UTC time */
        t = timegm(&tm);
        break;

    case '-':
        east = false;
    /* FALLTHRU */
    case '+':
        /* Localtime + utc offset */
        ++pos;
        if (!get_number(&pos, 2, &off_h))
            goto EXIT;

        if (off_h < 0 || off_h > 23)
            goto EXIT;

        if (!get_number(&pos, 2, &off_m))
            goto EXIT;

        if (off_m < 0 || off_m > 59)
            goto EXIT;

        off_s = (off_h * 60 + off_m) * 60;

        if (!east)
            off_s = -off_s;

        t = mktime(&tm);

        /* Adjust according to tz difference */
        t += (off_s - tm.tm_gmtoff);
        break;

    default:
        goto EXIT;
    }

    /* Something unhandled remaining? */
    if (*pos != 0)
        t = -1;

EXIT:
    return t;
}

static bool datetime_from_time_t(char *dt, size_t size, time_t t)
{
    /* Generate in format
     * "yyyymmddThhmmss+hhmm"
     */
    bool success = false;

    struct tm tm;

    bool east = true;
    long off_s = 0;
    long off_m = 0;
    long off_h = 0;
    int rc = 0;

    if (t == -1)
        goto EXIT;

    memset(&tm, 0, sizeof tm);
    localtime_r(&t, &tm);

    off_s = tm.tm_gmtoff;
    if (off_s < 0)
        off_s = -off_s, east = false;
    off_m = off_s / 60;
    off_h = off_m / 60;
    off_m %= 60;

    rc = snprintf(
        dt,
        size,
        "%04d%02d%02dT%02d%02d%02d%c%02ld%02ld",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        east ? '+' : '-',
        off_h,
        off_m);

    if (rc >= 0 && rc < (int) size)
        success = true;

EXIT:
    if (!success)
        *dt = 0;

    return success;
}

static time_t datetime_to_time_t(const QString &dt)
{
    QByteArray utf8 = dt.toUtf8();
    return datetime_to_time_t(utf8.constData());
}

static QString datetime_from_time_t(time_t t)
{
    QString dt;
    char buf[64];
    if (datetime_from_time_t(buf, sizeof buf, t))
        dt.append(buf);
    return dt;
}

#if MTP_LOG_LEVEL >= MTP_LOG_LEVEL_TRACE
static QString timeRepr(time_t t)
{
    QString res;
    struct tm tm;

    memset(&tm, 0, sizeof tm);
    if (localtime_r(&t, &tm) != 0) {
        char buf[64];
        size_t n = strftime(buf, sizeof buf - 1, "%a %b %d %H:%M:%S %Y", &tm);
        if (n > 0 && n < sizeof buf) {
            buf[n] = 0;
            res.append(buf);
        }
    }
    return res;
}
#endif

static time_t file_get_mtime(const QString &path)
{
    time_t t = -1;
    QByteArray utf8 = path.toUtf8();
    struct stat st;

    if (stat(utf8.constData(), &st) == -1)
        MTP_LOG_WARNING(path << "could not get mtime");
    else {
        t = st.st_mtime;
        MTP_LOG_TRACE(path << "mtime is" << timeRepr(t));
    }

    return t;
}

static void file_set_mtime(const QString &path, time_t t)
{
    if (t != -1) {
        QByteArray utf8 = path.toUtf8();
        struct timeval tv[2] = {{t, 0}, {t, 0}};
        if (utimes(utf8.constData(), tv) == -1) {
            MTP_LOG_WARNING(path << "could not set mtime");
        } else {
            MTP_LOG_TRACE(path << "mtime set to" << timeRepr(t));
        }
    }
}

/************************************************************
 * FSStoragePlugin::FSStoragePlugin
 ***********************************************************/
FSStoragePlugin::FSStoragePlugin(
    quint32 storageId, MTPStorageType storageType, QString storagePath, QString volumeLabel, QString storageDescription)
    : StoragePlugin(storageId)
    , m_storagePath(QDir(storagePath).canonicalPath())
    , m_root(0)
    , m_writeObjectHandle(0)
    , m_largestPuoid(0)
    , m_reportedFreeSpace(0)
    , m_dataFile(0)
{
    m_storageInfo.storageType = storageType;
    m_storageInfo.accessCapability = MTP_STORAGE_ACCESS_ReadWrite;
    m_storageInfo.filesystemType = MTP_FILE_SYSTEM_TYPE_GenHier;
    m_storageInfo.freeSpaceInObjects = 0xFFFFFFFF;
    m_storageInfo.storageDescription = storageDescription;
    m_storageInfo.volumeLabel = volumeLabel;

    // Ensure root folder of this storage exists.
    QDir().mkpath(m_storagePath);

    QByteArray ba = m_storagePath.toUtf8();
    struct statvfs stat;
    if (statvfs(ba.constData(), &stat)) {
        m_storageInfo.maxCapacity = 0;
        m_storageInfo.freeSpace = 0;
    } else {
        m_storageInfo.maxCapacity = (quint64) stat.f_blocks * stat.f_bsize;
        m_storageInfo.freeSpace = (quint64) stat.f_bavail * stat.f_bsize;
    }

    m_mtpPersistentDBPath = QDir::homePath() + "/.local/mtp";
    QDir dir = QDir(m_mtpPersistentDBPath);
    if (!dir.exists()) {
        dir.mkpath(m_mtpPersistentDBPath);
    }

    m_puoidsDbPath = m_mtpPersistentDBPath + "/mtppuoids";
    // Remove legacy PUOID database if it exists.
    QFile::remove(m_puoidsDbPath);
    m_puoidsDbPath += '-' + volumeLabel + '-' + filesystemUuid();

    m_objectReferencesDbPath = m_mtpPersistentDBPath + "/mtpreferences";

    buildSupportedFormatsList();

    // Populate puoids stored persistently and store them in the puoids map.
    populatePuoids();

    m_thumbnailer = new Thumbnailer();
    QObject::connect(
        m_thumbnailer, SIGNAL(thumbnailReady(const QString &)), this, SLOT(receiveThumbnail(const QString &)));
    clearCachedInotifyEvent(); // initialize
    m_inotify = new FSInotify(IN_MOVE | IN_CREATE | IN_DELETE | IN_CLOSE_WRITE);
    QObject::connect(
        m_inotify,
        SIGNAL(inotifyEventSignal(struct inotify_event *)),
        this,
        SLOT(inotifyEventSlot(struct inotify_event *)));

    MTP_LOG_INFO(storagePath << "exported as FS storage" << volumeLabel << '(' << storageDescription << ')');

#ifdef UT_ON
    m_testHandleProvider = 0;
#endif
}

/************************************************************
 * bool FSStoragePlugin::enumerateStorage()
 ***********************************************************/
bool FSStoragePlugin::enumerateStorage()
{
    bool result = true;

    // Do the real work asynchronously. Queue a call to it, so that
    // it can run directly from the event loop without making our caller wait.
    QMetaObject::invokeMethod(this, "enumerateStorage_worker", Qt::QueuedConnection);

    return result;
}

void FSStoragePlugin::enumerateStorage_worker()
{
    // Add the root folder to storage
    addToStorage(m_storagePath, &m_root);

    removeUnusedPuoids();

    // Populate object references stored persistently and add them to the storage.
    populateObjectReferences();

    /* Delay from waiting for "storage ready" is known cause
     * of issues. To ease debugging log when it is finished. */
    MTP_LOG_WARNING("storage" << m_storageId << "is ready");

    emit storagePluginReady(m_storageId);

    // enable thumbnailer after fs scan is finished
    m_thumbnailer->enableThumbnailing();
}

/************************************************************
 * FSStoragePlugin::~FSStoragePlugin
 ***********************************************************/
FSStoragePlugin::~FSStoragePlugin()
{
    storePuoids();
    storeObjectReferences();

    for (QHash<ObjHandle, StorageItem *>::iterator i = m_objectHandlesMap.begin(); i != m_objectHandlesMap.end(); ++i) {
        if (i.value()) {
            delete i.value();
        }
    }

    delete m_thumbnailer;
    m_thumbnailer = 0;
    delete m_inotify;
    m_inotify = 0;
}

void FSStoragePlugin::disableObjectEvents()
{
    for (QHash<ObjHandle, StorageItem *>::iterator i = m_objectHandlesMap.begin(); i != m_objectHandlesMap.end(); ++i) {
        if (i.value()) {
            i.value()->setEventsEnabled(false);
        }
    }
}

/************************************************************
 * void FSStoragePlugin::populatePuoids
 ***********************************************************/
void FSStoragePlugin::populatePuoids()
{
    QFile file(m_puoidsDbPath);
    if (!file.open(QIODevice::ReadOnly) || !file.size()) {
        return;
    }

    qint32 bytesRead = 0;
    char *name = 0;
    quint32 noOfPuoids = 0, pathnameLen = 0;
    MtpInt128 puoid;

    // Read the last used puoid
    bytesRead = file.read(reinterpret_cast<char *>(&m_largestPuoid), sizeof(MtpInt128));
    if (0 >= bytesRead) {
        return;
    }

    // Read the no. of puoids
    bytesRead = file.read(reinterpret_cast<char *>(&noOfPuoids), sizeof(quint32));
    if (0 >= bytesRead) {
        return;
    }

    for (quint32 i = 0; i < noOfPuoids; ++i) {
        // read pathname len
        bytesRead = file.read(reinterpret_cast<char *>(&pathnameLen), sizeof(quint32));
        if (0 >= bytesRead) {
            return;
        }

        // read the pathname
        name = new char[pathnameLen + 1];
        bytesRead = file.read(name, pathnameLen);
        if (0 >= bytesRead) {
            delete[] name;
            return;
        }
        name[pathnameLen] = '\0';

        // read the puoid
        bytesRead = file.read(reinterpret_cast<char *>(&puoid), sizeof(MtpInt128));
        if (0 >= bytesRead) {
            delete[] name;
            return;
        }

        // Store this in puoids map
        m_puoidsMap[name] = puoid;

        delete[] name;
    }
}

/************************************************************
 * void FSStoragePlugin::removeUnusedPuoids
 ***********************************************************/
void FSStoragePlugin::removeUnusedPuoids()
{
    QHash<QString, MtpInt128>::iterator i = m_puoidsMap.begin();
    while (i != m_puoidsMap.end()) {
        if (!m_pathNamesMap.contains(i.key())) {
            i = m_puoidsMap.erase(i);
        } else {
            ++i;
        }
    }
}

/************************************************************
 * void FSStoragePlugin::storePuoids
 ***********************************************************/
void FSStoragePlugin::storePuoids()
{
    // Store in the following order
    // Last used PUOID (128 bits)
    // No. Of puoids ( 4 bytes )
    // length of object's pathname : pathname : puoid ( for each puoid )
    qint32 bytesWritten = -1;
    QFile file(m_puoidsDbPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    // Write the last used puoid.
    bytesWritten = file.write(reinterpret_cast<const char *>(&m_largestPuoid), sizeof(MtpInt128));
    if (-1 == bytesWritten) {
        MTP_LOG_WARNING("ERROR writing last used puoid to db!!");
        file.resize(0);
        file.close();
        return;
    }

    // Write the no of puoids
    quint32 noOfPuoids = m_puoidsMap.size();
    bytesWritten = file.write(reinterpret_cast<const char *>(&noOfPuoids), sizeof(quint32));
    if (-1 == bytesWritten) {
        MTP_LOG_WARNING("ERROR writing no of puoids to db!!");
        file.resize(0);
        file.close();
        return;
    }

    // Write info for each puoid
    for (QHash<QString, MtpInt128>::const_iterator i = m_puoidsMap.constBegin(); i != m_puoidsMap.constEnd(); ++i) {
        quint32 pathnameLen = i.key().size();
        QString pathname = i.key();
        MtpInt128 puoid = i.value();

        // Write length of path name
        bytesWritten = file.write(reinterpret_cast<const char *>(&pathnameLen), sizeof(quint32));
        if (-1 == bytesWritten) {
            MTP_LOG_WARNING("ERROR writing pathname len to db!!");
            file.resize(0);
            file.close();
            return;
        }

        // Write path name
        QByteArray ba = pathname.toUtf8();
        bytesWritten = file.write(reinterpret_cast<const char *>(ba.constData()), pathnameLen);
        if (-1 == bytesWritten) {
            MTP_LOG_WARNING("ERROR writing pathname to db!!");
            file.resize(0);
            file.close();
            return;
        }

        // Write puoid
        bytesWritten = file.write(reinterpret_cast<const char *>(&puoid), sizeof(MtpInt128));
        if (-1 == bytesWritten) {
            MTP_LOG_WARNING("ERROR writing puoid to db!!");
            file.resize(0);
            file.close();
            return;
        }
    }
}

/************************************************************
 * void FSStoragePlugin::buildSupportedFormatsList
 ***********************************************************/
void FSStoragePlugin::buildSupportedFormatsList()
{
    /* The m_formatByExtTable lookup table is used for determining
     * mtp object format of files present in the device file system
     * and is accessed only from:
     *   FSStoragePlugin::getObjectFormatByExtension()
     *
     * Files with extensions not defined here will be handled as
     * MTP_OBF_FORMAT_Undefined - which should allow pc side software
     * to download any files from the device.
     *
     * Providing "more correct" mapping might however be desirable
     * in cases where there is reason to assume pc side sw will use
     * the mtp object format for something that is relevant for the
     * users (icons, thumbnails, binding to applications, etc).
     */

    m_formatByExtTable["pla"] = MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist;
    m_formatByExtTable["wav"] = MTP_OBF_FORMAT_WAV;
    m_formatByExtTable["mp3"] = MTP_OBF_FORMAT_MP3;
    m_formatByExtTable["ogg"] = MTP_OBF_FORMAT_OGG;
    m_formatByExtTable["txt"] = MTP_OBF_FORMAT_Text;
    m_formatByExtTable["htm"] = MTP_OBF_FORMAT_HTML;
    m_formatByExtTable["html"] = MTP_OBF_FORMAT_HTML;
    m_formatByExtTable["wmv"] = MTP_OBF_FORMAT_WMV;
    m_formatByExtTable["avi"] = MTP_OBF_FORMAT_AVI;
    m_formatByExtTable["mpg"] = MTP_OBF_FORMAT_MPEG;
    m_formatByExtTable["mpeg"] = MTP_OBF_FORMAT_MPEG;

    m_formatByExtTable["bmp"] = MTP_OBF_FORMAT_BMP;
    m_formatByExtTable["gif"] = MTP_OBF_FORMAT_GIF;
    m_formatByExtTable["jpg"] = MTP_OBF_FORMAT_EXIF_JPEG;
    m_formatByExtTable["jpeg"] = MTP_OBF_FORMAT_EXIF_JPEG;
    m_formatByExtTable["png"] = MTP_OBF_FORMAT_PNG;
    m_formatByExtTable["tif"] = MTP_OBF_FORMAT_TIFF;
    m_formatByExtTable["tiff"] = MTP_OBF_FORMAT_TIFF;

    m_formatByExtTable["wma"] = MTP_OBF_FORMAT_WMA;
    m_formatByExtTable["aac"] = MTP_OBF_FORMAT_AAC;
    m_formatByExtTable["mp4"] = MTP_OBF_FORMAT_MP4_Container;
    m_formatByExtTable["3gp"] = MTP_OBF_FORMAT_3GP_Container;
    m_formatByExtTable["pls"] = MTP_OBF_FORMAT_PLS_Playlist;
    m_formatByExtTable["alb"] = MTP_OBF_FORMAT_Abstract_Audio_Album;

    m_formatByExtTable["pbm"] = MTP_OBF_FORMAT_Unknown_Image_Object;
    m_formatByExtTable["pcx"] = MTP_OBF_FORMAT_Unknown_Image_Object;
    m_formatByExtTable["pgm"] = MTP_OBF_FORMAT_Unknown_Image_Object;
    m_formatByExtTable["ppm"] = MTP_OBF_FORMAT_Unknown_Image_Object;
    m_formatByExtTable["xpm"] = MTP_OBF_FORMAT_Unknown_Image_Object;
    m_formatByExtTable["xwd"] = MTP_OBF_FORMAT_Unknown_Image_Object;

    m_formatByExtTable["3g2"] = MTP_OBF_FORMAT_3G2;
    m_formatByExtTable["aiff"] = MTP_OBF_FORMAT_AIFF;
    m_formatByExtTable["aif"] = MTP_OBF_FORMAT_AIFF;
    m_formatByExtTable["mp2"] = MTP_OBF_FORMAT_MP2;
    m_formatByExtTable["flac"] = MTP_OBF_FORMAT_FLAC;
    m_formatByExtTable["mrk"] = MTP_OBF_FORMAT_DPOF;
    m_formatByExtTable["wpl"] = MTP_OBF_FORMAT_WPL_Playlist;
    m_formatByExtTable["m3u"] = MTP_OBF_FORMAT_M3U_Playlist;
    m_formatByExtTable["m3u8"] = MTP_OBF_FORMAT_M3U_Playlist;
    m_formatByExtTable["mpl"] = MTP_OBF_FORMAT_MPL_Playlist;
    m_formatByExtTable["mpls"] = MTP_OBF_FORMAT_MPL_Playlist;
    m_formatByExtTable["asx"] = MTP_OBF_FORMAT_ASX_Playlist;
    m_formatByExtTable["xml"] = MTP_OBF_FORMAT_XML_Document;
    m_formatByExtTable["mht"] = MTP_OBF_FORMAT_MHT_Compiled_HTML_Document;
    m_formatByExtTable["mhtml"] = MTP_OBF_FORMAT_MHT_Compiled_HTML_Document;
    m_formatByExtTable["asf"] = MTP_OBF_FORMAT_ASF;
    m_formatByExtTable["mts"] = MTP_OBF_FORMAT_AVCHD;
    m_formatByExtTable["m2ts"] = MTP_OBF_FORMAT_AVCHD;
    m_formatByExtTable["ts"] = MTP_OBF_FORMAT_DVB_TS;
    m_formatByExtTable["jp2"] = MTP_OBF_FORMAT_JP2;
    m_formatByExtTable["jpg2"] = MTP_OBF_FORMAT_JP2;
    m_formatByExtTable["jpx"] = MTP_OBF_FORMAT_JPX;
    m_formatByExtTable["wbmp"] = MTP_OBF_FORMAT_WBMP;
    m_formatByExtTable["fpx"] = MTP_OBF_FORMAT_FlashPix;
    m_formatByExtTable["dib"] = MTP_OBF_FORMAT_BMP;
    m_formatByExtTable["crw"] = MTP_OBF_FORMAT_CIFF;
    m_formatByExtTable["jfif"] = MTP_OBF_FORMAT_JFIF;
    m_formatByExtTable["jfi"] = MTP_OBF_FORMAT_JFIF;
    m_formatByExtTable["pcd"] = MTP_OBF_FORMAT_PCD;
    m_formatByExtTable["pict"] = MTP_OBF_FORMAT_PICT;
    m_formatByExtTable["pct"] = MTP_OBF_FORMAT_PICT;
    m_formatByExtTable["pic"] = MTP_OBF_FORMAT_PICT;
    m_formatByExtTable["jxr"] = MTP_OBF_FORMAT_JPEG_XR;
    m_formatByExtTable["hdp"] = MTP_OBF_FORMAT_JPEG_XR;
    m_formatByExtTable["wdp"] = MTP_OBF_FORMAT_JPEG_XR;
    m_formatByExtTable["m4a"] = MTP_OBF_FORMAT_M4A;
    m_formatByExtTable["aa"] = MTP_OBF_FORMAT_Audible;
    m_formatByExtTable["aax"] = MTP_OBF_FORMAT_Audible;
    m_formatByExtTable["qcp"] = MTP_OBF_FORMAT_QCELP;
    m_formatByExtTable["amr"] = MTP_OBF_FORMAT_AMR;

    // Populate format code->MIME type map
    m_imageMimeTable[MTP_OBF_FORMAT_BMP] = "image/bmp";
    m_imageMimeTable[MTP_OBF_FORMAT_GIF] = "image/gif";
    m_imageMimeTable[MTP_OBF_FORMAT_EXIF_JPEG] = "image/jpeg";
    m_imageMimeTable[MTP_OBF_FORMAT_PNG] = "image/png";
    m_imageMimeTable[MTP_OBF_FORMAT_TIFF] = "image/tiff";

    m_imageMimeTable[MTP_OBF_FORMAT_Unknown_Image_Object] = "application/octet-stream";
}

/************************************************************
 * quint32 FSStoragePlugin::requestNewObjectHandle
 ***********************************************************/
quint32 FSStoragePlugin::requestNewObjectHandle()
{
    ObjHandle handle = 0;
    emit objectHandle(handle);
#ifdef UT_ON
    if (handle == 0) {
        /* During unit testing, we might not have a StorageFactory instance to
         * give us handles. Use our own provider in this case. */
        handle = ++m_testHandleProvider;
    }
#endif
    return handle;
}

void FSStoragePlugin::requestNewPuoid(MtpInt128 &newPuoid)
{
    emit puoid(newPuoid);
    m_largestPuoid = newPuoid;
}

void FSStoragePlugin::getLargestPuoid(MtpInt128 &puoid)
{
    puoid = m_largestPuoid;
}

MTPResponseCode FSStoragePlugin::createFile(const QString &path, MTPObjectInfo *info)
{
    // Create the file in the file system.
    QFile file(path);

    bool already_exists = file.exists();

    if (!file.open(QIODevice::ReadWrite)) {
        MTP_LOG_WARNING("failed to create file:" << path);
        switch (file.error()) {
        case QFileDevice::OpenError:
            return MTP_RESP_AccessDenied;
        default:
            return MTP_RESP_GeneralError;
        }
    }

    if (!already_exists) {
        /* When creating new files, prefer using the real gid
         * (= "nemo") over the effective gid (= "privileged"). */
        if (fchown(file.handle(), getuid(), getgid()) == -1) {
            MTP_LOG_WARNING("failed to set file:" << path << " ownership");
        }
    }

    /* Resize to expected content length */
    quint64 size = info ? info->mtpObjectCompressedSize : 0;

    if (size > 0) {
        if (fallocate(file.handle(), 0, 0, size) == -1) {
            MTP_LOG_WARNING("failed to set file:" << path << " to size:" << size << " err:" << strerror(errno));
        }
    } else {
        if (ftruncate(file.handle(), 0) == -1) {
            MTP_LOG_WARNING("failed to truncate file:" << path << " err:" << strerror(errno));
        }
    }

    file.close();

    MTP_LOG_TRACE("created file:" << path << " with size:" << size);

    /* Touch to requested modify time */
    if (info) {
        time_t t = datetime_to_time_t(info->mtpModificationDate);
        file_set_mtime(path, t);
    }

    return MTP_RESP_OK;
}

MTPResponseCode FSStoragePlugin::createDirectory(const QString &path)
{
    // Create the directory in the file system.
    QDir dir = QDir(path);
    if (!dir.exists()) {
        if (!dir.mkpath(path)) {
            MTP_LOG_WARNING("failed to create directory:" << path);
            return MTP_RESP_GeneralError;
        }
    }

    MTP_LOG_TRACE("created directory:" << path);

    return MTP_RESP_OK;
}

/************************************************************
 * void FSStoragePlugin::linkChildStorageItem
 ***********************************************************/
void FSStoragePlugin::linkChildStorageItem(StorageItem *childStorageItem, StorageItem *parentStorageItem)
{
    if (!childStorageItem || !parentStorageItem) {
        return;
    }
    childStorageItem->m_parent = parentStorageItem;

    // Parent has no children
    if (!parentStorageItem->m_firstChild) {
        parentStorageItem->m_firstChild = childStorageItem;
    }
    // Parent already has children
    else {
        // Insert as first child
        StorageItem *oldFirstChild = parentStorageItem->m_firstChild;
        parentStorageItem->m_firstChild = childStorageItem;
        childStorageItem->m_nextSibling = oldFirstChild;
    }
}

/************************************************************
 * void FSStoragePlugin::unlinkChildStorageItem
 ***********************************************************/
void FSStoragePlugin::unlinkChildStorageItem(StorageItem *childStorageItem)
{
    if (!childStorageItem || !childStorageItem->m_parent) {
        return;
    }

    // If this is the first child.
    if (childStorageItem->m_parent->m_firstChild == childStorageItem) {
        childStorageItem->m_parent->m_firstChild = childStorageItem->m_nextSibling;
    }
    // not the first child
    else {
        StorageItem *itr = childStorageItem->m_parent->m_firstChild;
        while (itr && itr->m_nextSibling != childStorageItem) {
            itr = itr->m_nextSibling;
        }
        if (itr) {
            itr->m_nextSibling = childStorageItem->m_nextSibling;
        }
    }
    childStorageItem->m_nextSibling = 0;
}

/************************************************************
 * StorageItem* FSStoragePlugin::findStorageItemByPath
 ***********************************************************/
StorageItem *FSStoragePlugin::findStorageItemByPath(const QString &path)
{
    StorageItem *storageItem = 0;
    ObjHandle handle;
    if (m_pathNamesMap.contains(path)) {
        handle = m_pathNamesMap.value(path);
        storageItem = m_objectHandlesMap.value(handle);
    }
    return storageItem;
}

FSStoragePlugin::SymLinkPolicy FSStoragePlugin::s_symLinkPolicy = SymLinkPolicy::Undefined;

FSStoragePlugin::SymLinkPolicy FSStoragePlugin::symLinkPolicy()
{
    if (s_symLinkPolicy == SymLinkPolicy::Undefined) {
        FSStoragePlugin::SymLinkPolicy usePolicy = SymLinkPolicy::Default;
        QByteArray envData = qgetenv("BUTEO_MTP_SYMLINK_POLICY");
        QString envValue = QString::fromUtf8(envData.data()).toLower();
        if (envValue == "allowall" || envValue == "allow")
            usePolicy = SymLinkPolicy::AllowAll;
        else if (envValue == "allowwithinstorage" || envValue == "storage")
            usePolicy = SymLinkPolicy::AllowWithinStorage;
        else if (envValue == "denyall" || envValue == "deny")
            usePolicy = SymLinkPolicy::DenyAll;
        else if (!envValue.isEmpty())
            MTP_LOG_WARNING("unknown SymLinkPolicy:" << envValue);
        setSymLinkPolicy(usePolicy);
    }
    return s_symLinkPolicy;
}

void FSStoragePlugin::setSymLinkPolicy(FSStoragePlugin::SymLinkPolicy policy)
{
    static const char *const lut[] = {
        "Undefined",
        "AllowAll",
        "AllowWithinStorage",
        "DenyAll",
    };

    if (s_symLinkPolicy != policy) {
        MTP_LOG_INFO("SymLinkPolicy changed:" << lut[s_symLinkPolicy] << "->" << lut[policy]);
        s_symLinkPolicy = policy;
    }
}

/************************************************************
 * MTPrespCode FSStoragePlugin::addToStorage
 ***********************************************************/
MTPResponseCode FSStoragePlugin::addToStorage(
    const QString &path,
    StorageItem **storageItem,
    MTPObjectInfo *info,
    bool sendEvent,
    bool createIfNotExist,
    ObjHandle handle)
{
    if (m_excludePaths.contains(path)) {
        return MTP_RESP_AccessDenied;
    }

    // Handle symbolic link policy
    QFileInfo pathInfo(path);
    if (pathInfo.isSymLink()) {
        QString targetPath(pathInfo.canonicalFilePath());
        if (targetPath.isEmpty()) {
            MTP_LOG_WARNING("excluded broken symlink:" << path);
            return MTP_RESP_AccessDenied;
        }
        switch (symLinkPolicy()) {
        case SymLinkPolicy::AllowAll:
            break;
        case SymLinkPolicy::AllowWithinStorage: {
            // NB: m_storagePath is in canonical form
            int prefixLen = m_storagePath.length();
            if (targetPath.length() <= prefixLen || targetPath[prefixLen] != '/'
                || !targetPath.startsWith(m_storagePath)) {
                MTP_LOG_INFO("excluded out-of-storage symlink:" << path);
                return MTP_RESP_AccessDenied;
            }
        }
        break;
        default:
        case SymLinkPolicy::DenyAll:
            MTP_LOG_INFO("excluded symlink:" << path);
            return MTP_RESP_AccessDenied;
        }
    }

    // If we already have StorageItem for given path...
    if (m_pathNamesMap.contains(path)) {
        if (storageItem) {
            *storageItem = findStorageItemByPath(path);
        }
        return MTP_RESP_OK;
    }

    QScopedPointer<StorageItem> item(new StorageItem);
    item->m_path = path;

    QString parentPath(item->m_path.left(item->m_path.lastIndexOf('/')));
    StorageItem *parentItem = findStorageItemByPath(parentPath);
    linkChildStorageItem(item.data(), parentItem ? parentItem : m_root);

    if (info) {
        item->m_objectInfo = new MTPObjectInfo(*info);
        item->m_objectInfo->mtpStorageId = storageId();
    } else {
        populateObjectInfo(item.data());
    }

    // Root of the storage should have handle of 0.
    if (path == m_storagePath) {
        item->m_handle = 0;
    }
    // Assign a handle for this item and link to parent item.
    else {
        item->m_handle = handle ? handle : requestNewObjectHandle();
    }

    MTPResponseCode result;
    // Create file or directory
    switch (item->m_objectInfo->mtpObjectFormat) {
    // Directory.
    case MTP_OBF_FORMAT_Association: {
        if (createIfNotExist) {
            result = createDirectory(item->m_path);
            if (result != MTP_RESP_OK) {
                unlinkChildStorageItem(item.data());
                return result;
            }
        }

        addWatchDescriptor(item.data());

        addItemToMaps(item.data());

        // Recursively add StorageItems for the contents of the directory.
        QDir dir(item->m_path);
        dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
        QFileInfoList dirContents = dir.entryInfoList();
        int work = 0;
        foreach (const QFileInfo &info, dirContents) {
            if (work++ % 16 == 0) {
                QCoreApplication::sendPostedEvents();
                QCoreApplication::processEvents();
            }
            addToStorage(info.absoluteFilePath(), 0, 0, sendEvent, createIfNotExist);
        }
        break;
    }
    // File.
    default:
        if (createIfNotExist) {
            result = createFile(item->m_path, info);
            if (result != MTP_RESP_OK) {
                unlinkChildStorageItem(item.data());
                return result;
            }
        }

        addItemToMaps(item.data());
        // Add this PUOID to the PUOID->Object Handles map
        m_puoidToHandleMap[item->m_puoid] = item->m_handle;
        break;
    }

    if (sendEvent) {
        QVector<quint32> eventParams;
        eventParams.append(item->m_handle);
        emit eventGenerated(MTP_EV_ObjectAdded, eventParams);
    }

    // Dates from our device
    item->m_objectInfo->mtpCaptureDate = getCreatedDate(item.data());
    item->m_objectInfo->mtpModificationDate = getModifiedDate(item.data());

    if (storageItem) {
        *storageItem = item.take();
    } else {
        item.take();
    }

    return MTP_RESP_OK;
}

void FSStoragePlugin::addItemToMaps(StorageItem *item)
{
    // Path names map.
    m_pathNamesMap[item->m_path] = item->m_handle;

    // Object handles map.
    m_objectHandlesMap[item->m_handle] = item;

    if (!m_puoidsMap.contains(item->m_path)) {
        // Assign a new puoid
        requestNewPuoid(item->m_puoid);
        // Add the puoid to the map.
        m_puoidsMap[item->m_path] = item->m_puoid;
    } else {
        // Use the persistent puoid.
        item->m_puoid = m_puoidsMap[item->m_path];
    }
}

/************************************************************
 * MTPrespCode FSStoragePlugin::addItem
 ***********************************************************/
MTPResponseCode FSStoragePlugin::addItem(ObjHandle &parentHandle, ObjHandle &handle, MTPObjectInfo *info)
{
    MTPResponseCode response;
    StorageItem *storageItem = 0;

    if (!info) {
        return MTP_RESP_Invalid_Dataset;
    }

    // Initiator has left it to us to choose the parent, choose root folder.
    if (0xFFFFFFFF == info->mtpParentObject) {
        info->mtpParentObject = 0x00000000;
    }

    // Check if the parent is valid.
    if (!checkHandle(info->mtpParentObject)) {
        return MTP_RESP_InvalidParentObject;
    }

    QString path = m_objectHandlesMap[info->mtpParentObject]->m_path + "/" + info->mtpFileName;

    // Add the object ( file/dir ) to the filesystem storage.
    response = addToStorage(path, &storageItem, info, false, true);
    if (storageItem) {
        handle = storageItem->m_handle;
        parentHandle = storageItem->m_parent ? storageItem->m_parent->m_handle : 0x00000000;
    }

    return response;
}

MTPResponseCode FSStoragePlugin::copyHandle(StoragePlugin *sourceStorage, ObjHandle source, ObjHandle parent)
{
    if (m_objectHandlesMap.contains(source)) {
        return MTP_RESP_Invalid_Dataset;
    }

    // Initiator has left it to us to choose the parent; choose root folder.
    if (parent == 0xFFFFFFFF) {
        parent = 0;
    }

    if (!checkHandle(parent)) {
        return MTP_RESP_InvalidParentObject;
    }

    const MTPObjectInfo *info;
    MTPResponseCode result = sourceStorage->getObjectInfo(source, info);
    if (result != MTP_RESP_OK) {
        return result;
    }

    MTPObjectInfo newInfo(*info);
    newInfo.mtpParentObject = parent;

    QString path = m_objectHandlesMap[newInfo.mtpParentObject]->m_path + "/" + newInfo.mtpFileName;

    result = addToStorage(path, 0, &newInfo, false, true, source);
    if (result != MTP_RESP_OK) {
        return result;
    }

    if (newInfo.mtpObjectFormat == MTP_OBF_FORMAT_Association) {
        // Directory, copy recursively.
        QVector<ObjHandle> childHandles;
        sourceStorage->getObjectHandles(0, source, childHandles);
        foreach (ObjHandle handle, childHandles) {
            result = copyHandle(sourceStorage, handle, source);
            if (result != MTP_RESP_OK) {
                return result;
            }
        }

        return MTP_RESP_OK;
    } else {
        // Source and destination handles are the same, though each
        // in a different storage.
        return copyData(sourceStorage, source, this, source);
    }
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::deleteItem
 ***********************************************************/
MTPResponseCode FSStoragePlugin::deleteItem(const ObjHandle &handle, const MTPObjFormatCode &formatCode)
{
    // If handle == 0xFFFFFFFF, that means delete all objects that can be deleted ( this could be filered by fmtCode )
    bool deletedSome = false;
    bool failedSome = false;
    StorageItem *storageItem = 0;
    MTPResponseCode response = MTP_RESP_GeneralError;

    if (0xFFFFFFFF == handle) {
        // deleteItemHelper modifies m_objectHandlesMap so loop over a copy
        QHash<ObjHandle, StorageItem *> objectHandles = m_objectHandlesMap;
        for (QHash<ObjHandle, StorageItem *>::const_iterator i = objectHandles.constBegin();
             i != objectHandles.constEnd();
             ++i) {
            if (formatCode && MTP_OBF_FORMAT_Undefined != formatCode) {
                storageItem = i.value();
                if (storageItem->m_objectInfo && storageItem->m_objectInfo->mtpObjectFormat == formatCode) {
                    response = deleteItemHelper(i.key());
                }
            } else {
                response = deleteItemHelper(i.key());
            }
            if (MTP_RESP_OK == response) {
                deletedSome = true;
            } else if (MTP_RESP_InvalidObjectHandle != response) {
                // "invalid object handle" is not a failure because it
                // just means this item was deleted as part of a folder
                // before the loop got to it.
                failedSome = true;
            }
        }
    } else {
        response = deleteItemHelper(handle);
    }

    /* MTPv1.1 D.2.11 DeleteObject
     * "If a value of 0xFFFFFFFF is passed in the first parameter, and
     * some subset of objects are not deleted (but at least one object is
     * deleted), a response of Partial_Deletion shall be returned."
     */
    if (0xFFFFFFFF == handle && deletedSome && failedSome) {
        response = MTP_RESP_PartialDeletion;
    }

    return response;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::deleteItemHelper
 ***********************************************************/
MTPResponseCode FSStoragePlugin::deleteItemHelper(ObjHandle handle, bool removePhysically, bool sendEvent)
{
    MTPResponseCode response = MTP_RESP_GeneralError;
    bool itemNotDeleted = false;
    if (!checkHandle(handle)) {
        return MTP_RESP_InvalidObjectHandle;
    }

    // Get the corresponding storage item.
    StorageItem *storageItem = m_objectHandlesMap[handle];

    if (!storageItem) {
        return MTP_RESP_GeneralError;
    }

    // Allowing deletion of the root is too dangerous (might be $HOME)
    if (storageItem == m_root) {
        return MTP_RESP_ObjectWriteProtected;
    }

    // If this is a file or an empty dir, just delete this item.
    if (!storageItem->m_firstChild) {
        if (removePhysically && MTP_OBF_FORMAT_Association == storageItem->m_objectInfo->mtpObjectFormat
            && 0 != storageItem->m_handle) {
            QDir dir(storageItem->m_parent->m_path);
            if (!dir.rmdir(storageItem->m_path)) {
                return MTP_RESP_GeneralError;
            }
        } else if (removePhysically) {
            QFile file(storageItem->m_path);
            if (!file.remove()) {
                return MTP_RESP_GeneralError;
            }
        }

        removeFromStorage(handle, sendEvent);
    }
    // if this is a non-empty dir.
    else {
        StorageItem *itr = storageItem->m_firstChild;
        while (itr) {
            response = deleteItemHelper(itr->m_handle, removePhysically, sendEvent);
            if (MTP_RESP_OK != response) {
                itemNotDeleted = true;
                break;
            }
            //itr = itr->m_nextSibling;
            itr = storageItem->m_firstChild;
        }
        // Now delete the empty directory ( if empty! ).
        if (!itemNotDeleted) {
            response = deleteItemHelper(handle);
        } else {
            return MTP_RESP_PartialDeletion;
        }
    }
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::removeFromStorage
 ***********************************************************/
MTPResponseCode FSStoragePlugin::removeFromStorage(ObjHandle handle, bool sendEvent)
{
    StorageItem *storageItem = 0;
    // Remove the item from object handles map and delete the corresponding storage item.
    if (checkHandle(handle)) {
        storageItem = m_objectHandlesMap.value(handle);
        // Remove the item from the watch descriptor map if present.
        if (-1 != storageItem->m_wd) {
            // Remove watch on the path and then remove the wd from the map
            removeWatchDescriptor(storageItem);
        }
        m_objectHandlesMap.remove(handle);
        m_pathNamesMap.remove(storageItem->m_path);
        unlinkChildStorageItem(storageItem);
        delete storageItem;
    }

    if (sendEvent) {
        QVector<quint32> eventParams;
        eventParams.append(handle);
        emit eventGenerated(MTP_EV_ObjectRemoved, eventParams);
    }

    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::getObjectHandles
 ***********************************************************/
MTPResponseCode FSStoragePlugin::getObjectHandles(
    const MTPObjFormatCode &formatCode, const quint32 &associationHandle, QVector<ObjHandle> &objectHandles) const
{
    switch (associationHandle) {
    // Count of all objects in this storage.
    case 0x00000000:
        if (!formatCode) {
            for (QHash<ObjHandle, StorageItem *>::const_iterator i = m_objectHandlesMap.constBegin();
                 i != m_objectHandlesMap.constEnd();
                 ++i) {
                // Don't enumerate the root.
                if (0 == i.key()) {
                    continue;
                }
                objectHandles.append(i.key());
            }
        } else {
            for (QHash<ObjHandle, StorageItem *>::const_iterator i = m_objectHandlesMap.constBegin();
                 i != m_objectHandlesMap.constEnd();
                 ++i) {
                // Don't enumerate the root.
                if (0 == i.key()) {
                    continue;
                }
                if (i.value()->m_objectInfo && formatCode == i.value()->m_objectInfo->mtpObjectFormat) {
                    objectHandles.append(i.key());
                }
            }
        }
        break;

    // Count of all objects present in the root storage.
    case 0xFFFFFFFF:
        if (m_root) {
            StorageItem *storageItem = m_root->m_firstChild;
            while (storageItem) {
                if (!formatCode
                    || (MTP_OBF_FORMAT_Undefined != formatCode && storageItem->m_objectInfo
                        && formatCode == storageItem->m_objectInfo->mtpObjectFormat)) {
                    objectHandles.append(storageItem->m_handle);
                }
                storageItem = storageItem->m_nextSibling;
            }
        } else {
            return MTP_RESP_InvalidParentObject;
        }
        break;

    // Count of all objects that are children of an object whose handle = associationHandle;
    default:
        //Check if the association handle is valid.
        if (!m_objectHandlesMap.contains(associationHandle)) {
            return MTP_RESP_InvalidParentObject;
        }
        StorageItem *parentItem = m_objectHandlesMap[associationHandle];
        if (parentItem) {
            //Check if this is an association
            if (!parentItem->m_objectInfo || MTP_OBF_FORMAT_Association != parentItem->m_objectInfo->mtpObjectFormat) {
                return MTP_RESP_InvalidParentObject;
            }
            StorageItem *storageItem = parentItem->m_firstChild;
            while (storageItem) {
                if ((!formatCode)
                    || (MTP_OBF_FORMAT_Undefined != formatCode && storageItem->m_objectInfo
                        && formatCode == storageItem->m_objectInfo->mtpObjectFormat)) {
                    objectHandles.append(storageItem->m_handle);
                }
                storageItem = storageItem->m_nextSibling;
            }
        } else {
            return MTP_RESP_InvalidParentObject;
        }
        break;
    }
    return MTP_RESP_OK;
}

/************************************************************
 * bool FSStoragePlugin::checkHandle
 ***********************************************************/
bool FSStoragePlugin::checkHandle(const ObjHandle &handle) const
{
    return m_objectHandlesMap.contains(handle);
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::storageInfo
 ***********************************************************/
MTPResponseCode FSStoragePlugin::storageInfo(MTPStorageInfo &info)
{
    info = m_storageInfo;
    struct statvfs stat;
    MTPResponseCode responseCode = MTP_RESP_OK;

    //FIXME max capacity should be a const too, so can be computed once in the constructor.
    QByteArray ba = m_storagePath.toUtf8();
    if (statvfs(ba.constData(), &stat)) {
        responseCode = MTP_RESP_GeneralError;
    } else {
        info.maxCapacity = m_storageInfo.maxCapacity = (quint64) stat.f_blocks * stat.f_bsize;
        info.freeSpace = m_storageInfo.freeSpace = (quint64) stat.f_bavail * stat.f_bsize;
    }
    return responseCode;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::copyObject
 ***********************************************************/
MTPResponseCode FSStoragePlugin::copyObject(
    const ObjHandle &handle,
    const ObjHandle &parentHandle,
    StoragePlugin *destinationStorage,
    ObjHandle &copiedObjectHandle,
    quint32 recursionCounter /*= 0*/)
{
    if (!checkHandle(handle)) {
        return MTP_RESP_InvalidObjectHandle;
    }
    if (!destinationStorage) {
        destinationStorage = this;
    }
    if (!destinationStorage->checkHandle(parentHandle)) {
        return MTP_RESP_InvalidParentObject;
    }
    StorageItem *storageItem = m_objectHandlesMap[handle];
    if (!storageItem) {
        return MTP_RESP_GeneralError;
    }
    if (!m_objectHandlesMap[handle]->m_objectInfo) {
        return MTP_RESP_InvalidObjectHandle;
    }

    // Get the source object's objectinfo dataset.
    MTPObjectInfo objectInfo = *m_objectHandlesMap[handle]->m_objectInfo;

    MTPStorageInfo storageInfo;
    if (destinationStorage->storageInfo(storageInfo) != MTP_RESP_OK) {
        return MTP_RESP_GeneralError;
    }
    if (storageInfo.freeSpace < objectInfo.mtpObjectCompressedSize) {
        return MTP_RESP_StoreFull;
    }

    QString destinationPath;
    if (destinationStorage->getPath(parentHandle, destinationPath) != MTP_RESP_OK) {
        return MTP_RESP_InvalidParentObject;
    }
    destinationPath += '/' + objectInfo.mtpFileName;

    if ((0 == recursionCounter) && MTP_OBF_FORMAT_Association == objectInfo.mtpObjectFormat) {
        // Check if we copy a dir to a place where it already exists; don't
        // allow that.
        QVector<ObjHandle> handles;
        if (destinationStorage->getObjectHandles(0, parentHandle, handles) == MTP_RESP_OK) {
            foreach (ObjHandle handle, handles) {
                QString path;
                if (destinationStorage->getPath(handle, path) != MTP_RESP_OK) {
                    continue;
                }
                if (path == destinationPath) {
                    return MTP_RESP_InvalidParentObject;
                }
            }
        }
    }

    // Modify the objectinfo dataset to include the new storageId and new parent object.
    objectInfo.mtpParentObject = parentHandle;
    objectInfo.mtpStorageId = destinationStorage->storageId();

    // Workaround: remove watch descriptor on the destination directory so that
    // we don't receive inotify signals. This prevents adding them twice.
    FSStoragePlugin *destinationFsStorage = dynamic_cast<FSStoragePlugin *>(destinationStorage);
    StorageItem *parentItem = 0;
    if (destinationFsStorage) {
        parentItem = destinationFsStorage->m_objectHandlesMap[parentHandle];
        destinationFsStorage->removeWatchDescriptor(parentItem);
    }

    // Write the content to the new item.
    MTPResponseCode response = MTP_RESP_OK;

    // Create the new item.
    ObjHandle ignoredHandle;
    response = destinationStorage->addItem(ignoredHandle, copiedObjectHandle, &objectInfo);
    if (MTP_RESP_OK != response) {
        // Error.
    }
    // If this is a directory, copy recursively.
    else if (MTP_OBF_FORMAT_Association == objectInfo.mtpObjectFormat) {
        // Save the directory handle
        ObjHandle parentHandle = copiedObjectHandle;
        StorageItem *childItem = storageItem->m_firstChild;
        for (; childItem; childItem = childItem->m_nextSibling) {
            response = copyObject(
                childItem->m_handle, parentHandle, destinationStorage, copiedObjectHandle, ++recursionCounter);
            if (MTP_RESP_OK != response) {
                copiedObjectHandle = parentHandle;
                return response;
            }
        }
        // Restore directory handle
        copiedObjectHandle = parentHandle;
    }
    // this is a file, copy the data
    else {
        response = copyData(this, handle, destinationStorage, copiedObjectHandle);
        if (response != MTP_RESP_OK) {
            return response;
        }
    }

    if (destinationFsStorage) {
        destinationFsStorage->addWatchDescriptor(parentItem);
    }

    return response;
}

/************************************************************
 * void FSStoragePlugin::adjustMovedItemsPath
 ***********************************************************/
void FSStoragePlugin::adjustMovedItemsPath(QString newAncestorPath, StorageItem *movedItem)
{
    if (!movedItem) {
        return;
    }

    m_pathNamesMap.remove(movedItem->m_path);
    QString destinationPath = newAncestorPath + "/" + movedItem->m_objectInfo->mtpFileName;
    movedItem->m_path = destinationPath;
    m_pathNamesMap[movedItem->m_path] = movedItem->m_handle;
    StorageItem *itr = movedItem->m_firstChild;
    while (itr) {
        adjustMovedItemsPath(movedItem->m_path, itr);
        itr = itr->m_nextSibling;
    }
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::moveObject
 ***********************************************************/
MTPResponseCode FSStoragePlugin::moveObject(
    const ObjHandle &handle, const ObjHandle &parentHandle, StoragePlugin *destinationStorage, bool movePhysically)
{
    if (!checkHandle(handle)) {
        return MTP_RESP_InvalidObjectHandle;
    }

    if (destinationStorage != this) {
        MTPResponseCode response = destinationStorage->copyHandle(this, handle, parentHandle);
        if (response != MTP_RESP_OK) {
            return response;
        }

        return deleteItem(handle, MTP_OBF_FORMAT_Undefined);
    }

    if (!checkHandle(parentHandle)) {
        return MTP_RESP_InvalidParentObject;
    }

    StorageItem *storageItem = m_objectHandlesMap[handle], *parentItem = m_objectHandlesMap[parentHandle];
    if (!storageItem || !parentItem) {
        return MTP_RESP_GeneralError;
    }

    QString destinationPath = parentItem->m_path + "/" + storageItem->m_objectInfo->mtpFileName;

    // If this is a directory already exists, don't overwrite it.
    if (MTP_OBF_FORMAT_Association == storageItem->m_objectInfo->mtpObjectFormat) {
        if (m_pathNamesMap.contains(destinationPath)) {
            return MTP_RESP_InvalidParentObject;
        }
    }

    // Invalidate the watch descriptor for this item and it's children, as their paths will change.
    removeWatchDescriptorRecursively(storageItem);

    // Do the move.
    if (movePhysically) {
        QDir dir;
        if (!dir.rename(storageItem->m_path, destinationPath)) {
            // Move failed; restore original watch descriptors.
            addWatchDescriptorRecursively(storageItem);
            return MTP_RESP_InvalidParentObject;
        }
    }
    m_pathNamesMap.remove(storageItem->m_path);
    m_pathNamesMap[destinationPath] = handle;

    // Unlink this item from its current parent.
    unlinkChildStorageItem(storageItem);

    StorageItem *itr = storageItem->m_firstChild;
    while (itr) {
        adjustMovedItemsPath(destinationPath, itr);
        itr = itr->m_nextSibling;
    }

    // link it to the new parent
    linkChildStorageItem(storageItem, parentItem);
    //storageItem->m_nextSibling = 0;

    // update it's path and parent object.
    storageItem->m_path = destinationPath;
    storageItem->m_objectInfo->mtpParentObject = parentHandle;
    // create new watch descriptors for the moved item.
    addWatchDescriptorRecursively(storageItem);
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::getObjectInfo
 ***********************************************************/
MTPResponseCode FSStoragePlugin::getObjectInfo(const ObjHandle &handle, const MTPObjectInfo *&objectInfo)
{
    if (!checkHandle(handle)) {
        return MTP_RESP_InvalidObjectHandle;
    }
    StorageItem *storageItem = m_objectHandlesMap[handle];
    if (!storageItem) {
        return MTP_RESP_GeneralError;
    }

    /* Assumption: All mtp queries that the host can use to
     * "show interest in object" lead to getObjectInfo()
     * method call -> Flag the object so that any future
     * changes to object properties leads to the host getting
     * appropriate notification event. */
    storageItem->setEventsEnabled(true);

    populateObjectInfo(storageItem);
    objectInfo = storageItem->m_objectInfo;
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::populateObjectInfo
 ***********************************************************/
void FSStoragePlugin::populateObjectInfo(StorageItem *storageItem)
{
    if (!storageItem) {
        return;
    }
    // If we have already stored objectinfo for this item.
    if (storageItem->m_objectInfo) {
        return;
    }

    // Populate object info for this item.
    storageItem->m_objectInfo = new MTPObjectInfo;

    // storage id.
    storageItem->m_objectInfo->mtpStorageId = m_storageId;
    // file name
    QString name = storageItem->m_path;
    name = name.remove(0, storageItem->m_path.lastIndexOf("/") + 1);
    storageItem->m_objectInfo->mtpFileName = name;
    // object format.
    storageItem->m_objectInfo->mtpObjectFormat = getObjectFormatByExtension(storageItem);
    // protection status.
    storageItem->m_objectInfo->mtpProtectionStatus = getMTPProtectionStatus(storageItem);
    // object size.
    storageItem->m_objectInfo->mtpObjectCompressedSize = getObjectSize(storageItem);
    // thumb size
    storageItem->m_objectInfo->mtpThumbCompressedSize = getThumbCompressedSize(storageItem);
    // thumb format
    storageItem->m_objectInfo->mtpThumbFormat = getThumbFormat(storageItem);
    // thumb width
    storageItem->m_objectInfo->mtpThumbPixelWidth = getThumbPixelWidth(storageItem);
    // thumb height
    storageItem->m_objectInfo->mtpThumbPixelHeight = getThumbPixelHeight(storageItem);
    // image pix width
    storageItem->m_objectInfo->mtpImagePixelWidth = getImagePixelWidth(storageItem);
    // image pix height
    storageItem->m_objectInfo->mtpImagePixelHeight = getImagePixelHeight(storageItem);
    // image bit depth
    storageItem->m_objectInfo->mtpImageBitDepth = getImageBitDepth(storageItem);
    // parent object.
    storageItem->m_objectInfo->mtpParentObject = storageItem->m_parent ? storageItem->m_parent->m_handle : 0x00000000;
    // association type
    storageItem->m_objectInfo->mtpAssociationType = getAssociationType(storageItem);
    // association description
    storageItem->m_objectInfo->mtpAssociationDescription = getAssociationDescription(storageItem);
    // sequence number
    storageItem->m_objectInfo->mtpSequenceNumber = getSequenceNumber(storageItem);
    // date created
    storageItem->m_objectInfo->mtpCaptureDate = getCreatedDate(storageItem);
    // date modified
    storageItem->m_objectInfo->mtpModificationDate = getModifiedDate(storageItem);

    // keywords.
    storageItem->m_objectInfo->mtpKeywords = getKeywords(storageItem);
}

/************************************************************
 * quint16 FSStoragePlugin::getObjectFormatByExtension
 ***********************************************************/
quint16 FSStoragePlugin::getObjectFormatByExtension(StorageItem *storageItem)
{
    // TODO Fetch from tracker or determine from the file.
    quint16 format = MTP_OBF_FORMAT_Undefined;

    QFileInfo item(storageItem->m_path);
    if (item.isDir()) {
        format = MTP_OBF_FORMAT_Association;
    } else { //file
        QString ext = storageItem->m_path.section('.', -1).toLower();
        if (m_formatByExtTable.contains(ext)) {
            format = m_formatByExtTable[ext];
        }
    }
    return format;
}

/************************************************************
 * quint16 FSStoragePlugin::getMTPProtectionStatus
 ***********************************************************/
quint16 FSStoragePlugin::getMTPProtectionStatus(StorageItem * /*storageItem*/)
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint64 FSStoragePlugin::getObjectSize
 ***********************************************************/
quint64 FSStoragePlugin::getObjectSize(StorageItem *storageItem)
{
    if (!storageItem) {
        return 0;
    }
    QFileInfo item(storageItem->m_path);
    if (item.isFile()) {
        return item.size();
    }
    return 0;
}

/************************************************************
 * bool FSStoragePlugin::isThumbnailableImage
 ***********************************************************/
bool FSStoragePlugin::isThumbnailableImage(StorageItem *storageItem)
{
    static const char *const extension[] = {/* Things that thumbnailer can process */
                                            ".bmp",
                                            ".gif",
                                            ".jpeg",
                                            ".jpg",
                                            ".png",
    /* Things that would be nice to get supported */
#if 0
        ".tif", ".tiff",
        ".pbm", ".pcx", ".pgm", ".ppm", ".xpm", ".xwd",
#endif
                                            /* Sentinel */
                                            0};

    if (storageItem) {
        for (size_t i = 0; extension[i]; ++i) {
            if (storageItem->m_path.endsWith(extension[i]))
                return true;
        }
    }

    return false;
}

/************************************************************
 * quint16 FSStoragePlugin::getThumbFormat
 ***********************************************************/
quint16 FSStoragePlugin::getThumbFormat(StorageItem *storageItem)
{
    quint16 format = MTP_OBF_FORMAT_Undefined;
    if (isThumbnailableImage(storageItem)) {
        format = MTP_OBF_FORMAT_JFIF;
    }
    return format;
}

/************************************************************
 * quint32 FSStoragePlugin::getThumPixelWidth
 ***********************************************************/
quint32 FSStoragePlugin::getThumbPixelWidth(StorageItem *storageItem)
{
    quint16 width = 0;
    if (isThumbnailableImage(storageItem)) {
        width = THUMB_WIDTH;
    }
    return width;
}

/************************************************************
 * quint32 FSStoragePlugin::getThumPixelHeight
 ***********************************************************/
quint32 FSStoragePlugin::getThumbPixelHeight(StorageItem *storageItem)
{
    quint16 height = 0;
    if (isThumbnailableImage(storageItem)) {
        height = THUMB_HEIGHT;
    }
    return height;
}

/************************************************************
 * quint32 FSStoragePlugin::getThumbCompressedSize
 ***********************************************************/
quint32 FSStoragePlugin::getThumbCompressedSize(StorageItem *storageItem)
{
    quint32 size = 0;
    if (isThumbnailableImage(storageItem)) {
        QString thumbPath = m_thumbnailer->requestThumbnail(
            storageItem->m_path, m_imageMimeTable.value(storageItem->m_objectInfo->mtpObjectFormat));
        if (!thumbPath.isEmpty()) {
            size = QFileInfo(thumbPath).size();
        }
    }
    return size;
}

/************************************************************
 * quint32 FSStoragePlugin::getImagePixelWidth
 ***********************************************************/
quint32 FSStoragePlugin::getImagePixelWidth(StorageItem * /*storageItem*/)
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint32 FSStoragePlugin::getImagePixelHeight
 ***********************************************************/
quint32 FSStoragePlugin::getImagePixelHeight(StorageItem * /*storageItem*/)
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint16 FSStoragePlugin::getAssociationType
 ***********************************************************/
quint16 FSStoragePlugin::getAssociationType(StorageItem *storageItem)
{
    QFileInfo item(storageItem->m_path);
    if (item.isDir()) {
        // GenFolder is the only type used in MTP.
        // The others may be used for PTP compatibility but are not required.
        return MTP_ASSOCIATION_TYPE_GenFolder;
    } else
        return 0;
}

/************************************************************
 * quint32 FSStoragePlugin::getAssociationDescription
 ***********************************************************/
quint32 FSStoragePlugin::getAssociationDescription(StorageItem * /*storageItem*/)
{
    // 0 means it is not a bi-directionally linked folder.
    // See section 3.6.2.1 of the MTP spec.
    return 0;
}

/************************************************************
 * quint32 FSStoragePlugin::getImageBitDepth
 ***********************************************************/
quint32 FSStoragePlugin::getImageBitDepth(StorageItem * /*storageItem*/)
{
    // TODO Fetch from tracker or determine from the file.
    return 0;
}

/************************************************************
 * quint32 FSStoragePlugin::getSequenceNumber
 ***********************************************************/
quint32 FSStoragePlugin::getSequenceNumber(StorageItem * /*storageItem*/)
{
    // TODO Fetch from tracker
    return 0;
}

/************************************************************
 * QString FSStoragePlugin::getCreatedDate
 ***********************************************************/
QString FSStoragePlugin::getCreatedDate(StorageItem *storageItem)
{
    /* Unix file systems generally do not have time stamp
     * for create time and QDateTime::created() seems to
     * use hard-to-control and semi-useless changed time.
     *
     * Using that makes it difficult to avoid sending change
     * signals -> use the modify time instead. */
    return getModifiedDate(storageItem);
}

/************************************************************
 * char* FSStoragePlugin::getModifiedDate
 ***********************************************************/
QString FSStoragePlugin::getModifiedDate(StorageItem *storageItem)
{
    time_t t = file_get_mtime(storageItem->m_path);
    return datetime_from_time_t(t);
}

/************************************************************
 * char* FSStoragePlugin::getKeywords
 ***********************************************************/
char *FSStoragePlugin::getKeywords(StorageItem * /*storageItem*/)
{
    // Not supported.
    return 0;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::readData
 ***********************************************************/
MTPResponseCode FSStoragePlugin::readData(
    const ObjHandle &handle, char *readBuffer, quint32 readBufferLen, quint64 readOffset)
{
    MTP_LOG_INFO("handle:" << handle << "readBufferLen:" << readBufferLen << "readOffset:" << readOffset);

    MTPResponseCode resp = MTP_RESP_OK;
    StorageItem *storageItem = nullptr;

    if (!readBuffer) {
        resp = MTP_RESP_GeneralError;
    } else if (!(storageItem = m_objectHandlesMap.value(handle))) {
        resp = MTP_RESP_InvalidObjectHandle;
    } else {
        QFile file(storageItem->m_path);
        if (!file.open(QIODevice::ReadOnly)) {
            MTP_LOG_WARNING("failed to open:" << file.fileName());
            resp = MTP_RESP_AccessDenied;
        } else if (quint64(file.size()) < (readOffset + readBufferLen)) {
            MTP_LOG_WARNING("file is too small:" << file.fileName());
            resp = MTP_RESP_GeneralError;
        } else if (!file.seek(readOffset)) {
            MTP_LOG_WARNING("failed to seek:" << file.fileName());
            resp = MTP_RESP_GeneralError;
        } else
            while (resp == MTP_RESP_OK && readBufferLen > 0) {
                qint64 rc = file.read(readBuffer, readBufferLen);
                if (rc == -1) {
                    MTP_LOG_WARNING("failed to read:" << file.fileName());
                    resp = MTP_RESP_GeneralError;
                } else if (rc == 0) {
                    MTP_LOG_WARNING("unexpected eof:" << file.fileName());
                    resp = MTP_RESP_GeneralError;
                } else {
                    readBuffer += rc;
                    readBufferLen -= quint32(rc);
                }
            }
    }

    // FIXME: Repeated opening and closing of the file will happen when we do segmented reads.
    // This must be avoided

    if (resp != MTP_RESP_OK)
        MTP_LOG_WARNING("read from handle:" << handle << "failed:" << mtp_code_repr(resp));
    return resp;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::truncateItem
 ***********************************************************/
MTPResponseCode FSStoragePlugin::truncateItem(const ObjHandle &handle, const quint64 &size)
{
    if (!checkHandle(handle)) {
        return MTP_RESP_InvalidObjectHandle;
    }

    // Get the corresponding storage item.
    StorageItem *storageItem = m_objectHandlesMap[handle];
    if (!storageItem || !storageItem->m_objectInfo
        || MTP_OBF_FORMAT_Association == storageItem->m_objectInfo->mtpObjectFormat) {
        return MTP_RESP_GeneralError;
    }

    QFile file(storageItem->m_path);
    if (!file.resize(size)) {
        return MTP_RESP_GeneralError;
    }
    storageItem->m_objectInfo->mtpObjectCompressedSize = size;
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::writeData
 ***********************************************************/
MTPResponseCode FSStoragePlugin::writeData(
    const ObjHandle &handle, const char *writeBuffer, quint32 bufferLen, bool isFirstSegment, bool isLastSegment)
{
    if (!checkHandle(handle)) {
        return MTP_RESP_InvalidObjectHandle;
    }

    // Get the corresponding storage item.
    StorageItem *storageItem = m_objectHandlesMap[handle];
    if (!storageItem) {
        return MTP_RESP_GeneralError;
    }

    if (isLastSegment && !writeBuffer) {
        m_writeObjectHandle = 0;
        if (m_dataFile) {
            /* Truncate at current write offset */
            m_dataFile->flush();
            m_dataFile->resize(m_dataFile->pos());

            /* Close the file */
            m_dataFile->close();
            delete m_dataFile;
            m_dataFile = 0;

            /* Preceeding writes and/or close might have changed
             * the modify time -> put it back to cached/expected
             * value. */
            MTPObjectInfo *info = storageItem->m_objectInfo;
            time_t t = datetime_to_time_t(info->mtpModificationDate);
            file_set_mtime(storageItem->m_path, t);

            /* In any case update the cached values according to
             * what is actually used by thefilesystem. */
            info->mtpModificationDate = getModifiedDate(storageItem);
            info->mtpCaptureDate = info->mtpModificationDate;
        }
    } else {
        m_writeObjectHandle = handle;
        qint32 bytesRemaining = bufferLen;
        // Resize file to zero, if first segment
        if (isFirstSegment) {
            // Open the file and write to it.
            m_dataFile = new QFile(storageItem->m_path);

            bool already_exists = m_dataFile->exists();

            if (!m_dataFile->open(QIODevice::ReadWrite)) {
                delete m_dataFile;
                m_dataFile = 0;
                return MTP_RESP_GeneralError;
            }

            if (!already_exists) {
                /* When creating new files, prefer using the real gid
                 * (= "nemo") over the effective gid (= "privileged"). */
                if (fchown(m_dataFile->handle(), getuid(), getgid()) == -1) {
                    MTP_LOG_WARNING("failed to set file:" << storageItem->m_path << " ownership");
                }
            }

            /* In all likelihood we've already created the file
             * via createFile() method and it should  have correct
             * target size -> start overwriting from offset zero. */
            m_dataFile->seek(0);

            /* Opening the file changes modify time, put it back
             * to expected/cached value */
            MTPObjectInfo *info = storageItem->m_objectInfo;
            time_t t = datetime_to_time_t(info->mtpModificationDate);
            file_set_mtime(storageItem->m_path, t);
        }

        while (bytesRemaining && m_dataFile) {
            qint32 bytesWritten = m_dataFile->write(writeBuffer, bytesRemaining);
            if (-1 == bytesWritten) {
                MTP_LOG_WARNING("ERROR writing data to" << storageItem->m_path);
                //Send a store full event if there's no space.
                /*MTPResponseCode resp;
                MTPObjectInfo objectInfo;
                resp = getObjectInfo( handle, &objectInfo );
                quint64 freeSpaceRemaining = 0;
                resp = freeSpace( freeSpaceRemaining );
                if( freeSpaceRemaining < objectInfo.mtpObjectCompressedSize )
                {
                    QVector<quint32> params;
                    params.append(m_storageId);
                    emit eventGenerated(MTP_EV_StoreFull, params, QString());
                }*///TODO Fixme eventGenerated not working.
                return MTP_RESP_GeneralError;
            }
            bytesRemaining -= bytesWritten;
            writeBuffer += bytesWritten;
        }
    }
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::writePartialData
 ***********************************************************/
MTPResponseCode FSStoragePlugin::writePartialData(
    const ObjHandle &handle,
    quint64 offset,
    const quint8 *dataContent,
    quint32 dataLength,
    bool isFirstSegment,
    bool isLastSegment)
{
    MTPResponseCode code = MTP_RESP_OK;
    StorageItem *storageItem = nullptr;

    /* Lookup storage iterm for the handle */
    if (code == MTP_RESP_OK && !checkHandle(handle))
        code = MTP_RESP_InvalidObjectHandle;

    if (code == MTP_RESP_OK && !(storageItem = m_objectHandlesMap[handle]))
        code = MTP_RESP_GeneralError;

    /* Open file when dealing with the first segment */
    if (code == MTP_RESP_OK && isFirstSegment) {
        MTP_LOG_INFO("open for writing:" << storageItem->m_path);

        /* Start ignoring inotify events about this handle */
        m_writeObjectHandle = handle;

        if (m_dataFile)
            delete m_dataFile;
        m_dataFile = new QFile(storageItem->m_path);

        bool already_exists = m_dataFile->exists();

        if (!m_dataFile->open(QIODevice::ReadWrite)) {
            MTP_LOG_WARNING("failed to open file" << storageItem->m_path << " for writing");
            delete m_dataFile;
            m_dataFile = nullptr;
            code = MTP_RESP_GeneralError;
        } else if (!already_exists) {
            /* When creating new files, prefer using the real gid
             * (= "nemo") over the effective gid (= "privileged"). */
            if (fchown(m_dataFile->handle(), getuid(), getgid()) == -1)
                MTP_LOG_WARNING("failed to set file" << storageItem->m_path << " ownership");
        }
    }

    /* Write data when applicable */
    if (code == MTP_RESP_OK && m_dataFile && dataContent) {
        MTP_LOG_INFO("set read position:" << storageItem->m_path << "at offset:" << offset);

        if (m_writeObjectHandle != handle)
            code = MTP_RESP_GeneralError;

        if (code == MTP_RESP_OK && !m_dataFile->seek(offset)) {
            MTP_LOG_WARNING("ERROR setting write position in" << storageItem->m_path);
            code = MTP_RESP_GeneralError;
        }
        while (code == MTP_RESP_OK && dataLength > 0) {
            qint32 bytesWritten = m_dataFile->write(reinterpret_cast<const char *>(dataContent), dataLength);
            if (bytesWritten == -1) {
                MTP_LOG_WARNING("ERROR writing data to" << storageItem->m_path);
                code = MTP_RESP_GeneralError;
            } else {
                dataLength -= bytesWritten;
                dataContent += bytesWritten;
            }
        }
    }

    /* Close file when dealing with failures / the last segment */
    if (code != MTP_RESP_OK || isLastSegment) {
        if (m_dataFile) {
            MTP_LOG_INFO("close file:" << storageItem->m_path);
            m_dataFile->flush();
            m_dataFile->close();
            delete m_dataFile;
            m_dataFile = nullptr;

            /* Preceeding writes and/or close might have changed
             * the modify time -> put it back to cached/expected
             * value. */
            MTPObjectInfo *info = storageItem->m_objectInfo;
            time_t t = datetime_to_time_t(info->mtpModificationDate);
            file_set_mtime(storageItem->m_path, t);

            /* In any case update the cached values according to
             * what is actually used by thefilesystem. */
            info->mtpObjectCompressedSize = getObjectSize(storageItem);
            info->mtpModificationDate = getModifiedDate(storageItem);
            info->mtpCaptureDate = info->mtpModificationDate;
        }

        /* Stop ignoring inotify events
         *
         * It is higly likely that we are about to get notifications
         * due to having just closed the file. This should not lead
         * to external notifications because cached object info was
         * also updated match situation on filesystem.
         */
        m_writeObjectHandle = 0;
    }

    return code;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::getPath
 ***********************************************************/
MTPResponseCode FSStoragePlugin::getPath(const quint32 &handle, QString &path) const
{
    path = "";
    if (!m_objectHandlesMap.contains(handle)) {
        return MTP_RESP_GeneralError;
    }
    StorageItem *storageItem = m_objectHandlesMap.value(handle);
    if (!storageItem) {
        return MTP_RESP_GeneralError;
    }

    path = storageItem->m_path;
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::getEventsEnabled
 ***********************************************************/
MTPResponseCode FSStoragePlugin::getEventsEnabled(const quint32 &handle, bool &eventsEnabled) const
{
    MTPResponseCode result = MTP_RESP_OK;
    StorageItem *storageItem = m_objectHandlesMap.value(handle, 0);
    if (storageItem)
        eventsEnabled = storageItem->eventsAreEnabled();
    else
        result = MTP_RESP_GeneralError;
    return result;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::setEventsEnabled
 ***********************************************************/
MTPResponseCode FSStoragePlugin::setEventsEnabled(const quint32 &handle, bool eventsEnabled) const
{
    MTPResponseCode result = MTP_RESP_OK;
    StorageItem *storageItem = m_objectHandlesMap.value(handle, 0);
    if (storageItem)
        storageItem->setEventsEnabled(eventsEnabled);
    else
        result = MTP_RESP_GeneralError;
    return result;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::dumpStorageItem
 ***********************************************************/
void FSStoragePlugin::dumpStorageItem(StorageItem *storageItem, bool recurse)
{
    if (!storageItem) {
        return;
    }

    ObjHandle parentHandle = storageItem->m_parent ? storageItem->m_parent->m_handle : 0;
    QString parentPath = storageItem->m_parent ? storageItem->m_parent->m_path : "";
    MTP_LOG_INFO(
        "\n<" << storageItem->m_handle << "," << storageItem->m_path << "," << parentHandle << "," << parentPath << ">");

    if (recurse) {
        StorageItem *itr = storageItem->m_firstChild;
        while (itr) {
            dumpStorageItem(itr, recurse);
            itr = itr->m_nextSibling;
        }
    }
}

/************************************************************
 * void FSStoragePlugin::inotifyEventSlot
 ***********************************************************/
void FSStoragePlugin::inotifyEventSlot(struct inotify_event *event)
{
    const struct inotify_event *fromEvent = 0;
    QString fromNameString;
    const char *name = 0;

    getCachedInotifyEvent(&fromEvent, fromNameString);
    QByteArray ba = fromNameString.toUtf8();

    // Trick to handle the last non-paired IN_MOVED_FROM
    if (!event) {
        if (fromEvent) {
            // File/directory was moved from the storage.
            handleFSDelete(fromEvent, ba.data());
            clearCachedInotifyEvent();
        }
        return;
    }

    name = event->len ? event->name : NULL;
    if (!name) {
        return;
    }

    if (fromEvent && fromEvent->cookie != event->cookie) {
        // File/directory was moved from the storage.
        handleFSDelete(fromEvent, ba.data());
        clearCachedInotifyEvent();
    }

    // File/directory was created.
    if (event->mask & IN_CREATE) {
        handleFSCreate(event, name);
    }

    // File/directory was deleted.
    if (event->mask & IN_DELETE) {
        handleFSDelete(event, name);
    }

    if (event->mask & IN_MOVED_TO) {
        if (fromEvent && fromEvent->cookie == event->cookie) {
            // File/directory was moved/renamed within the storage.
            handleFSMove(fromEvent, ba.data(), event, name);
            clearCachedInotifyEvent();
        } else {
            // File/directory was moved to the storage.
            handleFSCreate(event, name);
        }
    }

    if (event->mask & IN_MOVED_FROM) {
        if (fromEvent) {
            // File/directory was moved from the storage.
            handleFSDelete(fromEvent, ba.data());
            clearCachedInotifyEvent();
        }
        // Don't know what to do with it yet. Save it for later.
        cacheInotifyEvent(event, name);
    }

    if (event->mask & IN_CLOSE_WRITE) {
        handleFSModify(event, name);
    }
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::getReferences
 ***********************************************************/
MTPResponseCode FSStoragePlugin::getReferences(const ObjHandle &handle, QVector<ObjHandle> &references)
{
    if (!m_objectHandlesMap.contains(handle)) {
        removeInvalidObjectReferences(handle);
        return MTP_RESP_InvalidObjectHandle;
    }
    if (!m_objectReferencesMap.contains(handle)) {
        return MTP_RESP_OK;
    }
    references = m_objectReferencesMap[handle];
    QVector<ObjHandle>::iterator i = references.begin();
    while (i != references.end()) {
        if (!m_objectHandlesMap.contains(*i)) {
            i = references.erase(i);
        } else {
            ++i;
        }
    }
    m_objectReferencesMap[handle] = references;
    return MTP_RESP_OK;
}

/************************************************************
 * MTPResponseCode FSStoragePlugin::setReferences
 ***********************************************************/
//TODO Do we have cases where we need to set our own references (ie not due to initiator's request)
MTPResponseCode FSStoragePlugin::setReferences(const ObjHandle &handle, const QVector<ObjHandle> &references)
{
    StorageItem *playlist = m_objectHandlesMap.value(handle);

    if (!playlist || !playlist->m_objectInfo) {
        return MTP_RESP_InvalidObjectHandle;
    }

    bool savePlaylist = (MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist == playlist->m_objectInfo->mtpObjectFormat);
    QStringList entries;
    for (int i = 0; i < references.size(); ++i) {
        StorageItem *reference = m_objectHandlesMap.value(references[i]);
        if (!reference || !reference->m_objectInfo) {
            return MTP_RESP_Invalid_ObjectReference;
        }
        if (savePlaylist) {
            // Append the path to the entries list
            entries.append(reference->m_path);
        }
    }
    m_objectReferencesMap[handle] = references;

    return MTP_RESP_OK;
}

/************************************************************
 * void FSStoragePlugin::removeInvalidObjectReferences
 ***********************************************************/
void FSStoragePlugin::removeInvalidObjectReferences(const ObjHandle &handle)
{
    QHash<ObjHandle, QVector<ObjHandle>>::iterator i = m_objectReferencesMap.begin();
    while (i != m_objectReferencesMap.end()) {
        QVector<ObjHandle>::iterator j = i.value().begin();
        while (j != i.value().end()) {
            if (handle == *j) {
                j = i.value().erase(j);
            } else {
                ++j;
            }
        }
        if (handle == i.key()) {
            i = m_objectReferencesMap.erase(i);
        } else {
            ++i;
        }
    }
}

/************************************************************
 * void FSStoragePlugin::storeObjectReferences
 ***********************************************************/
void FSStoragePlugin::storeObjectReferences()
{
    // Write as follows
    // 1. No. of objects that have references.
    // 2a. Object PUOID
    // 2b. No. of references for this object.
    // 2c. PUOIDs of the referred objects.

    qint32 bytesWritten = -1;
    QFile file(m_objectReferencesDbPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    quint32 noOfHandles = m_objectReferencesMap.count();
    // Recored the position to store the number of object handles at...
    qint64 posNoOfHandles = file.pos();
    bytesWritten = file.write(reinterpret_cast<const char *>(&noOfHandles), sizeof(quint32));
    if (-1 == bytesWritten) {
        MTP_LOG_WARNING("ERROR writing count to persistent objrefs db!!");
        file.resize(0);
        return;
    }
    for (QHash<ObjHandle, QVector<ObjHandle>>::iterator i = m_objectReferencesMap.begin();
         i != m_objectReferencesMap.end();
         ++i) {
        ObjHandle handle = i.key();
        // Get the object PUOID from the object handle (we need to store PUOIDs
        // in the ref DB as it is persistent, not object handles)
        StorageItem *item = m_objectHandlesMap.value(handle);
        if (!item || (MTP_OBF_FORMAT_Abstract_Audio_Video_Playlist == item->m_objectInfo->mtpObjectFormat)) {
            // Possibly, the handle was removed from the objectHandles map, but
            // still lingers in the object references map (It is cleared lazily
            // in getObjectReferences). Ignore this handle.
            noOfHandles--;
            continue;
        }
        // No NULL check for the item here, as it can safely be assumed that it
        // will exist in the object handles map.
        bytesWritten = file.write(reinterpret_cast<const char *>(&item->m_puoid), sizeof(MtpInt128));
        if (-1 == bytesWritten) {
            MTP_LOG_WARNING("ERROR writing a handle to persistent objrefs db!!");
            file.resize(0);
            return;
        }
        quint32 noOfRefs = i.value().size();
        // Record the position to write the number of object references into...
        qint64 posNoOfReferences = file.pos();
        bytesWritten = file.write(reinterpret_cast<const char *>(&noOfRefs), sizeof(quint32));
        if (-1 == bytesWritten) {
            MTP_LOG_WARNING("ERROR writing a handle's ref count to persistent objrefs db!!");
            file.resize(0);
            return;
        }
        for (int j = 0; j < i.value().size(); ++j) {
            ObjHandle reference = (i.value())[j];
            // Get PUOID from the reference
            item = m_objectHandlesMap.value(reference);
            if (!item) {
                // Ignore this handle...
                noOfRefs--;
                continue;
            }
            bytesWritten = file.write(reinterpret_cast<const char *>(&item->m_puoid), sizeof(MtpInt128));
            if (-1 == bytesWritten) {
                MTP_LOG_WARNING("ERROR writing a handle's reference to persistent objrefs db!!");
                file.resize(0);
                return;
            }
        }
        // Backup current position
        qint64 curPos = file.pos();
        // Seek back to write the number of references
        if (!file.seek(posNoOfReferences)) {
            MTP_LOG_WARNING("File seek failed!!");
            file.resize(0);
            return;
        }
        bytesWritten = file.write(reinterpret_cast<const char *>(&noOfRefs), sizeof(quint32));
        if (-1 == bytesWritten) {
            MTP_LOG_WARNING("ERROR writing a handle's ref count to persistent objrefs db!!");
            file.resize(0);
            return;
        }
        // Seek forward again
        if (!file.seek(curPos)) {
            MTP_LOG_WARNING("File seek failed!!");
            file.resize(0);
            return;
        }
    }
    // Seek backwards to write the number of object handles
    if (!file.seek(posNoOfHandles)) {
        MTP_LOG_WARNING("File seek failed!!");
        file.resize(0);
        return;
    }
    bytesWritten = file.write(reinterpret_cast<const char *>(&noOfHandles), sizeof(quint32));
    if (-1 == bytesWritten) {
        MTP_LOG_WARNING("ERROR writing count to persistent objrefs db!!");
        file.resize(0);
    }
    // No file.close() needed, handled in QFile's d'tor
}

/************************************************************
 * void FSStoragePlugin::populateObjectReferences
 ***********************************************************/
void FSStoragePlugin::populateObjectReferences()
{
    QFile file(m_objectReferencesDbPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    qint32 bytesRead = 0;
    quint32 noOfHandles = 0, noOfRefs = 0;
    MtpInt128 objPuoid, referencePuoid;
    QVector<ObjHandle> references;

    bytesRead = file.read(reinterpret_cast<char *>(&noOfHandles), sizeof(quint32));
    if (0 >= bytesRead) {
        return;
    }
    for (quint32 i = 0; i < noOfHandles; ++i) {
        bytesRead = file.read(reinterpret_cast<char *>(&objPuoid), sizeof(objPuoid));
        if (0 >= bytesRead) {
            return;
        }
        bytesRead = file.read(reinterpret_cast<char *>(&noOfRefs), sizeof(quint32));
        if (0 >= bytesRead) {
            return;
        }
        references.clear();
        for (quint32 j = 0; j < noOfRefs; ++j) {
            bytesRead = file.read(reinterpret_cast<char *>(&referencePuoid), sizeof(referencePuoid));
            if (0 >= bytesRead) {
                return;
            }
            // Get object handle from the PUOID
            if (m_puoidToHandleMap.contains(referencePuoid)) {
                references.append(m_puoidToHandleMap[referencePuoid]);
            }
        }
        if (m_puoidToHandleMap.contains(objPuoid)) {
            m_objectReferencesMap[m_puoidToHandleMap[objPuoid]] = references;
        }
    }
}

MTPResponseCode FSStoragePlugin::getObjectPropertyValueFromStorage(
    const ObjHandle &handle, MTPObjPropertyCode propCode, QVariant &value, MTPDataType dataType)
{
    MTPResponseCode code = MTP_RESP_OK;
    const MTPObjectInfo *objectInfo;
    code = getObjectInfo(handle, objectInfo);
    if (MTP_RESP_OK != code) {
        return code;
    }
    switch (propCode) {
    case MTP_OBJ_PROP_Association_Desc: {
        value = QVariant::fromValue(0);
    }
    break;
    case MTP_OBJ_PROP_Association_Type: {
        quint16 v = objectInfo->mtpAssociationType;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Parent_Obj: {
        quint32 v = objectInfo->mtpParentObject;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Obj_Size: {
        quint64 v = objectInfo->mtpObjectCompressedSize;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_StorageID: {
        quint32 v = objectInfo->mtpStorageId;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Obj_Format: {
        quint16 v = objectInfo->mtpObjectFormat;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Protection_Status: {
        quint16 v = objectInfo->mtpProtectionStatus;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Allowed_Folder_Contents: {
        // Not supported, return empty array
        QVector<qint16> v;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Date_Modified: {
        QString v = objectInfo->mtpModificationDate;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Date_Created: {
        QString v = objectInfo->mtpCaptureDate;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Date_Added: {
        QString v = objectInfo->mtpCaptureDate;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Obj_File_Name: {
        QString v = objectInfo->mtpFileName;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Rep_Sample_Format: {
        quint16 v = MTP_OBF_FORMAT_JFIF;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Rep_Sample_Size: {
        quint32 v = THUMB_MAX_SIZE;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Rep_Sample_Height: {
        value = QVariant::fromValue(THUMB_HEIGHT);
    }
    break;
    case MTP_OBJ_PROP_Rep_Sample_Width: {
        value = QVariant::fromValue(THUMB_WIDTH);
    }
    break;
    case MTP_OBJ_PROP_Video_FourCC_Codec: {
        quint32 v = fourcc_wmv3;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Corrupt_Unplayable:
    case MTP_OBJ_PROP_Hidden: {
        quint8 v = 0;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Persistent_Unique_ObjId: {
        StorageItem *storageItem = m_objectHandlesMap.value(handle);
        value = QVariant::fromValue(storageItem->m_puoid);
    }
    break;
    case MTP_OBJ_PROP_Non_Consumable: {
        quint8 v = 0;
        value = QVariant::fromValue(v);
    }
    break;
    case MTP_OBJ_PROP_Rep_Sample_Data: {
        /* Default to returning empty octet set */
        value = QVariant::fromValue(QVector<quint8>());

        StorageItem *storageItem = m_objectHandlesMap.value(handle);
        if (!storageItem) {
            MTP_LOG_WARNING("ObjectHandle" << handle << "does not exist");
            break;
        }

        /* Check if the file is an image that the thumbnailer can process */
        if (!isThumbnailableImage(storageItem)) {
            MTP_LOG_WARNING(storageItem->path() << "is not thumbnailable image");
            break;
        }

        /* Check if thumbnail already exists / request it to be generated */
        QString thumbPath
            = m_thumbnailer->requestThumbnail(storageItem->m_path, m_imageMimeTable.value(objectInfo->mtpObjectFormat));
        if (thumbPath.isEmpty()) {
            MTP_LOG_WARNING(storageItem->path() << "has no thumbnail yet");
            break;
        }

        QFile thumbFile(thumbPath);
        qint64 size = thumbFile.size();
        /* Refuse to load insanely large (>10MB) thumbnails */
        if (size > (10 << 20)) {
            MTP_LOG_WARNING(storageItem->path() << "thumbail" << thumbPath << "is too large" << size);
            break;
        }

        if (!thumbFile.open(QIODevice::ReadOnly)) {
            MTP_LOG_WARNING(storageItem->path() << "thumbail" << thumbPath << "can't be opened for reading");
            break;
        }

        MTP_LOG_INFO(storageItem->path() << "loading thumbnail:" << thumbPath << " - size:" << size;);
        QVector<quint8> fileData(size);
        // Read file data into the vector
        // FIXME: Assumes that the entire file will be read at once
        thumbFile.read(reinterpret_cast<char *>(fileData.data()), size);
        value = QVariant::fromValue(fileData);
    }
    break;
    default:
        code = MTP_RESP_ObjectProp_Not_Supported;
        break;
    }
    MTP_LOG_INFO(
        "object:" << handle << "prop:" << mtp_code_repr(propCode) << "type:" << mtp_data_type_repr(dataType)
                  << "data:" << value << "result:" << mtp_code_repr(code));
    return code;
}

MTPResponseCode FSStoragePlugin::getObjectPropertyValue(const ObjHandle &handle, QList<MTPObjPropDescVal> &propValList)
{
    StorageItem *storageItem = m_objectHandlesMap.value(handle);
    if (!storageItem || storageItem->m_path.isEmpty()) {
        return MTP_RESP_GeneralError;
    } else {
        // First, fill in the property values that are in the object info data
        // set or statically defined.
        QList<MTPObjPropDescVal>::iterator i;
        for (i = propValList.begin(); i != propValList.end(); ++i) {
            MTPResponseCode response
                = getObjectPropertyValueFromStorage(handle, i->propDesc->uPropCode, i->propVal, i->propDesc->uDataType);
            if (response != MTP_RESP_OK && response != MTP_RESP_ObjectProp_Not_Supported) {
                // Ignore ObjectProp_Not_Supported since the value may still be
                // available in Tracker.
                return response;
            }
        }
    }
    return MTP_RESP_OK;
}

MTPResponseCode FSStoragePlugin::getChildPropertyValues(
    ObjHandle handle, const QList<const MtpObjPropDesc *> &properties, QMap<ObjHandle, QList<QVariant>> &values)
{
    if (!checkHandle(handle)) {
        return MTP_RESP_InvalidObjectHandle;
    }

    StorageItem *item = m_objectHandlesMap[handle];
    if (item->m_objectInfo->mtpObjectFormat != MTP_OBF_FORMAT_Association) {
        // Not an association.
        return MTP_RESP_InvalidObjectHandle;
    }

    StorageItem *child = item->m_firstChild;
    for (; child; child = child->m_nextSibling) {
        QList<QVariant> &childValues = values.insert(child->m_handle, QList<QVariant>()).value();
        foreach (const MtpObjPropDesc *desc, properties) {
            childValues.append(QVariant());
            getObjectPropertyValueFromStorage(child->m_handle, desc->uPropCode, childValues.last(), desc->uDataType);
        }
    }

    return MTP_RESP_OK;
}

MTPResponseCode FSStoragePlugin::setObjectPropertyValue(
    const ObjHandle &handle, QList<MTPObjPropDescVal> &propValList, bool sendObjectPropList /*=false*/)
{
    Q_UNUSED(sendObjectPropList);

    MTPResponseCode code = MTP_RESP_OK;
    StorageItem *storageItem = m_objectHandlesMap.value(handle);
    if (!storageItem) {
        return MTP_RESP_GeneralError;
    }
    for (QList<MTPObjPropDescVal>::iterator i = propValList.begin(); i != propValList.end(); ++i) {
        const MtpObjPropDesc *propDesc = i->propDesc;
        QVariant &value = i->propVal;
        // Handle filename on our own
        if (MTP_OBJ_PROP_Obj_File_Name == propDesc->uPropCode) {
            QDir dir;
            QString path = storageItem->m_path;
            path.truncate(path.lastIndexOf("/") + 1);
            QString newName = QString(value.value<QString>());
            // Check if the file name is valid
            if (!isFileNameValid(newName, storageItem->m_parent)) {
                // Bad file name
                MTP_LOG_WARNING("Bad file name in setObjectProperty!" << newName);
                return MTP_RESP_Invalid_ObjectProp_Value;
            }
            path += newName;
            if (dir.rename(storageItem->m_path, path)) {
                m_pathNamesMap.remove(storageItem->m_path);
                m_puoidsMap.remove(storageItem->m_path);

                storageItem->m_path = path;
                storageItem->m_objectInfo->mtpFileName = newName;
                m_pathNamesMap[storageItem->m_path] = handle;
                m_puoidsMap[storageItem->m_path] = storageItem->m_puoid;
                removeWatchDescriptorRecursively(storageItem);
                addWatchDescriptorRecursively(storageItem);
                StorageItem *itr = storageItem->m_firstChild;
                while (itr) {
                    adjustMovedItemsPath(path, itr);
                    itr = itr->m_nextSibling;
                }
                code = MTP_RESP_OK;
            }
        }
    }

    return code;
}

void FSStoragePlugin::receiveThumbnail(const QString &path)
{
    // Thumbnail for the file "path" is ready
    ObjHandle handle = m_pathNamesMap.value(path);
    if (0 != handle) {
        StorageItem *storageItem = m_objectHandlesMap[handle];
        storageItem->m_objectInfo->mtpThumbCompressedSize = getThumbCompressedSize(storageItem);

        QVector<quint32> params;
        params.append(handle);
        emit eventGenerated(MTP_EV_ObjectInfoChanged, params);

        params.append(MTP_OBJ_PROP_Rep_Sample_Data);
        emit eventGenerated(MTP_EV_ObjectPropChanged, params);
    }
}

void FSStoragePlugin::handleFSDelete(const struct inotify_event *event, const char *name)
{
    if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {
        MTP_LOG_INFO("Handle FS Delete::" << name);
        // Search for the object by the event's WD
        if (m_watchDescriptorMap.contains(event->wd)) {
            ObjHandle parentHandle = m_watchDescriptorMap[event->wd];
            StorageItem *parentNode = m_objectHandlesMap[parentHandle];

            if (0 != parentNode) {
                QString fullPath = parentNode->m_path + QString("/") + QString(name);
                if (m_pathNamesMap.contains(fullPath)) {
                    MTP_LOG_INFO("Handle FS Delete, deleting file::" << name);
                    ObjHandle toBeDeleted = m_pathNamesMap[fullPath];
                    deleteItemHelper(toBeDeleted, false, true);
                }
                // Emit storageinfo changed events, free space may be different from before now
                sendStorageInfoChanged();
            }
        }
    }
}

void FSStoragePlugin::handleFSCreate(const struct inotify_event *event, const char *name)
{
    if (event->mask & (IN_CREATE | IN_MOVED_TO)) {
        ObjHandle parent = m_watchDescriptorMap.value(event->wd);
        StorageItem *parentNode = m_objectHandlesMap[parent];
        MTP_LOG_INFO("Handle FS Create::" << name);

        // The above QHash::value() may return a default constructed value of 0... so we double check the wd's here
        if (parentNode && (parentNode->m_wd == event->wd)) {
            QString addedPath = parentNode->m_path + QString("/") + QString(name);
            if (!m_pathNamesMap.contains(addedPath)) {
                MTP_LOG_INFO("Handle FS create, adding file::" << name);
                addToStorage(addedPath, 0, 0, true);

                // Emit storageinfo changed events, free space may be different from before now
                sendStorageInfoChanged();
            }
        }
    }
}

void FSStoragePlugin::handleFSMove(
    const struct inotify_event *fromEvent, const char *fromName, const struct inotify_event *toEvent, const char *toName)
{
    if ((fromEvent->mask & IN_MOVED_FROM) && (toEvent->mask & IN_MOVED_TO) && (fromEvent->cookie == toEvent->cookie)) {
        ObjHandle fromHandle = m_watchDescriptorMap.value(fromEvent->wd);
        ObjHandle toHandle = m_watchDescriptorMap.value(toEvent->wd);
        StorageItem *fromNode = m_objectHandlesMap.value(fromHandle);
        StorageItem *toNode = m_objectHandlesMap.value(toHandle);

        MTP_LOG_INFO("Handle FS Move::" << fromName << toName);
        if ((fromHandle == toHandle) && (0 == qstrcmp(fromName, toName))) {
            // No change!
            return;
        }
        if ((0 != fromNode) && (0 != toNode) && (fromNode->m_wd == fromEvent->wd) && (toNode->m_wd == toEvent->wd)) {
            MTP_LOG_INFO("Handle FS Move, moving file::" << fromName << toName);
            QString oldPath = fromNode->m_path + QString("/") + QString(fromName);
            ObjHandle movedHandle = m_pathNamesMap.value(oldPath);

            if (0 == movedHandle) {
                // Already handled
                return;
            }
            StorageItem *movedNode = m_objectHandlesMap.value(movedHandle);
            if (movedNode) {
                QString newPath = toNode->m_path + QString("/") + toName;
                if (m_pathNamesMap.contains(newPath)) { // Already Handled
                    // As the destination path is already present in our tree,
                    // we only need to delete the fromNode
                    MTP_LOG_INFO("The path to rename to is already present in our tree, hence, delete the moved node "
                                 "from our tree");
                    deleteItemHelper(m_pathNamesMap[oldPath], false, true);
                    return;
                }
                MTP_LOG_INFO("Handle FS Move, moving file, found!");
                if (fromHandle == toHandle) { // Rename
                    MTP_LOG_INFO("Handle FS Move, renaming file::" << fromName << toName);
                    // Remove the old path from the path names map
                    m_pathNamesMap.remove(oldPath);
                    movedNode->m_path = newPath;
                    movedNode->m_objectInfo->mtpFileName = QString(toName);
                    m_pathNamesMap[movedNode->m_path] = movedHandle;
                    StorageItem *itr = movedNode->m_firstChild;
                    while (itr) {
                        adjustMovedItemsPath(movedNode->m_path, itr);
                        itr = itr->m_nextSibling;
                    }
                    removeWatchDescriptorRecursively(movedNode);
                    addWatchDescriptorRecursively(movedNode);
                } else {
                    moveObject(movedHandle, toHandle, this, false);
                }

                // object info would need to be computed again
                delete movedNode->m_objectInfo;
                movedNode->m_objectInfo = 0;
                populateObjectInfo(movedNode);

                if (fromNode->eventsAreEnabled())
                    toNode->setEventsEnabled(true);

                // Emit an object info changed signal
                QVector<quint32> evtParams;
                evtParams.append(movedHandle);
                emit eventGenerated(MTP_EV_ObjectInfoChanged, evtParams);
            }
        }
    }
}

static QString inotifyEventMaskRepr(int mask)
{
    QString res;

#define X(f) \
    do { \
        if (mask & IN_##f) { \
            if (!res.isEmpty()) \
                res.append("|"); \
            res.append(#f); \
        } \
    } while (0)

    X(ACCESS);
    X(ATTRIB);
    X(CLOSE_WRITE);
    X(CLOSE_NOWRITE);
    X(CREATE);
    X(DELETE);
    X(DELETE_SELF);
    X(MODIFY);
    X(MOVE_SELF);
    X(MOVED_FROM);
    X(MOVED_TO);
    X(OPEN);

#undef X
    return res;
}

void FSStoragePlugin::sendStorageInfoChanged()
{
    MTPStorageInfo info;
    storageInfo(info);

    if (!info.maxCapacity)
        return;

    int oldPercent = 100 * m_reportedFreeSpace / info.maxCapacity;
    int newPercent = 100 * info.freeSpace / info.maxCapacity;

    if (oldPercent != newPercent) {
        MTP_LOG_INFO("freeSpace changed:" << oldPercent << "->" << newPercent);
        m_reportedFreeSpace = info.freeSpace;

        QVector<quint32> eventParams;
        eventParams.append(m_storageId);
        emit eventGenerated(MTP_EV_StorageInfoChanged, eventParams);
    }
}

void FSStoragePlugin::handleFSModify(const struct inotify_event *event, const char *name)
{
    MTP_LOG_INFO((name ?: "null") << inotifyEventMaskRepr(event->mask));

    if (event->mask & IN_CLOSE_WRITE) {
        ObjHandle parent = m_watchDescriptorMap.value(event->wd);
        StorageItem *parentNode = m_objectHandlesMap.value(parent);
        //MTP_LOG_INFO("Handle FS Modify::" << name);
        if (parentNode && (parentNode->m_wd == event->wd)) {
            QString changedPath = parentNode->m_path + QString("/") + QString(name);
            ObjHandle changedHandle = m_pathNamesMap.value(changedPath);
            // Don't fire the change signal in the case when there is a transfer to the device ongoing
            if ((0 != changedHandle) && (changedHandle != m_writeObjectHandle)) {
                StorageItem *item = m_objectHandlesMap.value(changedHandle);
                // object info would need to be computed again
                MTPObjectInfo *prev = item->m_objectInfo;
                item->m_objectInfo = 0;
                populateObjectInfo(item);
                bool changed = !prev || prev->differsFrom(item->m_objectInfo);
                delete prev;
                MTP_LOG_INFO(
                    "Handle FS Modify, file::" << name << "handle:" << changedHandle
                                               << "writing:" << m_writeObjectHandle << "changed:" << changed);
                // Emit an object info changed event
                QVector<quint32> eventParams;
                if (changed) {
                    eventParams.append(changedHandle);
                    emit eventGenerated(MTP_EV_ObjectInfoChanged, eventParams);
                }

                sendStorageInfoChanged();
            }
        }
    }
}

void FSStoragePlugin::cacheInotifyEvent(const struct inotify_event *event, const char *name)
{
    m_iNotifyCache.fromEvent = *event;
    m_iNotifyCache.fromName = QString(name);
}

void FSStoragePlugin::getCachedInotifyEvent(const struct inotify_event **event, QString &name)
{
    if (0 != m_iNotifyCache.fromEvent.cookie) {
        *event = &m_iNotifyCache.fromEvent;
        name = m_iNotifyCache.fromName;
    } else {
        *event = 0;
        name = "";
    }
}

void FSStoragePlugin::clearCachedInotifyEvent()
{
    m_iNotifyCache.fromName.clear();
    memset(&m_iNotifyCache.fromEvent, 0, sizeof(m_iNotifyCache.fromEvent));
}

void FSStoragePlugin::removeWatchDescriptorRecursively(StorageItem *item)
{
    StorageItem *itr;
    if (item && item->m_objectInfo && MTP_OBF_FORMAT_Association == item->m_objectInfo->mtpObjectFormat) {
        removeWatchDescriptor(item);
        for (itr = item->m_firstChild; itr; itr = itr->m_nextSibling) {
            removeWatchDescriptorRecursively(itr);
        }
    }
}

void FSStoragePlugin::removeWatchDescriptor(StorageItem *item)
{
    if (item && item->m_objectInfo && MTP_OBF_FORMAT_Association == item->m_objectInfo->mtpObjectFormat) {
        m_inotify->removeWatch(item->m_wd);
        m_watchDescriptorMap.remove(item->m_wd);
    }
}

void FSStoragePlugin::addWatchDescriptorRecursively(StorageItem *item)
{
    StorageItem *itr;
    if (item && item->m_objectInfo && MTP_OBF_FORMAT_Association == item->m_objectInfo->mtpObjectFormat) {
        addWatchDescriptor(item);
        for (itr = item->m_firstChild; itr; itr = itr->m_nextSibling) {
            addWatchDescriptorRecursively(itr);
        }
    }
}

void FSStoragePlugin::addWatchDescriptor(StorageItem *item)
{
    if (item && item->m_objectInfo && MTP_OBF_FORMAT_Association == item->m_objectInfo->mtpObjectFormat) {
        item->m_wd = m_inotify->addWatch(item->m_path);
        if (-1 != item->m_wd) {
            m_watchDescriptorMap[item->m_wd] = item->m_handle;
        }
    }
}

bool FSStoragePlugin::isFileNameValid(const QString &fileName, const StorageItem *parent)
{
    // Check if the file name contains illegal characters, or if the file with
    // the same name is already present under the parent
    if (fileName.contains(QRegExp(FILENAMES_FILTER_REGEX)) || QRegExp("[\\.]+").exactMatch(fileName)) {
        // Illegal characters, or all .'s
        return false;
    }
    if (m_pathNamesMap.contains(parent->m_path + "/" + fileName)) {
        // Already present
        return false;
    }
    return true;
}

void FSStoragePlugin::excludePath(const QString &path)
{
    m_excludePaths << (m_storagePath + "/" + path);
    MTP_LOG_INFO("Storage" << m_storageInfo.volumeLabel << "excluded" << path << "from being exported via MTP.");
}

QString FSStoragePlugin::filesystemUuid() const
{
#ifndef UT_ON
    typedef QScopedPointer<char, QScopedPointerPodDeleter> CharPointer;

    QString result;

    CharPointer mountpoint(mnt_get_mountpoint(m_storagePath.toUtf8().constData()));
    if (!mountpoint) {
        MTP_LOG_WARNING("mnt_get_mountpoint failed.");
        return result;
    }

    libmnt_table *mntTable = mnt_new_table_from_file("/proc/self/mountinfo");
    if (!mntTable) {
        MTP_LOG_WARNING("Couldn't parse /proc/self/mountinfo.");
        return result;
    }

    libmnt_fs *fs = mnt_table_find_target(mntTable, mountpoint.data(), MNT_ITER_FORWARD);
    const char *devicePath = mnt_fs_get_source(fs);

    if (devicePath) {
        blkid_cache cache;
        if (blkid_get_cache(&cache, NULL) != 0) {
            MTP_LOG_WARNING("Couldn't get blkid cache.");
        } else {
            char *uuid = blkid_get_tag_value(cache, "UUID", devicePath);
            blkid_put_cache(cache);

            result = uuid;
            free(uuid);
        }
    } else {
        MTP_LOG_WARNING("Couldn't determine block device for storage.");
    }

    mnt_free_table(mntTable);

    return result;
#else
    return QStringLiteral("FAKE-TEST-UUID");
#endif
}
