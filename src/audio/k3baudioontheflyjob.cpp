/***************************************************************************
                          k3baudiojob.cpp  -  description
                             -------------------
    begin                : Thu May 3 2001
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

#include "k3baudioontheflyjob.h"

#include "../k3b.h"
#include "../k3bglobals.h"
#include "../k3bdoc.h"
#include "../device/k3bdevice.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "input/k3baudiomodule.h"


#include <kprocess.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstddirs.h>

#include <qstring.h>
#include <qtimer.h>
#include <qdatetime.h>

#include <iostream>
#include <cmath>



K3bAudioOnTheFlyJob::K3bAudioOnTheFlyJob( K3bAudioDoc* doc )
  : K3bBurnJob( )
{
  m_doc = doc;
	
  m_iDocSize = doc->size();
  m_iTracksAlreadyWrittenSize = 0;
  m_currentModuleDataLength = 0;
  m_currentWrittenData = 0;
  m_streamingStarted = false;
  
  m_overallSize = m_doc->length();
  m_alreadyWritten = 0;

  m_streamingTimer = new QTimer( this );
  connect( m_streamingTimer, SIGNAL(timeout()), this, SLOT(slotTryWritingToProcess()) );

  //  m_ringBuffer = 0;


  // testing
  //-----------------------
  m_testFile = new QFile( "/home/trueg/download/test_k3b_job.cd" );
  m_testFile->open( IO_WriteOnly );
  //-----------------------

}


K3bAudioOnTheFlyJob::~K3bAudioOnTheFlyJob()
{
}


K3bDoc* K3bAudioOnTheFlyJob::doc() const
{
  return m_doc;
}


void K3bAudioOnTheFlyJob::slotParseCdrdaoOutput( KProcess*, char* output, int len )
{
  QString buffer = QString::fromLatin1( output, len );
	
  // split to lines
  QStringList lines = QStringList::split( "\n", buffer );
	
  // do every line
  for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
    {
      *str = (*str).stripWhiteSpace();

      bool _debug = false;

      // find some messages from cdrdao
      // -----------------------------------------------------------------------------------------
      if( (*str).startsWith( "Warning" ) || (*str).startsWith( "ERROR" ) ) {
	// TODO: parse the error messages!!
	emit infoMessage( *str );
      }
      else if( (*str).startsWith( "Executing power" ) ) {
	//emit infoMessage( i18n( *str ) );
	emit newSubTask( i18n(*str) );
      }
      else if( (*str).startsWith( "Power calibration successful" ) ) {
	emit infoMessage( i18n("Power calibration successful") );
	emit newSubTask( i18n("Preparing burn process...") );
      }
      else if( (*str).startsWith( "Flushing cache" ) ) {
	emit newSubTask( i18n("Flushing cache") );
      }
      else if( (*str).startsWith( "Writing CD-TEXT lead" ) ) {
	emit newSubTask( i18n("Writing CD-Text leadin...") );
      }
      else if( (*str).startsWith( "Turning BURN-Proof on" ) ) {
	emit infoMessage( i18n("Turning BURN-Proof on") );
      }

      else
	_debug = true;
      // -----------------------------------------------------------------------------------------


      // check if cdrdao starts a new track
      // -----------------------------------------------------------------------------------------
      if( (*str).contains( "Writing track" ) ) {
	// a new track has been started
	emit newTrack();
	if(!firstTrack) {
	  m_iNumTracksAlreadyWritten++;
	  m_iTracksAlreadyWrittenSize += m_doc->at(m_iNumTracksAlreadyWritten - 1)->size();
	}
	else
	  firstTrack = false;
			
	emit newSubTask( i18n("Writing track %1: '%2'").arg(m_iNumTracksAlreadyWritten + 1).arg(m_doc->at(m_iNumTracksAlreadyWritten)->fileName()) );
	emit infoMessage( *str );
	
	_debug = false;
      }
      // -----------------------------------------------------------------------------------------


      // parse the progress
      // -----------------------------------------------------------------------------------------
      // here "contains" has to be used since cdrdao sometimes "forgets" to do a newline!!
      if( (*str).contains( "Wrote " ) ) {
	// percentage
	int made, size, fifo;
	bool ok;
			
	// --- parse already written mb ------
	int pos1 = 6;
	int pos2 = (*str).find("of");
			
	if( pos2 == -1 )
	  return; // there is one line at the end of the writing process that has no 'of'
			
	made = (*str).mid( 6, pos2-pos1-1 ).toInt( &ok );
	if( !ok )
	  qDebug( "Parsing did not work for: " + (*str).mid( 6, pos2-pos1-1 ) );
			
	// ---- parse size ---------------------------
	pos1 = pos2 + 2;
	pos2 = (*str).find("MB");
	size = (*str).mid( pos1, pos2-pos1-1 ).toInt(&ok);
	if( !ok )
	  qDebug( "Parsing did not work for: " + (*str).mid( pos1, pos2-pos1-1 ) );
				
	// ----- parsing fifo ---------------------------
	pos1 = (*str).findRev(' ');
	pos2 =(*str).findRev('%');
	fifo = (*str).mid( pos1, pos2-pos1 ).toInt(&ok);
	if( !ok )
	  qDebug( "Parsing did not work for: " + (*str).mid( pos1, pos2-pos1 ) );
			
	emit bufferStatus( fifo );
			
	double _f = (double)size / (double)m_iDocSize;
	// calculate track progress
	int _trackMade = (int)( (double)made -_f*(double)m_iTracksAlreadyWrittenSize );
	int _trackSize = (int)( _f * (double)m_doc->at(m_iNumTracksAlreadyWritten)->size() );
	emit processedSubSize( _trackMade, _trackSize );
	emit subPercent( 100*_trackMade / _trackSize );
	emit processedSize( made, size );
	emit percent( 100*made / size );

	_debug = false;
      }
      // -----------------------------------------------------------------------------------------
      
      if( _debug ) {
	// debugging
	qDebug(*str);
      }
    }
}


void K3bAudioOnTheFlyJob::cancel()
{
  if( error() == K3b::WORKING ) {
    emit infoMessage("Writing canceled.");


    // cancel the module!!!
    m_currentProcessedTrack->module()->cancel();

    m_process.kill();
	
    // remove toc-file
    if( QFile::exists( m_tocFile ) ) {
      qDebug("(K3bAudioOnTheFlyJob) Removing temporary TOC-file");
      QFile::remove( m_tocFile );
      m_tocFile = QString::null;
    }
				
    m_error = K3b::CANCELED;
    emit finished( this );
  }
}


void K3bAudioOnTheFlyJob::start()
{
  m_iNumTracksAlreadyWritten = 0;
	
  emit started();
	
  m_error = K3b::WORKING;

  emit newTask( i18n("Writing on the fly") );
  emit infoMessage( i18n("Warning: Burning on-the-fly could cause buffer-underruns!") );
  emit newSubTask( i18n("Preparing write process...") );

  m_iDocSize = m_doc->size();
	
  m_process.clearArguments();
  m_process.disconnect();
	
  firstTrack = true;
	
  // write in dao-mode
  if( !m_doc->dao() ) {
    emit infoMessage( i18n("On-the-fly writing is only supported in DAO-mode.") );
    emit infoMessage( i18n( "Swiching to DAO-mode.") );
    m_doc->setDao( true );
  }
		
  // use cdrdao to burn the cd
  emit infoMessage( i18n("Writing TOC-file") );
  m_tocFile = m_doc->writeTOC( locateLocal( "appdata", "temp/" ) + "k3b_" + QTime::currentTime().toString() + ".toc" );

  if( m_tocFile.isEmpty() ) {
    emit infoMessage( i18n("Could not write TOC-file %1").arg( m_tocFile ) );
    m_error = K3b::IO_ERROR;
    emit finished( this );
  }
  else {
    // initialize ring buffer with a minimal size of 4 000 000 bytes
    // =======================================================================
//     kapp->config()->setGroup("Audio Settings");
//     m_bufferSize = kapp->config()->readNumEntry( "Buffer Size", 4000000 );
//     if( m_bufferSize < 4000000 )
//       m_bufferSize = 4000000;

//     delete ringBuffer;
//     m_ringBuffer = new char[m_bufferSize];
//     m_bufferReader = m_bufferWriter = 0;
    // =======================================================================

    // ----------------
    // now we need to fill the buffer and then start the writing from the buffer into the cdrdao-process
    // ----------------




    kapp->config()->setGroup("External Programs");
    m_process.clearArguments();
    m_process << kapp->config()->readEntry( "cdrdao path" );
    m_process << "write";

    // device
    m_process << "--device" << m_doc->burner()->devicename();
			
    // additional parameters from config
    QStringList _params = kapp->config()->readListEntry( "cdrdao parameters" );
    for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
      m_process << *it;
    
    if( m_doc->dummy() )
      m_process << "--simulate";
    if( k3bMain()->eject() )
      m_process << "--eject";
    
    // writing speed
    m_process << "--speed" << QString::number(  m_doc->speed() );
    
    // supress the 10 seconds gap to the writing
    m_process << "-n";
    
    // toc-file
    m_process << m_tocFile;
    
    // debugging output
//     QStrList* _args = m_process.args();
//     QStrListIterator _it(*_args);
//     while( _it ) {
//       cout << *_it << " ";
//       ++_it;
//     }
//     cout << endl << flush;
    
    // connect to the cdrdao slots
    connect( &m_process, SIGNAL(processExited(KProcess*)),
	     this, SLOT(slotCdrdaoFinished()) );
    connect( &m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	     this, SLOT(slotParseCdrdaoOutput(KProcess*, char*, int)) );
    connect( &m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotParseCdrdaoOutput(KProcess*, char*, int)) );
    connect( &m_process, SIGNAL(wroteStdin(KProcess*)), 
	     this, SLOT(slotWroteData()) );

    // first try to start the audiomodule
    m_currentProcessedTrack = m_doc->at(0);
    m_currentProcessedTrack->module()->disconnect( this );

    connect( m_currentProcessedTrack->module(), SIGNAL(output(int)), 
	     this, SLOT(slotModuleOutput(int)) );
    connect( m_currentProcessedTrack->module(), SIGNAL(finished(bool)),
	     this, SLOT(slotModuleFinished(bool)) );

    if( m_currentProcessedTrack->module()->getStream() ) {
      if( !m_process.start( KProcess::NotifyOnExit, KProcess::All ) )
	{
	  // something went wrong when starting the program
	  // it "should" be the executable
	  qDebug("(K3bAudioOnTheFlyJob) could not start cdrdao");
	  m_error = K3b::CDRDAO_ERROR;
	  
	  // remove toc-file
	  QFile::remove( m_tocFile );
	  m_tocFile = QString::null;
	  
	  emit infoMessage( i18n("Could not start cdrdao!") );
	  emit finished( this );
	}
      else
	{
	  qDebug( "(K3bAudioOnTheFlyJob) process started!" );

	  m_error = K3b::WORKING;
	  if( m_doc->dummy() )
	    emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_doc->speed()) );
	  else
	    emit infoMessage( i18n("Starting recording at %1x speed...").arg(m_doc->speed()) );

	  m_currentModuleDataLength = 0;
	  m_streamingStarted = false;

	  m_overallSize = m_doc->length();
	  m_alreadyWritten = 0;
	}
    }
  }
}


void K3bAudioOnTheFlyJob::slotModuleOutput( int len )
{
  m_currentModuleDataLength = len;

  // when we start the process it will not emit it's wroteStdin signal
  // so we need to start streaming manually
  if( !m_streamingStarted )
    m_streamingTimer->start(0);
}


void K3bAudioOnTheFlyJob::slotWroteData()
{
  m_streamingTimer->start(0);
}


void K3bAudioOnTheFlyJob::slotTryWritingToProcess()
{
  if( m_currentModuleDataLength > 0 && m_process.isRunning() ) {
    delete m_currentWrittenData;
    m_currentWrittenData = new char[m_currentModuleDataLength];

    int dataLength = m_currentProcessedTrack->module()->readData( m_currentWrittenData, m_currentModuleDataLength );

    m_process.writeStdin( m_currentWrittenData, dataLength );
    
    m_currentModuleDataLength = 0;
    m_streamingStarted = true;

    m_streamingTimer->stop();


    // testing
    //----------------------
    m_testFile->writeBlock( m_currentWrittenData, dataLength );
    m_testFile->flush();
    //----------------------
  }
} 

void K3bAudioOnTheFlyJob::slotModuleFinished( bool success )
{
  if( success ) {
    // start the next module
    m_currentProcessedTrack->module()->disconnect( this );
    
    m_currentProcessedTrack = m_doc->next();
    if( m_currentProcessedTrack != 0 ) {
      m_currentProcessedTrack->module()->disconnect( this );
      
      connect( m_currentProcessedTrack->module(), SIGNAL(output(int)), 
	       this, SLOT(slotModuleOutput(int)) );
      connect( m_currentProcessedTrack->module(), SIGNAL(finished(bool)),
	       this, SLOT(slotModuleFinished(bool)) );
      
      if( m_currentProcessedTrack->module()->getStream() ) {
	m_currentModuleDataLength = 0;
	qDebug( "streaming track : " + m_currentProcessedTrack->fileName() );
      }
      else {
	// error
	qDebug( "Error" );
	m_process.closeStdin();
      }
    }
    else {
      emit infoMessage( i18n("All tracks streamed") );
      qDebug("(K3bAudioOnTheFlyJob) closing stdin." );
      m_process.closeStdin();
    }
  }
  else {
    emit infoMessage( i18n("Error while streaming file") );
    m_process.closeStdin();
  }
}


void K3bAudioOnTheFlyJob::slotCdrdaoFinished()
{
  if( m_process.normalExit() )
    {
      // TODO: check the process' exitStatus()
      switch( m_process.exitStatus() )
	{
	case 0:
	  m_error = K3b::SUCCESS;
	  emit infoMessage( "Burning successfully finished" );
	  break;
				
	default:
	  // no recording device and also other errors!! :-(
	  emit infoMessage( "Cdrdao returned some error!" );
	  emit infoMessage( "Sorry, no error handling yet! :-((" );
	  emit infoMessage( "Please send me a mail with the last output..." );
	  m_error = K3b::CDRDAO_ERROR;
	  break;
	}
    }
  else
    {
      m_error = K3b::CDRDAO_ERROR;
      emit infoMessage( "cdrdao did not exit cleanly!" );
    }

  // remove toc-file
  qDebug("(K3bAudioOnTheFlyJob) Removing temporary TOC-file");
  QFile::remove( m_tocFile );
  m_tocFile = QString::null;
	
  emit finished( this );

  m_process.disconnect();
}


void K3bAudioOnTheFlyJob::slotEmitProgress( int trackMade, int trackSize )
{
  double trackPercent;
  if( trackSize > 0 )
    trackPercent = 100.0 * (double)trackMade/(double)trackSize;
  else
    trackPercent = 0;


  int alreadyWritten = m_alreadyWritten + (int)((double)m_currentProcessedTrack->length() * trackPercent);

  int overallPercent = 100 * alreadyWritten / m_overallSize;
	
  emit processedSubSize( trackMade, trackSize );
  emit subPercent( (int)trackPercent  );
  emit percent( overallPercent );
}
