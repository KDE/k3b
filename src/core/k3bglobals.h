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
#include <kio/global.h>
#include <kurl.h>

class KConfig;
class K3bVersion;

namespace K3b
{
  enum WritingApp { 
    DEFAULT = 1, 
    CDRECORD = 2, 
    CDRDAO = 4,
    DVDRECORD = 8,
    GROWISOFS = 16,
    DVD_RW_FORMAT = 32
  };

  /**
   * DATA_MODE_AUTO - let K3b determine the best mode
   * MODE1 - refers to the default Yellow book mode1
   * MODE2 - refers to CDROM XA mode2 form1
   */
  enum DataMode { 
    DATA_MODE_AUTO,
    MODE1, 
    MODE2
  };

  /**
   * AUTO  - let K3b determine the best mode
   * TAO   - Track at once
   * DAO   - Disk at once (or session at once)
   * RAW   - Raw mode
   *
   * may be or'ed together.
   */
  enum WritingMode { 
    WRITING_MODE_AUTO = 1, 
    TAO = 2, 
    DAO = 4, 
    RAW = 8,
    WRITING_MODE_INCR_SEQ = 16,  // Incremental Sequential
    WRITING_MODE_RES_OVWR = 32 // Restricted Overwrite
  };

  QString framesToString( int h, bool showFrames = true );
  QString sizeToTime( long size );

  Q_INT16 swapByteOrder( Q_INT16 i );
  Q_INT32 swapByteOrder( Q_INT32 i );

  int round( double );

  /**
   * This checks the free space on the filesystem path is in.
   * We use this since we encountered problems with the KDE version.
   * @returns true on success.
   */
  bool kbFreeOnFs( const QString& path, unsigned long& size, unsigned long& avail );

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

  /**
   * For now this just replaces multible occurences of / with a single /
   */
  QString fixupPath( const QString& );

  K3bVersion kernelVersion();
  QString systemName();

  /**
   * This gets introduced in QT 3.2 but we already need it now
   */
  unsigned long long toULongLong( const QString&, bool* ok = 0, int base = 10 );

  KIO::filesize_t filesize( const KURL& );
}

#endif
