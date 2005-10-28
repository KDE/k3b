/*
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
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


#include "k3baudiocueimagesource.h"

#include <k3bthread.h>
#include <k3bthreadjob.h>
#include <k3bdevice.h>
#include <k3baudiodecoder.h>
#include <k3bcuefileparser.h>

#include <kdebug.h>
#include <klocale.h>


class K3bAudioCueImageSource::AnalyserThread : public K3bThread
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



class K3bAudioCueImageSource::Private
{
public:
  Private( K3bAudioCueImageSource* s )
    : decoder(0),
      analyserThread(0),
      analyserJob(0) {
    // we always need the analyser thread
    analyserThread = new AnalyserThread();
    analyserJob = new K3bThreadJob( analyserThread, s, s );
    connect( analyserJob, SIGNAL(finished(bool)), s, SLOT(slotAnalyserThreadFinished(bool)) );
  }

  QString cueFile;

  bool canceled;

  K3bAudioDecoder* decoder;

  AnalyserThread* analyserThread;
  K3bThreadJob* analyserJob;
};


K3bAudioCueImageSource::K3bAudioCueImageSource( K3bJobHandler* hdl, QObject* parent )
  : K3bSimpleImageSource( hdl, parent )
{
  d = new Private( this );
}


K3bAudioCueImageSource::K3bAudioCueImageSource( const QString& cuefile, K3bJobHandler* hdl, QObject* parent )
  : K3bSimpleImageSource( hdl, parent )
{
  d = new Private( this );
  setCueFile( cuefile );
}


bool K3bAudioCueImageSource::init()
{
  d->canceled = false;
  if( d->decoder ) {
    d->decoder->seek( 0 );
    return true;
  }
  else 
    return false;
}


K3bAudioCueImageSource::~K3bAudioCueImageSource()
{
  // the threadjob does not delete the thread
  delete d->analyserThread;
  delete d;
}


void K3bAudioCueImageSource::setCueFile( const QString& f )
{
  d->cueFile = f;
}


long K3bAudioCueImageSource::simpleRead( char* data, long maxLen )
{
  if( d->decoder ) {
    long r = d->decoder->decode( data, maxLen );
    if( r < 0 ) {
      emit infoMessage( i18n("Error while decoding file '%1'").arg(d->decoder->filename()), ERROR );
    }

    return r;
  }
  else
    return -1;
}


void K3bAudioCueImageSource::determineToc()
{
  kdDebug() << k_funcinfo << endl;

  d->canceled = false;

  K3bCueFileParser parser( d->cueFile );
  if( parser.isValid() && parser.toc().contentType() == K3bDevice::AUDIO ) {
    setToc( parser.toc() );
    setCdText( parser.cdText() );

    d->decoder = K3bAudioDecoderFactory::createDecoder( parser.imageFilename() );
    if( d->decoder ) {
      emit infoMessage( i18n("Found %1 audio cue file.").arg(d->decoder->fileType()), INFO );

      d->decoder->setFilename( parser.imageFilename() );

      // now analyse the source
      emit newTask( i18n("Analysing the audio file") );
      emit newSubTask( i18n("Analysing %1").arg( parser.imageFilename() ) );

      // start the analyser thread
      d->analyserThread->setDecoder( d->decoder );
      d->analyserJob->start();
    }
    else {
      emit infoMessage( i18n("Unable to handle '%1' due to an unsupported format.").arg( d->cueFile ), ERROR );
      jobFinished(false);
    }
  }
  else {
    emit infoMessage( i18n("No valid audio cue file: '%1'").arg( d->cueFile ), ERROR );
    jobFinished(false);
  }
}


void K3bAudioCueImageSource::cancel()
{
  if( active() ) {
    d->canceled = true;

    if( d->analyserJob->active() )
      d->analyserJob->cancel();

    K3bSimpleImageSource::cancel();
  }
}


void K3bAudioCueImageSource::slotAnalyserThreadFinished( bool success )
{
  kdDebug() << k_funcinfo << endl;

  if( !d->canceled ) {
    // simply fix the last track's length
    // FIXME: check if the length fits
    K3bDevice::Toc t = toc();
    t.last().setLastSector( d->decoder->length()-1 );
    setToc( t );
    
    emit tocReady( success );
  }
}

#include "k3baudiocueimagesource.moc"
