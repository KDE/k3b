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


#ifndef _K3B_VERSION_H_
#define _K3B_VERSION_H_

#include <qstring.h>

class K3bVersion 
{
 public:
  /**
   * construct an empty version object
   * which is invalid
   * @ see isValid()
   */
  K3bVersion();

  /**
   * copy constructor
   */
  K3bVersion( const K3bVersion& );

  /**
   * this constructor tries to parse the given version string
   */
  K3bVersion( const QString& version );

  /**
   * sets the version and generates a version string from it
   */
  K3bVersion( int majorVersion, int minorVersion, int pachlevel = -1, const QString& suffix = QString::null );

  /**
   * tries to parse the version string
   * used by the constructor
   */
  void setVersion( const QString& );

  bool isValid() const;

  /**
   * sets the version and generates a version string from it
   * used by the constructor
   *
   * If minorVersion or pachlevel are -1 they will not be used when generating the version string.
   */
  void setVersion( int majorVersion, int minorVersion = -1, int patchlevel = -1, const QString& suffix = QString::null );

  const QString& versionString() const { return m_versionString; }
  int majorVersion() const { return m_majorVersion; }
  int minorVersion() const { return m_minorVersion; }
  int patchLevel() const { return m_patchLevel; }
  const QString& suffix() const { return m_suffix; }

  /**
   * just to make it possible to use as a QString
   */
  operator const QString& () const { return m_versionString; }
  K3bVersion& operator=( const QString& v );

  /**
   * If minorVersion or pachlevel are -1 they will not be used when generating the version string.
   * If minorVersion is -1 patchlevel will be ignored.
   */
  static QString createVersionString( int majorVersion, 
				      int minorVersion = -1, 
				      int patchlevel = -1, 
				      const QString& suffix = QString::null );

 private:
  static void splitVersionString( const QString& s, int& num, QString& suffix );

  QString m_versionString;
  int m_majorVersion;
  int m_minorVersion;
  int m_patchLevel;
  QString m_suffix;
};


bool operator<( const K3bVersion& v1, const K3bVersion& v2 );
bool operator>( const K3bVersion& v1, const K3bVersion& v2 );
bool operator==( const K3bVersion& v1, const K3bVersion& v2 );
bool operator<=( const K3bVersion& v1, const K3bVersion& v2 );
bool operator>=( const K3bVersion& v1, const K3bVersion& v2 );


#endif
