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

#include "k3bsimpleimagesource.h"
#include "k3bimagesourcestreamer.h"

#include <k3bglobals.h>

#include <qfile.h>
#include <qtimer.h>
#include <qcstring.h>

#include <klocale.h>


class K3bSimpleImageSource::Private
{
public:
  Private()
    : streamer( 0 ),
      canceled(false) {
  }

  K3bImageSourceStreamer* streamer;

  bool canceled;
  QByteArray data;

  // progress information
  KIO::filesize_t bytesToWrite;
  KIO::filesize_t writtenBytes;
  int lastProgress;
};


K3bSimpleImageSource::K3bSimpleImageSource( K3bJobHandler* hdl, QObject* parent )
  : K3bImageSource( hdl, parent )
{
  d = new Private();
}


K3bSimpleImageSource::~K3bSimpleImageSource()
{
  delete d;
}


void K3bSimpleImageSource::start()
{
  jobStarted();
  d->canceled = false;
  d->writtenBytes = 0;
  d->lastProgress = 0;
  d->bytesToWrite = tocSize();

  if( !init() ) {
    jobFinished( false );
    return;
  }

  // nothing to do unless we have to stream the data
  if( fdToWriteTo() != -1 ) {
    if( !d->streamer ) {
      // no need to conect to the finished signal since the read
      // method emits the finished signal
      d->streamer = new K3bImageSourceStreamer( this, this );
      d->streamer->setSource( this );
    }

    d->streamer->start();
  }
}


bool K3bSimpleImageSource::init()
{
  return true;
}


void K3bSimpleImageSource::cleanup()
{
  // do nothing
}


void K3bSimpleImageSource::cancel()
{
  if( active() ) {
    d->canceled = true;

    if( d->streamer )
      d->streamer->cancel();

    cleanup();

    emit canceled();
    jobFinished( false );
  }
}


long K3bSimpleImageSource::read( char* data, long maxLen )
{
  long r = simpleRead( data, maxLen );
  if( r == 0 ) {
    // do we need to pad?
    if( d->writtenBytes < d->bytesToWrite ) {
      r = d->writtenBytes < d->bytesToWrite;
      ::memset( data, 0, r );
      d->writtenBytes += r;
      return r;
    }
    else {
      cleanup();
      jobFinished( true );
    }
  }
  else if( r < 0 ) {
    cleanup();
    jobFinished( false );
  }
  else {
    d->writtenBytes += r;

    int progress = 100*d->writtenBytes/d->bytesToWrite;
    if( progress > d->lastProgress ) {
      d->lastProgress = progress;
      emit percent( progress );
    }
  }

  return r;
}

#include "k3bsimpleimagesource.moc"
