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


#include "k3baudiojob.h"

#include <k3baudiostreamer.h>
#include <k3baudiodoc.h>
#include <k3baudiotrack.h>
#include <k3baudionormalizejob.h>
#include "k3baudiojobtempdata.h"
#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>
#include <device/k3bmsf.h>
#include <k3bwavefilewriter.h>
#include <k3bglobals.h>
#include <k3bexternalbinmanager.h>
#include <k3bemptydiscwaiter.h>
#include <k3bcore.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>

#include <qfile.h>
#include <qvaluevector.h>

#include <kdebug.h>
#include <klocale.h>
#include <ktempfile.h>



K3bAudioJob::K3bAudioJob( K3bAudioDoc* doc, QObject* parent )
  : K3bBurnJob( parent ),
    m_doc( doc ),
    m_normalizeJob(0)
{
  m_audioStreamer = new K3bAudioStreamer( m_doc, this );
  connect( m_audioStreamer, SIGNAL(data(const char*, int)), this, SLOT(slotReceivedAudioDecoderData(const char*, int)) );
  connect( m_audioStreamer, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_audioStreamer, SIGNAL(percent(int)), this, SLOT(slotAudioDecoderPercent(int)) );
  connect( m_audioStreamer, SIGNAL(subPercent(int)), this, SLOT(slotAudioDecoderSubPercent(int)) );
  connect( m_audioStreamer, SIGNAL(finished(bool)), this, SLOT(slotAudioDecoderFinished(bool)) );
  connect( m_audioStreamer, SIGNAL(nextTrack(int, int)), this, SLOT(slotAudioDecoderNextTrack(int, int)) );

  m_waveFileWriter = new K3bWaveFileWriter();

  m_writer = 0;
  m_tocFile = 0;
  m_tempData = new K3bAudioJobTempData( m_doc, this );
}


K3bAudioJob::~K3bAudioJob()
{
  delete m_waveFileWriter;
  if( m_tocFile ) delete m_tocFile;
}


K3bDevice* K3bAudioJob::writer() const
{
  return m_doc->burner();
}


K3bDoc* K3bAudioJob::doc() const
{
  return m_doc;
}


void K3bAudioJob::start()
{
  emit started();

  m_written = true;
  m_canceled = false;
  m_errorOccuredAndAlreadyReported = false;


  // determine writing mode
  if( m_doc->writingMode() == K3b::WRITING_MODE_AUTO ) {
    // DAO is always the first choice
    // choose TAO if the user wants to use cdrecord since
    // there are none-DAO writers that are supported by cdrdao
    if( !writer()->dao() && writingApp() == K3b::CDRECORD )
      m_usedWritingMode = K3b::TAO;
    else
      m_usedWritingMode = K3b::DAO;
  }
  else
    m_usedWritingMode = m_doc->writingMode();

  bool cdrecordOnTheFly = false;
  bool cdrecordCdText = false;
  if( k3bcore->externalBinManager()->binObject("cdrecord") ) {
    cdrecordOnTheFly = k3bcore->externalBinManager()->binObject("cdrecord")->version >= K3bVersion( 2, 1, -1, "a13" );
    cdrecordCdText = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "cdtext" );
  }    

  // determine writing app
  if( writingApp() == K3b::DEFAULT ) {
    if( m_usedWritingMode == K3b::DAO ) {
      // there are none-DAO writers that are supported by cdrdao
      if( !writer()->dao() ||
	  ( !cdrecordOnTheFly && m_doc->onTheFly() ) ||
	  ( m_doc->cdText() &&
	    // the inf-files we use do only support artist and title in the global section
	    ( !m_doc->arranger().isEmpty() ||
	      !m_doc->songwriter().isEmpty() ||
	      !m_doc->composer().isEmpty() ||
	      !m_doc->cdTextMessage().isEmpty() ||
	      !cdrecordCdText )
	    ) ||
	  m_doc->hideFirstTrack()
	  )
	m_usedWritingApp = K3b::CDRDAO;
      else
	m_usedWritingApp = K3b::CDRECORD;
    }
    else
      m_usedWritingApp = K3b::CDRECORD;
  }
  else
    m_usedWritingApp = writingApp();

  // on-the-fly writing with cdrecord >= 2.01a13
  if( m_usedWritingApp == K3b::CDRECORD &&
      m_doc->onTheFly() &&
      !cdrecordOnTheFly ) {
    emit infoMessage( i18n("On-the-fly writing with cdrecord < 2.01a13 not supported."), ERROR );
    m_doc->setOnTheFly(false);
  }

  if( m_usedWritingApp == K3b::CDRECORD &&
      m_doc->cdText() ) {
    if( !cdrecordCdText ) {
      emit infoMessage( i18n("Cdrecord %1 does not support CD-Text writing.").arg(k3bcore->externalBinManager()->binObject("cdrecord")->version), ERROR );
      m_doc->writeCdText(false);
    }
    else {
      if( !m_doc->arranger().isEmpty() )
	emit infoMessage( i18n("K3b does not support Album arranger CD-Text with cdrecord."), ERROR );
      if( !m_doc->songwriter().isEmpty() )
	emit infoMessage( i18n("K3b does not support Album songwriter CD-Text with cdrecord."), ERROR );
      if( !m_doc->composer().isEmpty() )
	emit infoMessage( i18n("K3b does not support Album composer CD-Text with cdrecord."), ERROR );
      if( !m_doc->cdTextMessage().isEmpty() )
	emit infoMessage( i18n("K3b does not support Album comment CD-Text with cdrecord."), ERROR );
    }
  }

  if( !m_doc->onlyCreateImages() && m_doc->onTheFly() ) {
    if( !prepareWriter() ) {
      cleanupAfterError();
      emit finished(false);
      return;
    }

    if( startWriting() ) {

      // now the writer is running and we can get it's stdin
      // we only use this method when writing on-the-fly since
      // we cannot easily change the audioDecode fd while it's working
      // which we would need to do since we write into several
      // image files.
      m_audioStreamer->writeToFd( m_writer->fd() );
    }
    else {
      // startWriting() already did the cleanup
      return;
    }
  }
  else {
    emit burning(false);
    emit infoMessage( i18n("Creating image files in %1").arg(m_doc->tempDir()), INFO );
    emit newTask( i18n("Creating image files") );
  }
  m_audioStreamer->start();
}


