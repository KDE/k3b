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
#define WAVHEADER_SIZE       44

typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
#include <cdda_paranoia.h>
}

K3bCdda::K3bCdda(){
}

K3bCdda::~K3bCdda(){
}

bool K3bCdda::closeDrive( struct cdrom_drive *drive ){
    if ( cdda_close(drive) ){
        qDebug("(K3bCdda) closing drive failed.");
        return false;
    }
    return true;
}

bool K3bCdda::openDrive( struct cdrom_drive *drive ){
    if ( cdda_close(drive) ){
        qDebug("(K3bCdda) opening drive failed.");
        return false;
    }
    return true;
}

struct cdrom_drive* K3bCdda::pickDrive( QString newPath )
{
    qDebug("(K3bCdda) new drive: %s", newPath.latin1());
    QCString path( QFile::encodeName( newPath ) );
    struct cdrom_drive *drive = 0;
    qDebug("(K3bCdda) reformatted path: %s", path.data());

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
    if ( cdda_open( drive ) ) {
        qDebug("(K3bCdda) opening cdrom failed.");
    }
    return drive;
}

int K3bCdda::driveType( struct cdrom_drive *drive ){
    int result = -1;
    int count = drive->tracks;
    bool audio = false;
    bool data = false;
    for( int i=0; i < count; i++ ){
        unsigned char flag = drive->disc_toc[ i ].bFlags;
        if(  (flag | 0x10) == 0x10 ){
            audio = true;
        } else if( (flag | 0x14) == 0x14){
            data= true;
        }
    }
    if( data && audio)
        result = 2;
    else if ( data )
        result = 1;
    else if ( audio )
        result = 0;
    else
        result = -1;
    return result;
}

long K3bCdda::getRawTrackSize(int track, struct cdrom_drive *drive){
      long firstSector = cdda_track_firstsector(drive, track);
      long lastSector = cdda_track_lastsector(drive, track);
      return CD_FRAMESIZE_RAW * (lastSector - firstSector);
}

void K3bCdda::writeWavHeader(QDataStream *s, long byteCount) {
  static char riffHeader[] =
  {
    0x52, 0x49, 0x46, 0x46, // 0  "AIFF"
    0x00, 0x00, 0x00, 0x00, // 4  wavSize
    0x57, 0x41, 0x56, 0x45, // 8  "WAVE"
    0x66, 0x6d, 0x74, 0x20, // 12 "fmt "
    0x10, 0x00, 0x00, 0x00, // 16
    0x01, 0x00, 0x02, 0x00, // 20
    0x44, 0xac, 0x00, 0x00, // 24
    0x10, 0xb1, 0x02, 0x00, // 28
    0x04, 0x00, 0x10, 0x00, // 32
    0x64, 0x61, 0x74, 0x61, // 36 "data"
    0x00, 0x00, 0x00, 0x00  // 40 byteCount
  };

  Q_INT32 wavSize(byteCount + 44 - 8);


  riffHeader[4]   = (wavSize   >> 0 ) & 0xff;
  riffHeader[5]   = (wavSize   >> 8 ) & 0xff;
  riffHeader[6]   = (wavSize   >> 16) & 0xff;
  riffHeader[7]   = (wavSize   >> 24) & 0xff;

  riffHeader[40]  = (byteCount >> 0 ) & 0xff;
  riffHeader[41]  = (byteCount >> 8 ) & 0xff;
  riffHeader[42]  = (byteCount >> 16) & 0xff;
  riffHeader[43]  = (byteCount >> 24) & 0xff;

  s->writeRawBytes(riffHeader, WAVHEADER_SIZE);
}

