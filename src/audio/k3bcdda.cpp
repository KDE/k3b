/***************************************************************************
                          k3bcdda.cpp  -  description
                             -------------------
    begin                : Sat Nov 3 2001
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

#include "k3bcdda.h"

#include <qstring.h>
#include <qfile.h>
#include <qdatastream.h>
#include <qdatetime.h>
#define DEFAULT_CD_DEVICE "/dev/cdrom"

typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
#include <cdda_paranoia.h>
}

void paranoiaCallback(long, int){
  // Do we want to show info somewhere ?
  // Not yet.
}

K3bCdda::K3bCdda(){
}
K3bCdda::~K3bCdda(){
}

void K3bCdda::closeDrive( struct cdrom_drive *drive ) {
		if ( cdda_close(drive) )
		   		qDebug("(K3bCdda) closing drive failed.");
}

struct cdrom_drive *K3bCdda::pickDrive( QString newPath )
{
	 qDebug("(K3bCdda) new drive: " + newPath);
    QCString path( QFile::encodeName( newPath ) );
    struct cdrom_drive *drive = 0;
	 qDebug("(K3bCdda) reformatted path: " + path);

    if( !path.isEmpty(  ) && path != "/" )
        drive = cdda_identify( path, CDDA_MESSAGE_PRINTIT, 0 );
    else {
        drive = cdda_find_a_cdrom( CDDA_MESSAGE_PRINTIT, 0 );
        if( 0 == drive ) {
            if( QFile( DEFAULT_CD_DEVICE ).exists(  ) )
                drive = cdda_identify( DEFAULT_CD_DEVICE, CDDA_MESSAGE_PRINTIT, 0 );
        }
    }
    qDebug("(K3bCdda) open cdrom");
    if ( cdda_open( drive ) )
    	qDebug("(K3bCdda) opening cdrom failed.");
    return drive;
}

long K3bCdda::getRawTrackSize(int track, struct cdrom_drive *drive){
      long firstSector = cdda_track_firstsector(drive, track);
      long lastSector = cdda_track_lastsector(drive, track);
      return CD_FRAMESIZE_RAW * (lastSector - firstSector);
}

void K3bCdda::paranoiaRead(struct cdrom_drive * drive, int track){
   long firstSector = cdda_track_firstsector(drive, track);
   long lastSector = cdda_track_lastsector(drive, track);

  cdrom_paranoia *paranoia = paranoia_init(drive);

  if (0 == paranoia){
    qDebug("(K3bCdda) paranoia_init failed");
    return;
  }

  int paranoiaLevel = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;

  /*
  switch (d->paranoiaLevel)
  {
    case 0:
      paranoiaLevel = PARANOIA_MODE_DISABLE;
      break;

    case 1:
      paranoiaLevel |=  PARANOIA_MODE_OVERLAP;
      paranoiaLevel &= ~PARANOIA_MODE_VERIFY;
      break;

    case 2:
      paranoiaLevel |= PARANOIA_MODE_NEVERSKIP;
    default:
      break;
  }
*/
  paranoia_modeset(paranoia, paranoiaLevel);

  cdda_verbose_set(drive, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);

  paranoia_seek(paranoia, firstSector, SEEK_SET);

  //long processed(0);
  long currentSector(firstSector);

  QTime timer;
  timer.start();

  int lastElapsed = 0;
  QFile f("/home/ft0001/track_1.wav");
  f.open(IO_WriteOnly);
  QDataStream s( &f );
  while (currentSector < lastSector) {
    int16_t * buf = paranoia_read(paranoia, paranoiaCallback);

    if (0 == buf) {
      qDebug("(K3bCdda) Unrecoverable error in paranoia_read");
      break;
    } else {
      ++currentSector;
      QByteArray output;
      char * cbuf = reinterpret_cast<char *>(buf);
      s.writeRawBytes(cbuf, CD_FRAMESIZE_RAW);
      //s << (Q_INT16) buf;
      //output.setRawData(cbuf, CD_FRAMESIZE_RAW);
      //data(output);
      //output.resetRawData(cbuf, CD_FRAMESIZE_RAW);
      //processed += CD_FRAMESIZE_RAW;


      //int elapsed = timer.elapsed() / 1000;

      /*
      if (elapsed != lastElapsed)
      {
        processedSize(processed);

        if (0 != elapsed)
          speed(processed / elapsed);
      }

      lastElapsed = elapsed;
			*/
    }
  } // end while sector check
  paranoia_free(paranoia);
  paranoia = 0;
}


