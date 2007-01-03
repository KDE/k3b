/* 
 *
 * $Id$
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bmd5job.h"
#include <k3biso9660.h>
#include <k3bglobals.h>
#include <k3bdevice.h>
#include <k3bfilesplitter.h>

#include <kmdcodec.h>
#include <klocale.h>
#include <kdebug.h>
#include <kio/netaccess.h>

#include <qtimer.h>
#include <qcstring.h>
#include <qsocketnotifier.h>

#include <unistd.h>


class K3bMd5Job::K3bMd5JobPrivate
{
public:
  K3bMd5JobPrivate()
    : fileDes(-1),
      fdNotifier(0),
      finished(true),
      data(0),
      isoFile(0),
      maxSize(0),
      lastProgress(0) {
  }

  KMD5 md5;
  K3bFileSplitter file;
  QTimer timer;
  QString filename;
  int fileDes;
  K3bDevice::Device* device;
  QSocketNotifier* fdNotifier;

  bool finished;
  char* data;
  const K3bIso9660File* isoFile;

  unsigned long long maxSize;
  unsigned long long readData;

  int lastProgress;

  KIO::filesize_t imageSize;

  static const int BUFFERSIZE = 2048*10;
};


K3bMd5Job::K3bMd5Job( K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bJob( jh, parent, name )
{
  d = new K3bMd5JobPrivate;
  d->data = new char[K3bMd5JobPrivate::BUFFERSIZE];
  connect( &d->timer, SIGNAL(timeout()), 
	   this, SLOT(slotUpdate()) );
}


K3bMd5Job::~K3bMd5Job()
{
  delete [] d->data;
  delete d;
}


void K3bMd5Job::start()
{
  cancel();

  jobStarted();
  d->readData = 0;

  if( d->isoFile ) {
    d->imageSize = d->isoFile->size();
  }
  else if( !d->filename.isEmpty() ) {
    if( !QFile::exists( d->filename ) ) {
      emit infoMessage( i18n("Could not find file %1").arg(d->filename), ERROR );
      jobFinished(false);
      return;
    }

    d->file.setName( d->filename );
    if( !d->file.open( IO_ReadOnly ) ) {
      emit infoMessage( i18n("Could not open file %1").arg(d->filename), ERROR );
      jobFinished(false);
      return;
    }

    d->imageSize = K3b::filesize( KURL::fromPathOrURL(d->filename) );
  }
  else
    d->imageSize = 0;

  if( d->device ) {
    //
    // Let the drive determine the optimal reading speed
    //
    d->device->setSpeed( 0xffff, 0xffff );
  }
    
  d->md5.reset();
  d->finished = false;
  if( d->fileDes != -1 )
    setupFdNotifier();
  else
    d->timer.start(0);
}


void K3bMd5Job::setupFdNotifier()
{
  // the QSocketNotifier will fire once the fd is closed
  delete d->fdNotifier;
  d->fdNotifier = new QSocketNotifier( d->fileDes, QSocketNotifier::Read, this );
  connect( d->fdNotifier, SIGNAL(activated(int)), this, SLOT(slotUpdate()) );
  d->fdNotifier->setEnabled( true );
}


void K3bMd5Job::cancel()
{
  if( !d->finished ) {
    stopAll();

    emit canceled();
    jobFinished( false );
  }
}


void K3bMd5Job::setFile( const QString& filename )
{
  d->filename = filename;
  d->isoFile = 0;
  d->fileDes = -1;
  d->device = 0;
}


void K3bMd5Job::setFile( const K3bIso9660File* file )
{
  d->isoFile = file;
  d->fileDes = -1;
  d->filename.truncate(0);
  d->device = 0;
}


void K3bMd5Job::setFd( int fd )
{
  d->fileDes = fd;
  d->filename.truncate(0);
  d->isoFile = 0;
  d->device = 0;
}


void K3bMd5Job::setDevice( K3bDevice::Device* dev )
{
  d->device = dev;
  d->fileDes = -1;
  d->filename.truncate(0);
  d->isoFile = 0;
}


void K3bMd5Job::setMaxReadSize( unsigned long long size )
{
  d->maxSize = size;
}


void K3bMd5Job::slotUpdate()
{
  if( !d->finished ) {

    // determine bytes to read
    unsigned int readSize = K3bMd5JobPrivate::BUFFERSIZE;
    if( d->maxSize > 0 )
      readSize = QMIN( readSize, d->maxSize - d->readData );

    if( readSize <= 0 ) {
      kdDebug() << "(K3bMd5Job) reached max size of " << d->maxSize << ". Stopping." << endl;
      stopAll();
      emit percent( 100 );
      jobFinished(true);
    }
    else {
      int read = 0;

      //
      // read from the iso9660 file
      //
      if( d->isoFile ) {
	read = d->isoFile->read( d->readData, d->data, readSize );
      }

      //
      // read from the device
      //
      else if( d->device ) {
	//
	// when reading from a device we always read multiples of 2048 bytes.
	// Only the last sector may not be used completely.
	//
	unsigned long sector = d->readData/2048;
	unsigned int sectorCnt = QMAX( readSize/2048, 1 );
	read = -1;
	if( d->device->read10( reinterpret_cast<unsigned char*>(d->data),
			       sectorCnt*2048,
			       sector,
			       sectorCnt ) )
	  read = QMIN( readSize, sectorCnt*2048 );
      }

      //
      // read from the file
      //
      else if( d->fileDes < 0 ) {
	read = d->file.readBlock( d->data, readSize );
      }

      //
      // reading from the file descriptor
      //
      else {
	read = ::read( d->fileDes, d->data, readSize );
      }

      if( read < 0 ) {
	emit infoMessage( i18n("Error while reading from file %1").arg(d->filename), ERROR );
	stopAll();
	jobFinished(false);
      }
      else if( read == 0 ) {
	kdDebug() << "(K3bMd5Job) read all data. Total size: " << d->readData << ". Stopping." << endl;
	stopAll();
	emit percent( 100 );
	jobFinished(true);
      }
      else {
	d->readData += read;
	d->md5.update( d->data, read );
	int progress = 0;
	if( d->isoFile || !d->filename.isEmpty() )
	  progress = (int)((double)d->readData * 100.0 / (double)d->imageSize);
	else if( d->maxSize > 0 )
	  progress = (int)((double)d->readData * 100.0 / (double)d->maxSize);

	if( progress != d->lastProgress ) {
	  d->lastProgress = progress;
	  emit percent( progress );
	}
      }
    }
  }
}


QCString K3bMd5Job::hexDigest()
{
  if( d->finished )
    return d->md5.hexDigest();
  else
    return "";
}


QCString K3bMd5Job::base64Digest()
{
  if( d->finished )
    return d->md5.base64Digest();  
  else
    return "";

}


void K3bMd5Job::stop()
{
  stopAll();
  jobFinished( true );
}


void K3bMd5Job::stopAll()
{
  if( d->fdNotifier )
    d->fdNotifier->setEnabled( false );
  if( d->file.isOpen() )
    d->file.close();
  d->timer.stop();
  d->finished = true;
}

#include "k3bmd5job.moc"
