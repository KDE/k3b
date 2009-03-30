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

#ifndef _K3B_DATATRACK_READER_H_
#define _K3B_DATATRACK_READER_H_


#include "k3bthreadjob.h"

#include "k3bmsf.h"
#include "k3bglobals.h"

class QIODevice;

namespace K3b {
    namespace Device {
        class Device;
    }


    /**
     * This is a replacement for readcd. We need this since
     * it is not possible to influence the sector size used
     * by readcd and readcd is not very good to handle anyway.
     *
     * The sector size read is the following:
     * @li Mode1: 2048 bytes (only user data)
     * @li Mode2 Form1: 2056 bytes containing the subheader and the user data
     * @li Mode2 Form2: 2332 bytes containing the subheader and the user data
     *
     * Formless Mode2 sectors will not be read.
     */
    class DataTrackReader : public ThreadJob
    {
        Q_OBJECT

    public:
        DataTrackReader( JobHandler*, QObject* parent = 0 );
        ~DataTrackReader();

        enum SectorSize {
            AUTO = 0,
            MODE1 = SectorSizeData2048,
            MODE2FORM1 = SectorSizeData2048Subheader,
            MODE2FORM2 = SectorSizeData2324Subheader
        };

        void setSectorSize( SectorSize size );

        void setDevice( Device::Device* );

        void setImagePath( const QString& p );

        /**
         * @param start the first sector to be read
         * @end the last sector to be read
         */
        void setSectorRange( const Msf& start, const Msf& end );
        void setRetries( int );

        /**
         * If true unreadable sectors will be replaced by zero data to always
         * maintain the track length.
         */
        void setIgnoreErrors( bool b );

        void setNoCorrection( bool b );

        void writeTo( QIODevice* ioDev );

    private:
        bool run();

        int read( unsigned char* buffer, unsigned long sector, unsigned int len );
        bool retryRead( unsigned char* buffer, unsigned long startSector, unsigned int len );
        bool setErrorRecovery( Device::Device* dev, int code );

        class Private;
        Private* const d;
    };
}

#endif
