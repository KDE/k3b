/*
*
* $Id$
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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
#include <qptrlist.h>

// Kde Includes
#include <kio/global.h>

// K3b Includes
#include "mpeginfo/k3bmpeginfo.h"

class K3bVcdTrack
{
    public:
        K3bVcdTrack( QPtrList<K3bVcdTrack>* parent, const QString& filename );
        ~K3bVcdTrack();

        QString fileName() const
        {
            return QFileInfo( m_file ).fileName();
        }
        QString absPath() const
        {
            return QFileInfo( m_file ).absFilePath();
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
        void setSegment( bool segment )
        {
            m_segment = segment;
        }
        bool isSegment()
        {
            return m_segment;
        }

        // PBC
        enum PbcTracks { PREVIOUS, NEXT, RETURN, DEFAULT, AFTERTIMEOUT, _maxPbcTracks };
        enum PbcTypes { DISABLED, VIDEOEND };

        void addToRevRefList( K3bVcdTrack* revreftrack );
        void delFromRevRefList( K3bVcdTrack* revreftrack );
        bool hasRevRef();
        void delRefToUs();
        void delRefFromUs();

        void setPbcTrack( int, K3bVcdTrack* pbctrack = 0L );
        void setPbcNonTrack( int, int );
        void setUserDefined( int, bool );
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

        K3bVcdTrack* getPbcTrack( const int& );
        int getNonPbcTrack( const int& );
        bool isPbcUserDefined( int );
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

        const QString resolution();
        const QString highresolution();
        const QString video_frate();
        const QString video_bitrate();
        const QString audio_layer();
        const QString audio_bitrate();
        const QString audio_sampfreq();

        const QString duration()
        {
            return SecsToHMS( mpeg_info->playing_time );
        };
        const int version()
        {
            return mpeg_info->version;
        };
        const unsigned long muxrate()
        {
            return mpeg_info->muxrate;
        };
        const QString video_format( );
        const QString video_chroma( );
        const QString audio_mode( );
        const QString mpegTypeS( bool audio = false );
        const int mpegType();

        void PrintInfo();

        Mpeginfo* mpeg_info;

    protected:

        const QString audio_type2str( unsigned int , unsigned int, unsigned int );
        QString SecsToHMS( double );

        enum mpeg_version { MPEG_VERS_INVALID = 0, MPEG_VERS_MPEG1 = 1, MPEG_VERS_MPEG2 = 2 };
        enum mode { MPEG_STEREO = 1, MPEG_JOINT_STEREO, MPEG_DUAL_CHANNEL, MPEG_SINGLE_CHANNEL };

        QPtrList<K3bVcdTrack>* m_parent;

        // PBC
        QPtrList<K3bVcdTrack>* m_revreflist;          // List of Tracks which points to us
        QMap<int, K3bVcdTrack*> m_pbctrackmap;  // Pbc Tracks (Previous, Next, ...)
        QMap<int, int> m_pbcnontrackmap;             // Pbc NON Track types (Previous, Next, ...)
        QMap<int, bool> m_pbcusrdefmap;              // Pbc is userdefined or defaults (Previous, Next, ...)

        int m_pbcplaytime;
        int m_pbcwaittime;
        /*********************************************************************************************/

        bool m_segment;
        bool m_reactivity;
        int m_filetype;
        QFile m_file;
        QString m_title;
};

#endif
