/* 
 *
 * $Id: $
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


#ifndef K3B_CDPARANOIA_LIB_H
#define K3B_CDPARANOIA_LIB_H

// from cdda_interface.h
#define CD_FRAMESIZE_RAW 2352


#include <qstring.h>

#include <sys/types.h>


class K3bCdparanoiaLib
{
 public:
  ~K3bCdparanoiaLib();

  bool paranoiaInit( const QString& devicename );
  void paranoiaFree();

  /** default: 3 */
  void setParanoiaMode( int );
  void setNeverSkip( bool b );

  /** default: 20 */
  void setMaxRetries( int );

  int16_t* paranoiaRead(void(*callback)(long,int));
  long paranoiaSeek( long, int );

  long firstSector( int );
  long lastSector( int );

  /**
   * returns 0 if the cdparanoialib could not
   * be found on the system.
   * Otherwise you have to take care of
   * deleting.
   */
  static K3bCdparanoiaLib* create();

 private:
  K3bCdparanoiaLib();
  bool load();

  static void* s_libInterface;
  static void* s_libParanoia;
  static int s_counter;

  class Private;
  Private* d;
};


#endif
