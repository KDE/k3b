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
// bool K3b::parseFrames( const QString& str, int& value )
// {
// 	bool ok;
// 	int result = 0;

// 	// first check if we can parse the whole string to integer
// 	int buffer = str.toInt(&ok);
// 	if(ok) {
// 		value = buffer*100; // HACK to allow the user to enter the value in seconds (and not in hundredth)
// 		return true;
// 	}

// 	// from here on only strings with a length of 5 or 8 are allowed
// 	// TODO: handle values bigger than 99,99 minutes!!!!
// 	if( !( str.length() == 5 || str.length() == 8 ) ) {
// 		qDebug("(K3b) Parsing from '" + str + "' to integer did not work" );
// 		return false;
// 	}

// 	int m = str.mid(0,2).toInt(&ok);
// 	if(!ok) {
// 		qDebug("(K3b) Parsing from '" + str + "' to integer did not work" );
// 		return false;
// 	}
// 	result += m * 6000;

// 	int s = str.mid(2,2).toInt(&ok);
// 	if(!ok || m > 59) {
// 		qDebug("(K3b) Parsing from '" + str + "' to integer did not work" );
// 		return false;
// 	}
// 	result += s * 100;

// 	if( str.length() == 8 ) {
// 		int hs = str.mid(4,2).toInt(&ok);
// 		if(!ok) {
// 			qDebug("(K3b) Parsing from '" + str + "' to integer did not work" );
// 			return false;
// 		}
// 		result += hs;
// 	}

// 	value = result;
// 	return true;
// }
