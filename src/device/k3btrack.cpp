/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3btrack.h"


K3bCdDevice::Track::Track()
  : m_type(-1),
    m_mode(-1),
    m_copyPermitted(true),
    m_preEmphasis(false),
    m_session(0)
{
}


K3bCdDevice::Track::Track( const Track& track )
  : m_firstSector( track.firstSector() ),
    m_lastSector( track.lastSector() ),
    m_type( track.type() ),
    m_mode( track.mode() ),
    m_copyPermitted( track.copyPermitted() ),
    m_preEmphasis( track.preEmphasis() ),
    m_session( track.session() ),
    m_indices( track.indices() )
{
}


K3bCdDevice::Track::Track( const K3b::Msf& firstSector, 
			   const K3b::Msf& lastSector, 
			   int type, 
			   int mode )
  : m_firstSector( firstSector ), 
    m_lastSector( lastSector ), 
    m_type( type ), 
    m_mode( mode ), 
    m_copyPermitted(true),
    m_preEmphasis(false),
    m_session(0)
{
}


K3bCdDevice::Track& K3bCdDevice::Track::operator=( const K3bTrack& track )
{
  if( this != &track ) {
    m_firstSector = track.firstSector();
    m_lastSector = track.lastSector();
    m_type = track.type();
    m_mode = track.mode();
    m_indices = track.indices();
  }

  return *this;
}


K3b::Msf K3bCdDevice::Track::length() const
{
  // +1 since the last sector is included
  return m_lastSector - m_firstSector + 1;
}


K3b::Msf K3bCdDevice::Track::realAudioLength() const
{
  if( type() == DATA || index0() < firstSector().lba() )
    return length();
  else
    return length() - ( lastSector() - index0() + 1 );
}


long K3bCdDevice::Track::index( int i, bool absolute ) const
{
  if( (int)m_indices.count() > i && m_indices[i] >= 0 ) {
    if( absolute )
      return m_indices[i];
    else
      return m_indices[i] - firstSector().lba();
  }
  else
    return -1;
}


long K3bCdDevice::Track::index0() const
{
  return index( 0, true );
}


int K3bCdDevice::Track::indexCount() const
{
  return m_indices.count()-1;
}
