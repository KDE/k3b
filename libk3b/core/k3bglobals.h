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


#ifndef _K3B_GLOBALS_H_
#define _K3B_GLOBALS_H_

#include <config-k3b.h>

#include "k3bdevicetypes.h"
#include "k3b_export.h"

#include <KIO/Global>

#include <QFile>
#include <QString>
#include <QUrl>

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


namespace K3b {
    class ExternalBin;
    class Msf;
    class Version;
    namespace Device {
        class Device;
    }

    enum MediaSize {
        MediaSizeCd74Min = 74*60*75,
        MediaSizeCd80Min = 80*60*75,
        MediaSizeCd100Min = 100*60*75,

        MediaSizeDvd4Gb = 2295104,
        MediaSizeDvd8Gb = 4173824,

        MediaSizeBluRay25Gb = 12219392,
        MediaSizeBluRay50Gb = 24438784
    };

    enum WritingApp {
        WritingAppAuto = 0,
        WritingAppCdrecord = 1,
        WritingAppCdrdao = 2,
        WritingAppGrowisofs = 4,
        WritingAppDvdRwFormat = 8,
        WritingAppCdrskin = 9
    };
    Q_DECLARE_FLAGS( WritingApps, WritingApp )

    LIBK3B_EXPORT WritingApp writingAppFromString( const QString& );
    LIBK3B_EXPORT QString writingAppToString( WritingApp );

    /**
     * The data mode which determines the size of the user data in data
     * CD sectors.
     */
    enum DataMode {
        DataModeAuto, /**< let %K3b determine the best mode */
        DataMode1,    /**< refers to the default Yellow book mode1 */
        DataMode2     /**< refers to CDROM XA mode2 form1 */
    };

    /**
     * The sector size denotes the number of bytes K3b provides per sector.
     * This is based on the sizes cdrecord's -data, -xa, and -xamix parameters
     * demand.
     */
    enum SectorSize {
        SectorSizeAuto = 0,
        SectorSizeAudio = 2352,
        SectorSizeData2048 = 2048,
        SectorSizeData2048Subheader = 2056,
        SectorSizeData2324 = 2324,
        SectorSizeData2324Subheader = 2332,
        SectorSizeRaw = 2448
    };

    /**
     * WritingModeAuto  - let K3b determine the best mode
     * WritingModeTao   - Track at once
     * WritingModeSao   - Disk at once (or session at once)
     * WritingModeRaw   - Raw mode
     *
     * may be or'ed together (except for WritingModeAuto of course)
     */
    enum WritingMode {
        WritingModeAuto = 0,
        WritingModeTao = Device::WRITINGMODE_TAO,
        WritingModeSao = Device::WRITINGMODE_SAO,
        WritingModeRaw = Device::WRITINGMODE_RAW,
        WritingModeIncrementalSequential = Device::WRITINGMODE_INCR_SEQ,  // Incremental Sequential
        WritingModeRestrictedOverwrite = Device::WRITINGMODE_RES_OVWR // Restricted Overwrite
    };
    Q_DECLARE_FLAGS( WritingModes, WritingMode )

    /**
     * Unified mode for erasing/formatting of CD-RW/DVD-RW/BD-RW
     */
    enum FormattingMode {
        FormattingComplete = 0,
        FormattingQuick = 1
    };

    LIBK3B_EXPORT QString writingModeString( WritingModes );

    LIBK3B_EXPORT qint16 swapByteOrder( const qint16& i );
    LIBK3B_EXPORT qint32 swapByteOrder( const qint32& i );
    LIBK3B_EXPORT qint64 swapByteOrder( const qint64& i );

    /**
     * This checks the free space on the filesystem path is in.
     * We use this since we encountered problems with the KDE version.
     * @returns true on success.
     *
     * \deprecated Use KDiskFreeSpaceInfo
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
     * Wrapper around QStandardPaths::findExecutable which searches the PATH and some additional
     * directories to find system tools which are normally only in root's PATH.
     */
    LIBK3B_EXPORT QString findExe( const QString& name );

    /**
     * get the default K3b temp path to store image files
     *
     * \sa GlobalSettings::defaultTempPath
     */
    LIBK3B_EXPORT QString defaultTempPath();

    /**
     * makes sure a path ends with a "/"
     */
    LIBK3B_EXPORT QString prepareDir( const QString& dir );

    /**
     * returns the parent dir of a path.
     * CAUTION: this does only work well with absolute paths.
     *
     * Example: /usr/share/doc -> /usr/share/
     */
    LIBK3B_EXPORT QString parentDir( const QString& path );

    /**
     * For now this just replaces multiple occurrences of / with a single /
     */
    LIBK3B_EXPORT QString fixupPath( const QString& );

    /**
     * resolves a symlinks completely. Meaning it also handles links to links to links...
     */
    LIBK3B_EXPORT QString resolveLink( const QString& );

    LIBK3B_EXPORT Version kernelVersion();

    /**
     * Kernel version stripped of all suffixes
     */
    LIBK3B_EXPORT Version simpleKernelVersion();

