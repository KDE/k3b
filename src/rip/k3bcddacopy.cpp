/***************************************************************************
                          k3bcddacopy.cpp  -  description
                             -------------------
    begin                : Sun Nov 4 2001
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

#include <qthread.h>

#include "k3bcdda.h"
#include "k3bcddacopy.h"
#include "k3bcdview.h"
#include "k3bripperwidget.h"

#include <qlist.h>
#include <qstringlist.h>
#include <qstring.h>
#include <qfile.h>
#include <qdatastream.h>

#include <kprogress.h>

typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
#include <cdda_paranoia.h>
}
#define WAVHEADER_SIZE       44

void paranoiaCallback(long, int){
  // Do we want to show info somewhere ?
  // Not yet.
}


K3bCddaCopy::K3bCddaCopy(int arraySize) {
    m_count = arraySize;
    m_cdda = new K3bCdda();
    m_progress = 0;
}

K3bCddaCopy::~K3bCddaCopy(){
    delete m_cdda;
    qDebug("(K3bCddaCopy) Exit destructor." );
}

void K3bCddaCopy::run(){
    m_bytesAll = 0;
    m_interrupt = false;
    qDebug("(K3bCddaCopy) Start copying " + QString::number(m_bytes) );
    struct cdrom_drive *m_drive = m_cdda->pickDrive(m_device);
    for( int i=0; i< m_count; i++){
        qDebug("(K3bCddaCopy) Copy track: " + QString::number(m_track[i]) + " to " + m_list[i] );
        if( !paranoiaRead( m_drive, m_track[i], m_list[i] ))
            break;
        if( i +1 == m_count){
            qDebug("(K3bCddaCopy) Last track reached." );
            break;
        }
    }
    qDebug("(K3bCddaCopy) Finished copying." );
    m_cdda->closeDrive(m_drive);
    qDebug("(K3bCddaCopy) Exit." );
    //m_progress = 0;
}

void K3bCddaCopy::setProgressBar(KProgress *progress, long bytes){
    m_progress = progress;
    m_bytes = bytes;
}

void K3bCddaCopy::setCopyTracks(QArray<int> tracks){
    m_track = tracks;
}

void K3bCddaCopy::setCopyFiles(QStringList files){
    m_list = files;
}

void K3bCddaCopy::setCopyCount(int count){
    m_count = count;
}

void K3bCddaCopy::setDrive(QString device){
    m_device = device;
}

void K3bCddaCopy::setFinish(bool stop){
    m_interrupt = stop;
}

bool K3bCddaCopy::paranoiaRead(struct cdrom_drive *drive, int track, QString dest){
    long firstSector = cdda_track_firstsector(drive, track);
    long lastSector = cdda_track_lastsector(drive, track);
    long byteCount =  CD_FRAMESIZE_RAW * (lastSector - firstSector);

    qDebug("(K3bCdda) paranoia_init");
    cdrom_paranoia *paranoia = paranoia_init(drive);

    if (0 == paranoia){
        qDebug("(K3bCdda) paranoia_init failed");
        return false;
    }

    int paranoiaLevel = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;
    paranoia_modeset(paranoia, paranoiaLevel);

    cdda_verbose_set(drive, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);

    paranoia_seek(paranoia, firstSector, SEEK_SET);
    long currentSector(firstSector);

    qDebug("(K3bCdda) open files");
    QFile f(dest);
    f.open(IO_WriteOnly);
    QDataStream s( &f );
    writeWavHeader( &s, byteCount );
    int debugValue=0;
    while (currentSector < lastSector) {
        int16_t * buf = paranoia_read(paranoia, paranoiaCallback);

        if (0 == buf) {
            qDebug("(K3bCddaCopy) Unrecoverable error in paranoia_read");
            break;
        } else {
            ++currentSector;
            QByteArray output;
            char * cbuf = reinterpret_cast<char *>(buf);
            s.writeRawBytes(cbuf, CD_FRAMESIZE_RAW);
        }

        if( m_progress !=0 ){
            m_bytesAll += CD_FRAMESIZE_RAW;
            int value = (int) ((((double)m_bytesAll / (double)m_bytes ) * 100)+0.5);
            if( value > 100)
                value=100;
            if(value > debugValue){
                debugValue=value+10;
                qDebug("(K3bCddaCopy) Set progress bar to "+QString::number(value)+"%.");
            }
            m_progress->setValue(value);
            //m_progress->update();
        }

        if( m_interrupt)
            break;  // user has interrupted
    } // end while sector check
    f.close();
    if( m_interrupt ) {
        qDebug("(K3bCddaCopy) Interrupted by user!");
        if( !f.remove() ){
            qDebug("(K3bCddaCopy) Can't delete copied file <"+dest+">.");
        }
    }
    paranoia_free(paranoia);
    paranoia = 0;
    if( m_interrupt )
        return false;
    return true;
}

void K3bCddaCopy::writeWavHeader(QDataStream *s, long byteCount) {
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