void K3bAudioJob::cancel()
{
  m_canceled = true;

  if( m_writer )
    m_writer->cancel();

  m_audioStreamer->cancel();
  emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
  removeBufferFiles();
  emit canceled();
  emit finished(false);
}


void K3bAudioJob::slotDataWritten()
{
  m_written = true;
  m_audioStreamer->resume();
}


void K3bAudioJob::slotWriterFinished( bool success )
{
  if( m_canceled || m_errorOccuredAndAlreadyReported )
    return;

  if( !success ) {
    cleanupAfterError();
    emit finished(false);
    return;
  }
  else {
    if( !m_doc->onTheFly() && m_doc->removeImages() )
      removeBufferFiles();

    emit finished(true);
  }
}


void K3bAudioJob::slotAudioDecoderFinished( bool success )
{
  if( m_canceled || m_errorOccuredAndAlreadyReported )
    return;

  if( !success ) {
    emit infoMessage( i18n("Error while decoding audio tracks."), ERROR );
    cleanupAfterError();
    emit finished(false);
    return;
  }

  if( m_doc->onlyCreateImages() || !m_doc->onTheFly() ) {
    // close the last written wave file
    m_waveFileWriter->close();

    emit infoMessage( i18n("Successfully decoded all tracks."), STATUS );

    if( m_doc->normalize() ) {
	normalizeFiles();
    }
    else if( !m_doc->onlyCreateImages() ) {
      if( !prepareWriter() ) {
	cleanupAfterError();
	emit finished(false);
      }
      else
	startWriting();
    }
    else {
      emit finished(true);
    }
  }
}


void K3bAudioJob::slotReceivedAudioDecoderData( const char* data, int len )
{
  if( !m_doc->onlyCreateImages() && m_doc->onTheFly() ) {
    if( !m_writer->write( (char*)data, len ) ) {
      kdError() << "(K3bAudioJob) Error while writing data to Writer" << endl;
      emit infoMessage( i18n("IO error"), ERROR );
      if( !m_written )
	emit infoMessage( i18n("Internal Error! Please report!"), ERROR );
      cleanupAfterError();
      emit finished(false);
      return;
    }
    m_written = false;
  }
  else {
    m_waveFileWriter->write( data, len );
    m_audioStreamer->resume();
  }
}


