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


#include "../tools/k3bglobals.h"
#include "k3bcddacopy.h"
#include "k3bcdview.h"
#include "k3bripperwidget.h"
#include "k3bpatternparser.h"
#include "../device/k3bdevice.h"
#include "../cdinfo/k3bdiskinfodetector.h"

#include <qptrlist.h>
#include <qstringlist.h>
#include <qstring.h>
#include <qtimer.h>
#include <qdir.h>

#include <klocale.h>
#include <kio/global.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kapplication.h>

void paranoiaCallback(long, int){
  // Do we want to show info somewhere ?
  // Not yet.
}


K3bCddaCopy::K3bCddaCopy( QObject* parent ) 
  : K3bJob( parent ),
    m_bUsePattern( true )
{
  m_device = 0;

  m_diskInfoDetector = new K3bDiskInfoDetector( this );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bDiskInfo&)), 
	   this, SLOT(slotDiskInfoReady(const K3bDiskInfo&)) );

  m_rippingTimer = new QTimer( this );
  connect( m_rippingTimer, SIGNAL(timeout()), this, SLOT(slotReadData()) );
}

K3bCddaCopy::~K3bCddaCopy()
{
}

void K3bCddaCopy::start()
{
  emit infoMessage( i18n("Retrieving information about disk"), K3bJob::PROCESS );
  m_diskInfoDetector->detect( m_device );
}


void K3bCddaCopy::slotDiskInfoReady( const K3bDiskInfo& info )
{
  m_diskInfo = info;

  if( info.noDisk ) {
    emit infoMessage( i18n("No disk in drive"), K3bJob::ERROR );
    emit finished( false );
    return;
  }

  if( info.empty ) {
    emit infoMessage( i18n("Disk is empty"), K3bJob::ERROR );
    emit finished( false );
    return;
  }


  // bytes to copy
  m_bytes = 0;
  for( QValueList<int>::const_iterator it = m_tracksToCopy.begin();
       it != m_tracksToCopy.end(); ++it ) {
    m_bytes += info.toc[*it-1].length() * CD_FRAMESIZE_RAW;
  }

  m_lastOverallPercent = 0;
  m_bytesAll = 0;
  m_interrupt = false;
  emit started();
  emit newTask( i18n("Copy cdrom ")  );
  infoMessage( i18n("Copying started."), STATUS);

  createFilenames();

  m_drive = m_device->open();

  if( !startRip( m_currentTrackIndex = 0 ) ) {
    emit finished( false );
  }
}


bool K3bCddaCopy::startRip( unsigned int i )
{
  infoMessage( i18n("Ripping track: ") + 
	       QString::number( m_tracksToCopy[i] ), PROCESS );

  infoMessage( i18n("to ") + 
	       KIO::decodeFileName( m_list[i] ), PROCESS );

  QString dir = m_list[i].left( m_list[i].findRev("/") );
  if( !createDirectory( dir ) ) {
    infoMessage( i18n("Could not create directory %1").arg(dir), ERROR );
    return false;
  }
  
  if( !paranoiaRead( m_drive, m_tracksToCopy[i], m_list[i] )) {
    infoMessage( i18n("Could not rip track %1").arg(i), ERROR );
    return false;
  }

  return true;
}

void K3bCddaCopy::finishedRip(){
    kdDebug() << "(K3bCddaCopy) Finished copying." << endl;
    infoMessage( "Copying finished.", STATUS);
    m_device->close();
    kdDebug() << "(K3bCddaCopy) Exit." << endl;
   emit finished( true );
}


void K3bCddaCopy::cancel( ){
    m_interrupt = true;
}


bool K3bCddaCopy::paranoiaRead(struct cdrom_drive *drive, int track, QString dest)
{
    long firstSector = cdda_track_firstsector(drive, track);
    m_lastSector = cdda_track_lastsector(drive, track);
    // track length
    m_byteCount =  CD_FRAMESIZE_RAW * (m_lastSector - firstSector);
    m_trackBytesAll = 0;
    m_lastTrackPercent = 0;

    kdDebug() << "(K3bCddaCopy) paranoia_init" << endl;
    m_paranoia = paranoia_init(drive);

    if (0 == m_paranoia){
        infoMessage( i18n("paranoia_init failed."), ERROR);
        kdDebug() << "(K3bCddaCopy) paranoia_init failed" << endl;
        return false;
    }

    int paranoiaLevel = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;
    paranoia_modeset(m_paranoia, paranoiaLevel);

    cdda_verbose_set(drive, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);

    paranoia_seek(m_paranoia, firstSector, SEEK_SET);
    m_currentSector = firstSector;

    kdDebug() << "(K3bCddaCopy) open files" << endl;

    // create wave file
    m_currentWrittenFile = dest;
    bool isOpen = m_waveFileWriter.open( dest );

    if( !isOpen ){
        infoMessage( i18n("Couldn't rip to: %1").arg(dest), ERROR );
	m_currentWrittenFile = QString::null;
        paranoia_free(m_paranoia);
        m_paranoia = 0;
        finishedRip();
        m_interrupt = true;
        return false;
    }

    emit newSubTask( i18n("Copy %1").arg(dest) );

    m_rippingTimer->start(0);

    return true;
}

