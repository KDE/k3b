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
#include "k3bfileitem.h"
#include "k3bbootitem.h"
#include "k3bdiritem.h"
#include "k3bisooptions.h"

#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bdevicehandler.h>
#include <k3bmd5job.h>
#include <k3biso9660.h>

#include <kdebug.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>

#include <qcstring.h>
#include <qapplication.h>


class K3bDataVerifyingJob::Private
{
public:
  Private() 
    : running(false),
      canceled(false),
      md5Job(0),
      doc(0),
      device(0),
      iso9660(0),
      currentItem(0) {
  }

  bool running;
  bool canceled;
  bool success;
  K3bMd5Job* md5Job;
  K3bDataDoc* doc;
  K3bDevice::Device* device;
  K3bIso9660* iso9660;
  K3bDataDoc::MultiSessionMode usedMultiSessionMode;

  K3bDataItem* currentItem;
  bool originalCalculated;
  KIO::filesize_t alreadyCheckedData;
  QCString originalMd5Sum;
  bool filesDiffer;
  int lastProgress;
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

  // TODO: do not compare the files from old sessions
  // TODO: do not use doc->size() but calculate the sum of all filesizes in advance (mainly because of the above)
  
  // first we need to reload the device
  emit newTask( i18n("Reloading the media") );
  
  connect( K3bDevice::reload( d->device ), SIGNAL(finished(bool)),
	   this, SLOT(slotMediaReloaded(bool)) );
}


void K3bDataVerifyingJob::slotMediaReloaded( bool success )
{
  if( d->canceled ) {
    emit canceled();
    finishVerification( false );
  }
  else {
    if( !success )
      blockingInformation( i18n("Please reload the medium and press 'ok'"),
			   i18n("Unable to close the tray") );

    emit newTask( i18n("Reading TOC") );
    
    connect( K3bDevice::toc( d->device ), SIGNAL(finished(K3bDevice::DeviceHandler*)),
	     this, SLOT(slotTocRead(K3bDevice::DeviceHandler*)) );
  }
}


