/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiozerodata.h"
#include "k3baudiotrack.h"

#include <klocale.h>

#include <string.h>


K3bAudioZeroData::K3bAudioZeroData( K3bAudioDoc* doc, const K3b::Msf& len )
  : K3bAudioDataSource(doc),
    m_length(len),
    m_writtenData(0)
{
}


K3bAudioZeroData::~K3bAudioZeroData()
{
}


void K3bAudioZeroData::setLength( const K3b::Msf& msf )
{
  if( msf > 0 )
    m_length = msf;
  else
    m_length = 1; // 1 frame

  m_writtenData = 0;

  if( track() )
    track()->sourceChanged( this );
}


QString K3bAudioZeroData::type() const
{
  return i18n("None");
}


QString K3bAudioZeroData::sourceComment() const
{
  return i18n("Silence");
}


bool K3bAudioZeroData::seek( const K3b::Msf& msf )
{
  if( msf < length() ) {
    m_writtenData = msf.audioBytes();
    return true;
  }
  else
    return false;
}


int K3bAudioZeroData::read( char* data, unsigned int max )
{
  if( m_writtenData + max > length().audioBytes() )
    max = length().audioBytes() - m_writtenData;

  m_writtenData += max;

  ::memset( data, 0, max );

  return max;
}


K3bAudioDataSource* K3bAudioZeroData::copy() const
{
  K3bAudioZeroData* zero = new K3bAudioZeroData( doc(), length() );
  return zero;
}


K3bAudioDataSource* K3bAudioZeroData::split( const K3b::Msf& pos )
{
  if( pos < length() ) {
    K3bAudioZeroData* zero = new K3bAudioZeroData( doc(), length()-pos );
    setLength( pos );
    zero->moveAfter( this );
    emitChange();
    return zero;
  }
  else
    return 0;
}