void K3bCddaCopy::readDataFinished()
{
    m_waveFileWriter.close();
    if( m_interrupt ) {
        infoMessage( i18n("Interrupted by user"), STATUS);
        kdDebug() << "(K3bCddaCopy) Interrupted by user!" << endl;
        if( !QFile::remove( m_currentWrittenFile ) ){
            infoMessage( i18n("Can't delete part of copied file."), ERROR);
            kdDebug() << "(K3bCddaCopy) Can't delete copied file <>." << endl;
        }
    }
    m_currentWrittenFile = QString::null;

    paranoia_free(m_paranoia);
    m_paranoia = 0;
    ++m_currentTrackIndex;
    kdDebug() << "(K3bCddaCopy) Check index: " << m_currentTrackIndex << ", " << m_tracksToCopy.count() << endl;
    if( (m_currentTrackIndex < m_tracksToCopy.count()) && !m_interrupt ){
      if( !startRip( m_currentTrackIndex ) )
	emit finished( false );
    } else
        finishedRip();
}

void K3bCddaCopy::slotReadData(){

    if( m_interrupt){
        infoMessage( i18n("Interrupt reading."), PROCESS);
        kdDebug() << "(K3bCddaCopy) Interrupt reading." << endl;
        m_rippingTimer->stop();
        readDataFinished();
    } else {
        int16_t * buf = paranoia_read(m_paranoia, paranoiaCallback);

        if (0 == buf) {
            infoMessage( i18n("Unrecoverable error in paranoia_read."), ERROR);
            kdDebug() << "(K3bCddaCopy) Unrecoverable error in paranoia_read" << endl;
        } else {
            ++m_currentSector;
	    m_waveFileWriter.write( (char*)buf, CD_FRAMESIZE_RAW, K3bWaveFileWriter::LittleEndian );
        }

      m_bytesAll += CD_FRAMESIZE_RAW;
      m_trackBytesAll += CD_FRAMESIZE_RAW;
      int progressBarValue = (int) (((double) m_bytesAll / (double) m_bytes ) * 100);
      int trackProgressBarValue = (int) (((double) m_trackBytesAll / (double) m_byteCount ) * 100);

//       if( m_progressBarValue > 100)
//            m_progressBarValue=100;

      // avoid too many gui updates
      if( m_lastOverallPercent < progressBarValue ) {
	emit percent( progressBarValue );
	m_lastOverallPercent = progressBarValue;
      }
      if( m_lastTrackPercent < trackProgressBarValue ) {
	emit subPercent( trackProgressBarValue );
	m_lastTrackPercent = trackProgressBarValue;
      }

      if (m_currentSector >= m_lastSector) {
	m_rippingTimer->stop();
	readDataFinished();
      }
    }
} // end while sector check


void K3bCddaCopy::createFilenames()
{
  KConfig* c = kapp->config();
  c->setGroup( "Ripping" );

  m_list.clear();
  
  if( m_baseDirectory[m_baseDirectory.length()-1] != '/' )
    m_baseDirectory.append("/");

  for( QValueList<int>::const_iterator it = m_tracksToCopy.begin();
       it != m_tracksToCopy.end(); ++it ) {
    int index = *it - 1;
   
    QString extension = "." + c->readEntry( "last used filetype", "wav" );

    if( m_diskInfo.toc[index].type() == K3bTrack::DATA ) {
      extension = ".iso";
    }


    QString dir = m_baseDirectory;
    QString filename;

    if( m_bUsePattern ) {
      dir += K3bPatternParser::parsePattern( m_cddbEntry, *it, 
					     c->readEntry( "directory pattern", "%r/%m" ),
					     c->readBoolEntry( "replace blank in directory", false ),
					     c->readEntry( "directory replace string", "_" ) );
    }

    if( dir[dir.length()-1] != '/' )
      dir.append("/");

    if( m_bUsePattern ) {
      filename = K3bPatternParser::parsePattern( m_cddbEntry, *it, 
						 c->readEntry( "filename pattern", "%a - %t" ),
						 c->readBoolEntry( "replace blank in filename", false ),
						 c->readEntry( "filename replace string", "_" ) ) + extension;
    }
    else {
      filename = i18n("Track%1").arg( QString::number(*it).rightJustify( 2, '0' ) ) + extension;
    }

    m_list.append( dir + filename );
  }
}


bool K3bCddaCopy::createDirectory( const QString& path )
{
  QStringList dirs = QStringList::split( "/", path );

  QDir dir = QDir::root();

  for( QStringList::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
    if( !dir.cd( *it ) ) {
      if( !dir.mkdir( *it ) ) {
	kdDebug() << "(K3bCddaCopy) could not create dir: " << dir.path() << "/" << *it << endl;
	return false;
      }
      else
	dir.cd( *it );
    }
  }    

  return true;
}


#include "k3bcddacopy.moc"

