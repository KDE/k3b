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

#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qptrlist.h>

#include <kio/global.h>


class K3bVcdTrack
{
    public:
        K3bVcdTrack( QList<K3bVcdTrack>* parent, const QString& filename );
        ~K3bVcdTrack();

        QString fileName() const { return QFileInfo(m_file).fileName(); }
        QString absPath() const { return QFileInfo(m_file).absFilePath(); }
        KIO::filesize_t size() const;
        int index() const;

        const QString& title() const { return m_title; }
        void setTitle( const QString& t ) { m_title = t; }

        // PBC
        void setPrevious(K3bVcdTrack* pbc_previous = 0L) { m_pbcprevious = pbc_previous; }
        void setNext(K3bVcdTrack* pbc_next = 0L) { m_pbcnext = pbc_next; }
        void setReturn(K3bVcdTrack* pbc_return = 0L) { m_pbcreturn = pbc_return; }
        void setDefault(K3bVcdTrack* pbc_default = 0L) { m_pbcdefault = pbc_default; }

        void setPreviousEnabled(bool pbc_previous_enabled) { m_pbcprevious_enabled = pbc_previous_enabled; }
        void setNextEnabled(bool pbc_next_enabled) { m_pbcnext_enabled = pbc_next_enabled; }
        void setReturnEnabled(bool pbc_return_enabled) { m_pbcreturn_enabled = pbc_return_enabled; }
        void setDefaultEnabled(bool pbc_default_enabled) { m_pbcdefault_enabled = pbc_default_enabled; }

        void setUserDefined(bool pbc_userdefined) { m_pbcuserdefined = pbc_userdefined; }
        void setSegment(bool segment) { m_segment = segment; }

        K3bVcdTrack* Previous() { return m_pbcprevious; }
        K3bVcdTrack* Next() { return m_pbcnext; }
        K3bVcdTrack* Return() { return m_pbcreturn; }
        K3bVcdTrack* Default() { return m_pbcdefault; }

        bool isPreviousEnabled() { return m_pbcprevious_enabled; }
        bool isNextEnabled() { return m_pbcnext_enabled; }
        bool isReturnEnabled() { return m_pbcreturn_enabled; }
        bool isDefaultEnabled() { return m_pbcdefault_enabled; }

        bool isPbcUserDefined() { return m_pbcuserdefined; }
        bool isSegment() { return m_segment; }

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
        QList<K3bVcdTrack>* m_parent;

        // PBC
        K3bVcdTrack* m_pbcprevious;
        K3bVcdTrack* m_pbcnext;
        K3bVcdTrack* m_pbcreturn;
        K3bVcdTrack* m_pbcdefault;

        bool m_pbcprevious_enabled;
        bool m_pbcnext_enabled;
        bool m_pbcreturn_enabled;
        bool m_pbcdefault_enabled;

        bool m_pbcuserdefined;
        bool m_segment;
        /***************************/

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
