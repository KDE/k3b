/***************************************************************************
                          k3baudiotrack.cpp  -  description
                             -------------------
    begin                : Wed Mar 28 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#include "k3baudiotrack.h"
#include "../k3bglobals.h"

// ID3lib-includes
#include <id3/tag.h>
#include <id3/misc_support.h>


#include <kapp.h>
#include <kconfig.h>

#include <qstring.h>
#include <qfileinfo.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stream.h>


K3bAudioTrack::K3bAudioTrack( QList<K3bAudioTrack>* parent, const QString& filename )
: m_file(filename)
{
  m_parent = parent;
  m_copy = false;
  m_preEmp = false;
  m_length = 0;
  
  kapp->config()->setGroup( "Audio Defaults" );
  m_pregap = kapp->config()->readNumEntry( "Pregap", 150 );
  
  if( QFileInfo(m_file).extension(false).contains("mp3", false) )
    m_filetype = K3b::MP3;
  else
    m_filetype = K3b::WAV;
}


K3bAudioTrack::~K3bAudioTrack()
{
}


uint K3bAudioTrack::size() const
{
  // what is important? size of the file or size of the track?
  return m_file.size();
}


int K3bAudioTrack::index() const
{
  int i = m_parent->find( this );
  if( i < 0 )
    qDebug( "(K3bAudioTrack) I'm not part of my parent!");
  return i;
}

void K3bAudioTrack::setBufferFile( const QString& file )
{
  qDebug( "(K3bAudioTrack) cannot set bufferFile since I'm abstract!" );
}
