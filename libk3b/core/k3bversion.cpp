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

#include "k3bversion.h"

#include <qregexp.h>
#include <kdebug.h>


K3bVersion::K3bVersion()
  : m_majorVersion( -1 ),
    m_minorVersion( -1 ),
    m_patchLevel( -1 )
{
}

K3bVersion::K3bVersion( const K3bVersion& v )
  : m_versionString( v.versionString() ),
    m_majorVersion( v.majorVersion() ),
    m_minorVersion( v.minorVersion() ),
    m_patchLevel( v.patchLevel() ),
    m_suffix( v.suffix() )
{
}

K3bVersion::K3bVersion( const QString& version )
{
  setVersion( version );
}

K3bVersion::K3bVersion( int majorVersion, 
			int minorVersion, 
			int patchlevel, 
			const QString& suffix )
{
  setVersion( majorVersion, minorVersion, patchlevel, suffix );
}

void K3bVersion::setVersion( const QString& v )
{
  QString suffix;
  splitVersionString( v.stripWhiteSpace(), m_majorVersion, suffix );
  if( m_majorVersion >= 0 ) {
    if( suffix.startsWith(".") ) {
      suffix = suffix.mid( 1 );
      splitVersionString( suffix, m_minorVersion, suffix );
      if( m_minorVersion < 0 ) {
	kdDebug() << "(K3bVersion) suffix must not start with a dot!" << endl;
	m_majorVersion = -1;
	m_minorVersion = -1;
	m_patchLevel = -1;
	m_suffix = "";
      }
      else {
	if( suffix.startsWith(".") ) {
	  suffix = suffix.mid( 1 );
	  splitVersionString( suffix, m_patchLevel, suffix );
	  if( m_patchLevel < 0 ) {
	    kdDebug() << "(K3bVersion) suffix must not start with a dot!" << endl;
	    m_majorVersion = -1;
	    m_minorVersion = -1;
	    m_patchLevel = -1;
	    m_suffix = "";
	  }
	  else {
	    m_suffix = suffix;
	  }
	}
	else {
	  m_patchLevel = -1;
	  m_suffix = suffix;
	}
      }
    }
    else {
      m_minorVersion = -1;
      m_patchLevel = -1;
      m_suffix = suffix;
    }
  }

  m_versionString = createVersionString( m_majorVersion, m_minorVersion, m_patchLevel, m_suffix );
}


// splits the leading number from s and puts it in num
// the dot is removed and the rest put in suffix
// if s does not start with a digit or the first non-digit char is not a dot
// suffix = s and num = -1 is returned
void K3bVersion::splitVersionString( const QString& s, int& num, QString& suffix )
{
  int pos = s.find( QRegExp("\\D") );
  if( pos < 0 ) {
    num = s.toInt();
    suffix = "";
  }
  else if( pos == 0 ) {
    num = -1;
    suffix = s;
  }
  else {
    num = s.left( pos ).toInt();
    suffix = s.mid( pos );
  }
}


bool K3bVersion::isValid() const
{
  return (m_majorVersion >= 0);
}


void K3bVersion::setVersion( int majorVersion, 
			     int minorVersion, 
			     int patchlevel, 
			     const QString& suffix )
{
  m_majorVersion = majorVersion;
  m_minorVersion = minorVersion;
  m_patchLevel = patchlevel;
  m_suffix = suffix;
  m_versionString = createVersionString( majorVersion, minorVersion, patchlevel, suffix );
}

K3bVersion& K3bVersion::operator=( const QString& v )
{
  setVersion( v );
  return *this;
}

QString K3bVersion::createVersionString( int majorVersion, 
					 int minorVersion, 
					 int patchlevel, 
					 const QString& suffix )
{
  if( majorVersion >= 0 ) {
    QString s = QString::number(majorVersion);
    
    if( minorVersion > -1 ) {
      s.append( QString(".%1").arg(minorVersion) );
      if( patchlevel > -1 )
	s.append( QString(".%1").arg(patchlevel) );
    }
    
    if( !suffix.isNull() )
      s.append( suffix );

    return s;
  }
  else
    return "";
}


bool operator<( const K3bVersion& v1, const K3bVersion& v2 )
{
  // both version objects need to be valid

  if( v1.majorVersion() == v2.majorVersion() ) {

    // 1 == 1.0
    if( ( v1.minorVersion() == v2.minorVersion() )
	||
	( v1.minorVersion() == -1 && v2.minorVersion() == 0 )
	||
	( v2.minorVersion() == -1 && v1.minorVersion() == 0 ) 
	)
      {
	// 1.0 == 1.0.0
	if( ( v1.patchLevel() == v2.patchLevel() )
	    ||
	    ( v1.patchLevel() == -1 && v2.patchLevel() == 0 )
	    ||
	    ( v2.patchLevel() == -1 && v1.patchLevel() == 0 )
	    )
	  {
	    // we treat the version without suffix as newer as all versions that have a suffix
	    if( v1.suffix().isEmpty() && !v2.suffix().isEmpty() )
	      return false;
	    else if( v2.suffix().isEmpty() && !v1.suffix().isEmpty() )
	      return true;
	    // we need to handle this case specially since QString("") < QString::null is true
	    else if( v1.suffix().isEmpty() && v2.suffix().isEmpty() )
	      return false;
	    else
	      return ( v1.suffix() < v2.suffix() );
	  }
	else
	  return ( v1.patchLevel() < v2.patchLevel() );
      }
    else
      return ( v1.minorVersion() < v2.minorVersion() );
  }
  else 
    return ( v1.majorVersion() < v2.majorVersion() );
}

bool operator>( const K3bVersion& v1, const K3bVersion& v2 )
{
  return operator<( v2, v1 );
}


bool operator==( const K3bVersion& v1, const K3bVersion& v2 )
{
  return ( v1.majorVersion() == v2.majorVersion() &&
	   v1.minorVersion() == v2.minorVersion() &&
	   v1.patchLevel() == v2.patchLevel() &&
	   ( v1.suffix() == v2.suffix() ||
	     v1.suffix().isEmpty() && v2.suffix().isEmpty() ) );
}


bool operator<=( const K3bVersion& v1, const K3bVersion& v2 )
{
  return ( operator<( v1, v2 ) || operator==( v1, v2 ) );
}

bool operator>=( const K3bVersion& v1, const K3bVersion& v2 )
{
  return ( operator>( v1, v2 ) || operator==( v1, v2 ) );
}
