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

#include "k3baudiostreamer.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
//#include "input/k3baudiomodule.h"
#include <k3baudiodecoder.h>

#include <klocale.h>
#include <kdebug.h>
#include <kio/global.h>

#include <qcstring.h>
#include <qtimer.h>
#include <qsocketnotifier.h>

#include <unistd.h>


class K3bAudioStreamer::Private
{
public:
  Private() {
    fdToWriteTo = -1;
    currentModule = 0;
    currentTrack = 0;
  }

  int fdToWriteTo;
  QSocketNotifier* notifier;

  int currentTrackNumber;
  K3bAudioTrack* currentTrack;
  K3bAudioDecoder* currentModule;

  QByteArray buffer;
  long bufferDataLen;

  unsigned long writtenTrackData;
  unsigned long writtenOverallData;
  unsigned long writtenTrackPregapData;
  unsigned long overallDataToWrite;
  unsigned long trackDataToWrite;

  bool finished;
  bool canceled;

  bool nextTrackStarted;

  bool littleEndian;

  K3bAudioDoc* doc;
};



K3bAudioStreamer::K3bAudioStreamer( K3bAudioDoc* doc, QObject* parent, const char* name )
  : K3bJob( parent, name )
{
  d = new Private;
  d->doc = doc;
  d->buffer.resize( 2352*10 );
  d->littleEndian = false;
  d->notifier = 0;
}

K3bAudioStreamer::~K3bAudioStreamer()
{
  delete d;
}


void K3bAudioStreamer::start()
{
  d->canceled = false;
  d->finished = false;

  d->currentTrackNumber = 1;
  d->writtenOverallData = 0;

  // calculate overall data
  d->overallDataToWrite = 0;
  for( QPtrListIterator<K3bAudioTrack> it( *d->doc->tracks() ); *it; ++it ) {
    K3bAudioTrack* track = *it;
    d->overallDataToWrite += track->size();
    // we do not write a pregap for the first track
    // this is done by cdrecord/cdrdao
    if( track->index() != 0 )
      d->overallDataToWrite += track->pregap().audioBytes();
  }

  QTimer::singleShot( 0, this, SLOT(startModule()) );
}


void K3bAudioStreamer::cancel()
{
  d->canceled = true;
  emit canceled();
  cancelAll();
}


void K3bAudioStreamer::resume()
{
  if( !d->finished ) {
    // we do not write a pregap for the first track
    // this is done by cdrecord/cdrdao
    if( d->currentTrackNumber != 1 &&
	d->currentTrack->pregap().audioBytes() > d->writtenTrackPregapData )
      QTimer::singleShot( 0, this, SLOT(writePregap()) );
    else {
      // the pregap is part of the previous track!
      if( !d->nextTrackStarted ) {
	d->nextTrackStarted = true;
	emit nextTrack( d->currentTrackNumber, d->doc->numberOfTracks() );
      }

      QTimer::singleShot( 0, this, SLOT(decode()) );
    }
  }
  else
    kdError() << "(K3bAudioStreamer) calling resume to a finished decoder." << endl;
}


void K3bAudioStreamer::writeToFd( int fd )
{
  d->fdToWriteTo = fd;

  delete d->notifier;
  d->notifier = new QSocketNotifier( fd, QSocketNotifier::Write, this );
  d->notifier->setEnabled(false);
  connect( d->notifier, SIGNAL(activated(int)), this, SLOT(slotFdActivated(int)) );
}