void K3bAudioJob::slotAudioDecoderNextTrack( int t, int tt )
{
  if( m_doc->onlyCreateImages() || !m_doc->onTheFly() ) {
    K3bAudioTrack* track = m_doc->at(t-1);
    emit newSubTask( i18n("Decoding audiotrack %1 of %2 (%3)").arg(t).arg(tt).arg(track->fileName()) );

    // create next buffer file (WaveFileWriter will close the last written file)
    if( !m_waveFileWriter->open( m_tempData->bufferFileName(track) ) ) {
      emit infoMessage( i18n("Could not open file %1 for writing.").arg(m_waveFileWriter->filename()), ERROR );
      cleanupAfterError();
      emit finished( false );
      return;
    }
    else {
      kdDebug() << "(K3bAudioJob) Successfully opened Wavefilewriter on "
		<< m_waveFileWriter->filename() << endl;
//       m_audioStreamer->writeToFd( m_waveFileWriter->fd() );
//       m_audioStreamer->setLittleEndian(true);
    }
  }
}


bool K3bAudioJob::prepareWriter()
{
  if( m_writer ) delete m_writer;


  if( m_usedWritingApp == K3b::CDRECORD ) {

    if( !m_tempData->writeInfFiles() ) {
      kdDebug() << "(K3bAudioJob) could not write inf-files." << endl;
      emit infoMessage( i18n("IO Error"), ERROR );

      return false;
    }

    K3bCdrecordWriter* writer = new K3bCdrecordWriter( m_doc->burner(), this );

    writer->setWritingMode( m_usedWritingMode );
    writer->setSimulate( m_doc->dummy() );
    writer->setBurnproof( m_doc->burnproof() );
    writer->setBurnSpeed( m_doc->speed() );
    writer->setProvideStdin( m_doc->onTheFly() );

    writer->addArgument( "-useinfo" );

    if( m_doc->cdText() )
      writer->addArgument( "-text" );

    // we always pad because although K3b makes sure all tracks' lenght are multible of 2352
    // it seems that normalize sometimes corrupts these lenght
    writer->addArgument( "-pad" );

    // add all the audio tracks
    writer->addArgument( "-audio" );

    QPtrListIterator<K3bAudioTrack> it( *m_doc->tracks() );
    for( ; it.current(); ++it ) {
      K3bAudioTrack* track = it.current();

      if( m_doc->onTheFly() ) {
	// this is only supported by cdrecord versions >= 2.01a13
	writer->addArgument( QFile::encodeName( m_tempData->infFileName( track ) ) );
      }
      else {
	writer->addArgument( QFile::encodeName( m_tempData->bufferFileName( track ) ) );
      }
    }

    m_writer = writer;
  }
  else {  // DEFAULT
    if( !m_tempData->writeTocFile() ) {
      kdDebug() << "(K3bDataJob) could not write tocfile." << endl;
      emit infoMessage( i18n("IO Error"), ERROR );

      return false;
    }

    // create the writer
    // create cdrdao job
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_doc->burner(), this );
    writer->setCommand( K3bCdrdaoWriter::WRITE );
    writer->setSimulate( m_doc->dummy() );
    writer->setBurnSpeed( m_doc->speed() );
    writer->setProvideStdin( m_doc->onTheFly() );
    writer->setTocFile( m_tempData->tocFileName() );

    m_writer = writer;
  }

  connect( m_writer, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writer, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
  connect( m_writer, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_writer, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_writer, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_writer, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
  connect( m_writer, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writer, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
  connect( m_writer, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
  connect( m_writer, SIGNAL(dataWritten()), this, SLOT(slotDataWritten()) );
  //  connect( m_writer, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_writer, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writer, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}


void K3bAudioJob::slotWriterNextTrack( int t, int tt )
{
  K3bAudioTrack* track = m_doc->at(t-1);
  // t is in range 1..tt
  emit newSubTask( i18n("Writing track %1 of %2 (%3)")
		   .arg(t)
		   .arg(tt)
		   .arg( track->title().isEmpty() || track->artist().isEmpty() 
			 ? track->fileName()
			 : track->artist() + " - " + track->title() ) );
}


void K3bAudioJob::slotWriterJobPercent( int p )
{
  if( m_doc->onTheFly() )
    emit percent(p);
  else if( m_doc->normalize() )
    emit percent( (int)(66.6 + (double)p/3) );
  else
    emit percent( 50 + p/2 );
}


void K3bAudioJob::slotAudioDecoderPercent( int p )
{
  if( m_doc->onlyCreateImages() ) {
    if( m_doc->normalize() )
      emit percent( p/2 );
    else
      emit percent( p );
  }
  else if( !m_doc->onTheFly() ) {
    if( m_doc->normalize() )
      emit percent( p/3 );
    else
      emit percent( p/2 );
  }
}


void K3bAudioJob::slotAudioDecoderSubPercent( int p )
{
  // when writing on the fly the writer produces the subPercent
  if( m_doc->onlyCreateImages() || !m_doc->onTheFly() ) {
    emit subPercent( p );
  }
}


bool K3bAudioJob::startWriting()
{
  if( m_doc->dummy() )
    emit newTask( i18n("Simulating") );
  else
    emit newTask( i18n("Writing") );


  if( K3bEmptyDiscWaiter::wait( m_doc->burner() ) == K3bEmptyDiscWaiter::CANCELED ) {
    cancel();
    return false;
  }

  // just to be sure we did not get canceled during the async discWaiting
  if( m_canceled )
    return false;

  emit burning(true);
  m_writer->start();
  return true;
}


void K3bAudioJob::cleanupAfterError()
{
  m_errorOccuredAndAlreadyReported = true;
  m_audioStreamer->cancel();

  if( m_writer )
    m_writer->cancel();

  if( m_tocFile ) delete m_tocFile;
  m_tocFile = 0;

  // remove the temp files
  removeBufferFiles();
}


void K3bAudioJob::removeBufferFiles()
{
  emit infoMessage( i18n("Removing buffer files."), INFO );

  QPtrListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    const QString& f = m_tempData->bufferFileName(track);
    if( QFile::exists( f ) )
      if( !QFile::remove( f ) )
	emit infoMessage( i18n("Could not delete file %1.").arg(f), ERROR );
  }
}


void K3bAudioJob::normalizeFiles()
{
  if( !m_normalizeJob ) {
    m_normalizeJob = new K3bAudioNormalizeJob( this );

    connect( m_normalizeJob, SIGNAL(infoMessage(const QString&, int)),
	     this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_normalizeJob, SIGNAL(percent(int)), this, SLOT(slotNormalizeProgress(int)) );
    connect( m_normalizeJob, SIGNAL(subPercent(int)), this, SLOT(slotNormalizeSubProgress(int)) );
    connect( m_normalizeJob, SIGNAL(finished(bool)), this, SLOT(slotNormalizeJobFinished(bool)) );
    connect( m_normalizeJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_normalizeJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }

  // add all the files
  QValueVector<QString> files;
  QPtrListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    files.append( m_tempData->bufferFileName(track) );
  }

  m_normalizeJob->setFilesToNormalize( files );

  emit newTask( i18n("Normalizing volume levels") );
  m_normalizeJob->start();
}

void K3bAudioJob::slotNormalizeJobFinished( bool success )
{
  if( m_canceled || m_errorOccuredAndAlreadyReported )
    return;

  if( success ) {
    if( m_doc->onlyCreateImages() ) {
      emit finished(true);
    }
    else {
      // start the writing
      if( !prepareWriter() ) {
	cleanupAfterError();
	emit finished(false);
      }
      else
	startWriting();
    }
  }
  else {
    cleanupAfterError();
    emit finished(false);
  }
}

void K3bAudioJob::slotNormalizeProgress( int p )
{
  if( m_doc->onlyCreateImages() )
    emit percent( 50 + (int)((double)p/2.0) );
  else
    emit percent( (int)(33.3 + (double)p/3.0) );
}


void K3bAudioJob::slotNormalizeSubProgress( int p )
{
  emit subPercent( p );
}


QString K3bAudioJob::jobDescription() const
{
  if( m_doc->title().isEmpty() )
    return i18n("Writing audio CD");
  else
    return i18n("Writing audio CD (%1)").arg(m_doc->title());
}


QString K3bAudioJob::jobDetails() const
{
  return i18n("1 track (%1 minutes)", "%n tracks (%1 minutes)", m_doc->numOfTracks()).arg(m_doc->length().toString());
}

#include "k3baudiojob.moc"
