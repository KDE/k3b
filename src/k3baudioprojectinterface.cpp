/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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


K3b::AudioProjectInterface::AudioProjectInterface( K3b::AudioDoc* doc )
  : K3b::ProjectInterface( doc ),
    m_audioDoc(doc)
{
}


int K3b::AudioProjectInterface::trackCount() const
{
  return m_audioDoc->numOfTracks();
}


QString K3b::AudioProjectInterface::title() const
{
  return m_audioDoc->title();
}


QString K3b::AudioProjectInterface::artist() const
{
  return m_audioDoc->artist();
}


QString K3b::AudioProjectInterface::trackTitle( int trackNum ) const
{
  K3b::AudioTrack* track = m_audioDoc->getTrack( trackNum );
  if( track )
    return track->title();
  else
    return QString();
}


QString K3b::AudioProjectInterface::trackArtist( int trackNum ) const
{
  K3b::AudioTrack* track = m_audioDoc->getTrack( trackNum );
  if( track )
    return track->artist();
  else
    return QString();
}


void K3b::AudioProjectInterface::setTitle( const QString& title )
{
  m_audioDoc->setTitle( title );
}


void K3b::AudioProjectInterface::setArtist( const QString& artist )
{
  m_audioDoc->setArtist( artist );
}


void K3b::AudioProjectInterface::setTrackTitle( int trackNum, const QString& title )
{
  K3b::AudioTrack* track = m_audioDoc->getTrack( trackNum );
  if( track )
    track->setTitle( title );
}


void K3b::AudioProjectInterface::setTrackArtist( int trackNum, const QString& artist )
{
  K3b::AudioTrack* track = m_audioDoc->getTrack( trackNum );
  if( track )
    track->setArtist( artist );
}
