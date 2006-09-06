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

#include "k3bisoimageverificationjob.h"

#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bmd5job.h>
#include <k3bglobals.h>

#include <kdebug.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kio/netaccess.h>

#include <qcstring.h>
#include <qapplication.h>


class K3bIsoImageVerificationJob::Private
{
public:
  Private()
    : md5Job(0),
      device(0) {
  }

  bool canceled;
  bool needToCalcMd5;
  K3bMd5Job* md5Job;
  K3bDevice::Device* device;
  QString imageFileName;

  KIO::filesize_t imageSize;

  QCString imageMd5Sum;
};


K3bIsoImageVerificationJob::K3bIsoImageVerificationJob( K3bJobHandler* hdl, QObject* parent, const char* name )
  : K3bJob( hdl, parent, name )
{
  d = new Private();

  d->md5Job = new K3bMd5Job( this );
  connect( d->md5Job, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( d->md5Job, SIGNAL(percent(int)), this, SLOT(slotMd5JobProgress(int)) );
  connect( d->md5Job, SIGNAL(finished(bool)), this, SLOT(slotMd5JobFinished(bool)) );
}


K3bIsoImageVerificationJob::~K3bIsoImageVerificationJob()
{
  delete d;
}


void K3bIsoImageVerificationJob::cancel()
{
  d->canceled = true;
  if( d->md5Job )
    d->md5Job->cancel();
}


void K3bIsoImageVerificationJob::setDevice( K3bDevice::Device* dev )
{
  d->device = dev;
}


void K3bIsoImageVerificationJob::setImageFileName( const QString& f )
{
  d->imageFileName = f;
  d->imageMd5Sum.truncate(0);
}


void K3bIsoImageVerificationJob::setImageMD5Sum( const QCString& md5 )
{
  d->imageMd5Sum = md5;
}


void K3bIsoImageVerificationJob::start()
{
  jobStarted();

  d->canceled = false;
  d->needToCalcMd5 = d->imageMd5Sum.isEmpty();

  // first we need to reload and mount the device
  emit newTask( i18n("Reloading the medium") );

  connect( K3bDevice::reload( d->device ), SIGNAL(finished(bool)),
	     this, SLOT(slotMediaReloaded(bool)) );
}


void K3bIsoImageVerificationJob::slotMediaReloaded( bool success )
{
  if( !success )
    blockingInformation( i18n("Please reload the medium and press 'ok'"),
			 i18n("Unable to Close the Tray") );

  if( d->needToCalcMd5 ) {
    emit newTask( i18n("Reading original data") );
    
    // start it
    d->md5Job->setFile( d->imageFileName );
    d->md5Job->start();
  }
  else {
    slotMd5JobFinished( true );
  }
}


void K3bIsoImageVerificationJob::slotMd5JobFinished( bool success )
{
  if( d->canceled ) {
    finishVerification(false);
  }

  if( success ) {

    if( !d->imageMd5Sum.isEmpty() ) {
      // compare the two sums
      if( d->imageMd5Sum != d->md5Job->hexDigest() ) {
	emit infoMessage( i18n("Written data differs from original."), ERROR );
	finishVerification(false);
      }
      else {
	emit infoMessage( i18n("Written data verified."), SUCCESS );
	finishVerification(true);
      }
    }
    else {
      
      d->imageMd5Sum = d->md5Job->hexDigest();

      //
      // now we need to calculate the md5sum of the written image
      // since it is possible that the image has been padded while writing we need
      // the image filesize to make sure we do not compare too much data and thus get
      // a wrong result
      //

      d->imageSize = K3b::filesize( d->imageFileName );
      if( d->imageSize == (KIO::filesize_t)0 ) {
	emit infoMessage( i18n("Unable to determine size of file %1.").arg(d->imageFileName), ERROR );
	finishVerification(false);
      }
      else if( !d->device->open() ) {
	emit infoMessage( i18n("Unable to open device %1.").arg(d->device->blockDeviceName()), ERROR );
	finishVerification(false);
      }
      else {
	// start the written data check
	emit newTask( i18n("Reading written data") );
	d->md5Job->setDevice( d->device );
	d->md5Job->setMaxReadSize( d->imageSize );
	d->md5Job->start();
      }
    }
  }
  else {
    // The md5job emitted an error message. So there is no need to do this again
    finishVerification(false);
  }
}


void K3bIsoImageVerificationJob::slotMd5JobProgress( int p )
{
  if( d->needToCalcMd5 && !d->imageMd5Sum.isEmpty() )
    emit percent( 50 + p/2 );
  else if( d->needToCalcMd5 )
    emit percent( p/2 );
  else
    emit percent( p );
  emit subPercent( p );
}


void K3bIsoImageVerificationJob::finishVerification( bool success )
{
  // close the device
  if( d->device )
    d->device->close();

  if( d->canceled )
    emit canceled();

  jobFinished(success);
}

#include "k3bisoimageverificationjob.moc"