    LIBK3B_EXPORT QString systemName();

    LIBK3B_EXPORT KIO::filesize_t filesize( const QUrl& );

    /**
     * Calculate the total size of an image file. This also includes
     * images split by a FileSplitter.
     *
     * \returns the total size of the image file at url
     */
    LIBK3B_EXPORT KIO::filesize_t imageFilesize( const QUrl& url );

    /**
     * true if the kernel supports ATAPI devices without SCSI emulation.
     * use in combination with the ExternalProgram feature "plain-atapi"
     */
    LIBK3B_EXPORT bool plainAtapiSupport();

    /**
     * true if the kernel supports ATAPI devices without SCSI emulation
     * via the ATAPI: pseudo stuff
     * use in combination with the ExternalProgram feature "hacked-atapi"
     */
    LIBK3B_EXPORT bool hackedAtapiSupport();

    /**
     * Used to create a parameter for cdrecord, cdrdao or readcd.
     * Takes care of SCSI and ATAPI.
     */
    LIBK3B_EXPORT QString externalBinDeviceParameter( Device::Device* dev, const ExternalBin* );

    /**
     * Tries to convert urls from local protocols != "file" to file (for now supports media:/)
     */
    LIBK3B_EXPORT QUrl convertToLocalUrl( const QUrl& url );
    LIBK3B_EXPORT QList<QUrl> convertToLocalUrls( const QList<QUrl>& l );

    LIBK3B_EXPORT qint16 fromLe16( char* );
    LIBK3B_EXPORT qint32 fromLe32( char* );
    LIBK3B_EXPORT qint64 fromLe64( char* );

    LIBK3B_EXPORT bool isMounted( Device::Device* );

    /**
     * Tries to unmount the device ignoring its actual mounting state.
     * This method uses both KIO::unmount and pumount if available.
     */
    LIBK3B_EXPORT bool unmount( Device::Device* );

    /**
     * Tries to mount the medium. Since K3b does not gather any information
     * about mount points the only methods used are pmount and HAL::mount
     */
    LIBK3B_EXPORT bool mount( Device::Device* );

    /**
     * Ejects the medium in the device or simply opens the tray.
     * This method improves over Device::Device::eject in that it
     * unmounts before ejecting and introduces HAL support.
     *
     * It also makes sure the MediaCache is up to date. This is very
     * important in case one uses the EmptyDiscWaiter directly after
     * ejecting. If the MediaCache would not be updated, it might still
     * contain the old media information.
     *
     * \sa MediaCache::reset
     */
    LIBK3B_EXPORT bool eject( Device::Device* );

    /**
     * Get the speed multiplicator for a media type.
     * \sa K3b::Device::SpeedMultiplicator
     */
    LIBK3B_EXPORT K3b::Device::SpeedMultiplicator speedMultiplicatorForMediaType( K3b::Device::MediaType mediaType );

    /**
     * Describes format of writing speed produced by formatWritingSpeedFactor function.
     * \sa K3b::formatWritingSpeedFactor
     */
    enum SpeedFormat {
        SpeedFormatInteger, /**< Format as integer number */
        SpeedFormatReal     /**< Format as real number (integer or fraction) */
    };

    /**
     * Create a string representation of the speed factor to be used in command line
     * commands like cdrecord and growisofs.
     *
     * \param speed The speed in KB/s
     * \param mediaType The media type that is going to be written. This is used to
     * determine the multiplicator factor.
     * \param speedFormat specifies format of speed value. E.g. cdrecord only accepts
     * integral speed values, in that case SpeedFormatInteger can be used.
     *
     * This method takes small variances into account and rounds them properly. Also
     * the "weird" burn speeds like 2.4 are handled.
     */
    LIBK3B_EXPORT QString
    formatWritingSpeedFactor( int speed, K3b::Device::MediaType mediaType, SpeedFormat speedFormat = SpeedFormatReal );

    /**
     * Checks if overburn can be performed taking into consideration
     * project size and 'overburn' setting in GlobalSettings.
     * \param projectSize Size of project to be written
     * \param capacity Declared capacity of a medium
     */
    LIBK3B_EXPORT bool IsOverburnAllowed( const Msf& projectSize, const Msf& capacity );

    /**
     * Checks if overburn can be performed taking into consideration
     * project size, size of data already written to disk and 'overburn' setting in GlobalSettings.
     * \param projectSize Size of project to be written
     * \param capacity Declared capacity of a medium
     * \param usedCapacity Size of the used part of a medium
     */
    LIBK3B_EXPORT bool IsOverburnAllowed( const Msf& projectSize, const Msf& capacity, const Msf& usedCapacity );

    QDebug& operator<<( QDebug& dbg, WritingMode );
    QDebug& operator<<( QDebug& dbg, WritingModes );
}

Q_DECLARE_OPERATORS_FOR_FLAGS(K3b::WritingApps)
Q_DECLARE_OPERATORS_FOR_FLAGS(K3b::WritingModes)

#endif
