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

#ifndef _K3B_VERIFICATION_JOB_H_
#define _K3B_VERIFICATION_JOB_H_

#include "k3bjob.h"

#include <QByteArray>

namespace K3b {
    namespace Device {
        class Device;
        class DeviceHandler;
    }


    /**
     * Generic verification job. Add tracks to be verified via addTrack.
     * The job will then verifiy the tracks set against the set checksums.
     *
     * The different track types are handled as follows:
     * \li Data/DVD tracks: Read the track with a 2048 bytes sector size.
     *     Tracks length on DVD+RW media will be read from the iso9660
     *     descriptor.
     * \li Audio tracks: Rip the track with a 2352 bytes sector size.
     *     In the case of audio tracks the job will not fail if the checksums
     *     differ because audio CD tracks do not contain error correction data.
     *     In this case only a warning will be emitted.
     *
     * Other sector sizes than 2048 bytes for data tracks are not supported yet,
     * i.e. Video CDs cannot be verified.
     *
     * TAO written tracks have two run-out sectors that are not read.
     */
    class VerificationJob : public Job
    {
        Q_OBJECT

    public:
        explicit VerificationJob( JobHandler*, QObject* parent = 0 );
        ~VerificationJob() override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;
        void setDevice( Device::Device* dev );

        void clear();

        /**
         * Add a track to be verified.
         * \param tracknum The number of the track. If \a tracknum is 0
         *        the last track will be verified.
         * \param length Set to override the track length from the TOC. This may be
         *        useful when writing to DVD+RW media and the iso descriptor does not
         *        contain the exact image size (as true for many commercial Video DVDs)
         */
        void addTrack( int tracknum, const QByteArray& checksum, const Msf& length = Msf() );

        /**
         * Handle the special case of iso session growing
         */
        void setGrownSessionSize( const Msf& );

    private Q_SLOTS:
        void slotMediaLoaded();
        void slotDiskInfoReady( K3b::Device::DeviceHandler* dh );
        void readTrack();
        void slotReaderProgress( int p );
        void slotReaderFinished( bool success );

    private:
        class Private;
        Private* d;
    };
}

#endif
