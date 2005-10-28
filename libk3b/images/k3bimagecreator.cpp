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

#include "k3bimagecreator.h"
#include "k3bimage.h"
#include "k3bimagesource.h"

#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bglobals.h>

#include <qtimer.h>

#include <klocale.h>
#include <kdebug.h>



class K3bImageCreator::Private
{
public:
  QString imagePath;
  bool canceled;

  KIO::filesize_t writtenTrackBytes;
  KIO::filesize_t writtenTocBytes;
  unsigned int currentTrack;
  unsigned int currentSession;
  QByteArray buffer;

  int lastTrackProgress;
  int lastTocProgress;
};


K3bImageCreator::K3bImageCreator( K3bJobHandler* hdl, QObject* parent )
  : K3bJob( hdl, parent ),
    m_image( 0 )
{
  d = new Private;
}


K3bImageCreator::~K3bImageCreator()
{
  delete m_image;
  delete d;
}


void K3bImageCreator::setImageFilename( const QString& filename )
{
  d->imagePath = filename;
}


void K3bImageCreator::start()
{
  kdDebug() << k_funcinfo << endl;

  jobStarted();

  d->canceled = false;
  d->writtenTocBytes = 0ULL;
  d->writtenTrackBytes = 0ULL;
  d->currentTrack = 1;
  d->currentSession = 1;
  d->lastTocProgress = 0;

  // connect source
  source()->disconnect( this );
  connect( source(), SIGNAL(tocReady(bool)), SLOT(slotTocReady(bool)) );
  connect( source(), SIGNAL(infoMessage(const QString&, int)), SIGNAL(infoMessage(const QString&, int)) );

  // FIXME: emit some newTask
  emit newTask( "Doing stuff with the toc (I am a temp message, REPLACEME)" );

  // we start by determining the toc
  source()->determineToc();
}


void K3bImageCreator::slotTocReady( bool success )
{
  kdDebug() << k_funcinfo << endl;

  if( success ) {

    if( !m_image )
      m_image = new K3bImage();

    if( !m_image->open( d->imagePath, IO_WriteOnly ) ) {
      emit infoMessage( i18n("Unable to open '%1' for writing.").arg(d->imagePath), ERROR );
      jobFinished( false );
      return;
    }

    m_image->setToc( source()->toc() );
    m_image->setCdText( source()->cdText() );

    if( !startNextTrack() ) {
      jobFinished( false );
      return;
    }

    source()->writeToFd( -1 );
    source()->start( 1 );
    
    // source->start() may already result in a failure
    if( active() )
      QTimer::singleShot( 0, this, SLOT(slotReadAsync()) );
  }
  else {
    // FIXME: should the source emit a proper error message or we?
    jobFinished( false );
  }
}


void K3bImageCreator::cancel()
{
  if( active() ) {
    d->canceled = true;

    source()->cancel();

    // FIXME: do something
    // FIXME: remove the image file

    emit canceled();
    jobFinished( false );
  }
}


bool K3bImageCreator::startNextTrack()
{
  kdDebug() << k_funcinfo << endl;

  K3b::SectorSize secSize = source()->sectorSize( d->currentTrack );

  d->buffer.resize( 10*secSize );

  //
  // In case the next track is in another session we have to restart the source
  //
  if( (int)d->currentSession < m_image->toc()[d->currentTrack-1].session() )
    source()->start( ++d->currentSession );

  if( !m_image->writeTrack( d->currentTrack, secSize ) ) {
    emit infoMessage( i18n("Internal error: Unable to open track for writing."), ERROR );
    // FIXME: remove the image file
    jobFinished( false );
    return false;
  }

  emit nextTrack( d->currentTrack, m_image->toc().count() );

  // FIXME: some subtask or something including cdtext if available

  d->writtenTrackBytes = 0ULL;

  return true;
}


void K3bImageCreator::slotReadAsync()
{
  if( d->canceled || !active() )
    return;

  int maxSize = d->buffer.size();
  if( d->writtenTrackBytes + (KIO::filesize_t)maxSize > source()->trackSize( d->currentTrack ) )
    maxSize = (int)(source()->trackSize( d->currentTrack ) - d->writtenTrackBytes);

  long read = source()->read( d->buffer.data(), maxSize );

  if( read < 0 ) {
    // FIXME: remove the image file
    jobFinished( false );
    return;
  }

  if( !m_image->writeTrackData( d->buffer.data(), read ) ) {
    emit infoMessage( i18n("Internal error: Unable to write track data."), ERROR );
    // FIXME: remove the image file
    jobFinished( false );
    return;
  }

  d->writtenTrackBytes += (KIO::filesize_t)read;
  d->writtenTocBytes += (KIO::filesize_t)read;

  // calculate progress
  int trackProgress = (int)(100ULL*d->writtenTrackBytes/source()->trackSize( d->currentTrack ));
  int tocProgress = (int)(100ULL*d->writtenTocBytes/source()->tocSize());
  if( trackProgress > d->lastTrackProgress ) {
    emit subPercent( trackProgress );
    d->lastTrackProgress = trackProgress;
  }
  if( tocProgress > d->lastTocProgress ) {
    emit percent( tocProgress );
    d->lastTocProgress = tocProgress;
  }

  // FIXME: processes size signals

  if( d->writtenTrackBytes == source()->trackSize( d->currentTrack ) ) {
    d->currentTrack++;

    // do we have to open another track file?
    if( d->currentTrack <= m_image->toc().count() ) {
      if( !startNextTrack() ) {
	source()->cancel();
	// FIXME: remove the image file
	jobFinished( false );
	return;
      }
    }

    // finished!
    else {
      m_image->close();
      jobFinished( true );
      return;
    }
  }

  QTimer::singleShot( 0, this, SLOT(slotReadAsync()) );
}


#include "k3bimagecreator.moc"
