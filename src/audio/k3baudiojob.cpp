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

#include <audio/k3baudiodecoder.h>
#include <audio/k3baudiodoc.h>
#include <audio/k3baudiotrack.h>
#include <audio/k3baudiotocfilewriter.h>
#include <audio/k3baudionormalizejob.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>
#include <device/k3bmsf.h>
#include <tools/k3bwavefilewriter.h>
#include <tools/k3bglobals.h>
#include <k3bemptydiscwaiter.h>
#include <k3b.h>
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
  m_audioDecoder = new K3bAudioDecoder( m_doc, this );
  connect( m_audioDecoder, SIGNAL(data(const char*, int)), this, SLOT(slotReceivedAudioDecoderData(const char*, int)) );
  connect( m_audioDecoder, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_audioDecoder, SIGNAL(percent(int)), this, SLOT(slotAudioDecoderPercent(int)) );
  connect( m_audioDecoder, SIGNAL(subPercent(int)), this, SLOT(slotAudioDecoderSubPercent(int)) );
  connect( m_audioDecoder, SIGNAL(finished(bool)), this, SLOT(slotAudioDecoderFinished(bool)) );
  connect( m_audioDecoder, SIGNAL(nextTrack(int, int)), this, SLOT(slotAudioDecoderNextTrack(int, int)) );

  m_waveFileWriter = new K3bWaveFileWriter();

  m_writer = 0;
  m_tocFile = 0;
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
      m_audioDecoder->writeToFd( m_writer->fd() );
    }
    else {
      // startWriting() already did the cleanup
      return; 
    }
  }
  else {
    emit infoMessage( i18n("Creating image files in %1").arg(m_doc->tempDir()), INFO );
    emit newTask( i18n("Creating image files") );
    m_tempFilePrefix = K3b::findUniqueFilePrefix( m_doc->title(), m_doc->tempDir() );
  }
  m_audioDecoder->start();
}


void K3bAudioJob::cancel()
{
  m_canceled = true;

  if( m_writer )
    m_writer->cancel();

  m_audioDecoder->cancel();
  emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
  removeBufferFiles();
  emit canceled();
  emit finished(false);
}


void K3bAudioJob::slotDataWritten()
{
  m_written = true;
  m_audioDecoder->resume();
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
    if( !m_doc->onTheFly() && m_doc->removeBufferFiles() )
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
    m_audioDecoder->resume();
  }
}


void K3bAudioJob::slotAudioDecoderNextTrack( int t, int tt )
{
  if( m_doc->onlyCreateImages() || !m_doc->onTheFly() ) {
    emit newSubTask( i18n("Decoding audiotrack %1 of %2 (%3)").arg(t).arg(tt).arg(m_doc->at(t-1)->fileName()) );

    // create next buffer file (WaveFileWriter will close the last written file)
    QString bf = m_tempFilePrefix + "_track" + QString::number(t) + ".wav";
    if( !m_waveFileWriter->open( bf ) ) {
      emit infoMessage( i18n("Could not open file %1 for writing.").arg(m_waveFileWriter->filename()), ERROR );
      cleanupAfterError();
      emit finished( false );
      return;
    }

    m_doc->at(t-1)->setBufferFile( bf );
  }
}


