/***************************************************************************
                          k3bcdcopyjob.cpp  -  description
                             -------------------
    begin                : Sun Mar 17 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#include "k3bcdcopyjob.h"

#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../device/k3bemptydiscwaiter.h"
#include "../device/k3bdevice.h"
#include "../cdinfo/k3bdiskinfo.h"
#include "../cdinfo/k3bdiskinfodetector.h"

#include <kprocess.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <klocale.h>

#include <qtimer.h>
#include <qstringlist.h>
#include <qfile.h>


K3bCdCopyJob::K3bCdCopyJob( QObject* parent )
  : K3bBurnJob( parent )
{
  m_process = new KProcess();

  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(parseCdrdaoStdout(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(parseCdrdaoStdout(KProcess*, char*, int)) );

  m_diskInfoDetector = new K3bDiskInfoDetector( this );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bDiskInfo&)), 
	   this, SLOT(diskInfoReady(const K3bDiskInfo&)) );
}


K3bCdCopyJob::~K3bCdCopyJob()
{
  delete m_process;
}


void K3bCdCopyJob::start()
{
  // audio:
  // für copy mit images cdda2wav + cdrecord
  // on-the-fly: cdrdao

  // data:
  // cdrdao

  // mixed:
  // cdrdao

  if( m_copies < 1 )
    m_copies = 1;
  m_finishedCopies = 0;

  emit infoMessage( i18n("Retrieving information about source disk"), K3bJob::PROCESS );
  m_diskInfoDetector->detect( m_reader );
}


void K3bCdCopyJob::diskInfoReady( const K3bDiskInfo& info )
{
  if( info.noDisk ) {
    emit infoMessage( i18n("No disk in reading drive"), K3bJob::ERROR );
    cancelAll();
    return;
  }

  if( info.empty ) {
    emit infoMessage( i18n("Source disk is empty"), K3bJob::ERROR );
    cancelAll();
    return;
  }

  if( info.tocType == K3bDiskInfo::DVD ) {
    emit infoMessage( i18n("Source disk seems to be a DVD."), K3bJob::ERROR );
    emit infoMessage( i18n("K3b is not able to copy DVDs yet."), K3bJob::ERROR );
    cancelAll();
    return;
  }


  // TODO: check size and free space on disk


  switch( info.tocType ) {
  case K3bDiskInfo::DATA:
    emit infoMessage( i18n("Source disk seems to be data cd"), K3bJob::STATUS );
    break;
  case K3bDiskInfo::AUDIO:
    emit infoMessage( i18n("Source disk seems to be audio cd"), K3bJob::STATUS );
    break;
  case K3bDiskInfo::MIXED:
    emit infoMessage( i18n("Source disk seems to be mixed mode cd"), K3bJob::STATUS );
    break;
  }

  // determine length of disk
  m_blocksToCopy = info.toc.length();


  m_tempPath = k3bMain()->findTempFile( "iso", m_tempPath );


  if( m_dummy )
    emit newTask( i18n("CD copy simulation") );
  else
    emit newTask( i18n("CD copy") );

  if( m_onTheFly ) {
    QTimer::singleShot( 0, this, SLOT(cdrdaoCopy()) );
  }
  else {
    QTimer::singleShot( 0, this, SLOT(cdrdaoRead()) );
  }

  emit started();
}


void K3bCdCopyJob::cancel()
{
  emit canceled();
  m_process->disconnect( SIGNAL(processExited(KProcess*)) );
  cancelAll();
}


void K3bCdCopyJob::cdrdaoCopy()
{
  m_process->clearArguments();

  m_process->disconnect( SIGNAL(processExited(KProcess*)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(cdrdaoCopyFinished()) );


  // remove toc-file
  if( QFile::exists( m_tocFile ) )
    QFile::remove( m_tocFile );

  if( !k3bMain()->externalBinManager()->foundBin( "cdrdao" ) ) {
    qDebug("(K3bAudioJob) could not find cdrdao executable" );
    emit infoMessage( i18n("Cdrdao executable not found."), K3bJob::ERROR );
    cancelAll();
    return;
  }

  *m_process << k3bMain()->externalBinManager()->binPath( "cdrdao" );

//   if( m_addCddbInfo )
//     *m_process << "--with-cddb";  // this should be done by K3b with a selection box like in ripping


  *m_process << "copy" << "--on-the-fly";

  if( m_fastToc )
    *m_process << "--fast-toc";
  
  *m_process << "--source-device" << m_reader->busTargetLun();
  
  if( m_reader->cdrdaoDriver() != "auto" )
    *m_process << "--source-driver" << m_reader->cdrdaoDriver();
  
  addCdrdaoWriteArguments();


  K3bEmptyDiscWaiter waiter( m_writer, k3bMain() );
  if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
    cancelAll();
    return;
  }


  if( m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    if( m_dummy ) {
      emit infoMessage( i18n("Starting CD copy simulation at %1x speed...").arg(m_speed), K3bJob::PROCESS );
    }
    else {
      emit infoMessage( i18n("Starting CD copy %1 at %2x speed...").arg(m_finishedCopies+1).arg(m_speed), K3bJob::PROCESS );
    }
  }
  else {
    qDebug( "(K3bCdCopyJob) could not start cdrdao" );
    emit infoMessage( i18n("Could not start cdrdao!"), K3bJob::ERROR );
    cancelAll();
  }
}



void K3bCdCopyJob::cdrdaoCopyFinished()
{
  if( m_process->normalExit() ) {
      // TODO: check the process' exitStatus()
      switch( m_process->exitStatus() ) {
      case 0:
	m_finishedCopies++;

	if( m_dummy ) {
	  emit infoMessage( i18n("CD Copy simulation successfully completed"), K3bJob::STATUS );
	  finishAll();
	}
	else {
	  emit infoMessage( i18n("CD Copy %1 successfully completed").arg(m_finishedCopies), K3bJob::STATUS );

	  if( m_finishedCopies < m_copies )
	    cdrdaoCopy();
	  else
	    finishAll();
	}
	break;
	
      default:
	// no recording device and also other errors!! :-(
	emit infoMessage( i18n("Cdrdao returned some error!"), K3bJob::ERROR );
	emit infoMessage( i18n("Sorry, no error handling yet!") + " :-((", K3bJob::ERROR );
	emit infoMessage( i18n("Please send me a mail with the last output..."), K3bJob::ERROR );
	
	cancelAll();
      }
  }
  else {
    emit infoMessage( i18n("Cdrdao did not exit cleanly!"), K3bJob::ERROR );
    cancelAll();
  }
}


void K3bCdCopyJob::cdrdaoRead()
{
  m_process->clearArguments();

  m_process->disconnect( SIGNAL(processExited(KProcess*)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(cdrdaoReadFinished()) );

  // remove toc-file
  if( QFile::exists( m_tocFile ) )
    QFile::remove( m_tocFile );

  if( !k3bMain()->externalBinManager()->foundBin( "cdrdao" ) ) {
    qDebug("(K3bAudioJob) could not find cdrdao executable" );
    emit infoMessage( i18n("Cdrdao executable not found."), K3bJob::ERROR );
    cancelAll();
    return;
  }

  *m_process << k3bMain()->externalBinManager()->binPath( "cdrdao" );

  *m_process << "read-cd";
  *m_process << "--datafile" << m_tempPath;

  if( m_fastToc )
    *m_process << "--fast-toc";

  *m_process << "--device" << m_reader->busTargetLun();
  
  if( m_reader->cdrdaoDriver() != "auto" )
    *m_process << "--driver" << m_reader->cdrdaoDriver();
  
  // add toc-file
  m_tocFile = locateLocal( "appdata", "temp/k3btemptoc.toc");
  *m_process << QFile::encodeName(m_tocFile);


  if( m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    emit infoMessage( i18n("Reading CD to image %1").arg(m_tempPath), K3bJob::PROCESS );
    emit newSubTask( i18n("Creating image") );
  }
  else {
    qDebug( "(K3bCdCopyJob) could not start cdrdao" );
    emit infoMessage( i18n("Could not start cdrdao!"), K3bJob::ERROR );
    cancelAll();
  }
}


void K3bCdCopyJob::cdrdaoReadFinished()
{
  m_reader->eject();

  if( m_process->normalExit() ) {
      // TODO: check the process' exitStatus()
      switch( m_process->exitStatus() ) {
      case 0:
	emit infoMessage( i18n("Image successfully created."), K3bJob::STATUS );

	cdrdaoWrite();
	break;
	
      default:
	// no recording device and also other errors!! :-(
	emit infoMessage( i18n("Cdrdao returned some error!"), K3bJob::ERROR );
	emit infoMessage( i18n("Sorry, no error handling yet!") + " :-((", K3bJob::ERROR );
	emit infoMessage( i18n("Please send me a mail with the last output..."), K3bJob::ERROR );
	
	cancelAll();
      }
  }
  else {
    emit infoMessage( i18n("Cdrdao did not exit cleanly!"), K3bJob::ERROR );
    cancelAll();
  }
}


void K3bCdCopyJob::cdrdaoWrite()
{
  m_process->clearArguments();

  m_process->disconnect( SIGNAL(processExited(KProcess*)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(cdrdaoCopyFinished()) );


  if( !k3bMain()->externalBinManager()->foundBin( "cdrdao" ) ) {
    qDebug("(K3bAudioJob) could not find cdrdao executable" );
    emit infoMessage( i18n("Cdrdao executable not found."), K3bJob::ERROR );
    cancelAll();
    return;
  }

  *m_process << k3bMain()->externalBinManager()->binPath( "cdrdao" );

  *m_process << "write";

  addCdrdaoWriteArguments();


  K3bEmptyDiscWaiter waiter( m_writer, k3bMain() );
  if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
    cancelAll();
    return;
  }


  if( m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    if( m_dummy ) {
      emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_speed), K3bJob::PROCESS );
      emit newSubTask( i18n("CD write simulation") );
    }
    else {
      emit infoMessage( i18n("Starting CD copy %1 at %2x speed...").arg(m_finishedCopies+1).arg(m_speed), K3bJob::PROCESS );
      emit newSubTask( i18n("Writing CD copy %1").arg(m_finishedCopies+1) );
    }
  }
  else {
    qDebug( "(K3bCdCopyJob) could not start cdrdao" );
    emit infoMessage( i18n("Could not start cdrdao!"), K3bJob::ERROR );
    cancelAll();
  }
}


void K3bCdCopyJob::cdrdaoWriteFinished()
{
  if( m_process->normalExit() ) {
      // TODO: check the process' exitStatus()
      switch( m_process->exitStatus() ) {
      case 0:
	m_finishedCopies++;

	if( m_dummy ) {
	  emit infoMessage( i18n("CD Copy simulation successfully completed"), K3bJob::STATUS );
	  finishAll();
	}
	else {
	  emit infoMessage( i18n("CD Copy %1 successfully completed").arg(m_finishedCopies), K3bJob::STATUS );
	  
	  if( m_finishedCopies < m_copies )
	    cdrdaoWrite();
	  else
	    finishAll();
	}
	break;
	
      default:
	// no recording device and also other errors!! :-(
	emit infoMessage( i18n("Cdrdao returned some error!"), K3bJob::ERROR );
	emit infoMessage( i18n("Sorry, no error handling yet!") + " :-((", K3bJob::ERROR );
	emit infoMessage( i18n("Please send me a mail with the last output..."), K3bJob::ERROR );
	
	cancelAll();
      }
  }
  else {
    emit infoMessage( i18n("Cdrdao did not exit cleanly!"), K3bJob::ERROR );
    cancelAll();
  }
}


void K3bCdCopyJob::addCdrdaoWriteArguments()
{
  // do not wait 10 seconds before starting
  *m_process << "-n";
  
  // eject cd after finished?
  if( k3bMain()->eject() )
    *m_process << "--eject";
  
  // writer device
  *m_process << "--device" << m_writer->busTargetLun();
  
  if( m_writer->cdrdaoDriver() != "auto" ) {
    *m_process << "--driver";
    if( m_writer->cdTextCapable() == 1 )
      *m_process << QString("%1:0x00000010").arg( m_writer->cdrdaoDriver() );
    else
      *m_process << m_writer->cdrdaoDriver();
  }

  // writing speed
  *m_process << "--speed" << QString::number( m_speed );

  // simulate?
  if( m_dummy )
    *m_process << "--simulate";
  
  // additional parameters from config
  QStringList _params = kapp->config()->readListEntry( "cdrdao parameters" );
  for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
    *m_process << *it;

  // manual buffer size
  k3bMain()->config()->setGroup( "General Options" );
  bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_process << "--buffer" << QString::number( k3bMain()->config()->readNumEntry( "Cdrdao buffer", 32 ) );
  }

  // add toc-file
  m_tocFile = locateLocal( "appdata", "temp/k3btemptoc.toc");
  *m_process << QFile::encodeName(m_tocFile);
}



void K3bCdCopyJob::parseCdrdaoStdout( KProcess*, char* data, int len )
{
  QStringList bufferList = QStringList::split( "\n", QString::fromLatin1( data, len ) );

  for( QStringList::const_iterator it = bufferList.begin(); 
       it != bufferList.end(); ++it ) {
    const QString& str = *it;

    emit debuggingOutput( "cdrdao", str );

    if( str.startsWith( "Copying" ) ) {
      // information about what will be read
      emit infoMessage( str, K3bJob::PROCESS );
    }
    else if( str.startsWith( "Track" ) ) {
      int end = str.find("...");
      bool ok;
      int trNum = str.mid( 6, end-6 ).toInt( &ok );
      if( ok ) {
	emit infoMessage( i18n("Reading track %1").arg(trNum), K3bJob::PROCESS );
      }
      emit infoMessage( str, K3bJob::PROCESS );
    }
    else if( str.startsWith( "Found ISRC" ) ) {
      emit infoMessage( i18n("Found ISRC code"), K3bJob::PROCESS );
    }
    else if( str.startsWith( "Found pre-gap" ) ) {
      emit infoMessage( i18n("Found pregap: %1").arg( str.mid(str.find(":")+1) ), K3bJob::PROCESS );
    }
    else {
      // check if progress
      bool ok;
      int min = str.left(2).toInt(&ok);
      if( ok ) {
	int sec = str.mid(3,2).toInt(&ok);
	if( ok ) {
	  // create progress info
	  sec += min*60;

	  emit subPercent( 100*sec*75 / m_blocksToCopy );
	  emit percent( 100*sec*75 / m_blocksToCopy / (m_copies+1) );
	}
      }

      if( !ok ) {
	parseCdrdaoStdoutLine( str );
      }
    }

  }
}


void K3bCdCopyJob::createCdrdaoProgress( int made, int size )
{
  int x = m_copies;
  if( !m_onTheFly )
    x++;

  int y = m_finishedCopies;
  if( !m_onTheFly )
    y++;

  emit percent( 100*(y+(made/size))/x );
  emit subPercent( 100 * made / size );
}


void K3bCdCopyJob::startNewCdrdaoTrack()
{

}


void K3bCdCopyJob::parseCdrdaoStderr( KProcess*, char* data, int len )
{
  emit debuggingOutput( "cdrdao", QString::fromLatin1(data, len) );
}


void K3bCdCopyJob::finishAll()
{
  // remove toc-file
  if( QFile::exists( m_tocFile ) )
    QFile::remove( m_tocFile );

  if( !m_keepImage && !m_onTheFly ) {
    QFile::remove( m_tempPath );
    emit infoMessage( i18n("Removed image files"), K3bJob::STATUS );
  }


  emit finished( true );
}


void K3bCdCopyJob::cancelAll()
{
  if( m_process->isRunning() ) {
    m_process->kill();

    // we need to unlock the writer because cdrdao/cdrecord locked it while writing
    if( !m_reader->block( false ) )
      emit infoMessage( i18n("Could not unlock cd reader."), K3bJob::ERROR );
    if( !m_writer->block( false ) )
      emit infoMessage( i18n("Could not unlock cd writer."), K3bJob::ERROR );
  }

  // remove toc-file
  if( QFile::exists( m_tocFile ) )
    QFile::remove( m_tocFile );

  if( !m_onTheFly ) {
    QFile::remove( m_tempPath );
    emit infoMessage( i18n("Removed image files"), K3bJob::STATUS );
  }

  emit finished( false );
}


#include "k3bcdcopyjob.moc"
