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

#include "k3baudiojob.h"

#include "../k3b.h"
#include "../tools/k3bglobals.h"
#include "../k3bdoc.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "input/k3baudiomodule.h"
#include "../device/k3bdevice.h"
#include "../device/k3bemptydiscwaiter.h"
#include "../tools/k3bexternalbinmanager.h"

#include <kprocess.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <qstring.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qtimer.h>
#include <kdebug.h>

#include <iostream>
#include <cmath>




K3bAudioJob::K3bAudioJob( K3bAudioDoc* doc )
  : K3bBurnJob( )
{
  m_doc = doc;
  m_process = new KShellProcess();
	
  m_bytesFinishedTracks = 0;
  m_working = false;
}


K3bAudioJob::~K3bAudioJob()
{
  delete m_process;
}


K3bDoc* K3bAudioJob::doc() const
{
  return m_doc;
}


K3bDevice* K3bAudioJob::writer() const
{
  return doc()->burner();
}


// TODO: move all parsing except progress to K3bBurnJob
void K3bAudioJob::slotParseCdrecordOutput( KProcess*, char* output, int len )
{
  QString buffer = QString::fromLatin1( output, len );
	

  // split to lines
  QStringList lines = QStringList::split( "\n", buffer );
	
  // do every line
  for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
    {
      *str = (*str).stripWhiteSpace();

      emit debuggingOutput( "cdrecord", *str );

      if( (*str).startsWith( "Track" ) )
	{
	  if( (*str).contains( "fifo", false ) > 0 )
	    {
	      // parse progress
	      int num, made, size, fifo;
	      bool ok;

	      // --- parse number of track ---------------------------------------				
	      // ----------------------------------------------------------------------
	      int pos1 = 5;
	      int pos2 = (*str).find(':');
	      if( pos1 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      // now pos2 to the first colon :-)
	      num = (*str).mid(pos1,pos2-pos1).toInt(&ok);				
	      if(!ok)
		kdDebug() << "parsing did not work" << endl;
				
	      // --- parse already written Megs -----------------------------------				
	      // ----------------------------------------------------------------------
	      pos1 = (*str).find(':');
	      if( pos1 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      pos2 = (*str).find("of");
	      if( pos2 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      // now pos1 point to the colon and pos2 to the 'o' of 'of' :-)
	      pos1++;
	      made = (*str).mid(pos1,pos2-pos1).toInt(&ok);
	      if(!ok)
		kdDebug() << "parsing did not work" << endl;
					
	      // --- parse total size of track ---------------------------------------
	      // ------------------------------------------------------------------------
	      pos1 = (*str).find("MB");
	      if( pos1 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      // now pos1 point to the 'M' of 'MB' and pos2 to the 'o' of 'of' :-)
	      pos2 += 2;
	      size = (*str).mid(pos2,pos1-pos2).toInt(&ok);
	      if(!ok)
		kdDebug() << "parsing did not work" << endl;
				
	      // --- parse status of fifo --------------------------------------------
	      // ------------------------------------------------------------------------
	      pos1 = (*str).find("fifo");
	      if( pos1 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      pos2 = (*str).find('%');
	      if( pos2 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      // now pos1 point to the 'f' of 'fifo' and pos2 to the %o'  :-)
	      pos1+=4;
	      fifo = (*str).mid(pos1,pos2-pos1).toInt(&ok);
	      if(!ok)
		kdDebug() << "parsing did not work" << endl;

	      // -------------------------------------------------------------------
	      // -------- parsing finished --------------------------------------

	      emit subPercent( 100 * made / size );
	      emit processedSubSize( made, size );

	      emit processedSize( m_bytesFinishedTracks/1024/1024 + made, m_doc->size()/1024/1024 );

	      double relOverallWritten = ( (double)m_bytesFinishedTracks + (double)(made*1024*1024) ) / (double)m_doc->size();

	      if( !m_onTheFly ) {
		// decoding is part of the overall progress
		emit percent( m_decodingPercentage + (( 100 - m_decodingPercentage ) * relOverallWritten ));
	      }
	      else
		emit percent( (int)(100.0 * relOverallWritten)  );

	      emit bufferStatus( fifo );
	    }
	}
      else if( (*str).startsWith( "Fixating" ) ) {
	emit newSubTask( i18n("Fixating") );
      }
      else if( (*str).contains("seconds.") ) {
	emit infoMessage( "in " + (*str).mid( (*str).find("seconds") - 2 ), K3bJob::PROCESS );
      }
      else if( (*str).startsWith( "Writing pregap" ) ) {
	emit newSubTask( i18n("Writing pregap") );
      }
      else if( (*str).startsWith( "Performing OPC" ) ) {
	emit infoMessage( i18n("Performing OPC"), K3bJob::PROCESS );
      }
      else if( (*str).startsWith( "Sending" ) ) {
	emit infoMessage( i18n("Sending CUE sheet"), K3bJob::PROCESS );
      }
      else if( (*str).contains( "Turning BURN-Proof" ) ) {
	emit infoMessage( i18n("Enabled BURN-Proof"), K3bJob::PROCESS );
      }
      else if( (*str).startsWith( "Starting new" ) )
	{
	  if(!firstTrack) {
	    m_bytesFinishedTracks += m_doc->at(m_currentWrittenTrackNumber)->size();
	    m_currentWrittenTrackNumber++;
	  }
	  else
	    firstTrack = false;

	  m_currentWrittenTrack = m_doc->at( m_currentWrittenTrackNumber );
	  emit newSubTask( i18n("Writing track %1: '%2'").arg(m_currentWrittenTrackNumber + 1).arg(m_currentWrittenTrack->fileName()) );
	}
      else {
	// debugging
	kdDebug() << "(cdrecord) " << (*str) << endl;
      }
    } // for every line
}


void K3bAudioJob::createCdrdaoProgress( int made, int size )
{
  if( size == 0 ) {
    kdDebug() << "(K3bAudioJob) got progress: " << made << ", " << size << endl;
    return;
  }

  double f = (double)size / (double)m_doc->size();
  // calculate track progress
  int trackMade = (int)( (double)made -f*(double)m_bytesFinishedTracks );
  int trackSize = (int)( f * (double)m_currentWrittenTrack->size() );
  emit processedSubSize( trackMade, trackSize );
  if( trackSize > 0 )
    emit subPercent( 100*trackMade / trackSize );
  else
    kdDebug() << "(K3bAudioJob) got trackSize " << trackSize << endl;
  
  emit processedSize( made, size );
  if( !m_onTheFly ) {
    // decoding is part of the overall progress
    emit percent( m_decodingPercentage + ((100 - m_decodingPercentage) * made / size) );
  }
  else
    emit percent( 100*made / size );
}


void K3bAudioJob::startNewCdrdaoTrack()
{
  if(!firstTrack) {
    m_bytesFinishedTracks += m_doc->at(m_currentWrittenTrackNumber)->size();
    m_currentWrittenTrackNumber++;
  }
  else
    firstTrack = false;
  
  m_currentWrittenTrack = m_doc->at( m_currentWrittenTrackNumber );			
  emit newSubTask( i18n("Writing track %1: '%2'").arg(m_currentWrittenTrackNumber + 1).arg(m_currentWrittenTrack->fileName()) );
}


void K3bAudioJob::cancel()
{
  if( m_working ) {
    cancelAll();

    emit infoMessage( i18n("Writing canceled by user."), K3bJob::ERROR );
    emit finished( false );
  }
  else {
    kdDebug() << "(K3bAudioJob) canceled without starting job." << endl;
  }
}


void K3bAudioJob::cancelAll()
{
  if( m_process->isRunning() ) {
    m_process->disconnect(this);
    m_process->kill();

    // we need to unlock the writer because cdrdao/cdrecord locked it while writing
    bool block = m_doc->burner()->block( false );
    if( !block )
      emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
    else
      m_doc->burner()->eject();
  }

  // remove toc-file
  if( QFile::exists( m_tocFile ) ) {
     kdDebug() << "(K3bAudioOnTheFlyJob) Removing temporary TOC-file" << endl;
     QFile::remove( m_tocFile );
  }
  m_tocFile = QString::null;

  if( m_currentDecodedTrack )
    if( m_currentDecodedTrack->module() ) {
      m_currentDecodedTrack->module()->disconnect(this);
      m_currentDecodedTrack->module()->cancel();
    }

  m_waveFileWriter.close();
  
  clearBufferFiles();

  m_working = false;
}


void K3bAudioJob::start()
{
  m_dataToDecode = 0;
  m_decodedData = 0;
  m_bytesFinishedTracks = 0;
  m_bLengthInfoEmited = false;
  m_currentWrittenTrackNumber = 0;
  m_currentWrittenTrack = m_doc->at(0);
  m_currentDecodedTrack = m_currentWrittenTrack;
  m_currentDecodedTrackNumber = 0;
  m_working = true;

  emit started();

  m_processWroteStdin = true;

  QTimer::singleShot( 0, this, SLOT(slotTryStart()) );
}


void K3bAudioJob::slotTryStart()
{
  // check if we have all length calculated
  QListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    if( it.current()->length() == 0 ) {
      kdDebug() << "(K3bAudioJob) not all length ready." << endl;
      if( !m_bLengthInfoEmited ) {
	emit infoMessage( i18n("Waiting for all tracks' length to be calculated."), STATUS );
	m_bLengthInfoEmited = true;
      }
      QTimer::singleShot( 1000, this, SLOT(slotTryStart()) );
      return;
    }
  }
  
  if( m_bLengthInfoEmited ) {
    emit infoMessage( i18n("All tracks' length ready."), STATUS );
  }
  
  // calculate length of overall data to decode
  for( it.toFirst(); it.current(); ++it ) {
    if( !it.current()->isWave() ) {
      m_dataToDecode += it.current()->size();
    }
  }
  m_decodingPercentage = (int)( 50.0 * (double)m_dataToDecode / (double)m_doc->size() );

  kdDebug() << "(K3bAudioJob) data to decode: " << m_dataToDecode << "i" << endl;

  // now decide what to do
  // what program to use (cdrdao or cdrecord)
  // writing on the fly or not
  // -----------------------------------------
  if( m_doc->writingApp() == K3b::DEFAULT ) {
    // choose the better one
    // which is always cdrdao except if we write in TAO mode
    if( !m_doc->dao() )
      m_writingApp = K3b::CDRECORD;
    else
      m_writingApp = K3b::CDRDAO;
  }
  else
    m_writingApp = m_doc->writingApp();


  if( m_writingApp == K3b::CDRECORD ) {
    if( m_doc->onTheFly() ) {
      emit infoMessage( i18n("On the fly writing not supported with cdrecord."), ERROR );
      m_doc->setOnTheFly( false );
    }
    if( m_doc->hideFirstTrack() ) {
      emit infoMessage( i18n("Hiding first track not supported by cdrecord"), ERROR );
    }
    if( m_doc->cdText() ) {
      emit infoMessage( i18n("CD-TEXT not supported by cdrecord."), ERROR );
    }
  }

  m_onTheFly = m_doc->onTheFly();

  if( m_onTheFly ) {
    startWriting();
  }
  else {
    emit infoMessage( i18n("Buffering files"), K3bJob::STATUS );
    emit newTask( i18n("Buffering files") );
  }

  // decoding needs to be started in both cases
  QTimer::singleShot(0, this, SLOT(slotDecodeNextFile()) );
}


void K3bAudioJob::startWriting()
{
  K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );

  if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
    cancel();
    return;
  }

  emit newTask( i18n("Writing") );
  emit newSubTask( i18n("Preparing write process...") );
  
  if( m_process->isRunning() )
    m_process->kill();
  
  m_process->clearArguments();
  m_process->disconnect(this);

  if( m_writingApp == K3b::CDRDAO )
    cdrdaoWrite();
  else
    cdrecordWrite();
}


void K3bAudioJob::slotDecodeNextFile()
{
  if( m_onTheFly ) {
    // wait for the process to write to stdin
    if( !m_processWroteStdin ) {
      QTimer::singleShot(0, this, SLOT(slotDecodeNextFile()) );
      return;
    }
  }

  // find next file to decode
  while( m_currentDecodedTrack && 
	 m_currentDecodedTrack->isWave() ) {
    m_currentDecodedTrackNumber++;
    m_currentDecodedTrack = m_doc->at(m_currentDecodedTrackNumber);
  }

  if( m_currentDecodedTrack ) {
    K3bAudioModule* module = m_currentDecodedTrack->module();
    if( module == 0 ) {
      kdDebug() << "(K3bAudioModule) track no. " << m_currentDecodedTrackNumber << " is no wave file and has no module." << endl;
      emit infoMessage( i18n("Internal error: No module. Please report!"), K3bJob::ERROR );

      cancelAll();
      emit finished( false );
      return;
    }
    else {
      m_currentModuleDataLength = 0;
      connect( module, SIGNAL(finished(bool)), this, SLOT(slotModuleFinished(bool)) );
      connect( module, SIGNAL(output(const unsigned char*, int)), this, SLOT(slotModuleOutput(const unsigned char*, int)) );

      if( m_onTheFly ) {
	module->setConsumer( m_process, SIGNAL(wroteStdin(KProcess*)) );
	connect( m_process, SIGNAL(wroteStdin(KProcess*)), this, SLOT(slotProcessWroteStdin()) );
	kdDebug() << "(K3bAudioJob) streaming track " << m_currentDecodedTrackNumber << endl;
      }
      else {
	module->setConsumer( 0 );
	connect( module, SIGNAL(percent(int)), this, SLOT(slotModuleProgress(int)) );

	emit newSubTask( i18n("Buffering file %1").arg(m_currentDecodedTrack->fileName()) );

	KURL bufferFile = k3bMain()->findTempFile( "wav", doc()->tempDir() );

	// start the K3bWaveFileWriter
	if( !m_waveFileWriter.open( bufferFile.path() ) ) {
	  kdDebug() << "(K3bAudioModule) Could not open file " << bufferFile.path() << endl;
	  emit infoMessage( i18n("Could not open buffer file %1").arg(bufferFile.path()), K3bJob::ERROR );
	  
	  cancelAll();
	  emit finished( false );
	  return;
	}
      }

      module->start();
    }
  }
  else {

    kdDebug() << "(K3bAudioJob) decoded " << m_decodedData << "i bytes via K3bAudioModules:" << endl;

    if( m_onTheFly ) {
      // everything streamed. 
      kdDebug() << "(K3bAudioJob) streaming finished." << endl;
      m_process->closeStdin();
    }
    else {
      // all tracks buffered
      kdDebug() << "(K3bAudioJob) all tracks buffered." << endl;
      startWriting();
    }
  }
}


void K3bAudioJob::slotModuleOutput( const unsigned char* data, int len )
{
  if( m_onTheFly ) {
    // if we receive this signal from the module the process must have signaled "wroteStdin" before
    // otherwise this will fail

    m_currentModuleDataLength += len;  //only for debugging
    m_process->writeStdin( (const char*)data, len );

    m_processWroteStdin = false;
  }
  else {
    m_waveFileWriter.write( (const char*)data, len );
  }

  m_decodedData += len;
}


void K3bAudioJob::slotModuleFinished( bool success )
{
  m_currentDecodedTrack->module()->disconnect(this);

  if( m_onTheFly ) {
    if( success ) {
      // check if the data fits the track's length
      if( m_currentModuleDataLength != m_currentDecodedTrack->size() ) {
	kdDebug() << "(K3bAudioOnTheFlyJob) track size: " << m_currentDecodedTrack->size() << "i and module output: " << m_currentModuleDataLength << "i" << endl;
      }
      
      kdDebug() << "(K3bAudioOnTheFlyJob) finished streaming track " << m_currentDecodedTrackNumber << "." << endl;
    }
    else {
      emit infoMessage( i18n("Error while streaming file"), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
      return;
    }

  }
  else {
    // close the written file which is open even if the module did not succeed
    QString bufferFilename = m_waveFileWriter.filename();
    m_waveFileWriter.close();
  
    if( success ) {
      m_currentDecodedTrack->setBufferFile( bufferFilename );

      kdDebug() << "(K3bAudioJob) Successfully buffered track " << m_currentDecodedTrack->fileName() << endl;
      emit infoMessage( i18n("Written buffer file for %1 to %2").arg(m_currentDecodedTrack->fileName()).arg(bufferFilename), STATUS );
    }
    else {
      kdDebug() << "(K3bAudioJob) Could not buffer track " << m_currentDecodedTrack->fileName() << endl;
      emit infoMessage( i18n("Error while buffering track %1").arg( m_currentDecodedTrack->fileName() ), ERROR );

      cancelAll();
      emit finished( false );
      return;
    }
  }

  m_currentDecodedTrackNumber++;
  m_currentDecodedTrack = m_doc->at(m_currentDecodedTrackNumber);
  kdDebug() << "(K3bAudioJob) m_currentDecodedTrack = " << ((m_currentDecodedTrack ? "x" : "null")) << endl;

  QTimer::singleShot(0, this, SLOT(slotDecodeNextFile()) );
}


void K3bAudioJob::cdrdaoWrite()
{
  m_process->clearArguments();
    
  firstTrack = true;
    
  if( !k3bMain()->externalBinManager()->foundBin( "cdrdao" ) ) {
    kdDebug() << "(K3bAudioJob) could not find cdrdao executable" << endl;
    emit infoMessage( i18n("cdrdao executable not found."), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
    return;
  }

  // use cdrdao to burn the cd
  emit infoMessage( i18n("Writing TOC-file"), K3bJob::STATUS );
  m_tocFile = locateLocal( "appdata", "temp/k3btemptoc.toc");

  if( !m_doc->writeTOC( m_tocFile ) ) {
    kdDebug() << "(K3bAudioJob) Could not write TOC-file." << endl;
    emit infoMessage( i18n("Could not write correct TOC-file."), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
    return;
  }

  m_process->clearArguments();
  *m_process << k3bMain()->externalBinManager()->binPath( "cdrdao" );
  *m_process << "write";


  // device
  // TODO: check if device is in use and throw exception if so

  *m_process << "--device" << m_doc->burner()->busTargetLun();
  if( m_doc->burner()->cdrdaoDriver() != "auto" ) {
    *m_process << "--driver";
    if( m_doc->burner()->cdTextCapable() == 1 )
      *m_process << QString("%1:0x00000010").arg( m_doc->burner()->cdrdaoDriver() );
    else
      *m_process << m_doc->burner()->cdrdaoDriver();
  }
    
  // additional parameters from config
  QStringList _params = kapp->config()->readListEntry( "cdrdao parameters" );
  for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
    *m_process << *it;

  k3bMain()->config()->setGroup( "General Options" );
  bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_process << "--buffers" << QString::number( k3bMain()->config()->readNumEntry( "Cdrdao buffer", 32 ) );
  }
  bool overburn = k3bMain()->config()->readBoolEntry( "Allow overburning", false );

  if( overburn )
    *m_process << "--overburn";

  if( m_doc->dummy() )
    *m_process << "--simulate";
  if( k3bMain()->eject() )
    *m_process << "--eject";
    
  // writing speed
  *m_process << "--speed" << QString::number(  m_doc->speed() );
    
  // supress the 10 seconds gap to the writing
  *m_process << "-n";
    
  // toc-file
  *m_process << QString("\"%1\"").arg(QFile::encodeName(m_tocFile));
    
  // debugging output
//   QStrList* _args = m_process->args();
//   QStrListIterator _it(*_args);
//   while( _it ) {
//     cout << *_it << " ";
//     ++_it;
//   }
//   cout << endl << flush;
    
    
    
  // connect to the cdrdao slots
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotCdrdaoFinished()) );
  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(parseCdrdaoOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(parseCdrdaoOutput(KProcess*, char*, int)) );

    
  if( !m_process->start( KProcess::NotifyOnExit, KProcess::All ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bAudioJob) could not start cdrdao" << endl;

    emit infoMessage( i18n("Could not start cdrdao!"), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
    return;
  }

  kdDebug() << "(K3bAudioJob) process started!" << endl;
    
  if( m_doc->dummy() )
    emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_doc->speed()), K3bJob::STATUS );
  else
    emit infoMessage( i18n("Starting recording at %1x speed...").arg(m_doc->speed()), K3bJob::STATUS ); 
}


void K3bAudioJob::cdrecordWrite()
{
  // use cdrecord to burn the cd
  if( !k3bMain()->externalBinManager()->foundBin( "cdrecord" ) ) {
    kdDebug() << "(K3bAudioJob) could not find cdrecord executable" << endl;
    emit infoMessage( i18n("cdrecord executable not found."), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
    return;
  }

  // OK, we need a new cdrecord process...
  *m_process << k3bMain()->externalBinManager()->binPath( "cdrecord" );

  // and now we add the needed arguments...
  // display progress
  *m_process << "-v";

  k3bMain()->config()->setGroup( "General Options" );
  bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_process << QString("fs=%1m").arg( k3bMain()->config()->readNumEntry( "Cdrecord buffer", 4 ) );
  }
  bool overburn = k3bMain()->config()->readBoolEntry( "Allow overburning", false );

  if( overburn )
    *m_process << "-overburn";

  if( m_doc->dummy() )
    *m_process << "-dummy";
  if( m_doc->dao() )
    *m_process << "-dao";
  if( k3bMain()->eject() )
    *m_process << "-eject";

  // add speed
  QString s = QString("-speed=%1").arg( m_doc->speed() );
  *m_process << s;
    
  // add the device
  s = QString("dev=%1").arg( m_doc->burner()->genericDevice() );
  *m_process << s;
    
  if( m_doc->padding() )
    *m_process << "-pad";

  // additional parameters from config
  QStringList _params = kapp->config()->readListEntry( "cdrecord parameters" );
  for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
    *m_process << *it;

  *m_process << "-audio";

  // add all the tracks
  QListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it )
    {
      K3bAudioTrack* track = it.current();

      s = QString("-pregap=%1").arg( track->pregap() );
      *m_process << s;
      if( track->isWave() )
	*m_process << QString("\"%1\"").arg(QFile::encodeName(track->absPath()));
      else if( !track->bufferFile().isEmpty() )
	*m_process << QString("\"%1\"").arg(QFile::encodeName(track->bufferFile()));
//       else if( m_onTheFly ) {
// 	// TODO: implement me
//       }
      else {
	kdDebug() << "(K3bAudioJob) missing buffer file" << endl;
	emit infoMessage( i18n("Not all files have been buffered."), ERROR );
	
	cancelAll();
	emit finished( false );
	return;
      }
    }

	
  // debugging output
//   QStrList* _args = m_process->args();
//   QStrListIterator _it(*_args);
//   while( _it ) {
//     cout << *_it << " ";
//     ++_it;
//   }
//   cout << flush;
	
  // connect to the cdrecord slots
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotCdrecordFinished()) );
  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotParseCdrecordOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotParseCdrecordOutput(KProcess*, char*, int)) );
	
  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bAudioOnTheFlyJob) could not start cdrecord" << endl;
      
    emit infoMessage( i18n("Could not start cdrecord!"), K3bJob::ERROR );

    cancelAll();
    emit finished( false );
    return;
  }

  if( m_doc->dummy() )
    emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_doc->speed()), K3bJob::STATUS );
  else
    emit infoMessage( i18n("Starting recording at %1x speed...").arg(m_doc->speed()), K3bJob::STATUS );
}


