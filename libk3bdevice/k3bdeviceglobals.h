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


#ifndef _K3B_DEVICE_GLOBALS_H_
#define _K3B_DEVICE_GLOBALS_H_

#include <qstring.h>
#include <k3bmsf.h>
#include "k3b_export.h"

namespace K3bDevice 
{
  class Device;

  LIBK3BDEVICE_EXPORT QString deviceTypeString( int );
  LIBK3BDEVICE_EXPORT QString writingModeString( int );
  /**
   * @param simplyfied if true the formatting state of DVD media is left out.
   */
  LIBK3BDEVICE_EXPORT QString mediaTypeString( int, bool simplyfied = false );
  void debugBitfield( unsigned char* data, long len );

  LIBK3BDEVICE_EXPORT unsigned short from2Byte( unsigned char* );
  unsigned long from4Byte( unsigned char* );
  
  char fromBcd( const char& );
  char toBcd( const char& );
  bool isValidBcd( const char& );

  /**
   * @return the maximum nuber of sectors that can be read from device @p dev starting
   * at sector @p firstSector.
   */
  int determineMaxReadingBufferSize( Device* dev, const K3b::Msf& firstSector );
}

#endif
