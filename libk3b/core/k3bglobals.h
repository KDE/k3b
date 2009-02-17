/* 
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BGLOBALS_H
#define K3BGLOBALS_H

#include <config-k3b.h>

#include <qstring.h>
#include <qfile.h>
#include <kio/global.h>
#include <kurl.h>
#include <k3bdevicetypes.h>
#include "k3b_export.h"

class K3bVersion;
class K3bExternalBin;


#include <sys/stat.h>


#ifdef HAVE_STAT64
#define k3b_struct_stat struct stat64
#define k3b_stat        ::stat64
#define k3b_lstat       ::lstat64
#else
#define k3b_struct_stat struct stat
#define k3b_stat        ::stat
#define k3b_lstat       ::lstat
#endif


namespace K3bDevice {
    class Device;
}

namespace K3b
{
    enum WritingApp { 
        WRITING_APP_DEFAULT = 1, 
        WRITING_APP_CDRECORD = 2, 
        WRITING_APP_CDRDAO = 4,
        WRITING_APP_GROWISOFS = 16,
        WRITING_APP_DVD_RW_FORMAT = 32
    };
    Q_DECLARE_FLAGS( WritingApps, WritingApp )

    LIBK3B_EXPORT K3b::WritingApp writingAppFromString( const QString& );
    LIBK3B_EXPORT QString writingAppToString( K3b::WritingApp );

    /**
     * The data mode which determines the size of the user data in data
     * CD sectors.
     */
    enum DataMode { 
        DATA_MODE_AUTO, /**< let %K3b determine the best mode */
        DATA_MODE_1,    /**< refers to the default Yellow book mode1 */
        DATA_MODE_2     /**< refers to CDROM XA mode2 form1 */
    };

    /**
     * The sector size denotes the number of bytes K3b provides per sector.
     * This is based on the sizes cdrecord's -data, -xa, and -xamix parameters
     * demand.
     */
    enum SectorSize {
        SECTORSIZE_AUDIO = 2352,
        SECTORSIZE_DATA_2048 = 2048,
        SECTORSIZE_DATA_2048_SUBHEADER = 2056,
        SECTORSIZE_DATA_2324 = 2324,
        SECTORSIZE_DATA_2324_SUBHEADER = 2332,
        SECTORSIZE_RAW = 2448
    };

    /**
     * WRITING_MODE_AUTO  - let K3b determine the best mode
     * WRITING_MODE_TAO   - Track at once
     * WRITING_MODE_DAO   - Disk at once (or session at once)
     * WRITING_MODE_RAW   - Raw mode
     *
     * may be or'ed together (except for WRITING_MODE_AUTO of course)
     */
    enum WritingMode { 
        WRITING_MODE_AUTO = 0, 
        WRITING_MODE_TAO = K3bDevice::WRITINGMODE_TAO, 
        WRITING_MODE_DAO = K3bDevice::WRITINGMODE_SAO, 
        WRITING_MODE_RAW = K3bDevice::WRITINGMODE_RAW,
        WRITING_MODE_INCR_SEQ = K3bDevice::WRITINGMODE_INCR_SEQ,  // Incremental Sequential
        WRITING_MODE_RES_OVWR = K3bDevice::WRITINGMODE_RES_OVWR // Restricted Overwrite
    };
    Q_DECLARE_FLAGS( WritingModes, WritingMode )

    LIBK3B_EXPORT QString writingModeString( K3b::WritingModes );

    LIBK3B_EXPORT QString framesToString( int h, bool showFrames = true );
    /*LIBK3B_EXPORT QString sizeToTime( long size );*/

    LIBK3B_EXPORT qint16 swapByteOrder( const qint16& i );
    LIBK3B_EXPORT qint32 swapByteOrder( const qint32& i );
    LIBK3B_EXPORT qint64 swapByteOrder( const qint64& i );

    int round( double );

    /**
     * This checks the free space on the filesystem path is in.
     * We use this since we encountered problems with the KDE version.
     * @returns true on success.
     */
    LIBK3B_EXPORT bool kbFreeOnFs( const QString& path, unsigned long& size, unsigned long& avail );

    /**
     * Cut a filename preserving the extension
     */
    LIBK3B_EXPORT QString cutFilename( const QString& name, int len );

    LIBK3B_EXPORT QString removeFilenameExtension( const QString& name );

    /**
     * Append a number to a filename preserving the extension.
     * The resulting name's length will not exceed @p maxlen
     */
    LIBK3B_EXPORT QString appendNumberToFilename( const QString& name, int num, unsigned int maxlen );

    LIBK3B_EXPORT QString findUniqueFilePrefix( const QString& _prefix = QString(), const QString& path = QString() );

    /**
     * Find a unique filename in directory d (if d is empty the method uses the defaultTempPath)
     */
    LIBK3B_EXPORT QString findTempFile( const QString& ending = QString(), const QString& d = QString() );

    /**
     * Wrapper around KStandardDirs::findExe which searches the PATH and some additional
     * directories to find system tools which are normally only in root's PATH.
     */
    LIBK3B_EXPORT QString findExe( const QString& name );

    /**
     * get the default K3b temp path to store image files
     */
    LIBK3B_EXPORT QString defaultTempPath();

    /**
     * makes sure a path ends with a "/"
     */
    LIBK3B_EXPORT QString prepareDir( const QString& dir );

    /**
     * returns the parent dir of a path.
     * CAUTION: this does only work well with absolut paths.
     *
     * Example: /usr/share/doc -> /usr/share/
     */
    QString parentDir( const QString& path );

    /**
     * For now this just replaces multiple occurrences of / with a single /
     */
    LIBK3B_EXPORT QString fixupPath( const QString& );

    /**
     * resolves a symlinks completely. Meaning it also handles links to links to links...
     */
    LIBK3B_EXPORT QString resolveLink( const QString& );

    LIBK3B_EXPORT K3bVersion kernelVersion();

    /**
     * Kernel version stripped of all suffixes
     */
    LIBK3B_EXPORT K3bVersion simpleKernelVersion();

    QString systemName();

    LIBK3B_EXPORT KIO::filesize_t filesize( const KUrl& );

    /**
     * Calculate the total size of an image file. This also includes
     * images splitted by a K3bFileSplitter.
     *
     * \returns the total size of the image file at url
     */
    LIBK3B_EXPORT KIO::filesize_t imageFilesize( const KUrl& url );

    /**
     * true if the kernel supports ATAPI devices without SCSI emulation.
     * use in combination with the K3bExternalProgram feature "plain-atapi"
     */
    LIBK3B_EXPORT bool plainAtapiSupport();
  
    /**
     * true if the kernel supports ATAPI devices without SCSI emulation
     * via the ATAPI: pseudo stuff
     * use in combination with the K3bExternalProgram feature "hacked-atapi"
     */
    LIBK3B_EXPORT bool hackedAtapiSupport();

    /**
     * Used to create a parameter for cdrecord, cdrdao or readcd.
     * Takes care of SCSI and ATAPI.
     */
    QString externalBinDeviceParameter( K3bDevice::Device* dev, const K3bExternalBin* );

    /**
     * Tries to convert urls from local protocols != "file" to file (for now supports media:/)
     */
    LIBK3B_EXPORT KUrl convertToLocalUrl( const KUrl& url );
    LIBK3B_EXPORT KUrl::List convertToLocalUrls( const KUrl::List& l );

    LIBK3B_EXPORT qint16 fromLe16( char* );
    LIBK3B_EXPORT qint32 fromLe32( char* );
    LIBK3B_EXPORT qint64 fromLe64( char* );

    LIBK3B_EXPORT bool isMounted( K3bDevice::Device* );

    /**
     * Tries to unmount the device ignoring its actual mounting state.
     * This method uses both KIO::unmount and pumount if available.
     */
    LIBK3B_EXPORT bool unmount( K3bDevice::Device* );

    /**
     * Tries to mount the medium. Since K3b does not gather any information
     * about mount points the only methods used are pmount and HAL::mount
     */
    LIBK3B_EXPORT bool mount( K3bDevice::Device* );

    /**
     * Ejects the medium in the device or simply opens the tray.
     * This method improves over K3bDevice::Device::eject in that it
     * unmounts before ejecting and introduces HAL support.
     */
    LIBK3B_EXPORT bool eject( K3bDevice::Device* );
}

Q_DECLARE_OPERATORS_FOR_FLAGS(K3b::WritingApps)
Q_DECLARE_OPERATORS_FOR_FLAGS(K3b::WritingModes)

#endif