void K3bAudioStreamer::startModule()
{
  d->writtenTrackPregapData = 0;
  d->writtenTrackData = 0;
  d->nextTrackStarted = false;

  if( d->currentTrackNumber > d->doc->numberOfTracks() ) {
    kdDebug() << "(K3bAudioStreamer) decoded all tracks: " << d->writtenOverallData << " bytes." << endl;
    d->finished = true;
    emit finished(true);
  }
  else {
    d->currentTrack = d->doc->at(d->currentTrackNumber-1);
    d->currentModule = d->currentTrack->module();

    d->trackDataToWrite = d->currentTrack->size();
    // we do not write a pregap for the first track
    // this is done by cdrecord/cdrdao
    if( d->currentTrack->index() != 0 )
      d->trackDataToWrite += d->currentTrack->pregap().audioBytes();
    
    if( !d->currentModule->initDecoder() ) {
      kdDebug() << "(K3bAudioStreamer) unable to initialize module for track " 
		<< d->currentTrackNumber << ": " << d->currentTrack->absPath() << endl;
      cancelAll();
    }
    else {
      kdDebug() << "(K3bAudioStreamer) successfully initialized module for track "
		<< d->currentTrackNumber << ": " << d->currentTrack->absPath() << endl;
      resume();
    }
  }
}


void K3bAudioStreamer::decode()
{
  long len = d->currentModule->decode( d->buffer.data(), d->buffer.size() );

  if( len < 0 ) {
    kdDebug() << "(K3bAudioStreamer) Error while decoding track " << d->currentTrackNumber << endl;
    cancelAll();
  }
  else if( len == 0 ) {
    kdDebug() << "(K3bAudioStreamer) finished decoding track " << d->currentTrackNumber << endl;
    ++d->currentTrackNumber;
    d->currentModule->cleanup();
    startModule();
  }
  else {
    if( !writeData( len ) ) {
      cancelAll();
    }
  }
}


void K3bAudioStreamer::cancelAll()
{
  if( d->currentModule )
    d->currentModule->cleanup();

  emit finished( false );
}


void K3bAudioStreamer::writePregap()
{
  long pSize = d->currentTrack->pregap().audioBytes() - d->writtenTrackPregapData;
  if( pSize > 0 ) {
    d->buffer.fill( '\0' );
    if( pSize > (int)d->buffer.size() )
      pSize = d->buffer.size();
    d->writtenTrackPregapData += pSize;

    if( !writeData( pSize ) ) {
      cancelAll();
    }
  }
}


bool K3bAudioStreamer::writeData( long len )
{
  // to make sure the percent signals get emitted every time
  // we do them before emiting any data
  d->writtenOverallData += len;
  d->writtenTrackData += len;
  d->bufferDataLen = len;

  if( d->overallDataToWrite <= 0 ) {
    kdDebug() << "(K3bAudioStreamer) ERROR: overallDataToWrite <= 0!" << endl;
    d->overallDataToWrite = 1;
  }
  if( d->trackDataToWrite <= 0 ) {
    kdDebug() << "(K3bAudioStreamer) ERROR: trackDataToWrite <= 0!" << endl;
    d->trackDataToWrite = 1;
  }

  // this includes the pregap data
  emit percent( (int)( (double)d->writtenOverallData 
		       * 100.0 
		       / (double)d->overallDataToWrite ) );

  emit subPercent( (int)( (double)d->writtenTrackData
			  * 100.0
			  / (double)d->trackDataToWrite ) );

  if( d->littleEndian ) {
    // the modules produce big endian samples
    // so we need to swap the bytes here
    char b;
    for( int i = 0; i < len-1; i+=2 ) {
      b = d->buffer[i];
      d->buffer[i] = d->buffer[i+1];
      d->buffer[i+1] = b;
    }
  }

  if( d->fdToWriteTo != -1 ) {
    d->notifier->setEnabled( true );
  }
  else {
    emit data( d->buffer.data(), len );
  }

  return true;
}


void K3bAudioStreamer::slotFdActivated( int )
{
  d->notifier->setEnabled( false );

  if( ::write( d->fdToWriteTo, d->buffer.data(), d->bufferDataLen ) != d->bufferDataLen ) {
    kdError() << "(K3bAudioStreamer) could not write to " << d->fdToWriteTo << endl;
    d->finished = true;
    cancelAll();
  }
  else {
    // if we write to a fd the data is written immediately
    // so we resume ourselves
    resume();
  }
}


void K3bAudioStreamer::setLittleEndian( bool b )
{
  d->littleEndian = b;
}

#include "k3baudiostreamer.moc"
