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
    m_title( track.title() )
{
}


K3bCdDevice::Track::Track( const K3b::Msf& firstSector, 
			   const K3b::Msf& lastSector, 
			   int type, 
			   int mode, 
			   const QString& title )
  : m_firstSector( firstSector ), 
    m_lastSector( lastSector ), 
    m_type( type ), 
    m_mode( mode ), 
    m_copyPermitted(true),
    m_preEmphasis(false),
    m_session(0),
    m_title( title )
{
}


K3bCdDevice::Track& K3bCdDevice::Track::operator=( const K3bTrack& track )
{
  m_firstSector = track.firstSector();
  m_lastSector = track.lastSector();
  m_type = track.type();
  m_mode = track.mode();
  m_title = track.title();

  return *this;
}


K3b::Msf K3bCdDevice::Track::length() const
{
  // +1 since the last sector is included
  return m_lastSector - m_firstSector + 1;
}


void K3bCdDevice::Track::setTitle( const QString& title )
{
  m_title = title;
}

