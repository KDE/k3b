/***************************************************************************
                          k3bglobals.cpp  -  description
                             -------------------
    begin                : Wen Jun 13 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "k3bglobals.h"
#include "device/Sample.h"
#include "device/util.h"

#include <qdatastream.h>


QString K3b::framesToString( int h, bool showFrames )
{
  int m = h / 4500;
  int s = (h % 4500) / 75;
  int f = h % 75;

  QString str;

  if( showFrames ) {
    // cdrdao needs the MSF format where 1 second has 75 frames!
    str.sprintf( "%.2i:%.2i:%.2i", m, s, f );
  }
  else
    str.sprintf( "%.2i:%.2i", m, s );

  return str;
}

QString K3b::sizeToTime(long size){
	int h = size / sizeof(Sample) / 588;
	return framesToString(h, false);
}


Q_INT16 K3b::swapByteOrder( Q_INT16 i )
{
  return (i << 8) | (i >> 8 );
}


Q_INT32 K3b::swapByteOrder( Q_INT32 i )
{
  return (i << 24) | ((i << 8) & 0xff0000) | ((i >> 8) & 0xff00) | (i >> 24);
}

