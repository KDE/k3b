/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <k3bglobals.h>
#include "k3bcddacopy.h"
#include "k3bpatternparser.h"
#include <device/k3bdevice.h>
#include <device/k3bdiskinfodetector.h>
#include <tools/k3bcdparanoialib.h>
#include <k3bprogressinfoevent.h>
#include <k3bthreadjob.h>
#include <k3bcore.h>
#include <k3bapplication.h>

#include "songdb/k3bsong.h"
#include "songdb/k3bsongmanager.h"

#include <qptrlist.h>
#include <qstringlist.h>
#include <qstring.h>
#include <qdir.h>
#include <qapplication.h>
#include <qtimer.h>

#include <klocale.h>
#include <kio/global.h>
#include <kdebug.h>
#include <kapplication.h>



K3bCddaCopy::K3bCddaCopy( QObject* parent ) 
  : K3bJob( parent ),
    m_bUsePattern( true ),
    m_singleFile(false)
{
  m_device = 0;
  
  m_audioRip = new K3bAudioRipThread();
  K3bThreadJob* threadJob = new K3bThreadJob( m_audioRip, this );

  connect( threadJob, SIGNAL(data(const char*, int)), this, SLOT(slotTrackOutput(const char*, int)) );
  connect( threadJob, SIGNAL(percent(int)), this, SLOT(slotTrackPercent(int)) );
  connect( threadJob, SIGNAL(finished(bool)), this, SLOT(slotTrackFinished(bool)) );
  connect( threadJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );

  m_diskInfoDetector = new K3bDiskInfoDetector( this );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bCdDevice::DiskInfo&)), 
	   this, SLOT(slotDiskInfoReady(const K3bCdDevice::DiskInfo&)) );
}

K3bCddaCopy::~K3bCddaCopy()
{
  delete m_audioRip;
}

void K3bCddaCopy::start()
{
  emit started();
  emit newTask( i18n("Reading Digital Audio")  );
  emit infoMessage( i18n("Retrieving information about disk"), K3bJob::PROCESS );
  m_diskInfoDetector->detect( m_device );
}


void K3bCddaCopy::slotDiskInfoReady( const K3bCdDevice::DiskInfo& info )
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
  m_bytesToCopy = 0;
  for( QValueList<int>::const_iterator it = m_tracksToCopy.begin();
       it != m_tracksToCopy.end(); ++it ) {
    m_bytesToCopy += info.toc[*it-1].length() * CD_FRAMESIZE_RAW;
  }

  m_lastOverallPercent = 0;
  m_bytesAll = 0;

  createFilenames();

  // check if libcdparanoia is available
  K3bCdparanoiaLib* lib = K3bCdparanoiaLib::create();
  if( !lib ) {
    emit infoMessage( i18n("Could not dlopen libcdda_paranoia. Please install."), ERROR );
    emit finished(false);
    return;
  }
  delete lib;

  if( !startRip( m_currentTrackIndex = 0 ) ) {
    emit finished( false );
  }
}


bool K3bCddaCopy::startRip( unsigned int i )
{
  m_currentRippedTrackNumber = m_tracksToCopy[i];

  infoMessage( i18n("Reading track %1").arg( m_currentRippedTrackNumber ), PROCESS );

  // if we are writing a single file we only need the first filename
  if( !m_singleFile || i == 0 )
    infoMessage( i18n("to ") + KIO::decodeFileName( m_list[i] ), PROCESS );

  QString dir = m_list[i].left( m_list[i].findRev("/") );
  if( !createDirectory( dir ) ) {
    infoMessage( i18n("Unable to create directory %1").arg(dir), ERROR );
    return false;
  }
  
  // open a file to write to
  // create wave file
  m_currentWrittenFile = m_list[i];

  bool isOpen = true;
  // if we write a single file we only need to open the writer once
  if( !m_singleFile || i == 0 )
    isOpen = m_waveFileWriter.open( m_list[i] );
  
  if( !isOpen ){
    infoMessage( i18n("Unable to rip to: %1").arg(m_list[i]), ERROR );
    m_currentWrittenFile = QString::null;

    return false;
  }

  kdDebug() << "(K3bCddaCopy) starting K3bAudioRip" << endl;

  m_audioRip->setTrackToRip( m_currentRippedTrackNumber );
  m_audioRip->setDevice( m_device );

  m_audioRip->start();


  if( !m_cddbEntry.artists[m_currentRippedTrackNumber-1].isEmpty() &&
      !m_cddbEntry.titles[m_currentRippedTrackNumber-1].isEmpty() )
    emit newSubTask( i18n("Reading track %1 (%2 - %3)").arg(m_currentRippedTrackNumber).arg(m_cddbEntry.artists[m_currentRippedTrackNumber-1]).arg(m_cddbEntry.titles[m_currentRippedTrackNumber-1]) );
  else
    emit newSubTask( i18n("Reading track %1").arg(m_currentRippedTrackNumber) );

  return true;
}