void K3bAudioJob::slotCdrecordFinished()
{
  if( m_process->normalExit() )
    {
      // TODO: check the process' exitStatus()
      switch( m_process->exitStatus() )
	{
	case 0:
	  if( doc()->dummy() )
	    emit infoMessage( i18n("Simulation successfully completed"), K3bJob::STATUS );
	  else
	    emit infoMessage( i18n("Writing successfully completed"), K3bJob::STATUS );

	  emit finished( true );
	  break;
				
	default:
	  // no recording device and also other errors!! :-(
	  emit infoMessage( i18n("cdrecord returned an error!"), K3bJob::ERROR );
	  emit infoMessage( i18n("Error handling not implemented yet!"), K3bJob::ERROR );
	  emit infoMessage( i18n("Please send an email to the author with the last output..."), K3bJob::ERROR );

	  cancelAll();
	  emit finished( false );
	  return;
	}
    }
  else
    {
      emit infoMessage( i18n("cdrecord did not exit cleanly!"), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
      return;
    }

  clearBufferFiles();
  m_process->disconnect(this);
}


void K3bAudioJob::slotCdrdaoFinished()
{
  if( m_process->normalExit() )
    {
      // TODO: check the process' exitStatus()
      switch( m_process->exitStatus() )
	{
	case 0:
	  if( doc()->dummy() )
	    emit infoMessage( i18n("Simulation successfully completed"), K3bJob::STATUS );
	  else
	    emit infoMessage( i18n("Writing successfully completed"), K3bJob::STATUS );

	  emit finished( true );
	  break;
				
	default:
	  // no recording device and also other errors!! :-(
	  emit infoMessage( i18n("cdrdao returned an error!"), K3bJob::ERROR );
	  emit infoMessage( i18n("Error handling not implemented yet!"), K3bJob::ERROR );
	  emit infoMessage( i18n("Please send an email to the author with the last output..."), K3bJob::ERROR );

	  cancelAll();
	  emit finished( false );
	  return;
	}
    }
  else
    {
      emit infoMessage( i18n("cdrdao did not exit cleanly!"), K3bJob::ERROR );

      cancelAll();
      emit finished( false );
      return;
    }

  // remove toc-file
  if( QFile::exists( m_tocFile ) ) {
     kdDebug() << "(K3bAudioOnTheFlyJob) Removing temporary TOC-file" << endl;
     QFile::remove( m_tocFile );
  }
  m_tocFile = QString::null;
  
  clearBufferFiles();

  m_process->disconnect(this);
}


void K3bAudioJob::slotModuleProgress( int p )
{
  if( !m_onTheFly ) {
    double rel = (double)p / 100.0;
    double percentAbs = ( (double)m_currentDecodedTrack->size() * rel + (double)m_decodedData ) / (double)m_dataToDecode;
    emit percent( percentAbs * m_decodingPercentage );
    emit subPercent( p );
  }
}


void K3bAudioJob::clearBufferFiles()
{
  if( !m_onTheFly && m_doc->removeBufferFiles() ) {
    
    emit infoMessage( i18n("Removing temporary files!"), K3bJob::STATUS );
    
    for( QListIterator<K3bAudioTrack> it(*m_doc->tracks()); it.current(); ++it ) {
      K3bAudioTrack* track = it.current();
      if( QFile::exists( track->bufferFile() ) ) {
	emit infoMessage( i18n("Removing file %1").arg( track->bufferFile() ), STATUS );
	QFile::remove( track->bufferFile() );
	track->setBufferFile( QString::null );
      }
    }
  }
}


void K3bAudioJob::slotProcessWroteStdin()
{
  m_processWroteStdin = true;
}


#include "k3baudiojob.moc"
