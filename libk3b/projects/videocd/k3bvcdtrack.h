/*
 *
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3BVCDTRACK_H
#define K3BVCDTRACK_H

// Qt Includes
#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>

// Kde Includes
#include <kio/global.h>

// K3b Includes
#include "mpeginfo/k3bmpeginfo.h"
#include "k3b_export.h"

namespace K3b {
    class LIBK3B_EXPORT VcdTrack
    {
    public:
        VcdTrack( QList<VcdTrack*>* parent, const QString& filename );
        ~VcdTrack();

        QString fileName() const
        {
            return QFileInfo( m_file ).fileName();
        }
        QString absolutePath() const
        {
            return QFileInfo( m_file ).absoluteFilePath();
        }
        KIO::filesize_t size() const;
        int index() const;

        const QString& title() const
        {
            return m_title;
        }
        void setTitle( const QString& t )
        {
            m_title = t;
        }
        bool isSegment()
        {
            return mpegType() == 1;
        };

        // PBC
        enum PbcTracks { PREVIOUS, NEXT, RETURN, DEFAULT, AFTERTIMEOUT };
        enum PbcTypes { DISABLED, VIDEOEND };
        static QList<PbcTracks> trackPlaybackValues();

        void addToRevRefList( VcdTrack* revreftrack );
        void delFromRevRefList( VcdTrack* revreftrack );
        bool hasRevRef();
        void delRefToUs();
        void delRefFromUs();

        void setPbcTrack( PbcTracks which, VcdTrack* pbctrack = 0L );
        void setPbcNonTrack( PbcTracks which, PbcTypes type );
        void setUserDefined( PbcTracks, bool );
        void setPlayTime( int t )
        {
            m_pbcplaytime = t;
        }
        void setWaitTime( int t )
        {
            m_pbcwaittime = t;
        }
        void setReactivity( bool b )
        {
            m_reactivity = b;
        }
        void setPbcNumKeys( const bool& b )
        {
            m_pbcnumkeys = b;
        }
        bool PbcNumKeys() const
        {
            return m_pbcnumkeys;
        };
        void setPbcNumKeysUserdefined( const bool& b )
        {
            m_pbcnumkeysuserdefined = b;
        };
        bool PbcNumKeysUserdefined() const
        {
            return m_pbcnumkeysuserdefined;
        };

        VcdTrack* getPbcTrack( PbcTracks which );
        int getNonPbcTrack( PbcTracks which );
        bool isPbcUserDefined( PbcTracks which );
        int getPlayTime()
        {
            return m_pbcplaytime;
        }
        int getWaitTime()
        {
            return m_pbcwaittime;
        }
        bool Reactivity()
        {
            return m_reactivity;
        }

        // Numeric keys
        void setDefinedNumKey( int key, VcdTrack* track )
        {
            m_definedkeysmap.insert( key, track );
        }
        void delDefinedNumKey( int key )
        {
            m_definedkeysmap.remove( key );
        }
        void delDefinedNumKey()
        {
            m_definedkeysmap.clear();
        }
        QMap<int, VcdTrack*> DefinedNumKey()
        {
            return m_definedkeysmap;
        }

        // Mpeg Infos
        QString resolution();
        QString highresolution();
        QString video_frate();
        QString video_bitrate();
        QString audio_layer();
        QString audio_bitrate();
        QString audio_sampfreq();

        QString duration()
        {
            return SecsToHMS( mpeg_info->playing_time );
        };
        int version()
        {
            return mpeg_info->version;
        };
        unsigned long muxrate()
        {
            return mpeg_info->muxrate;
        };
        QString video_format( );
        QString video_chroma( );
        QString audio_mode( );
        QString audio_copyright( );
        QString mpegTypeS( bool audio = false );
        int mpegType();

        void PrintInfo();

        Mpeginfo* mpeg_info;

    protected:

        QString audio_type2str( unsigned int , unsigned int, unsigned int );
        QString SecsToHMS( double );

        QList<VcdTrack*>* m_parent;

        // PBC
        QList<VcdTrack*> m_revreflist;          // List of Tracks which points to us
        QMap<PbcTracks, VcdTrack*> m_pbctrackmap;        // Pbc Tracks (Previous, Next, ...)
        QMap<PbcTracks, PbcTypes> m_pbcnontrackmap;              // Pbc NON Track types (Previous, Next, ...)
        QMap<PbcTracks, bool> m_pbcusrdefmap;               // Pbc is userdefined or defaults (Previous, Next, ...)
        QMap<int, VcdTrack*> m_definedkeysmap;

        bool m_pbcnumkeys;
        bool m_pbcnumkeysuserdefined;

        int m_pbcplaytime;
        int m_pbcwaittime;
        /********************************************************************************/

        bool m_reactivity;
        int m_filetype;
        QFile m_file;
        QString m_title;
    };
}

#endif
