/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
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


class K3bVcdTrack
{
    public:
        K3bVcdTrack( QPtrList<K3bVcdTrack>* parent, const QString& filename );
        ~K3bVcdTrack();

        QString fileName() const { return QFileInfo(m_file).fileName(); }
        QString absPath() const { return QFileInfo(m_file).absFilePath(); }
        KIO::filesize_t size() const;
        int index() const;

        const QString& title() const { return m_title; }
        void setTitle( const QString& t ) { m_title = t; }
        void setSegment(bool segment) { m_segment = segment; }
        bool isSegment() { return m_segment; }

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
        void setPlayTime( int t ) { m_pbcplaytime = t; }
        void setWaitTime( int t ) { m_pbcwaittime = t; }
        void setReactivity( bool b ) { m_reactivity = b; }
        
        K3bVcdTrack* getPbcTrack( const int& );
        int getNonPbcTrack( const int& );
        bool isPbcUserDefined( int );
        int getPlayTime() { return m_pbcplaytime; }
        int getWaitTime() { return m_pbcwaittime; }
        bool Reactivity() { return m_reactivity; }
        
        // video
        int mpegType() const { return m_mpegtype; }
        int mpegVideoVersion() const { return m_mpegvideoversion; }
        int mpegMuxRate() const { return m_mpegmuxrate; }
        int MpegFormat() const { return m_mpegformat; }
        const QString& mpegVersion() const { return m_mpegversion; }
        const QString& mpegDuration() const { return m_mpegduration; }
        const QString& mpegSize() const { return m_mpegsize; }
        const QString& mpegDisplaySize() const { return m_mpegdisplaysize; }
        const QString& mpegFps() const { return m_mpegfps; }
        const QString& mpegMbps() const { return m_mpegmbps; }
        int MpegAspectRatio() const { return m_mpegaspect_ratio; }
        bool MpegSExt() const { return m_mpegprogressive; }
        bool MpegDExt() const { return m_mpegsext; }
        bool MpegProgressive() const { return m_mpegdext; }
        int MpegChromaFormat() const { return m_mpegchroma_format; }

        // audio
        bool hasAudio() const { return m_hasaudio; }
        bool MpegAudioCopyright() const { return m_mpegaudiocopyright; }
        bool MpegAudioOriginal() const { return m_mpegaudiooriginal; }
        int MpegAudioType() const { return m_mpegaudiotype; }
        int MpegAudioLayer() const { return m_mpegaudiolayer; }
        const QString& MpegAudioDuration() const { return m_mpegaudioduration; }
        const QString& MpegAudioKbps() const { return m_mpegaudiokbps; }
        const QString& MpegAudioHz() const { return m_mpegaudiohz; }
        const QString& MpegAudioFrame() const { return m_mpegaudioframe; }
        int MpegAudioMode() const { return m_mpegaudiomode; }
        int MpegAudioModeExt() const { return m_mpegaudiomodeext; }
        int MpegAudioEmphasis() const { return m_mpegaudioemphasis; }

        // video
        void setMpegType(const int&);
        void setMpegVideoVersion(const int&);
        void setMpegMuxRate(const int&);
        void setMpegFormat(const int&);
        void setMpegVersion(const QString&);
        void setMpegDuration(const QString&);
        void setMpegSize(const QString&);
        void setMpegDisplaySize(const QString&);
        void setMpegFps(const QString&);
        void setMpegMbps(const QString&);
        void setMpegAspectRatio(const int&);
        void setMpegProgressive(const bool&);
        void setMpegSExt(const bool&);
        void setMpegDExt(const bool&);
        void setMpegChromaFormat(const int&);

        // audio
        void setHasAudio(const bool&);
        void setMpegAudioType(const int&);
        void setMpegAudioLayer(const int&);
        void setMpegAudioDuration(const QString&);
        void setMpegAudioKbps(const QString&);
        void setMpegAudioHz(const QString&);
        void setMpegAudioFrame(const QString&);
        void setMpegAudioMode(const int&);
        void setMpegAudioModeExt(const int&);
        void setMpegAudioEmphasis(const int&);
        void setMpegAudioCopyright(const bool&);
        void setMpegAudioOriginal(const bool&);

    protected:
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
        int m_filetype;
        QFile m_file;
        QString m_title;
        // video
        int m_mpegtype;
        int m_mpegvideoversion;
        int m_mpegmuxrate;
        int m_mpegformat;
        QString m_mpegversion;
        QString m_mpegduration;
        QString m_mpegsize;
        QString m_mpegdisplaysize;
        QString m_mpegfps;
        QString m_mpegmbps;
        int m_mpegaspect_ratio;
        bool m_mpegprogressive;
        bool m_mpegsext;
        bool m_mpegdext;
        int m_mpegchroma_format;
        // audio
        bool m_hasaudio;
        bool m_mpegaudiocopyright;
        bool m_mpegaudiooriginal;
        bool m_reactivity;
        int m_mpegaudiotype;
        int m_mpegaudiolayer;
        QString m_mpegaudioduration;
        QString m_mpegaudiokbps;
        QString m_mpegaudiohz;
        QString m_mpegaudioframe;
        int m_mpegaudiomode;
        int m_mpegaudiomodeext;
        int m_mpegaudioemphasis;
};

#endif
