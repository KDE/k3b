/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

#include <kapplication.h>
#include <kconfig.h>

#include <qstring.h>
#include <qfileinfo.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <kdebug.h>

// K3b Includes
#include "k3bvcdtrack.h"
#include <k3bglobals.h>

K3bVcdTrack::K3bVcdTrack( QPtrList<K3bVcdTrack>* parent, const QString& filename )
        : m_file( filename )
{
    m_parent = parent;
    m_title = QFileInfo( m_file ).baseName( true );

    m_revreflist = new QPtrList<K3bVcdTrack>;

    for ( int i = 0; i < K3bVcdTrack::_maxPbcTracks; i++ ) {
        m_pbctrackmap.insert( i, 0L );
        m_pbcnontrackmap.insert( i, K3bVcdTrack::DISABLED );
        m_pbcusrdefmap.insert( i, false );
    }

    m_segment = false;
		m_reactivity = false;
}


K3bVcdTrack::~K3bVcdTrack()
{}


KIO::filesize_t K3bVcdTrack::size() const
{
    return m_file.size();
}

void K3bVcdTrack::setMpegType( const int& mt )
{
    m_mpegtype = mt;
}

void K3bVcdTrack::setMpegVideoVersion( const int& version )
{
    m_mpegvideoversion = version;
}

void K3bVcdTrack::setMpegVersion( const QString& version )
{
    m_mpegversion = version;
}

void K3bVcdTrack::setMpegMuxRate( const int& mux )
{
    m_mpegmuxrate = mux;
}

void K3bVcdTrack::setMpegFormat( const int& format )
{
    m_mpegformat = format;
}

void K3bVcdTrack::setMpegDuration( const QString& time )
{
    m_mpegduration = time;
}

void K3bVcdTrack::setMpegSize( const QString& size )
{
    m_mpegsize = size;
}

void K3bVcdTrack::setMpegDisplaySize( const QString& size )
{
    m_mpegdisplaysize = size;
}

void K3bVcdTrack::setMpegFps( const QString& fps )
{
    m_mpegfps = fps;
}

void K3bVcdTrack::setMpegMbps( const QString& mbps )
{
    m_mpegmbps = mbps;
}

void K3bVcdTrack::setMpegAspectRatio( const int& ratio )
{
    m_mpegaspect_ratio = ratio;
}

void K3bVcdTrack::setMpegSExt( const bool& sext )
{
    m_mpegsext = sext;
}

void K3bVcdTrack::setMpegDExt( const bool& dext )
{
    m_mpegdext = dext;
}

void K3bVcdTrack::setMpegProgressive( const bool& progressive )
{
    m_mpegprogressive = progressive;
}

void K3bVcdTrack::setMpegChromaFormat( const int& chromaformat )
{
    m_mpegchroma_format = chromaformat;
}

// audio
void K3bVcdTrack::setHasAudio( const bool& audio )
{
    m_hasaudio = audio;
}

void K3bVcdTrack::setMpegAudioType( const int& type )
{
    m_mpegaudiotype = type;
}

void K3bVcdTrack::setMpegAudioCopyright( const bool& copyright )
{
    m_mpegaudiocopyright = copyright;
}

void K3bVcdTrack::setMpegAudioOriginal( const bool& original )
{
    m_mpegaudiooriginal = original;
}

void K3bVcdTrack::setMpegAudioLayer( const int& layer )
{
    m_mpegaudiolayer = layer;
}

void K3bVcdTrack::setMpegAudioDuration( const QString& duration )
{
    m_mpegaudioduration = duration;
}

void K3bVcdTrack::setMpegAudioKbps( const QString& kbps )
{
    m_mpegaudiokbps = kbps;
}

void K3bVcdTrack::setMpegAudioHz( const QString& hz )
{
    m_mpegaudiohz = hz;
}

void K3bVcdTrack::setMpegAudioFrame( const QString& frame )
{
    m_mpegaudioframe = frame;
}

void K3bVcdTrack::setMpegAudioMode( const int& mode )
{
    m_mpegaudiomode = mode;
}

void K3bVcdTrack::setMpegAudioModeExt( const int& modeext )
{
    m_mpegaudiomodeext = modeext;
}

void K3bVcdTrack::setMpegAudioEmphasis( const int& e )
{
    m_mpegaudioemphasis = e;
}

int K3bVcdTrack::index() const
{
    int i = m_parent->find( this );
    if ( i < 0 )
        kdDebug() << "(K3bVcdTrack) I'm not part of my parent!" << endl;
    return i;
}

void K3bVcdTrack::addToRevRefList( K3bVcdTrack* revreftrack )
{
    kdDebug() << "K3bVcdTrack::addToRevRefList: track = " << revreftrack << endl;

    m_revreflist->append( revreftrack );

    kdDebug() << "K3bVcdTrack::hasRevRef count = " << m_revreflist->count() << " empty = " << m_revreflist->isEmpty() << endl;
}

void K3bVcdTrack::delFromRevRefList( K3bVcdTrack* revreftrack )
{
    if ( !m_revreflist->isEmpty() ) {
        m_revreflist->remove
        ( revreftrack );
    }
}

bool K3bVcdTrack::hasRevRef()
{
    return !m_revreflist->isEmpty() ;
}

void K3bVcdTrack::delRefToUs()
{
    for ( K3bVcdTrack* track = m_revreflist->first(); track; track = m_revreflist->next() ) {
        for ( int i = 0; i < K3bVcdTrack::_maxPbcTracks; i++ ) {
            kdDebug() << "K3bVcdTrack::delRefToUs count = " << m_revreflist->count() << " empty = " << m_revreflist->isEmpty() << " track = " << track << " this = " << this << endl;
            if ( this == track->getPbcTrack( i ) ) {
                track->setPbcTrack( i );
                track->setUserDefined( i, false );
                track->delFromRevRefList( this );
            }
        }
    }
}

void K3bVcdTrack::delRefFromUs()
{
    for ( int i = 0; i < K3bVcdTrack::_maxPbcTracks; i++ ) {
        if ( this->getPbcTrack( i ) ) {
            this->getPbcTrack( i ) ->delFromRevRefList( this );
        }
    }
}

void K3bVcdTrack::setPbcTrack( int which, K3bVcdTrack* pbctrack )
{
    kdDebug() << "K3bVcdTrack::setPbcTrack " << which << ", " << pbctrack << endl;
    m_pbctrackmap.replace( which, pbctrack );
}

void K3bVcdTrack::setPbcNonTrack( int which, int type )
{
    kdDebug() << "K3bVcdTrack::setNonPbcTrack " << which << ", " << type << endl;
    m_pbcnontrackmap.replace( which, type );
}

void K3bVcdTrack::setUserDefined( int which, bool ud )
{
    m_pbcusrdefmap.replace( which, ud );
}

K3bVcdTrack* K3bVcdTrack::getPbcTrack( const int& which )
{
    if ( m_pbctrackmap.find( which ) == m_pbctrackmap.end() )
        return 0;
    else
        return m_pbctrackmap[ which ];
}

int K3bVcdTrack::getNonPbcTrack( const int& which )
{
    if ( m_pbcnontrackmap.find( which ) == m_pbcnontrackmap.end() )
        return 0;
    else
        return m_pbcnontrackmap[ which ];
}

bool K3bVcdTrack::isPbcUserDefined( int which )
{
    return m_pbcusrdefmap[ which ];
}
