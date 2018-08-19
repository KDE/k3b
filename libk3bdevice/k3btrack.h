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



#ifndef K3BTRACK_H
#define K3BTRACK_H

#include "k3bmsf.h"
#include "k3bdevice_export.h"

#include <QSharedDataPointer>

namespace K3b {
    namespace Device
    {
        class LIBK3BDEVICE_EXPORT Track
        {
        public:
            enum TrackType {
                TYPE_AUDIO,
                TYPE_DATA,
                TYPE_UNKNOWN = -1
            };

            enum DataMode {
                MODE1,
                MODE2,
                XA_FORM1,
                XA_FORM2,
                DVD,
                UNKNOWN
            };

            Track();
            Track( const Track& );
            Track( const K3b::Msf& firstSector,
                   const K3b::Msf& lastSector,
                   TrackType type,
                   DataMode mode = UNKNOWN );
            ~Track();

            Track& operator=( const Track& );

            TrackType type() const;

            void setType( TrackType );

            /**
             * UNKNOWN for DVDs and Audio CDs
             */
            DataMode mode() const;

            void setMode( DataMode );

            /**
             * Invalid for DVDs
             */
            bool copyPermitted() const;
            void setCopyPermitted( bool b );

            /**
             * Only valid for audio tracks
             */
            bool preEmphasis() const;
            void setPreEmphasis( bool b );

            bool recordedIncremental() const;
            bool recordedUninterrupted() const;

            QByteArray isrc() const;
            void setIsrc( const QByteArray& s );

            K3b::Msf firstSector() const;
            K3b::Msf lastSector() const;
            void setFirstSector( const K3b::Msf& msf );
            void setLastSector( const K3b::Msf& msf );

            K3b::Msf nextWritableAddress() const;
            void setNextWritableAddress( const K3b::Msf& );

            K3b::Msf freeBlocks() const;
            void setFreeBlocks( const K3b::Msf& );

            K3b::Msf length() const;

            /**
             * This takes index0 into account
             */
            K3b::Msf realAudioLength() const;

            /**
             * 0 if unknown
             */
            int session() const;
            void setSession( int s );

            /**
             * @return number of indices. This does not include index 0.
             */
            int indexCount() const;

            /**
             * Returns the index relative to the track's start.
             * If it is zero there is no index0.
             */
            K3b::Msf index0() const;

            /**
             * Set the track's index0 value.
             * @param msf offset relative to track start.
             */
            void setIndex0( const K3b::Msf& msf );

            /**
             * All indices. Normally this list is empty as indices are rarely used.
             * Starts with index 2 (since index 1 are all other sectors FIXME)
             */
            QList<K3b::Msf> indices() const;

            void setIndices( const QList<K3b::Msf>& );

            bool operator==( const Track& ) const;
            bool operator!=( const Track& ) const;

        private:
            class Private;
            QSharedDataPointer<Private> d;
        };
    }
}

LIBK3BDEVICE_EXPORT QDebug operator<<( QDebug s, const K3b::Device::Track& track );

/**
 * Dummy implementation to allow compilation on Windows
 */
uint qHash( const K3b::Device::Track& key );

#endif
