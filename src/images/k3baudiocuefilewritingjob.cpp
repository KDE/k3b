/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudiocuefilewritingjob.h"

#include <k3baudiodoc.h>
#include <k3baudiojob.h>
#include <k3bdevice.h>
#include <k3baudiodecoder.h>
#include <k3baudiotrack.h>
#include <k3baudiofile.h>
#include <k3bcuefileparser.h>
#include <k3bthread.h>
#include <k3bthreadjob.h>

#include <kdebug.h>
#include <klocale.h>


class K3bAudioCueFileWritingJob::AnalyserThread : public K3bThread
{
public:
  AnalyserThread() 
    : K3bThread() {
  }

  void setDecoder( K3bAudioDecoder* dec ) { m_decoder = dec; }

protected:
  void run() {
    emitStarted();
    m_decoder->analyseFile();
    emitFinished(true);
  }

private:
  K3bAudioDecoder* m_decoder;
};


K3bAudioCueFileWritingJob::K3bAudioCueFileWritingJob( K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bBurnJob( jh, parent, name ),
    m_decoder(0)
{
  m_analyserThread = new AnalyserThread();
  m_analyserJob = new K3bThreadJob( m_analyserThread, this, this );
  connect( m_analyserJob, SIGNAL(finished(bool)), this, SLOT(slotAnalyserThreadFinished(bool)) );

  m_audioDoc = new K3bAudioDoc( this );
  m_audioDoc->newDocument();
  m_audioJob = new K3bAudioJob( m_audioDoc, this, this );

  // just loop all through
  connect( m_audioJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_audioJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_audioJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  connect( m_audioJob, SIGNAL(infoMessage(const QString&, int)),
	   this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_audioJob, SIGNAL(finished(bool)), this, SIGNAL(finished(bool)) );
  connect( m_audioJob, SIGNAL(canceled()), this, SIGNAL(canceled()) );
  connect( m_audioJob, SIGNAL(percent(int)), this, SIGNAL(percent(int)) );
  connect( m_audioJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_audioJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_audioJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_audioJob, SIGNAL(burning(bool)), this, SIGNAL(burning(bool)) );
  connect( m_audioJob, SIGNAL(bufferStatus(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_audioJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
  connect( m_audioJob, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );

  m_canceled = false;
  m_audioJobRunning = false;
}


K3bAudioCueFileWritingJob::~K3bAudioCueFileWritingJob()
{
  // the threadjob does not delete the thread
  delete m_analyserThread;
}


K3bDevice::Device* K3bAudioCueFileWritingJob::writer() const
{
  return m_audioDoc->burner();
}


QString K3bAudioCueFileWritingJob::jobDescription() const
{
  return i18n("Writing Audio Cue File");
}


QString K3bAudioCueFileWritingJob::jobDetails() const
{
  return m_cueFile.section( '/', -1 );
}


void K3bAudioCueFileWritingJob::start()
{
  // FIXME: here we trust that a job won't be started twice :(
  emit started();
  m_canceled = false;
  m_audioJobRunning = false;
  importCueInProject();
}


void K3bAudioCueFileWritingJob::cancel()
{
  m_canceled = true;

  // the AudioJob cancel method is very stupid. It emits the canceled signal even if it was never running :(
  if( m_audioJobRunning )
    m_audioJob->cancel();
  m_analyserJob->cancel();
}


void K3bAudioCueFileWritingJob::setCueFile( const QString& s )
{
  m_cueFile = s;
}


void K3bAudioCueFileWritingJob::setOnTheFly( bool b )
{
  m_audioDoc->setOnTheFly( b );
}


void K3bAudioCueFileWritingJob::setSpeed( int s )
{
  m_audioDoc->setSpeed( s );
}


void K3bAudioCueFileWritingJob::setBurnDevice( K3bDevice::Device* dev )
{
  m_audioDoc->setBurner( dev );
}


void K3bAudioCueFileWritingJob::setWritingMode( int mode )
{
  m_audioDoc->setWritingMode( mode );
}


void K3bAudioCueFileWritingJob::setSimulate( bool b )
{
  m_audioDoc->setDummy( b );
}


void K3bAudioCueFileWritingJob::setCopies( int c )
{
  m_audioDoc->setCopies( c );
}


void K3bAudioCueFileWritingJob::slotAnalyserThreadFinished( bool )
{
  if( !m_canceled ) {
    if( m_audioDoc->lastTrack()->length() == 0 ) {
      emit infoMessage( i18n("Analysing the audio file failed. Corrupt file?"), ERROR );
      emit finished(false);
    }
    else {
      // FIXME: m_audioJobRunning is never reset
      m_audioJobRunning = true;
      m_audioJob->start(); // from here on the audio job takes over completely
    }
  }
  else {
    emit canceled();
    emit finished(false);
  }
}


void K3bAudioCueFileWritingJob::importCueInProject()
{
  // cleanup the project (this wil also delete the decoder)
  // we do not use newDocument as that would overwrite the settings already made
  while( m_audioDoc->firstTrack() )
    delete m_audioDoc->firstTrack()->take();

  m_decoder = 0;

  K3bCueFileParser parser( m_cueFile );
  if( parser.isValid() && parser.toc().contentType() == K3bDevice::AUDIO ) {

    kdDebug() << "(K3bAudioCueFileWritingJob::importCueFile) parsed with image: " << parser.imageFilename() << endl;

    // global cd-text
    m_audioDoc->setTitle( parser.cdText().title() );
    m_audioDoc->setPerformer( parser.cdText().performer() );
    m_audioDoc->writeCdText( !parser.cdText().title().isEmpty() );

    m_decoder = K3bAudioDecoderFactory::createDecoder( parser.imageFilename() );
    if( m_decoder ) {
      m_decoder->setFilename( parser.imageFilename() );

      K3bAudioTrack* after = 0;
      K3bAudioFile* newFile = 0;
      unsigned int i = 0;
      for( K3bDevice::Toc::const_iterator it = parser.toc().begin();
	   it != parser.toc().end(); ++it ) {
	const K3bDevice::Track& track = *it;

	newFile = new K3bAudioFile( m_decoder, m_audioDoc );
	newFile->setStartOffset( track.firstSector() );
	newFile->setEndOffset( track.lastSector()+1 );

	K3bAudioTrack* newTrack = new K3bAudioTrack( m_audioDoc );
	newTrack->addSource( newFile );
	newTrack->moveAfter( after );

	// cd-text
	newTrack->setTitle( parser.cdText()[i].title() );
	newTrack->setPerformer( parser.cdText()[i].performer() );

	// add the next track after this one
	after = newTrack;
	++i;
      }

      // let the last source use the data up to the end of the file
      if( newFile )
	newFile->setEndOffset(0);

      // now analyse the source
      emit newTask( i18n("Analysing the audio file") );
      emit newSubTask( i18n("Analysing %1").arg( parser.imageFilename() ) );

      // start the analyser thread
      m_analyserThread->setDecoder( m_decoder );
      m_analyserJob->start();
    }
    else {
      emit infoMessage( i18n("Unable to handle '%1' due to an unsupported format.").arg( m_cueFile ), ERROR );
      emit finished(false);
    }
  }
  else {
    emit infoMessage( i18n("No valid audio cue file: '%1'").arg( m_cueFile ), ERROR );
    emit finished(false);
  }
}

#include "k3baudiocuefilewritingjob.moc"
