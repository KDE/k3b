/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bclonejob.h"

#include <k3breadcdreader.h>
#include <k3bcdrecordwriter.h>
#include <k3bexternalbinmanager.h>
#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bglobals.h>
#include <k3bcore.h>
#include <k3bclonetocreader.h>
#include <k3bglobalsettings.h>

#include <kdebug.h>
#include <klocale.h>

#include <qfile.h>
#include <qfileinfo.h>



class K3bCloneJob::Private
{
public:
  Private()
    : doneCopies(0) {
  }

  int doneCopies;
};


K3bCloneJob::K3bCloneJob( K3bJobHandler* hdl, QObject* parent )
  : K3bBurnJob( hdl, parent ),
    m_writerDevice(0),
    m_readerDevice(0),
    m_writerJob(0),
    m_readcdReader(0),
    m_removeImageFiles(false),
    m_canceled(false),
    m_running(false),
    m_simulate(false),
    m_speed(1),
    m_copies(1),
    m_onlyCreateImage(false),
    m_onlyBurnExistingImage(false),
    m_readRetries(128)
{
  d = new Private;
}


K3bCloneJob::~K3bCloneJob()
{
  delete d;
}


void K3bCloneJob::start()
{
  jobStarted();

  m_canceled = false;
  m_running = true;


  // TODO: check the cd size and warn the user if not enough space

  //
  // We first check if cdrecord has clone support
  // The readcdReader will check the same for readcd
  //
  const K3bExternalBin* cdrecordBin = k3bcore->externalBinManager()->binObject( "cdrecord" );
  if( !cdrecordBin ) {
    emit infoMessage( i18n("Could not find %1 executable.",QString("cdrecord")), ERROR );
    jobFinished(false);
    m_running = false;
    return;
  }
  else if( !cdrecordBin->hasFeature( "clone" ) ) {
    emit infoMessage( i18n("Cdrecord version %1 does not have cloning support.",cdrecordBin->version), ERROR );
    jobFinished(false);
    m_running = false;
    return;
  }

  if( (!m_onlyCreateImage && !writer()) ||
      (!m_onlyBurnExistingImage && !readingDevice()) ) {
    emit infoMessage( i18n("No device set."), ERROR );
    jobFinished(false);
    m_running = false;
    return;
  }

  if( !m_onlyCreateImage ) {
    if( !writer()->supportsWritingMode( K3bDevice::RAW_R96R ) &&
	!writer()->supportsWritingMode( K3bDevice::RAW_R16 ) ) {
      emit infoMessage( i18n("CD writer %1 (%2) does not support cloning.",
			     writer()->vendor(),
			     writer()->description()), ERROR );
      m_running = false;
      jobFinished(false);
      return;
    }
  }

  if( m_imagePath.isEmpty() ) {
    m_imagePath = K3b::findTempFile( "img" );
  }
  else if( QFileInfo(m_imagePath).isDir() ) {
    m_imagePath = K3b::findTempFile( "img", m_imagePath );
  }

  if( m_onlyBurnExistingImage ) {
    startWriting();
  }
  else {
    emit burning( false );

    prepareReader();

    if( waitForMedia( readingDevice(),
		      K3bDevice::STATE_COMPLETE,
		      K3bDevice::MEDIA_WRITABLE_CD|K3bDevice::MEDIA_CD_ROM ) < 0 ) {
      m_running = false;
      emit canceled();
      jobFinished(false);
      return;
    }

    emit newTask( i18n("Reading clone image") );

    m_readcdReader->start();
  }
}


