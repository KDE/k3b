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


#ifndef _K3B_MSF_H_
#define _K3B_MSF_H_

#include <qstring.h>
#include <kio/global.h>


namespace K3b
{
  /**
   * int values are always treated as frames
   * except in the set methods
   * A MSF is never < 0.
   */
  class Msf
    {
    public:
      Msf();
      Msf( const Msf& );
      Msf( int, int, int );
      Msf( int );

      Msf& operator=( const Msf& );
      Msf& operator=( int );
      Msf& operator+=( const Msf& );
      Msf& operator+=( int );
      Msf& operator-=( const Msf& );
      Msf& operator-=( int );

      int minutes() const { return m_minutes; }
      int seconds() const { return m_seconds; }
      int frames() const { return m_frames; }

      int totalFrames() const { return ( m_minutes*60 + m_seconds )*75 + m_frames; }

      void setMinutes( int m ) { m_minutes = m < 0 ? 0 : m; }
      void setSeconds( int s ) { m_seconds = s < 0 ? 0 : s; }
      void setFrames( int f ) { m_frames = f < 0 ? 0 : f; }

      QString toString( bool showFrames = true ) const;
      KIO::filesize_t mode1Form1Bytes() const;
      KIO::filesize_t mode1Form2Bytes() const;
      KIO::filesize_t mode2Form1Bytes() const;
      KIO::filesize_t mode2Form2Bytes() const;
      KIO::filesize_t audioBytes() const;

    private:
      void makeValid();
      int m_minutes;
      int m_seconds;
      int m_frames;
    };

  Msf operator+( const Msf&, const Msf& );
  Msf operator+( const Msf&, int );
  Msf operator-( const Msf&, const Msf& );
  Msf operator-( const Msf&, int );
  bool operator==( const Msf&, const Msf& );
  bool operator<( const Msf&, const Msf& );
  bool operator>( const Msf&, const Msf& );
};

typedef K3b::Msf K3bMsf;

#endif
