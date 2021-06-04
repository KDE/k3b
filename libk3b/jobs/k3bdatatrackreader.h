/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef _K3B_DATATRACK_READER_H_
#define _K3B_DATATRACK_READER_H_

#include "k3bthreadjob.h"

#include "k3bglobals.h"
#include "k3bmsf.h"

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
        explicit DataTrackReader( JobHandler*, QObject* parent = 0 );
        ~DataTrackReader() override;

        enum ReadSectorSize {
            AUTO = K3b::SectorSizeAuto,
            MODE1 = K3b::SectorSizeData2048,
            MODE2FORM1 = K3b::SectorSizeData2048Subheader,
            MODE2FORM2 = K3b::SectorSizeData2324Subheader
        };

        void setSectorSize( ReadSectorSize size );

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
        bool run() override;

        int read( unsigned char* buffer, unsigned long sector, unsigned int len );
        bool retryRead( unsigned char* buffer, unsigned long startSector, unsigned int len );
        bool setErrorRecovery( Device::Device* dev, int code );

        class Private;
        Private* const d;
    };
}

#endif
