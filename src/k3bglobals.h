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

namespace K3b
{
  enum Error { NOT_STARTED = 1, SUCCESS = 2, IMAGE_FINISHED = 3,
	       CANCELED = 4, FILE_NOT_FOUND = 5,
	       BUFFER_UNDERRUN = 6, WRITE_ERROR = 7,
	       COULD_NOT_OPEN_IMAGE = 8, DEVICE_NOT_FOUND = 9,
	       NO_TRACKS = 10, WORKING = 11, CDRECORD_ERROR = 12,
	       MPG123_ERROR = 13, WRONG_FILE_FORMAT = 14, CORRUPT_MP3 = 15,
	       MALFORMED_URL = 16, DEVICE_ERROR = 17, IO_ERROR = 18,
	       CDRDAO_ERROR = 19, MKISOFS_ERROR = 20};
  
  enum FileType { MP3 = 1, WAV = 2 };
  
  
	QString framesToString( int h, bool showFrames = true );
/* 	bool parseFrames( const QString&, int& ); */
};

#endif