/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_INF_FILE_WRITER_H_
#define _K3B_INF_FILE_WRITER_H_

#include "k3bmsf.h"


namespace K3b {
    namespace Device {
        class Track;
        class TrackCdText;
        class CdText;
    }

    class InfFileWriter
    {
    public:
        InfFileWriter();

        bool save( QTextStream& );
        bool save( const QString& filename );

        /**
         * Use this to set:
         * @li trackStart
         * @li trackLength
         * @li index0
         * @li all indices
         * @li preemphasis
         * @li copyPermitted
         * @li ISRC
         *
         * Endianess is set to big.
         *
         * Tracknumber needs to be set manually.
         */
        void setTrack( const Device::Track& );

        void clearIndices() { m_indices.clear(); }

        /**
         * This is relative to the track start
         */
        void setIndex0( int i ) { m_index0 = i; }
        void addIndex( long i );

        void setTrackNumber( int i ) { m_trackNumber = i; }

        void setTrackStart( const Msf& i ) { m_trackStart = i; }
        void setTrackLength( const Msf& i ) { m_trackLength = i; }

        void setPreEmphasis( bool b ) { m_preEmphasis = b; }
        void setCopyPermitted( bool b ) { m_copyPermitted = b; }

        /**
         * Cdrecord seems to ignore this anyway and always expect big endian
         * data on stdin and wavs are little endian anyway.
         */
        void setBigEndian( bool b ) { m_bigEndian = b; }

        void setTrackCdText( const Device::TrackCdText& );
        void setTrackTitle( const QString& s ) { m_trackTitle = s; }
        void setTrackPerformer( const QString& s ) { m_trackPerformer = s; }
        void setTrackSongwriter( const QString& s ) { m_trackSongwriter = s; }
        void setTrackComposer( const QString& s ) { m_trackComposer = s; }
        void setTrackArranger( const QString& s ) { m_trackArranger = s; }
        void setTrackMessage( const QString& s ) { m_trackMessage = s; }

        void setCdText( const Device::CdText& );
        void setAlbumTitle( const QString& s ) { m_albumTitle = s; }
        void setAlbumPerformer( const QString& s ) { m_albumPerformer = s; }

        void setIsrc( const QByteArray& s ) { m_isrc = s; }
        void setMcn( const QByteArray& s ) { m_mcn = s; }

    private:
        long m_index0;

        QList<long> m_indices;

        int m_trackNumber;
        Msf m_trackStart;
        Msf m_trackLength;
        bool m_preEmphasis;
        bool m_copyPermitted;
        bool m_bigEndian;

        QString m_trackTitle;
        QString m_trackPerformer;
        QString m_trackSongwriter;
        QString m_trackComposer;
        QString m_trackArranger;
        QString m_trackMessage;

        QString m_albumTitle;
        QString m_albumPerformer;

        QByteArray m_isrc;
        QByteArray m_mcn;
    };
}

#endif
