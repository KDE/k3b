/* 
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_DEVICE_GLOBALS_H_
#define _K3B_DEVICE_GLOBALS_H_

#include <qstring.h>
#include <k3bmsf.h>
#include "k3bdevice_export.h"

namespace K3bDevice 
{
  typedef Q_UINT8 uint8;
  typedef Q_UINT16 uint16;
  typedef Q_UINT32 uint32;

  class Device;

  LIBK3BDEVICE_EXPORT QString deviceTypeString( int );
  LIBK3BDEVICE_EXPORT QString writingModeString( int );
  /**
   * @param simplyfied if true the formatting state of DVD media is left out.
   */
  LIBK3BDEVICE_EXPORT QString mediaTypeString( int, bool simplyfied = false );
  LIBK3BDEVICE_EXPORT void debugBitfield( unsigned char* data, long len );

  LIBK3BDEVICE_EXPORT uint16 from2Byte( unsigned char* );
  LIBK3BDEVICE_EXPORT uint32 from4Byte( unsigned char* );
  
  LIBK3BDEVICE_EXPORT char fromBcd( const char& );
  LIBK3BDEVICE_EXPORT char toBcd( const char& );
  LIBK3BDEVICE_EXPORT bool isValidBcd( const char& );

  /**
   * @return the maximum nuber of sectors that can be read from device @p dev starting
   * at sector @p firstSector.
   */
  int determineMaxReadingBufferSize( Device* dev, const K3b::Msf& firstSector );
}

#endif
