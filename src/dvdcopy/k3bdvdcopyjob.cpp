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

#include "k3bdvdcopyjob.h"

#include <k3breadcdreader.h>
#include <k3bexternalbinmanager.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicehandler.h>
#include <device/k3bdiskinfo.h>
#include <k3bglobals.h>
#include <k3bcore.h>
#include <k3bemptydiscwaiter.h>
#include <k3bgrowisofswriter.h>
#include <k3breadcdreader.h>

#include <kdebug.h>
#include <klocale.h>
#include <kio/global.h>
#include <kmessagebox.h>

#include <qfile.h>
#include <qapplication.h>


class K3bDvdCopyJob::Private
{
public:
  Private() 
    : doneCopies(0),
      running(false),
      canceled(false),
      writerJob(0),
      readcdReader(0) {
  }

  int doneCopies;

  bool running;
  bool canceled;

  K3bGrowisofsWriter* writerJob;
  K3bReadcdReader* readcdReader;
  //  QFile bufferFile;
};


K3bDvdCopyJob::K3bDvdCopyJob( QObject* parent, const char* name )
  : K3bBurnJob( parent, name ),
    m_writerDevice(0),
    m_readerDevice(0),
    m_onTheFly(false),
    m_removeImageFiles(false),
    m_simulate(false),
    m_speed(1),
    m_copies(1),
    m_onlyCreateImage(false),
    m_writingMode( K3b::WRITING_MODE_AUTO )
{
  d = new Private();
}


K3bDvdCopyJob::~K3bDvdCopyJob()
{
  delete d;
}


void K3bDvdCopyJob::start()
{
  emit started();

  d->canceled = false;
  d->running = true;


  // TODO: check the cd size and warn the user if not enough space

  emit infoMessage( i18n("Checking source media..."), PROCESS );
  emit newSubTask( i18n("Checking source media") );

  connect( K3bCdDevice::sendCommand( K3bCdDevice::DeviceHandler::NG_DISKINFO, m_readerDevice ),
           SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
           this,
           SLOT(slotDiskInfoReady(K3bCdDevice::DeviceHandler*)) );
}


void K3bDvdCopyJob::slotDiskInfoReady( K3bCdDevice::DeviceHandler* dh )
{
  if( d->canceled ) {
    emit canceled();
    emit finished(false);
    d->running = false;
  }

  if( dh->ngDiskInfo().empty() || dh->ngDiskInfo().diskState() == K3bCdDevice::STATE_NO_MEDIA ) {
    emit infoMessage( i18n("No source media found."), ERROR );
    emit finished(false);
    d->running = false;
  }
  else {
    if( dh->ngDiskInfo().capacity() > K3b::Msf( 510*60*75 ) ) {
      kdDebug() << "(K3bDvdCopyJob) DVD too large." << endl;

      if( KMessageBox::warningYesNo( qApp->activeWindow(), 
				     i18n("The source DVD seems to be too large (%1) to make it's contents fit on "
					  "a normal writable DVD media which have a capacity of approximately "
					  "4.3 Gigabytes. Do you really want to continue?").arg(KIO::convertSize( dh->ngDiskInfo().capacity().mode1Bytes() ) ),
				     i18n("Source DVD too large") ) == KMessageBox::No ) {
	emit finished(false);
	d->running = false;
	return;
      }
    }

    // now we may really start.

    //      if( m_onlyCreateImage || !m_onTheFly ) {
    //        emit newTask( i18n("Reading image") );
    //        d->bufferFile.setName( m_imagePath );
    //        if( !d->bufferFile.open( IO_ReadOnly ) {
    //          emit infoMessage( i18n("Could not open %1 for writing").arg(m_imagePath), ERROR );
    //          emit finished(false);
    //          d->running = false;
    //          return;
    //        }
    //      }

    if( m_onlyCreateImage || !m_onTheFly ) {
      emit newTask( i18n("Creating DVD image") );
    }
    else if( m_onTheFly && !m_onlyCreateImage ) {
      prepareWriter();
      if( waitForDvd() ) {
	if( m_simulate )
	  emit newTask( i18n("Simulating DVD copy") );
	else
	  emit newTask( i18n("Writing DVD copy %1").arg(d->doneCopies+1) );

	d->writerJob->start();
      }
      else {
	emit finished(false);
	d->running = false;
	return;
      }
    }

    prepareReader();
    d->readcdReader->start();
  }
}


