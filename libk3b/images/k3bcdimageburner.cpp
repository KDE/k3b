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

#include "k3bcdimageburner.h"
#include "k3bimagesource.h"

#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>
#include <k3binffilewriter.h>
#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3btrack.h>

#include <klocale.h>


class K3bCDImageBurner::Private
{
public:
  Private()
    : writerJob(0) {
  }

  K3b::WritingMode usedWritingMode;
  K3b::WritingApp usedWritingApp;

  K3bDevice::Toc toc;
  K3bDevice::CdText cdText;

  K3bAbstractWriter* writerJob;

  unsigned int currentSession;
};



K3bCDImageBurner::K3bCDImageBurner( K3bJobHandler* hdl, QObject* parent )
  : K3bImageBurner( hdl, parent )
{
  d = new Private();
}


K3bCDImageBurner::~K3bCDImageBurner()
{
  delete d;
}


void K3bCDImageBurner::startInternal()
{
  //
  // first of all cache the toc since we need it quite often
  // and it might be created on-the-fly in the source. So this
  // is just for simple speedup.
  //
  d->toc = source()->toc();
  d->cdText = source()->cdText();

  d->currentSession = 1;

  // FIXME:

  // 1. analyse toc
  // 2. determine usedWritingApp and usedWritingMode
  // 3. create writer class
  // 4. write inf files or toc file and cdtext file
  // 5. connect writer class + source->setFdToWriteTo
  // 6. start writer job
  // 7. start source

  setupBurningParameters();
  setupBurningJob();

  // FIXME: what about adding a session to an appendable CD? How should we handle that?
  //        As a first session on an appendable CD or as session n. The latter would mean
  //        that we had to adapt K3bDevice::Toc::sessions() to allow the first track to have
  //        a session number > 1.
  //        Maybe it's best to allow a special case where the session number of the first track
  //        is set to something above 1 (for example 2) since we would not use the exact session
  //        number anyway.
  //        Another possibility is to add another parameter...

  int mediaState = K3bDevice::STATE_EMPTY;
  if( d->currentSession > 1 && !simulate() )
    mediaState = K3bDevice::STATE_INCOMPLETE;

  emit newSubTask( i18n("Waiting for a medium") );
  if( waitForMedia( burnDevice(), 

// 		    d->usedMultiSessionMode == K3bDataDoc::CONTINUE ||
// 		    d->usedMultiSessionMode == K3bDataDoc::FINISH ?
// 		    K3bDevice::STATE_INCOMPLETE :
		    K3bDevice::STATE_EMPTY,
		    K3bDevice::MEDIA_WRITABLE_CD ) < 0 ) {
    cancel();
  }
  else {
    d->writerJob->start();
    source()->writeToFd( d->writerJob->fd() );
    source()->start( d->currentSession );
  }
}


void K3bCDImageBurner::setupBurningParameters()
{
  //
  // We try to use cdrecord if possible since it seems to work with the majority of CD writers while
  // cdrdao still has problems with some writers (although it has a way better interface)
  //
  // cdrecord is not able to write single-session CDs containing multiple data tracks on-the-fly. At least
  // I think so...
  //

  // FIXME

  // determine the writing mode
//   if( writingMode() == K3b::WRITING_MODE_AUTO ) {
//     if( burnDevice()->dao() && d->usedDataMode == K3b::MODE1 &&
// 	d->usedMultiSessionMode == K3bDataDoc::NONE )
//       d->usedWritingMode = K3b::DAO;
//     else
//       d->usedWritingMode = K3b::TAO;
//   }
//   else
//     d->usedWritingMode = d->doc->writingMode();


//   // cdrecord seems to have problems writing xa 1 disks in dao mode? At least on my system!
//   if( writingApp() == K3b::DEFAULT ) {
//     if( d->usedWritingMode == K3b::DAO ) {
//       if( d->usedMultiSessionMode != K3bDataDoc::NONE )
// 	d->usedWritingApp = K3b::CDRDAO;
//       else if( d->usedDataMode == K3b::MODE2 )
// 	d->usedWritingApp = K3b::CDRDAO;
//       else
// 	d->usedWritingApp = K3b::CDRECORD;
//     }
//     else
//       d->usedWritingApp = K3b::CDRECORD;
//   }
//   else
//     d->usedWritingApp = writingApp();


}


void K3bCDImageBurner::setupBurningJob()
{
  delete d->writerJob;

  //
  // setup the job
  //
  if( d->usedWritingApp == K3b::CDRECORD ) {
    // FIXME
  }
  else {
    // FIXME
  }

  //
  // connect the job
  //
  connect( d->writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( d->writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
  connect( d->writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( d->writerJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( d->writerJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( d->writerJob, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
  connect( d->writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( d->writerJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
  connect( d->writerJob, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
  connect( d->writerJob, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
  connect( d->writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( d->writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
}


void K3bCDImageBurner::cancelInternal()
{
  if( d->writerJob && d->writerJob->active() )
    d->writerJob->cancel();
  // FIXME: I don't think we should emit finished here. It does not make sense. If the K3bImageBurner
  //        is doing someting like determining the toc it should emit the finished signal itself. Otherwise
  //        we should handle it somewhere else. For example after waiting for a medium.
  else
    jobFinished(false);
}


// TODO: can we do some of the progress stuff generically in K3bImageBurner?
void K3bCDImageBurner::slotWriterNextTrack( int, int )
{
  // FIXME
}


void K3bCDImageBurner::slotWriterJobPercent( int p )
{
  // FIXME
}


void K3bCDImageBurner::slotWriterJobFinished( bool success )
{
  if( canceled() ) {
    cleanup();
    jobFinished( false );
    return;
  }

  if( success ) {
    // FIXME
    
    // handle multiple sessions
  }
  else {
    cleanup();
    // The writer job should have emitted a proper error message
    // FIXME: maybe we should check if it was the source which failed and not the writer?
    jobFinished( false );
  }
}


void K3bCDImageBurner::cleanup()
{
  // FIXME: remove temp files
}

#include "k3bcdimageburner.moc"
