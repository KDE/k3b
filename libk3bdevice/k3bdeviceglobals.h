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


namespace K3bDevice 
{
  QString deviceTypeString( int );
  QString writingModeString( int );
  /**
   * @param simplyfied if true the formatting state of DVD media is left out.
   */
  QString mediaTypeString( int, bool simplyfied = false );
  void debugBitfield( unsigned char* data, long len );

  unsigned short from2Byte( unsigned char* );
  unsigned long from4Byte( unsigned char* );
  
  char fromBcd( const char& );
  char toBcd( const char& );
  bool isValidBcd( const char& );
}

#endif
