/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bvideodvdtime.h"

K3bVideoDVD::Time::Time()
  : m_hour(0),
    m_minute(0),
    m_second(0),
    m_frame(0)
{
}


K3bVideoDVD::Time::Time( unsigned short hour,
			 unsigned short min,
			 unsigned short sec,
			 unsigned short frame )
  : m_hour(hour),
    m_minute(min),
    m_second(sec),
    m_frame(frame)
{
}


double K3bVideoDVD::Time::totalSeconds() const
{
  double s = (double)second();
  s += 60.0 * (double)minute();
  s += 3600.0 * (double)hour();

  return s + (double)( frame() / frameRate() );
}


unsigned int K3bVideoDVD::Time::totalFrames() const
{
  double f = (double)second();
  f += 60.0 * (double)minute();
  f += 3600.0 * (double)hour();

  return (int)( f * frameRate() ) + frame();
}


double K3bVideoDVD::Time::frameRate() const
{
  //
  // This is how it is done in libdvdread
  // I don't really understand it, though... :(
  //
  switch( (frame() & 0xc0) >> 6 ) {
  case 1:
    // PAL?
    return 25.0;
  case 3:
    // NTSC?
    return 29.97;
  default:
    // should only happen for time == 0?
    return 0.0;
  }
}


QString K3bVideoDVD::Time::toString( bool includeFrames ) const
{
  // FIXME: use a d-pointer
  const_cast<K3bVideoDVD::Time*>(this)->makeValid();

  if( includeFrames )
    return QString().sprintf( "%02d:%02d:%02d.%02d", 
			      m_hour,
			      m_minute,
			      m_second,
			      m_frame & 0x3f );
  else
    return QString().sprintf( "%02d:%02d:%02d", 
			      m_hour,
			      m_minute,
			      m_second + ( m_frame > 0 ? 1 : 0 ) );
}


void K3bVideoDVD::Time::makeValid()
{
  // FIXME: how to handle the frames?

  m_minute += m_second/60;
  m_second = m_second % 60;
  m_hour += m_minute/60;
  m_minute = m_minute % 60;
}
