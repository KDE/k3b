/***************************************************************************
                          k3bvcdtrack.cpp  -  description
                             -------------------
    begin                : Mon Nov 4 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bvcdtrack.h"
#include "../tools/k3bglobals.h"

#include <kapplication.h>
#include <kconfig.h>

#include <qstring.h>
#include <qfileinfo.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <kdebug.h>


K3bVcdTrack::K3bVcdTrack( QPtrList<K3bVcdTrack>* parent, const QString& filename )
: m_file(filename)
{
  m_parent = parent;
}


K3bVcdTrack::~K3bVcdTrack()
{
}


unsigned long K3bVcdTrack::size() const
{
  return m_file.size();
}

void K3bVcdTrack::setMpegType(const int& mt)
{
  m_mpegtype = mt;
}

void K3bVcdTrack::setMpegVideoVersion(const int& version)
{
m_mpegvideoversion = version;
}

void K3bVcdTrack::setMpegVersion(const QString& version)
{
  m_mpegversion = version;
}

void K3bVcdTrack::setMpegMuxRate(const int& mux)
{
  m_mpegmuxrate = mux;
}

void K3bVcdTrack::setMpegFormat(const int& format)
{
  m_mpegformat = format;  
}

void K3bVcdTrack::setMpegDuration(const QString& time)
{
  m_mpegduration = time;
}

void K3bVcdTrack::setMpegSize(const QString& size)
{
  m_mpegsize = size;
}

void K3bVcdTrack::setMpegDisplaySize(const QString& size)
{
  m_mpegdisplaysize = size;
}

void K3bVcdTrack::setMpegFps(const QString& fps)
{
  m_mpegfps = fps;
}

void K3bVcdTrack::setMpegMbps(const QString& mbps)
{
  m_mpegmbps = mbps;
}

void K3bVcdTrack::setMpegAspectRatio(const int& ratio)
{
  m_mpegaspect_ratio = ratio;
}

void K3bVcdTrack::setMpegSExt(const bool& sext)
{
  m_mpegsext = sext;  
}

void K3bVcdTrack::setMpegDExt(const bool& dext)
{
  m_mpegdext = dext;    
}

void K3bVcdTrack::setMpegProgressive(const bool& progressive)
{
  m_mpegprogressive = progressive;
}

void K3bVcdTrack::setMpegChromaFormat(const int& chromaformat)
{
  m_mpegchroma_format = chromaformat;
}

// audio
void K3bVcdTrack::setHasAudio(const bool& audio)
{
  m_hasaudio = audio;
}

void K3bVcdTrack::setMpegAudioType(const int& type)
{
  m_mpegaudiotype = type;
}

void K3bVcdTrack::setMpegAudioCopyright(const bool& copyright)
{
  m_mpegaudiocopyright = copyright;  
}

void K3bVcdTrack::setMpegAudioOriginal(const bool& original)
{
  m_mpegaudiooriginal = original;  
}

void K3bVcdTrack::setMpegAudioLayer(const int& layer)
{
  m_mpegaudiolayer = layer;
}

void K3bVcdTrack::setMpegAudioDuration(const QString& duration)
{
  m_mpegaudioduration = duration;
}

void K3bVcdTrack::setMpegAudioKbps(const QString& kbps)
{
  m_mpegaudiokbps = kbps;
}

void K3bVcdTrack::setMpegAudioHz(const QString& hz)
{
  m_mpegaudiohz = hz;
}

void K3bVcdTrack::setMpegAudioFrame(const QString& frame)
{
  m_mpegaudioframe = frame;
}

void K3bVcdTrack::setMpegAudioMode(const int& mode)
{
  m_mpegaudiomode = mode;
}

void K3bVcdTrack::setMpegAudioModeExt(const int& modeext)
{
  m_mpegaudiomodeext = modeext;
}

void K3bVcdTrack::setMpegAudioEmphasis(const int& e)
{
  m_mpegaudioemphasis = e;
}

int K3bVcdTrack::index() const
{
  int i = m_parent->find( this );
  if( i < 0 )
    kdDebug() << "(K3bVcdTrack) I'm not part of my parent!" << endl;
  return i;
}

