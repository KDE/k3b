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

#include "k3bmsf.h"

K3b::Msf::Msf()
  : m_minutes(0),
    m_seconds(0),
    m_frames(0)
{
}

K3b::Msf::Msf( const K3b::Msf& m )
  : m_minutes(m.minutes()),
    m_seconds(m.seconds()),
    m_frames(m.frames())
{
}

K3b::Msf::Msf( int m, int s, int f )
  : m_minutes(m),
    m_seconds(s),
    m_frames(f)
{
  makeValid();
}

K3b::Msf::Msf( int i )
  : m_minutes(0),
    m_seconds(0),
    m_frames(i)
{
  makeValid();
}

K3b::Msf& K3b::Msf::operator=( const K3b::Msf& m )
{
  m_frames = m.frames();
  m_seconds = m.seconds();
  m_minutes = m.minutes();
  return *this;
}

K3b::Msf& K3b::Msf::operator=( int i )
{
  m_frames = i;
  m_seconds = 0;
  m_minutes = 0;
  makeValid();
  return *this;
}

K3b::Msf& K3b::Msf::operator+=( const K3b::Msf& m )
{
  m_frames += m.frames();
  m_seconds += m.seconds();
  m_minutes += m.minutes();
  makeValid();
  return *this;
}

K3b::Msf& K3b::Msf::operator+=( int i )
{
  m_frames += i;
  makeValid();
  return *this;
}

K3b::Msf& K3b::Msf::operator-=( const K3b::Msf& m )
{
  m_frames -= m.frames();
  m_seconds -= m.seconds();
  m_minutes -= m.minutes();
  makeValid();
  return *this;
}

K3b::Msf& K3b::Msf::operator-=( int i )
{
  m_frames -= i;
  makeValid();
  return *this;
}


QString K3b::Msf::toString( bool showFrames ) const
{
  QString str;

  if( showFrames )
    str.sprintf( "%.2i:%.2i:%.2i", m_minutes, m_seconds, m_frames );
  else
    str.sprintf( "%.2i:%.2i", m_minutes, m_seconds );

  return str;
}


KIO::filesize_t K3b::Msf::mode1Form1Bytes() const
{
  return (KIO::filesize_t)2048 * ( (KIO::filesize_t)totalFrames() );
}

KIO::filesize_t K3b::Msf::mode1Form2Bytes() const
{
  return (KIO::filesize_t)2336 * ( (KIO::filesize_t)totalFrames() );
}

KIO::filesize_t K3b::Msf::mode2Form1Bytes() const
{
  return (KIO::filesize_t)2048 * ( (KIO::filesize_t)totalFrames() );
}

KIO::filesize_t K3b::Msf::mode2Form2Bytes() const
{
  return (KIO::filesize_t)2324 * ( (KIO::filesize_t)totalFrames() );
}

KIO::filesize_t K3b::Msf::audioBytes() const
{
  return (KIO::filesize_t)2352 * ( (KIO::filesize_t)totalFrames() );
}


void K3b::Msf::makeValid()
{
  if( m_frames < 0 ) {
    int newFrames = m_frames/-75 + 1;
    m_seconds -= newFrames;
    m_frames += 75*newFrames;
  }
  m_seconds += m_frames/75;
  m_frames = m_frames % 75;
  if( m_seconds < 0 ) {
    int newSecs = m_seconds/-60 + 1;
    m_minutes -= newSecs;
    m_seconds += 60*newSecs;
  }
  m_minutes += m_seconds/60;
  m_seconds = m_seconds % 60;
  if( m_minutes < 0 ) {
    m_minutes = 0;
    m_seconds = 0;
    m_frames = 0;
  }
}


K3b::Msf K3b::operator+( const K3b::Msf& m1, const K3b::Msf& m2 )
{
  K3b::Msf msf(m1);
  return msf += m2;
}

K3b::Msf K3b::operator+( const K3b::Msf& m, int i )
{
  K3b::Msf msf(m);
  return msf += i;
}

K3b::Msf K3b::operator-( const K3b::Msf& m1, const K3b::Msf& m2 )
{
  K3b::Msf msf(m1);
  return msf -= m2;
}

K3b::Msf K3b::operator-( const K3b::Msf& m, int i )
{
  K3b::Msf msf(m);
  return msf -= i;
}

bool K3b::operator==( const K3b::Msf& m1, const K3b::Msf& m2 )
{
  return ( m1.minutes() == m2.minutes() &&
	   m1.seconds() == m2.seconds() &&
	   m1.frames() == m2.frames() );
}

bool K3b::operator!=( const K3b::Msf& m1, const K3b::Msf& m2 )
{
	  return !operator==(m1,m2);
}

bool K3b::operator<( const K3b::Msf& m1, const K3b::Msf& m2 )
{
  return ( m1.totalFrames() < m2.totalFrames() );
}

bool K3b::operator>( const K3b::Msf& m1, const K3b::Msf& m2 )
{
  return ( m1.totalFrames() > m2.totalFrames() );
}

kdbgstream& K3b::operator<<( kdbgstream& s, const Msf& m )
{
  return s << m.toString();
}
