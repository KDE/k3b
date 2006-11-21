/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdataverifyingjob.h"
#include "k3bdatadoc.h"
#include "k3bisoimager.h"

#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bdevicehandler.h>
#include <k3bmd5job.h>
#include <k3bdatatrackreader.h>
#include <k3bpipe.h>
#include <k3bglobals.h>
#include <k3biso9660.h>
#include <k3biso9660backend.h>

#include <kdebug.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>

#include <qcstring.h>
#include <qapplication.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>


class K3bDataVerifyingJob::Private
{
public:
  Private() 
    : running(false),
      canceled(false),
      md5Job(0),
      doc(0),
      device(0),
      dataTrackReader(0),
      usedMultiSessionMode( K3bDataDoc::NONE ) {
  }

  bool running;
  bool canceled;
  bool success;
  K3bMd5Job* md5Job;
  K3bDataDoc* doc;
  K3bDevice::Device* device;
  K3bDevice::DiskInfo diskInfo;
  K3bDevice::Toc toc;
  K3bDataTrackReader* dataTrackReader;

  K3bIsoImager* imager;

  int usedMultiSessionMode;

  int lastProgress;

  K3bPipe comm;
};


K3bDataVerifyingJob::K3bDataVerifyingJob( K3bJobHandler* hdl, QObject* parent, const char* name )
  : K3bJob( hdl, parent, name )
{
  d = new Private();
}


K3bDataVerifyingJob::~K3bDataVerifyingJob()
{
  delete d;
}


bool K3bDataVerifyingJob::active() const
{
  return d->running;
}


void K3bDataVerifyingJob::start()
{
  d->canceled = false;
  d->running = true;

  jobStarted();

  emit newTask( i18n("Preparing Data") );
  
  // first we need to reload the device
  emit newSubTask( i18n("Reloading the medium") );
  
  connect( K3bDevice::reload( d->device ), SIGNAL(finished(bool)),
	   this, SLOT(slotMediaReloaded(bool)) );
}


void K3bDataVerifyingJob::slotMediaReloaded( bool success )
{
  if( d->canceled ) {
    finishVerification( false );
  }
  else {
    if( !success )
      blockingInformation( i18n("Please reload the medium and press 'ok'"),
			   i18n("Unable to close the tray") );

    emit newSubTask( i18n("Reading TOC") );
    
    connect( K3bDevice::diskInfo( d->device ), SIGNAL(finished(K3bDevice::DeviceHandler*)),
	     this, SLOT(slotTocRead(K3bDevice::DeviceHandler*)) );
  }
}


