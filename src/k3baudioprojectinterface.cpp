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

#include "k3baudioprojectinterface.h"

#include <k3baudiodoc.h>
#include <k3baudiotrack.h>


K3bAudioProjectInterface::K3bAudioProjectInterface( K3bAudioDoc* doc, const char* name )
  : K3bProjectInterface( doc, name ),
    m_audioDoc(doc)
{
}


int K3bAudioProjectInterface::trackCount() const
{
  return m_audioDoc->numOfTracks();
}


QString K3bAudioProjectInterface::title() const
{
  return m_audioDoc->title();
}


QString K3bAudioProjectInterface::artist() const
{
  return m_audioDoc->artist();
}


QString K3bAudioProjectInterface::trackTitle( int trackNum ) const
{
  K3bAudioTrack* track = m_audioDoc->getTrack( trackNum-1 );
  if( track )
    return track->title();
  else
    return QString::null;
}


QString K3bAudioProjectInterface::trackArtist( int trackNum ) const
{
  K3bAudioTrack* track = m_audioDoc->getTrack( trackNum-1 );
  if( track )
    return track->artist();
  else
    return QString::null;
}


void K3bAudioProjectInterface::setTitle( const QString& title )
{
  m_audioDoc->setTitle( title );
}


void K3bAudioProjectInterface::setArtist( const QString& artist )
{
  m_audioDoc->setArtist( artist );
}


void K3bAudioProjectInterface::setTrackTitle( int trackNum, const QString& title )
{
  K3bAudioTrack* track = m_audioDoc->getTrack( trackNum-1 );
  if( track )
    track->setTitle( title );
}


void K3bAudioProjectInterface::setTrackArtist( int trackNum, const QString& artist )
{
  K3bAudioTrack* track = m_audioDoc->getTrack( trackNum-1 );
  if( track )
    track->setArtist( artist );
}