void K3bDataVerifyingJob::slotTocRead( K3bDevice::DeviceHandler* dh )
{
  if( d->canceled ) {
    emit canceled();
    finishVerification(false);
  }
  else if( !dh->success() ) {
    emit infoMessage( i18n("Reading TOC failed."), ERROR );
    finishVerification(false);
  }
  else {
    emit newTask( i18n("Verifying written data") );

    delete d->iso9660;
    unsigned long startSec = 0;
    if( d->usedMultiSessionMode == K3bDataDoc::CONTINUE ||
	d->usedMultiSessionMode == K3bDataDoc::FINISH ) {
      // in this case we only compare the files from the new session
      K3bDevice::Toc::const_iterator it = dh->toc().end();
      --it; // this is valid since there is at least one data track
      while( it != dh->toc().begin() && (*it).type() != K3bDevice::Track::DATA )
	--it;
      startSec = (*it).firstSector().lba();
    }

    d->iso9660 = new K3bIso9660( d->device, startSec );
    if( !d->iso9660->open() ) {
      emit infoMessage( i18n("Unable to read ISO9660 filesystem."), ERROR );
      finishVerification(false);
    }
    else {
      // initialize some variables
      d->currentItem = d->doc->root();
      d->originalCalculated = false;
      d->alreadyCheckedData = 0;
      d->lastProgress = 0;
      d->filesDiffer = false;

      // initialize the job
      if( !d->md5Job ) {
	d->md5Job = new K3bMd5Job( this );
	connect( d->md5Job, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
	connect( d->md5Job, SIGNAL(percent(int)), this, SLOT(slotMd5JobProgress(int)) );
	connect( d->md5Job, SIGNAL(finished(bool)), this, SLOT(slotMd5JobFinished(bool)) );
      }

      compareNextFile();
    }
  }
}


void K3bDataVerifyingJob::compareNextFile()
{
  // we only compare files which have been written to cd
  do {
    d->currentItem = d->currentItem->nextSibling();
  } while( d->currentItem && 
	   (!d->currentItem->isFile() || 
	    !d->currentItem->writeToCd() || 
	    d->currentItem->isFromOldSession() ||
	    (d->currentItem->isSymLink() && !d->doc->isoOptions().followSymbolicLinks()) ) );
  
  d->originalCalculated = false;
  if( d->currentItem ) {
    d->md5Job->setFile( d->currentItem->localPath() );
    d->md5Job->start();
  }
  else {
    // all files have been compared
    if( d->filesDiffer ) {
      finishVerification( false );
    }
    else {
      emit infoMessage( i18n("Written data verified."), SUCCESS );
      finishVerification( true );
    }
  }
}


void K3bDataVerifyingJob::cancel()
{
  if( active() ) {
    d->canceled = true;
    if( d->md5Job )
      d->md5Job->cancel();
  }
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


void K3bDataVerifyingJob::setUsedMultiSessionMode( K3bDataDoc::MultiSessionMode usedMultiSessionMode )
{
  d->usedMultiSessionMode = usedMultiSessionMode;
}


void K3bDataVerifyingJob::slotMd5JobFinished( bool success )
{
  if( d->canceled ) {
    emit canceled();
    finishVerification(false);
  }

  if( success ) {
    if( d->originalCalculated ) {
      // compare the two md5sums
      if( d->originalMd5Sum != d->md5Job->hexDigest() ) {

	bool fileDiffers = true;

	//
	// there is one case when it's ok that original and written file differ:
	// a boot image with enabled boot info table. In this case mkisofs will
	// modify the source (which is a temporary copy of the original we compare to)
	// 
	if( K3bBootItem* bootItem = dynamic_cast<K3bBootItem*>( d->currentItem ) ) {
	  if( bootItem->bootInfoTable() ) {
	    fileDiffers = false;
	  }
	}

	if( fileDiffers ) {
	  d->filesDiffer = true;
	  emit infoMessage( i18n("%1 differs.").arg( d->currentItem->k3bPath() ), ERROR );
	  emit debuggingOutput( "Invalid files", 
				QString("%1 (%2)")
				.arg( d->currentItem->k3bPath() )
				.arg( d->currentItem->iso9660Path() ) );
	}
      }

      d->alreadyCheckedData += d->currentItem->size();

      // go on with the next file
      compareNextFile();
    }
    else {
      d->originalCalculated = true;
      d->originalMd5Sum = d->md5Job->hexDigest();
      const K3bIso9660File* isoFile = 
	dynamic_cast<const K3bIso9660File*>(d->iso9660->firstIsoDirEntry()->iso9660Entry( d->currentItem->iso9660Path() ) );
      if( isoFile ) {
	d->md5Job->setFile( isoFile );
	d->md5Job->start();
      }
      else {
	kdDebug() << "(K3bDataVerifyingJob) could not find " 
		  << d->currentItem->iso9660Path()
		  << " in filesystem." << endl;
	emit infoMessage( i18n("Could not find file %1.").arg(d->currentItem->writtenName()), ERROR );
	d->iso9660->debug();
	finishVerification(false);
      }
    }
  }
  else {
    // The md5job emitted an error message. So there is no need to do this again
    finishVerification(false);
  }
}


void K3bDataVerifyingJob::slotMd5JobProgress( int p )
{
  double percentCurrentFile = (double)p/2.0;
  if( d->originalCalculated )
    percentCurrentFile += 50.0;

  double doneCurrentFile = (double)d->currentItem->size()*percentCurrentFile/100.0;
  int newProgress = (int)( 100.0 * ((double)d->alreadyCheckedData + doneCurrentFile) / (double)d->doc->burningSize() );

  if( newProgress > d->lastProgress ) {
    d->lastProgress = newProgress;
    emit percent( newProgress );
  }
}


void K3bDataVerifyingJob::finishVerification( bool success )
{
  d->success = success;
  d->running = false;
  if( d->iso9660 ) 
    d->iso9660->close();
  jobFinished(d->success);
}

#include "k3bdataverifyingjob.moc"