void K3bCloneJob::prepareReader()
{
  if( !m_readcdReader ) {
    m_readcdReader = new K3bReadcdReader( this, this );
    connect( m_readcdReader, SIGNAL(percent(int)), this, SLOT(slotReadingPercent(int)) );
    connect( m_readcdReader, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
    connect( m_readcdReader, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
    connect( m_readcdReader, SIGNAL(finished(bool)), this, SLOT(slotReadingFinished(bool)) );
    connect( m_readcdReader, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_readcdReader, SIGNAL(newTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_readcdReader, SIGNAL(debuggingOutput(const QString&, const QString&)),
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }

  m_readcdReader->setReadDevice( readingDevice() );
  m_readcdReader->setReadSpeed( 0 ); // MAX
  m_readcdReader->setDisableCorrection( m_noCorrection );
  m_readcdReader->setImagePath( m_imagePath );
  m_readcdReader->setClone( true );
  m_readcdReader->setRetries( m_readRetries );
}


void K3bCloneJob::prepareWriter()
{
  if( !m_writerJob ) {
    m_writerJob = new K3bCdrecordWriter( writer(), this, this );
    connect( m_writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterPercent(int)) );
    connect( m_writerJob, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
    connect( m_writerJob, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
    connect( m_writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
    connect( m_writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
    connect( m_writerJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( m_writerJob, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
    connect( m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
    //    connect( m_writerJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
    connect( m_writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }

  m_writerJob->clearArguments();
  m_writerJob->setWritingMode( K3b::WRITING_MODE_RAW );
  m_writerJob->setClone( true );
  m_writerJob->setSimulate( m_simulate );
  m_writerJob->setBurnSpeed( m_speed );
  m_writerJob->addArgument( m_imagePath );
}


void K3bCloneJob::cancel()
{
  if( m_running ) {
    m_canceled = true;
    if( m_readcdReader )
      m_readcdReader->cancel();
    if( m_writerJob )
      m_writerJob->cancel();
  }
}


void K3bCloneJob::slotWriterPercent( int p )
{
  if( m_onlyBurnExistingImage )
    emit percent( (int)((double)(d->doneCopies)*100.0/(double)(m_copies) + (double)p/(double)(m_copies)) );
  else
    emit percent( (int)((double)(1+d->doneCopies)*100.0/(double)(1+m_copies) + (double)p/(double)(1+m_copies)) );
}


void K3bCloneJob::slotWriterNextTrack( int t, int tt )
{
  emit newSubTask( i18n("Writing Track %1 of %2",t,tt) );
}


void K3bCloneJob::slotWriterFinished( bool success )
{
    if( m_canceled ) {
        removeImageFiles();
        m_running = false;
        emit canceled();
        jobFinished(false);
        return;
    }

    if( success ) {
        d->doneCopies++;

        emit infoMessage( i18n("Successfully written clone copy %1.",d->doneCopies), INFO );

        if( d->doneCopies < m_copies ) {
            K3bDevice::eject( writer() );
            startWriting();
        }
        else {
            if ( k3bcore->globalSettings()->ejectMedia() ) {
                K3bDevice::eject( writer() );
            }

            if( m_removeImageFiles )
                removeImageFiles();
            m_running = false;
            jobFinished(true);
        }
    }
    else {
        removeImageFiles();
        m_running = false;
        jobFinished(false);
    }
}


void K3bCloneJob::slotReadingPercent( int p )
{
  emit percent( m_onlyCreateImage ? p : (int)((double)p/(double)(1+m_copies)) );
}


void K3bCloneJob::slotReadingFinished( bool success )
{
  if( m_canceled ) {
    removeImageFiles();
    m_running = false;
    emit canceled();
    jobFinished(false);
    return;
  }

  if( success ) {
    //
    // Make a quick test if the image is really valid.
    // Readcd does not seem to have proper exit codes
    //
    K3bCloneTocReader ctr( m_imagePath );
    if( ctr.isValid() ) {
      emit infoMessage( i18n("Successfully read disk."), INFO );
      if( m_onlyCreateImage ) {
	m_running = false;
	jobFinished(true);
      }
      else {
	if( writer() == readingDevice() )
	  K3bDevice::eject( writer() );
	startWriting();
      }
    }
    else {
      emit infoMessage( i18n("Failed to read disk completely in clone mode."), ERROR );
      removeImageFiles();
      m_running = false;
      jobFinished(false);
    }
  }
  else {
    emit infoMessage( i18n("Error while reading disk."), ERROR );
    removeImageFiles();
    m_running = false;
    jobFinished(false);
  }
}


void K3bCloneJob::startWriting()
{
  emit burning( true );

  // start writing
  prepareWriter();

  if( waitForMedia( writer(),
		    K3bDevice::STATE_EMPTY,
		    K3bDevice::MEDIA_WRITABLE_CD ) < 0 ) {
    removeImageFiles();
    m_running = false;
    emit canceled();
    jobFinished(false);
    return;
  }

  if( m_simulate )
    emit newTask( i18n("Simulating clone copy") );
  else
    emit newTask( i18n("Writing clone copy %1",d->doneCopies+1) );

  m_writerJob->start();
}


void K3bCloneJob::removeImageFiles()
{
  if( !m_onlyBurnExistingImage ) {
    emit infoMessage( i18n("Removing image files."), INFO );
    if( QFile::exists( m_imagePath ) )
      QFile::remove( m_imagePath );
    if( QFile::exists( m_imagePath + ".toc" ) )
      QFile::remove( m_imagePath + ".toc"  );
  }
}


QString K3bCloneJob::jobDescription() const
{
  if( m_onlyCreateImage )
    return i18n("Creating Clone Image");
  else if( m_onlyBurnExistingImage ) {
    if( m_simulate )
      return i18n("Simulating Clone Image");
    else
      return i18n("Burning Clone Image");
  }
  else if( m_simulate )
    return i18n("Simulating CD Cloning");
  else
    return i18n("Cloning CD");
}


QString K3bCloneJob::jobDetails() const
{
    return i18np("Creating 1 clone copy",
                 "Creating %1 clone copies",
                 (m_simulate||m_onlyCreateImage) ? 1 : m_copies );
}

#include "k3bclonejob.moc"