void K3bDataVerifyingJob::slotTocRead( K3bDevice::DeviceHandler* dh )
{
  d->toc = dh->toc();
  d->diskInfo = dh->diskInfo();

  if( d->canceled ) {
    finishVerification(false);
  }
  else if( !dh->success() ) {
    emit infoMessage( i18n("Reading TOC failed."), ERROR );
    finishVerification(false);
  }
//   else if( ( dh->diskInfo().mediaType() & (K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR) ) &&
// 	   ( d->usedMultiSessionMode == K3bDataDoc::CONTINUE || d->usedMultiSessionMode == K3bDataDoc::FINISH ) ) {
//     emit infoMessage( i18n("Sorry, no data verification if growing sessions on DVD+RW and DVD-RW media"), ERROR );
//     finishVerification(false);
//   }
  else {
    emit newTask( i18n("Verifying written data") );

    // initialize some variables
    d->lastProgress = 0;

    // initialize the job
    if( !d->md5Job ) {
      d->md5Job = new K3bMd5Job( this );
      connect( d->md5Job, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
      //      connect( d->md5Job, SIGNAL(percent(int)), this, SLOT(slotMd5JobProgress(int)) );
      connect( d->md5Job, SIGNAL(finished(bool)), this, SLOT(slotMd5JobFinished(bool)) );
    }

    // start the verification
    readWrittenChecksum();
  }
}


void K3bDataVerifyingJob::readWrittenChecksum()
{
  if( !d->dataTrackReader ) {
    d->dataTrackReader = new K3bDataTrackReader( this, this );
    connect( d->dataTrackReader, SIGNAL(infoMessage(const QString&, int)), 
	     this, SIGNAL(infoMessage(const QString&, int)) );
    connect( d->dataTrackReader, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
    connect( d->dataTrackReader, SIGNAL(percent(int)), this, SLOT(slotDataTrackReaderProgress(int)) );
    connect( d->dataTrackReader, SIGNAL(finished(bool)), this, SLOT(slotDataTrackReaderFinished(bool)) );
  }

  // create fd pair for imager <-> md5job communication
  if( !d->comm.open() ) {
    finishVerification( false );
    return;
  }

  emit newSubTask( i18n("Reading written data") );

  d->dataTrackReader->setDevice( d->device );
  d->dataTrackReader->setSectorSize( K3bDataTrackReader::MODE1 );

  if( ( d->diskInfo.mediaType() & (K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR) ) &&
      ( d->usedMultiSessionMode == K3bDataDoc::CONTINUE || d->usedMultiSessionMode == K3bDataDoc::FINISH ) ) {
    //
    // we always have just one track and the filesystem has been grown. Thus, the complete filesystem to compare 
    // can be found on the DVD at the end.
    //

    // force the backend since we don't need decryption
    // which just slows down the whole process
    K3bIso9660 iso( new K3bIso9660DeviceBackend( d->device ) );
    if( !iso.open() ) {
      emit infoMessage( i18n("Failed to read file system."), ERROR );
      finishVerification( false );
      return;
    }

    int firstSector = iso.primaryDescriptor().volumeSpaceSize - imageSize();
    d->dataTrackReader->setSectorRange( firstSector,
					iso.primaryDescriptor().volumeSpaceSize -1 );
  }
  else {
    //
    // we always read the last track
    // and use the size from the project instead of the toc since
    // TAO recorded tracks have two run-out sectors and this way we do not
    // have to care about those
    //
    d->dataTrackReader->setSectorRange( d->toc.last().firstSector(),
					d->toc.last().firstSector() + imageSize() - 1 );
  }

  d->dataTrackReader->writeToFd( d->comm.in() );
  d->md5Job->setFd( d->comm.out() );
  d->md5Job->start();
  d->dataTrackReader->start();
}


void K3bDataVerifyingJob::slotDataTrackReaderProgress( int p )
{
  if( p > d->lastProgress ) {
    d->lastProgress = p;
    emit percent( p );
  }
}


void K3bDataVerifyingJob::slotDataTrackReaderFinished( bool success )
{
  d->comm.closeIn();
  //  d->md5Job->stop();

  // FIXME: if( !success ) ...
}


void K3bDataVerifyingJob::cancel()
{
  if( active() ) {
    d->canceled = true;
    if( d->md5Job )
      d->md5Job->cancel();
    if( d->dataTrackReader )
      d->dataTrackReader->cancel();
  }
}


void K3bDataVerifyingJob::setImager( K3bIsoImager* imager )
{
  d->imager = imager;
}


void K3bDataVerifyingJob::setDoc( K3bDataDoc* doc )
{
  d->doc = doc;
  if( d->device == 0 )
    d->device = doc->burner();
}


void K3bDataVerifyingJob::setDevice( K3bDevice::Device* dev )
{
  d->device = dev;
}


void K3bDataVerifyingJob::setUsedMultisessionMode( int mode )
{
  d->usedMultiSessionMode = mode;
}


void K3bDataVerifyingJob::slotMd5JobFinished( bool success )
{
  if( !d->dataTrackReader->active() ) {
    d->comm.close();
    if( d->canceled || !success ) {
      finishVerification(false);
    }
    else {
      compareChecksums();
    }
  }
}


void K3bDataVerifyingJob::compareChecksums()
{
  if( d->imager->checksum() == d->md5Job->hexDigest() ) {
    emit infoMessage( i18n("Written data verified."), SUCCESS );
    finishVerification(true);
  }
  else {
    emit infoMessage( i18n("Written data differs from original."), ERROR );
    kdDebug() << "(K3bDataVerifyingJob) original: " << d->imager->checksum()
	      << " burned: " << d->md5Job->hexDigest() << endl;
    finishVerification(false);
  }
}


void K3bDataVerifyingJob::finishVerification( bool success )
{
  d->success = success;
  d->running = false;

  if( d->canceled ) {
    emit canceled();
  }

  jobFinished(d->success);
}


int K3bDataVerifyingJob::imageSize() const
{
  if( d->doc->onTheFly() )
    return d->imager->size();
  else
    return K3b::filesize( d->doc->tempDir() )/2048;
}

#include "k3bdataverifyingjob.moc"
