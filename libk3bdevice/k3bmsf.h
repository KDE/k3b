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


#ifndef _K3B_MSF_H_
#define _K3B_MSF_H_

#include <qstring.h>
#include <qregexp.h>

#include <kdebug.h>
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
      const Msf operator++( int );
      Msf& operator++();
      const Msf operator--( int );
      Msf& operator--();

      int minutes() const { return m_minutes; }
      int seconds() const { return m_seconds; }
      int frames() const { return m_frames; }

      int totalFrames() const { return ( m_minutes*60 + m_seconds )*75 + m_frames; }
      int lba() const { return totalFrames(); }

      //      operator int () const { return lba(); }

      void setValue( int m, int s, int f );

      void addMinutes( int m );
      void addSeconds( int s );
      void addFrames( int f );

      QString toString( bool showFrames = true ) const;

      KIO::filesize_t mode1Bytes() const;
      KIO::filesize_t mode2Form1Bytes() const;
      KIO::filesize_t mode2Form2Bytes() const;
      KIO::filesize_t audioBytes() const;
      unsigned long long pcmSamples() const { return lba()*588; }

      /**
       * Convert a string representation into an Msf object.
       *
       * Valid strings include:
       * \li 100       - treated as 100 frames
       * \li 100:23    - treated as 100 minutes and 23 seconds
       * \li 100:23:57 - treated as 100 minutes, 23 seconds, and 57 frames
       * \li 100:23.57 - treated as 100 minutes, 23 seconds, and 57 frames
       */
      static Msf fromString( const QString&, bool* ok = 0 );

      /**
       * @param ms seconds
       * frames will be rounded up
       */
      static Msf fromSeconds( double ms );

      static QRegExp regExp();

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
  bool operator!=( const Msf&, const Msf& );
  bool operator<( const Msf&, const Msf& );
  bool operator>( const Msf&, const Msf& );
  bool operator<=( const Msf&, const Msf& );
  bool operator>=( const Msf&, const Msf& );

  kdbgstream& operator<<( kdbgstream&, const Msf& );
  inline kndbgstream& operator<<( kndbgstream &stream, const Msf& ) { return stream; }
}

typedef K3b::Msf K3bMsf;

#endif