void K3bCddaCopy::slotTrackOutput( const char* data, int len )
{
  m_waveFileWriter.write( data, len, K3bWaveFileWriter::LittleEndian );
  m_bytesAll += len;

  m_audioRip->resume();
}


void K3bCddaCopy::cancel( ){
  m_audioRip->cancel();
  m_interrupt = true;

  // what if paranoia is stuck in paranoia_read?
  // we need to terminate in that case
  // wait for 1 second. I the thread still is working terminate it
  // and trigger the finished slot manually
  emit infoMessage( i18n("Cancellation could take a while..."), INFO );
  QTimer::singleShot( 1000, this, SLOT(slotCheckIfThreadStillRunning()) );
}


void K3bCddaCopy::slotTrackFinished( bool success )
{
  if( !m_singleFile || !success )
    m_waveFileWriter.close();

  if( success ) {
  
    kdDebug() << "(K3bCddaCopy) creating new entry in SongDb." << endl;
    K3bSong* song = new K3bSong( m_currentWrittenFile.right( m_currentWrittenFile.length() - 1 - m_currentWrittenFile.findRev("/") ),
				 m_cddbEntry.cdTitle,
				 m_cddbEntry.artists[m_currentRippedTrackNumber-1],
				 m_cddbEntry.titles[m_currentRippedTrackNumber-1],
				 m_cddbEntry.discid,
				 m_currentRippedTrackNumber );
    k3bapp->songManager()->addSong( m_currentWrittenFile.left(m_currentWrittenFile.findRev("/")), song );


    ++m_currentTrackIndex;

    if( m_currentTrackIndex < m_tracksToCopy.count() ) {
      if( !startRip( m_currentTrackIndex ) )
	emit finished( false );
    } else {
      emit infoMessage( i18n("Successfully read all tracks"), STATUS );
      // close the single file
      if( m_singleFile )
	m_waveFileWriter.close();

      emit finished( true );
    }
  }
  else {
    if( m_interrupt ) {
      infoMessage( i18n("Canceled by user"), ERROR );
      kdDebug() << "(K3bCddaCopy) Interrupted by user!" << endl;
      if( !QFile::remove( m_currentWrittenFile ) ){
	infoMessage( i18n("Unable to delete part of copied file"), ERROR );
	kdDebug() << "(K3bCddaCopy) Can't delete copied file <>." << endl;
      }
    }
    else {
      emit infoMessage( i18n("Error while ripping track %1").arg( m_currentTrackIndex ), ERROR );
    }

    if( m_interrupt )
      emit canceled();
    emit finished( false );
  }
}


void K3bCddaCopy::createFilenames()
{
  m_list.clear();
  
  if( m_baseDirectory[m_baseDirectory.length()-1] != '/' )
    m_baseDirectory.append("/");

  for( QValueList<int>::const_iterator it = m_tracksToCopy.begin();
       it != m_tracksToCopy.end(); ++it ) {
    int index = *it - 1;
   
    QString extension = ".wav";

    if( m_diskInfo.toc[index].type() == K3bTrack::DATA ) {
      extension = ".iso";
    }


    QString dir = m_baseDirectory;
    QString filename;

    if( m_bUsePattern ) {
      dir += K3bPatternParser::parsePattern( m_cddbEntry, *it, 
					     m_dirPattern,
					     m_replaceBlanksInDir,
					     m_dirReplaceString );
    }

    if( dir[dir.length()-1] != '/' )
      dir.append("/");

    if( m_bUsePattern ) {
      filename = K3bPatternParser::parsePattern( m_cddbEntry, *it, 
						 m_filenamePattern,
						 m_replaceBlanksInFilename,
						 m_filenameReplaceString ) + extension;
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


QString K3bCddaCopy::jobDescription() const 
{
  if( m_cddbEntry.titles.count() == 0 )
    return i18n("Ripping audio tracks");
  else
    return i18n("Ripping audio tracks from %1").arg(m_cddbEntry.cdTitle);
}

QString K3bCddaCopy::jobDetails() const 
{
  return i18n("1 track", "%n tracks", m_tracksToCopy.count() );
}


void K3bCddaCopy::slotCheckIfThreadStillRunning()
{
  if( m_audioRip->running() ) {
    // this could happen if the thread is stuck in paranoia_read
    // because of an unreadable cd
    m_audioRip->terminate();
    slotTrackFinished( false );
  }
}


void K3bCddaCopy::slotTrackPercent( int p )
{
  emit subPercent(p);
  emit percent( (int)(100.0*(double)m_bytesAll/(double)m_bytesToCopy) );
}

#include "k3bcddacopy.moc"
