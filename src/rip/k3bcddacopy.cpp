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


#include "k3bcdda.h"
#include "k3bcddacopy.h"
#include "k3bcdview.h"
#include "k3bripperwidget.h"

#include <qlist.h>
#include <qstringlist.h>
#include <qstring.h>
#include <qfile.h>
#include <qdatastream.h>
#include <qtimer.h>

#include <klocale.h>
#include <kio/global.h>

#define WAVHEADER_SIZE       44

void paranoiaCallback(long, int){
  // Do we want to show info somewhere ?
  // Not yet.
}


K3bCddaCopy::K3bCddaCopy(int arraySize) : K3bJob() {
    m_count = arraySize;
    m_cdda = new K3bCdda();
}

K3bCddaCopy::~K3bCddaCopy(){
    delete m_cdda;
}

void K3bCddaCopy::start(){
    //bool result = true;
    m_bytesAll = 0;
    m_interrupt = false;
    qDebug("(K3bCddaCopy) Start copying " + QString::number( m_bytes) );
    m_drive = m_cdda->pickDrive(m_device);
    emit started();
    emit newTask( i18n("Copy cdrom ")  );
    infoMessage( i18n("Copying started."), STATUS);
    if( !startRip( m_currentTrackIndex=0 ) )
        emit finished( false );
}

bool K3bCddaCopy::startRip(int i){
    infoMessage( i18n("Copy track: ") + QString::number(m_track[i]) + i18n(" to ") + KIO::decodeFileName( m_list[i] ), PROCESS);
    if( !paranoiaRead( m_drive, m_track[i], m_list[i] ))
        return false;
    return true;
}

void K3bCddaCopy::finishedRip(){
    qDebug("(K3bCddaCopy) Finished copying." );
    infoMessage( "Copying finished.", STATUS);
    m_cdda->closeDrive(m_drive);
    qDebug("(K3bCddaCopy) Exit." );
   emit finished( true );
}

void K3bCddaCopy::setBytes( long bytes){
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

void K3bCddaCopy::cancel( ){
    m_interrupt = true;
}

bool K3bCddaCopy::paranoiaRead(struct cdrom_drive *drive, int track, QString dest){
    long firstSector = cdda_track_firstsector(drive, track);
    m_lastSector = cdda_track_lastsector(drive, track);
    // track length
    m_byteCount =  CD_FRAMESIZE_RAW * (m_lastSector - firstSector);
    m_trackBytesAll = 0;

    qDebug("(K3bCddaCopy) paranoia_init");
    m_paranoia = paranoia_init(drive);

    if (0 == m_paranoia){
        infoMessage( i18n("paranoia_init failed."), ERROR);
        qDebug("(K3bCddaCopy) paranoia_init failed");
        return false;
    }

    int paranoiaLevel = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;
    paranoia_modeset(m_paranoia, paranoiaLevel);

    cdda_verbose_set(drive, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);

    paranoia_seek(m_paranoia, firstSector, SEEK_SET);
    m_currentSector = firstSector;

    qDebug("(K3bCddaCopy) open files");
    m_f = new QFile(dest);
    bool isOpen = m_f->open(IO_WriteOnly);
    if( !isOpen ){
        infoMessage( i18n("Couldn't rip to: ") + dest, ERROR );
        paranoia_free(m_paranoia);
        m_paranoia = 0;
        finishedRip();
        m_interrupt = true;
        return false;
    }
    m_stream = new QDataStream( m_f );
    writeWavHeader( m_stream, m_byteCount );
    emit newSubTask( i18n("Copy ") + dest  );

    t = new QTimer( this );
    connect( t, SIGNAL(timeout()), SLOT(slotReadData()) );
    t->start( 0, FALSE );
    return true;
}

void K3bCddaCopy::readDataFinished(){
    m_f->close();
    if( m_interrupt ) {
        infoMessage( i18n("Interrupted by user"), STATUS);
        qDebug("(K3bCddaCopy) Interrupted by user!");
        if( !m_f->remove() ){
            infoMessage( i18n("Can't delete part of copied file."), ERROR);
            qDebug("(K3bCddaCopy) Can't delete copied file <>.");
        }
    }
    paranoia_free(m_paranoia);
    m_paranoia = 0;
    ++m_currentTrackIndex;
    qDebug("(K3bCddaCopy) Check index: %i, %i", m_currentTrackIndex, m_count);
    if( (m_currentTrackIndex < m_count) && !m_interrupt ){
        startRip( m_currentTrackIndex );
    } else
        finishedRip();
}

void K3bCddaCopy::slotReadData(){

    if( m_interrupt){
        infoMessage( i18n("Interrupt reading."), PROCESS);
        qDebug("(K3bCddaCopy) Interrupt reading.");
        t->stop();
        readDataFinished();
    } else {
        int16_t * buf = paranoia_read(m_paranoia, paranoiaCallback);

        if (0 == buf) {
            infoMessage( i18n("Unrecoverable error in paranoia_read."), ERROR);
            qDebug("(K3bCddaCopy) Unrecoverable error in paranoia_read");
        } else {
            ++m_currentSector;
            QByteArray output;
            char * cbuf = reinterpret_cast<char *>(buf);
            m_stream->writeRawBytes(cbuf, CD_FRAMESIZE_RAW);
        }

      m_bytesAll += CD_FRAMESIZE_RAW;
      m_trackBytesAll += CD_FRAMESIZE_RAW;
      m_progressBarValue = (int) ((((double) m_bytesAll / (double) m_bytes ) * 100)+0.5);
      int trackProgressBarValue = (int) ((((double) m_trackBytesAll / (double) m_byteCount ) * 100)+0.5);

      if( m_progressBarValue > 100)
           m_progressBarValue=100;
      emit percent( m_progressBarValue );
      emit subPercent( trackProgressBarValue );

        if (m_currentSector < m_lastSector)
            t->start(0, FALSE);
        else {
            t->stop();
            readDataFinished();
        }
    }
} // end while sector check

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

#include "k3bcddacopy.moc"

