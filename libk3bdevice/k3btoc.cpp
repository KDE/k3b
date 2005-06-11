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


#include "k3btoc.h"

#include <qstring.h>


K3bDevice::Toc::Toc()
  : QValueList<K3bDevice::Track>(),
    m_discId(0)
{
}


K3bDevice::Toc::Toc( const Toc& toc )
  : QValueList<K3bDevice::Track>( toc )
{
  m_firstSector = toc.firstSector();
  m_discId = toc.discId();
}


K3bDevice::Toc::~Toc()
{
}


K3bDevice::Toc& K3bDevice::Toc::operator=( const Toc& toc )
{
  if( &toc == this ) return *this;

  m_firstSector = toc.firstSector();
  m_discId = toc.discId();

  QValueList<K3bDevice::Track>::operator=( toc );

  return *this;
}


unsigned int K3bDevice::Toc::discId() const
{
  return m_discId;
}


const K3b::Msf& K3bDevice::Toc::firstSector() const
{
  return m_firstSector;
}


K3b::Msf K3bDevice::Toc::lastSector() const
{
  if( isEmpty() )
    return 0;
  // the last track's last sector should be the last sector of the entire cd
  return last().lastSector();
}


K3b::Msf K3bDevice::Toc::length() const
{
  // +1 since the last sector is included
  return lastSector() - firstSector() + 1;
}


unsigned int K3bDevice::Toc::calculateDiscId()
{
  // calculate cddb-id
  unsigned int id = 0;
  for( K3bToc::const_iterator it = constBegin(); it != constEnd(); ++it ) {
    unsigned int n = (*it).firstSector().lba() + 150;
    n /= 75;
    while( n > 0 ) {
      id += n % 10;
      n /= 10;
    }
  }
  unsigned int l = length().lba();
  l /= 75;
  id = ( ( id % 0xff ) << 24 ) | ( l << 8 ) | count();

  setDiscId( id );

  return discId();
}


int K3bDevice::Toc::contentType() const
{
  int audioCnt = 0, dataCnt = 0;
  for( K3bToc::const_iterator it = constBegin(); it != constEnd(); ++it ) {
    if( (*it).type() == K3bDevice::Track::AUDIO )
      audioCnt++;
    else
      dataCnt++;
  }

  if( audioCnt + dataCnt == 0 )
    return K3bDevice::NONE;
  if( audioCnt == 0 )
    return K3bDevice::DATA;
  if( dataCnt == 0 )
    return K3bDevice::AUDIO;
  return K3bDevice::MIXED;
}
