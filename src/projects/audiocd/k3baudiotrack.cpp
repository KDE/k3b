/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3baudiotrack.h"

#include <k3baudiodecoder.h>
#include <k3bcore.h>

#include <kconfig.h>

#include <qstring.h>
#include <qfileinfo.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <kdebug.h>



K3bAudioTrack::K3bAudioTrack( QPtrList<K3bAudioTrack>* parent, const QString& filename )
  : QObject(),
    m_filename(filename),
    m_status(0)
{
  m_parent = parent;
  m_copy = false;
  m_preEmp = false;

  k3bcore->config()->setGroup( "Audio project settings" );
  setPregap( k3bcore->config()->readNumEntry( "default pregap", 150 ) );

  m_module = 0;
}


K3bAudioTrack::~K3bAudioTrack()
{
  delete m_module;
}


K3b::Msf K3bAudioTrack::fileLength() const
{
  // make sure a track is always at least 4 seconds in length as defined in
  // the Red Book
  if( m_module && m_module->length() > 0 )
    return QMAX( m_module->length(), K3b::Msf( 0, 4, 0 ) );
  else
    return 0;
}


K3b::Msf K3bAudioTrack::length() const
{
  // not valid until the module determined the length
  if( fileLength() > 0 )
    return trackEnd() - trackStart();
  else
    return 0;
}


KIO::filesize_t K3bAudioTrack::size() const
{
  return length().audioBytes();
}


int K3bAudioTrack::index() const
{
  int i = m_parent->find( this );
  if( i < 0 )
    kdDebug() << "(K3bAudioTrack) I'm not part of my parent!" << endl;
  return i;
}


void K3bAudioTrack::setPregap( const K3b::Msf& p )
{
  m_pregap = p;
  emit changed();
}


void K3bAudioTrack::setModule( K3bAudioDecoder* module )
{
  m_module = module;
  m_module->setFilename( path() );
}


const K3b::Msf& K3bAudioTrack::trackStart() const
{
  return m_trackStartOffset;
}


K3b::Msf K3bAudioTrack::trackEnd() const
{
  return fileLength() - m_trackEndOffset;
}


void K3bAudioTrack::setTrackStart( const K3b::Msf& msf )
{
  // make sure a track is always at least 4 seconds in length as defined in
  // the Red Book
  if( msf > trackEnd() - K3b::Msf( 0, 4, 0 ) || msf > fileLength() )
    kdDebug() << "(K3bAudioTrack) invalid track start value: " << msf.toString() << endl;
  else {
    m_trackStartOffset = msf;
    emit changed();
  }
}


void K3bAudioTrack::setTrackEnd( const K3b::Msf& msf )
{
  // make sure a track is always at least 4 seconds in length as defined in
  // the Red Book
  if( msf < trackStart() + K3b::Msf( 0, 4, 0 ) )
    kdDebug() << "(K3bAudioTrack) invalid track end value: " << msf.toString() << endl;
  else {
    if( msf > fileLength() )
      m_trackEndOffset = 0;
    else
      m_trackEndOffset = fileLength() - msf;

    emit changed();
  }
}

#include "k3baudiotrack.moc"