void K3bDvdCopyJob::cancel()
{
  if( d->running ) {
    d->canceled = true;
    if( d->readcdReader  )
      d->readcdReader->cancel();
    if( d->writerJob )
      d->writerJob->cancel();
  }
  else {
    kdDebug() << "(K3bDvdCopyJob) not running." << endl;
  }
}


void K3bDvdCopyJob::prepareReader()
{
  if( !d->readcdReader ) {
    d->readcdReader = new K3bReadcdReader( this );
    connect( d->readcdReader, SIGNAL(percent(int)), this, SLOT(slotReaderProgress(int)) );
    connect( d->readcdReader, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
    connect( d->readcdReader, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
    connect( d->readcdReader, SIGNAL(finished(bool)), this, SLOT(slotReaderFinished(bool)) );
    connect( d->readcdReader, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( d->readcdReader, SIGNAL(newTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( d->readcdReader, SIGNAL(debuggingOutput(const QString&, const QString&)), 
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }

  d->readcdReader->setReadDevice( m_readerDevice );
  d->readcdReader->setReadSpeed( 0 ); // MAX

  if( m_onTheFly && !m_onlyCreateImage )
    d->readcdReader->writeToFd( d->writerJob->fd() );
  else {
    d->readcdReader->writeToFd( -1 );
    d->readcdReader->setImagePath( m_imagePath );
  }
}


void K3bDvdCopyJob::prepareWriter()
{
  delete d->writerJob;

  d->writerJob = new K3bGrowisofsWriter( m_writerDevice, this );

  connect( d->writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( d->writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterProgress(int)) );
  connect( d->writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( d->writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( d->writerJob, SIGNAL(writeSpeed(int)), this, SIGNAL(writeSpeed(int)) );
  connect( d->writerJob, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
  //  connect( d->writerJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( d->writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( d->writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  // these do only make sense with DVD-R(W)
  d->writerJob->setSimulate( m_simulate );
  d->writerJob->setBurnSpeed( m_speed );
  
  if( m_onTheFly )
    d->writerJob->setImageToWrite( QString::null ); // write to stdin
  else
    d->writerJob->setImageToWrite( m_imagePath );
}


void K3bDvdCopyJob::slotReaderProgress( int p )
{
  emit subPercent( p );

  if( m_onlyCreateImage )
    emit percent( p );
  else if( !m_onTheFly )
    emit percent( p/2 );
}


void K3bDvdCopyJob::slotWriterProgress( int p )
{
  emit subPercent( p );

  if( m_onTheFly )
    emit percent( p );
  else
    emit percent( 50 + p/2 );
}


void K3bDvdCopyJob::slotReaderFinished( bool success )
{
  if( d->canceled ) {
    emit canceled();
    emit finished(false);
    d->running = false;
  }

  if( success ) {
    emit infoMessage( i18n("Successfully read source DVD."), STATUS );
    if( m_onlyCreateImage ) {
      emit finished(true);
      d->running = false;
    }
    else if( !m_onTheFly ) {
      prepareWriter();
      if( waitForDvd() )
	d->writerJob->start();
      else {
	removeImageFiles();
	emit finished(false);
	d->running = false;
      }
    }
  }
  else {
    emit finished(false);
    d->running = false;
  }
}


void K3bDvdCopyJob::slotWriterFinished( bool success )
{
  if( d->canceled ) {
    emit canceled();
    emit finished(false);
    d->running = false;
  }

  if( success ) {
    d->doneCopies++;

    emit infoMessage( i18n("Successfully written DVD copy %1.").arg(d->doneCopies), INFO );

    if( d->doneCopies < m_copies ) {

      if( waitForDvd() ) {
	emit newTask( i18n("Writing DVD copy %1").arg(d->doneCopies+1) );
	d->writerJob->start();
      }
      else {
	emit finished(false);
	d->running = false;
	return;
      }
      
      if( m_onTheFly ) {
	prepareReader();
	d->readcdReader->start();
      }
    }
    else {
      if( m_removeImageFiles )
	removeImageFiles();
      d->running = false;
      emit finished(true);
    }
  }
  else {
    removeImageFiles();
    d->running = false;
    emit finished(false);
  }
}


// this is basicly the same code as in K3bDvdJob... :(
bool K3bDvdCopyJob::waitForDvd()
{
  int mt = 0;
  if( m_writingMode == K3b::WRITING_MODE_INCR_SEQ || m_writingMode == K3b::DAO )
    mt = K3bCdDevice::MEDIA_DVD_RW_SEQ|K3bCdDevice::MEDIA_DVD_R_SEQ;
  else if( m_writingMode == K3b::WRITING_MODE_RES_OVWR ) // we treat DVD+R(W) as restricted overwrite media
    mt = K3bCdDevice::MEDIA_DVD_RW_OVWR|K3bCdDevice::MEDIA_DVD_PLUS_RW|K3bCdDevice::MEDIA_DVD_PLUS_R;
  else
    mt = K3bCdDevice::MEDIA_WRITABLE_DVD;

  int m = K3bEmptyDiscWaiter::wait( m_writerDevice, false, mt );

  if( m == -1 ) {
    cancel();
    return false;
  }
  
  if( m == 0 ) {
    emit infoMessage( i18n("Forced by user. Growisofs will be called without further tests."), INFO );
  }

  else {
    if( m & (K3bCdDevice::MEDIA_DVD_PLUS_RW|K3bCdDevice::MEDIA_DVD_PLUS_R) ) {
      if( m_simulate ) {
	if( KMessageBox::warningYesNo( qApp->activeWindow(),
				       i18n("K3b does not support simulation with DVD+R(W) media. "
					    "Do you really want to continue? The media will be written "
					    "for real."),
				       i18n("No simulation with DVD+R(W)") ) == KMessageBox::No ) {
	  cancel();
	  return false;
	}

	m_simulate = false;
      }
      
      if( m_speed > 0 ) {
	emit infoMessage( i18n("DVD+R(W) writers do take care of the writing speed themselves."), INFO );
	emit infoMessage( i18n("The K3b writing speed setting is ignored for DVD+R(W) media."), INFO );
      }

      if( m & K3bCdDevice::MEDIA_DVD_PLUS_RW ) {
	emit infoMessage( i18n("Writing DVD+RW."), INFO );
      }
    }
    else if( m & K3bCdDevice::MEDIA_DVD_RW_OVWR ) {
      emit infoMessage( i18n("Writing DVD-RW in restricted overwrite mode."), INFO );
    }
    else if( m & (K3bCdDevice::MEDIA_DVD_RW_SEQ|
		  K3bCdDevice::MEDIA_DVD_R_SEQ|
		  K3bCdDevice::MEDIA_DVD_R|
		  K3bCdDevice::MEDIA_DVD_RW) ) {
      emit infoMessage( i18n("Writing DVD-R(W) in sequential mode."), INFO );
    }
  }

  return true;
}



void K3bDvdCopyJob::removeImageFiles()
{
  if( QFile::exists( m_imagePath ) ) {
    QFile::remove( m_imagePath );
    emit infoMessage( i18n("Removed image file %1").arg(m_imagePath), K3bJob::STATUS );
  }
}


QString K3bDvdCopyJob::jobDescription() const
{
  return i18n("Copying DVD");
}


QString K3bDvdCopyJob::jobDetails() const
{
  if( m_onlyCreateImage )
    return i18n("Creating DVD image");
  else if( m_simulate )
    return i18n("Simulating DVD copy");
  else
    return i18n("Creating 1 DVD copy", "Creating %n DVD copies", m_copies );
}

#include "k3bdvdcopyjob.moc"
