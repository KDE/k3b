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


#ifndef K3BGLOBALS_H
#define K3BGLOBALS_H

#include <qstring.h>


namespace K3b
{
  enum WritingApp { DEFAULT = 1, CDRECORD = 2, CDRDAO = 4 };

  /**
   * AUTO - let K3b determine the best mode
   * MODE1 - refers to the default Yellow book mode1
   * MODE2 - refers to CDROM XA mode2 form1
   */
  enum DataMode { AUTO, MODE1, MODE2 };

  QString framesToString( int h, bool showFrames = true );
  QString sizeToTime( long size );

  Q_INT16 swapByteOrder( Q_INT16 i );
  Q_INT32 swapByteOrder( Q_INT32 i );

  int round( double );

  QString globalConfig();


  QString findUniqueFilePrefix( const QString& _prefix = QString::null, const QString& path = QString::null );

  /**
   * Find a unique filename in directory d (if d is empty the method uses the defaultTempPath)
   */
  QString findTempFile( const QString& ending = QString::null, const QString& d = QString::null );

  /**
   * get the default K3b temp path to store image files
   */
  QString defaultTempPath();

  /**
   * makes sure a path ends with a "/"
   */
  QString prepareDir( const QString& dir );
};

#endif
