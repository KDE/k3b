/***************************************************************************
                          k3bglobals.h  -  description
                             -------------------
    begin                : Sat Mar 31 2001
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

#ifndef K3BGLOBALS_H
#define K3BGLOBALS_H

#include <qstring.h>

class QDataStream;

namespace K3b
{
  enum WritingApp { DEFAULT, CDRECORD, CDRDAO };
  
  QString framesToString( int h, bool showFrames = true );
  QString sizeToTime( long size );

  Q_INT16 swapByteOrder( Q_INT16 i );
  Q_INT32 swapByteOrder( Q_INT32 i );

  int round( double );
};

#endif