bool K3bAudioJob::prepareWriter()
{
  if( m_writer ) delete m_writer;

  if( writingApp() == K3b::CDRECORD ||
      ( writingApp() == K3b::DEFAULT && !m_doc->dao() ) ) {

    K3bCdrecordWriter* writer = new K3bCdrecordWriter( m_doc->burner(), this );

    writer->setDao( m_doc->dao() );
    writer->setSimulate( m_doc->dummy() );
    writer->setBurnproof( m_doc->burnproof() );
    writer->setBurnSpeed( m_doc->speed() );
    writer->setProvideStdin( false/*m_doc->onTheFly() */);
    writer->prepareArgumentList();

//     if( m_doc->onTheFly() )
//       writer->addArgument("-waiti");

    // add all the audio tracks
    writer->addArgument( "-audio" );

    QListIterator<K3bAudioTrack> it( *m_doc->tracks() );
    for( ; it.current(); ++it ) {
      K3bAudioTrack* track = it.current();
    
      if( !track->copyProtection() )
	writer->addArgument( "-copy" );
      else
	writer->addArgument( "-nocopy" );
      if( track->preEmp() )
	writer->addArgument( "-preemp" );
      else
	writer->addArgument( "-nopreemp" );

      writer->addArgument( QString("-pregap=%1").arg(track->pregap().totalFrames()) );
//       if( m_doc->onTheFly() ) {
// 	//       QString fifoname = QString("/home/trueg/tmp/fifo_track%1").arg(track->index());
// 	//       if( ::mkfifo( fifoname.latin1(), S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH ) == -1 ) {
// 	// 	kdDebug() << "(K3bAudioJob) could not create fifo" << endl;
// 	//       }
// 	//       track->setBufferFile( fifoname );  // just a temp solution
// 	//       writer->addArgument( QString("-tsize=%1f").arg(track->length()) )->addArgument(fifoname);
//       }
//      else
	writer->addArgument( QFile::encodeName(track->bufferFile()) );
    }

    m_writer = writer;
  }
  else {  // DEFAULT
    if( !writeTocFile() ) {
      kdDebug() << "(K3bDataJob) could not write tocfile." << endl;
      emit infoMessage( i18n("IO Error"), ERROR );
    
      return false;
    }

    // create the writer
    // create cdrdao job
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_doc->burner(), this );
    writer->setSimulate( m_doc->dummy() );
    writer->setBurnSpeed( m_doc->speed() );
    writer->setProvideStdin(m_doc->onTheFly());
    writer->setTocFile( m_tocFile->name() );

    m_writer = writer;
  }

  connect( m_writer, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writer, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
  connect( m_writer, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_writer, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_writer, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_writer, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
  connect( m_writer, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writer, SIGNAL(writeSpeed(int)), this, SIGNAL(writeSpeed(int)) );
  connect( m_writer, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
  connect( m_writer, SIGNAL(dataWritten()), this, SLOT(slotDataWritten()) );
  //  connect( m_writer, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_writer, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writer, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}


bool K3bAudioJob::writeTocFile()
{
  if( m_tocFile ) delete m_tocFile;
  m_tocFile = new KTempFile( QString::null, "toc" );
  m_tocFile->setAutoDelete(true);

  // write the toc-file
  if( QTextStream* s = m_tocFile->textStream() ) {
    *s << "CD_DA" << "\n\n";
    
    K3bAudioTocfileWriter::writeAudioToc( m_doc, *s );
  
    m_tocFile->close();
    
    return true;
  }
  else 
    return false;
}


void K3bAudioJob::slotWriterNextTrack( int t, int tt )
{
  // t is in range 1..tt
  emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(m_doc->at(t-1)->fileName()) );
}


void K3bAudioJob::slotWriterJobPercent( int p )
{
  if( m_doc->onTheFly() )
    emit percent(p);
  else if( m_doc->normalize() )
    emit percent( 67 + p/3 );
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
  else if( !m_doc->onTheFly() )
    emit percent( p/2 );
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


  K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );
  if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
    cancel();
    return false;
  }
	
  m_writer->start();
  return true;
}


void K3bAudioJob::cleanupAfterError()
{
  m_errorOccuredAndAlreadyReported = true;
  m_audioDecoder->cancel();

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

  QListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    if( QFile::exists( track->bufferFile() ) )
      if( !QFile::remove( track->bufferFile() ) )
	emit infoMessage( i18n("Could not delete file %1.").arg(track->bufferFile()), ERROR );
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
  QListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    files.append( track->bufferFile() );
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
    return i18n("Writing audio cd");
  else
    return i18n("Writing audio cd (%1)").arg(m_doc->title());
}


QString K3bAudioJob::jobDetails() const
{
  return i18n("1 track (%2 minutes)", "%n track (%2 minutes)", m_doc->numOfTracks()).arg(m_doc->length().toString());
}

#include "k3baudiojob.moc"
