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
#include <tools/k3bcdparanoialib.h>
#include <k3bprogressinfoevent.h>

#include <qptrlist.h>
#include <qstringlist.h>
#include <qstring.h>
#include <qdir.h>
#include <qapplication.h>
#include <qtimer.h>

#include <klocale.h>
#include <kio/global.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kapplication.h>



K3bCddaCopy::K3bCddaCopy( QObject* parent ) 
  : K3bJob( parent ),
    m_bUsePattern( true ),
    m_singleFile(false)
{
  m_device = 0;
  
#ifdef QT_THREAD_SUPPORT
  m_audioRip = new K3bAudioRipThread( this );
#else
  m_audioRip = new K3bAudioRip( this );
#endif

  connect( m_audioRip, SIGNAL(output(const QByteArray&)), this, SLOT(slotTrackOutput(const QByteArray&)) );

#ifndef QT_THREAD_SUPPORT
  connect( m_audioRip, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_audioRip, SIGNAL(finished(bool)), this, SLOT(slotTrackFinished(bool)) );
  connect( m_audioRip, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
#endif

  m_diskInfoDetector = new K3bDiskInfoDetector( this );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bDiskInfo&)), 
	   this, SLOT(slotDiskInfoReady(const K3bDiskInfo&)) );
}

K3bCddaCopy::~K3bCddaCopy()
{
}

void K3bCddaCopy::start()
{
  emit started();
  emit newTask( i18n("Reading Digital Audio")  );
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
  infoMessage( i18n("Reading track %1").arg( m_tracksToCopy[i] ), PROCESS );

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

  m_audioRip->setTrackToRip( m_tracksToCopy[i] );
  m_audioRip->setDevice( m_device );

  m_audioRip->start();

  emit newSubTask( i18n("Reading track %1").arg(m_tracksToCopy[i]) );

  return true;
}


void K3bCddaCopy::slotTrackOutput( const QByteArray& data )
{
  m_waveFileWriter.write( data.data(), data.size(), K3bWaveFileWriter::LittleEndian );

  m_bytesAll += data.size();
}


void K3bCddaCopy::cancel( ){
  m_audioRip->cancel();
  m_interrupt = true;

#ifdef QT_THREAD_SUPPORT
  // what if paranoia is stuck in paranoia_read?
  // we need to terminate in that case
  // wait for 1 second. I the thread still is working terminate it
  // and trigger the finished slot manually
  emit infoMessage( i18n("Cancellation could take a while..."), INFO );
  QTimer::singleShot( 1000, this, SLOT(slotCheckIfThreadStillRunning()) );
#endif
}


void K3bCddaCopy::slotTrackFinished( bool success )
{
  if( !m_singleFile || !success )
    m_waveFileWriter.close();

  if( success ) {
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

    emit finished( false );
  }
}


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

#ifdef QT_THREAD_SUPPORT
void K3bCddaCopy::customEvent( QCustomEvent* e )
{
  K3bProgressInfoEvent* be = (K3bProgressInfoEvent*)e;
  switch( be->type() ) {
  case K3bProgressInfoEvent::Progress:
    {
      emit subPercent( be->firstValue() );
      
      int progressBarValue = (int) (((double) m_bytesAll / (double) m_bytes ) * 100);
      
      // avoid too many gui updates
      if( m_lastOverallPercent < progressBarValue ) {
	emit percent( progressBarValue );
	m_lastOverallPercent = progressBarValue;
      }
    }
    break;
  case K3bProgressInfoEvent::SubProgress:
 
    break;
  case K3bProgressInfoEvent::ProcessedSize:
 
    break;
  case K3bProgressInfoEvent::ProcessedSubSize:
 
    break;
  case K3bProgressInfoEvent::InfoMessage:
    emit infoMessage( be->firstString(), be->firstValue() ); 
    break;
  case K3bProgressInfoEvent::Started:
 
    break;
  case K3bProgressInfoEvent::Canceled:
 
    break;
  case K3bProgressInfoEvent::Finished:
    // wait for the thread to finish
    while( !m_audioRip->finished() )
      qApp->processEvents();
    slotTrackFinished( (bool)be->firstValue() ); 
    break;
  case K3bProgressInfoEvent::NewTask:
 
    break;
  case K3bProgressInfoEvent::NewSubTask:
 
    break;
  case K3bProgressInfoEvent::DebuggingOutput:
 
    break;
  case K3bProgressInfoEvent::BufferStatus:
 
    break;
  }
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
#endif

#include "k3bcddacopy.moc"

