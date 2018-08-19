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


#ifndef K3B_CDPARANOIA_LIB_H
#define K3B_CDPARANOIA_LIB_H

// from cdda_interface.h
#define CD_FRAMESIZE_RAW 2352

#include "k3b_export.h"

#include <QString>

#include <sys/types.h>

#ifdef Q_OS_WIN32
#undef S_OK
#endif

namespace K3b {
    namespace Device {
        class Device;
        class Toc;
    }


    /**
     * CdparanoiaLib is a convenience wrapper around libcdda_interface
     * and libcdda_paranoia.
     *
     * It uses four paranoia levels 0-3 which can be set via setParanoiaMode
     * and are used the same way as in cdrdao:
     * \li 0: No checking, data is copied directly from the drive.
     * \li 1: Perform overlapped reading to avoid jitter.
     * \li 2: Like 1 but with additional checks of the read audio data.
     * \li 3: Like 2 but with additional scratch detection and repair.
     *
     * CdparanoiaLib is based on a shared data approach which makes sure
     * that each device can only be opened once. This is necessary since
     * libcdda_interface opens the device exclusively on most distributions.
     *
     * However, it is perfectly possible to have two instances of CdparanoiaLib
     * accessing the same device at the same time. CdparanoiaLib will take care
     * of the syncing and seeking issues automatically.
     *
     * CdparanoiaLib is thread-safe.
     *
     * Usage:
     * <pre>
     * CdparanoiaLib lib;
     * lib.initParanoia( mydevice );
     * lib.initReading( tracknumber );
     * while( char* data = lib.read() )
     *   dosomethingcoolwithdata( data );
     * </pre>
     */
    class LIBK3B_EXPORT CdparanoiaLib
    {
    public:
        ~CdparanoiaLib();

        /** default: 1 */
        void setParanoiaMode( int );
        void setNeverSkip( bool b );

        /** default: 5 */
        void setMaxRetries( int );

        /**
         * This will read the Toc and initialize some stuff.
         * It will also call paranoiaInit( const QString& )
         */
        bool initParanoia( Device::Device* dev );

        /**
         * Use for faster initialization without reading the toc
         */
        bool initParanoia( Device::Device* dev, const Device::Toc& );

        void close();

        /**
         * Call this after initParanoia to set the data to rip.
         *
         * Rip all audio tracks.
         */
        bool initReading();

        /**
         * Call this after initParanoia to set the data to rip.
         */
        bool initReading( int track );

        /**
         * Call this after initParanoia to set the data to rip.
         */
        bool initReading( long startSector, long endSector );

        /**
         * Read data.
         * \param statusCode If not 0 will be set.
         * \param track the tracknumer the data belongs to
         *
         * This method takes care of swapping the byte-order depending on the
         * machine type.
         *
         * \return The read sector data or 0 if all data within the specified range
         *         has been read or an error has occurred.
         */
        char* read( int* statusCode = 0, unsigned int* track = 0, bool littleEndian = true );

        /**
         * This only is valid after a call to read()
         */
        int status() const;

        enum Status {
            S_OK,
            S_ERROR
            // to be extended with Jitter and stuff...
        };

        /**
         * Only valid after a call to initParanoia()
         */
        const Device::Toc& toc() const;

        long rippedDataLength() const;

        /**
         * returns 0 if the cdparanoialib could not
         * be found on the system.
         * Otherwise you have to take care of
         * deleting.
         */
        static CdparanoiaLib* create();

    private:
        void cleanup();

        CdparanoiaLib();
        bool load();

        class Private;
        Private* d;
    };
}


#endif
