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


#include "k3bdvdjob.h"
#include "k3bdvddoc.h"
#include "k3bgrowisofsimager.h"

#include <k3bcore.h>
#include <k3bdvdrecordwriter.h>
#include <k3bisoimager.h>
#include <k3bdataverifyingjob.h>
#include <k3bgrowisofswriter.h>
#include <k3bemptydiscwaiter.h>
#include <k3bglobals.h>
#include <k3bemptydiscwaiter.h>
#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bdiskinfo.h>
#include <k3bdeviceglobals.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kconfig.h>


class K3bDvdJob::Private
{
public:
  Private()
    : verificationJob(0) {
  }

  K3bDataVerifyingJob* verificationJob;
  bool imageError;
};


K3bDvdJob::K3bDvdJob( K3bDataDoc* doc, QObject* parent )
  : K3bBurnJob( parent ),
    m_isoImager( 0 ),
    m_growisofsImager( 0 ),
    m_writerJob( 0 ),
    m_doc( doc )
{
  d = new Private();
}


K3bDvdJob::~K3bDvdJob()
{
  delete d;
}


K3bDoc* K3bDvdJob::doc() const
{
  return m_doc;
}


K3bCdDevice::CdDevice* K3bDvdJob::writer() const
{
  return m_doc->burner();
}


void K3bDvdJob::start()
{
  //
  // When writing multisession the design of growisofs does not allow creating an image
  // before writing.
  // In this case we use the K3bGrowisofsWriter, in all other cases the K3bGrowisofsImager.
  // 

  emit started();

  m_canceled = false;
  m_writingStarted = false;

  if( m_doc->dummy() )
    m_doc->setVerifyData( false );

  if( !m_doc->onTheFly() || m_doc->onlyCreateImages() ) {
    emit newTask( i18n("Writing data") );
    emit burning(false);
    writeImage();
  }
  else {
    prepareGrowisofsImager();
    
    if( waitForDvd() ) {
      emit burning(true);
      m_growisofsImager->start();
    }
    else
      emit finished(false);
  }
}


void K3bDvdJob::writeImage()
{
  //
  // disable all multisession since we do only support multisession in on-the-fly mode
  //
  m_doc->setMultiSessionMode( K3bDataDoc::NONE );
  prepareIsoImager();

  // get image file path
  if( m_doc->tempDir().isEmpty() )
    m_doc->setTempDir( K3b::findUniqueFilePrefix( m_doc->isoOptions().volumeID() ) + ".iso" );
    
  emit infoMessage( i18n("Writing image file to %1").arg(m_doc->tempDir()), INFO );
  emit newSubTask( i18n("Creating image file") );

  m_isoImager->writeToImageFile( m_doc->tempDir() );
  m_isoImager->start();
}


void K3bDvdJob::prepareIsoImager()
{
  if( !m_isoImager ) {
    m_isoImager = new K3bIsoImager( m_doc, this );
    connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), 
	     this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
    connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
    connect( m_isoImager, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }
}


void K3bDvdJob::prepareGrowisofsImager()
{
  if( !m_growisofsImager ) {
    m_growisofsImager = new K3bGrowisofsImager( m_doc, this );
    connect( m_growisofsImager, SIGNAL(infoMessage(const QString&, int)), 
	     this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_growisofsImager, SIGNAL(percent(int)), this, SLOT(slotGrowisofsImagerPercent(int)) );
    connect( m_growisofsImager, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
    connect( m_growisofsImager, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
    connect( m_growisofsImager, SIGNAL(finished(bool)), this, SLOT(slotWritingFinished(bool)) );
    connect( m_growisofsImager, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
    connect( m_growisofsImager, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_growisofsImager, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }
}


void K3bDvdJob::slotIsoImagerPercent( int p )
{
  if( !m_doc->onTheFly() || m_doc->onlyCreateImages() )
    emit subPercent( p );

  if( m_doc->onlyCreateImages() )
    emit percent( p );
  else if( !m_doc->onTheFly() ) {
    if( m_doc->verifyData() ) 
      emit percent( p/3 );
    else
      emit percent( p/2 );
  }
}


void K3bDvdJob::slotGrowisofsImagerPercent( int p )
{
  emit subPercent( p );

  if( m_doc->verifyData() )
    emit percent( p/2 );
  else
    emit percent( p );

  if( !m_writingStarted ) {
    m_writingStarted = true;
    emit newSubTask( i18n("Writing data") );
  }
}


void K3bDvdJob::slotIsoImagerFinished( bool success )
{
  if( m_canceled ) {
    emit canceled();
    emit finished(false);
    return;
  }

  d->imageError = success;

  if( m_doc->onTheFly() && m_writerJob )
    m_writerJob->closeFd();

  if( success ) {
    if( m_doc->onlyCreateImages() || !m_doc->onTheFly() ) {
      emit infoMessage( i18n("Image successfully created in %1").arg(m_doc->tempDir()), K3bJob::SUCCESS );
      
      if( m_doc->onlyCreateImages() ) {
	emit finished( true );
      }
      else {
	if( prepareWriterJob() ) {
	  if( waitForDvd() ) {
	    emit burning(true);
	    m_writerJob->start();
	  }
	  else
	    emit finished(false);
	}
	else {
	  emit finished(false);
	}
      }
    }
  }
  
  else {
    emit infoMessage( i18n("Error while creating iso image"), ERROR );
    cleanup();
    emit finished( false );
  }
}


void K3bDvdJob::cancel()
{
  m_canceled = true;

  if( m_isoImager )
    m_isoImager->cancel();
  if( m_growisofsImager )
    m_growisofsImager->cancel();
  if( d->verificationJob )
    d->verificationJob->cancel();

  cleanup();
}


bool K3bDvdJob::prepareWriterJob()
{
  if( m_writerJob )
    delete m_writerJob;

  K3bGrowisofsWriter* writer = new K3bGrowisofsWriter( m_doc->burner(), this );
  
  // these do only make sense with DVD-R(W)
  writer->setSimulate( m_doc->dummy() );
  writer->setBurnSpeed( m_doc->speed() );

  if( m_doc->writingMode() == K3b::DAO ||
      ( m_doc->writingMode() == K3b::WRITING_MODE_AUTO &&
	m_doc->multiSessionMode() == K3bDataDoc::NONE ) )
    writer->setWritingMode( K3b::DAO );

  writer->setCloseDvd( m_doc->multiSessionMode() == K3bDataDoc::NONE ||
		       m_doc->multiSessionMode() == K3bDataDoc::FINISH );
  
  if( m_doc->onTheFly() )
    writer->setImageToWrite( QString::null );  // read from stdin
  else
    writer->setImageToWrite( m_doc->tempDir() );

  m_writerJob = writer;


  connect( m_writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
  connect( m_writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  //  connect( m_writerJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  //  connect( m_writerJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  //  connect( m_writerJob, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
  connect( m_writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writerJob, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
  connect( m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotWritingFinished(bool)) );
  //  connect( m_writerJob, SIGNAL(dataWritten()), this, SLOT(slotDataWritten()) );
  connect( m_writerJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}


void K3bDvdJob::slotWriterJobPercent( int p )
{
  if( m_doc->onTheFly() ) {
    emit subPercent( p );
    if( m_doc->verifyData() )
      emit percent( 50 + p/2 ); 
    else
      emit percent( p );
  }
  else {
    if( m_doc->verifyData() )
      emit percent( 33 + p/3 ); 
    else
      emit percent( 50 + p/2 );
  }
}


void K3bDvdJob::slotWritingFinished( bool success )
{
  if( m_canceled ) {
    emit canceled();
    emit finished(false);
    return;
  }

  if( success ) {
    // allright
    // the writerJob should have emited the "simulation/writing successful" signal

    if( m_doc->verifyData() ) {
      if( !d->verificationJob ) {
	d->verificationJob = new K3bDataVerifyingJob( this );
	connect( d->verificationJob, SIGNAL(infoMessage(const QString&, int)),
		 this, SIGNAL(infoMessage(const QString&, int)) );
	connect( d->verificationJob, SIGNAL(newTask(const QString&)),
		 this, SIGNAL(newSubTask(const QString&)) );
	connect( d->verificationJob, SIGNAL(percent(int)),
		 this, SLOT(slotVerificationProgress(int)) );
	connect( d->verificationJob, SIGNAL(percent(int)),
		 this, SIGNAL(subPercent(int)) );
	connect( d->verificationJob, SIGNAL(finished(bool)),
		 this, SLOT(slotVerificationFinished(bool)) );
      }
      d->verificationJob->setDoc( m_doc );
      d->verificationJob->setDevice( m_doc->burner() );

      emit newTask( i18n("Verifying written data") );
      emit burning(false);

      d->verificationJob->start();
    }
    else {
      cleanup();
      emit finished(true);
    }
  }
  else {
    cleanup();
    emit finished( false );
  }
}


void K3bDvdJob::slotVerificationProgress( int p )
{
  // we simply think of the verification as 1/2 or 1/3 of the work...
  if( m_doc->onTheFly() )
    emit percent( 50 + p/2 );
  else {
    emit percent( 66 + p/3 );
  }
}


void K3bDvdJob::slotVerificationFinished( bool success )
{
  if( m_canceled ) {
    emit canceled();
    emit finished(false);
    return;
  }

  cleanup();

  k3bcore->config()->setGroup("General Options");
  if( !k3bcore->config()->readBoolEntry( "No cd eject", false ) )
    K3bCdDevice::eject( m_doc->burner() );
  
  emit finished( success );
}


void K3bDvdJob::cleanup()
{
  if( !m_doc->onTheFly() && ( d->imageError || m_canceled || m_doc->removeImages() ) ) {
    if( QFile::exists( m_doc->tempDir() ) ) {
      QFile::remove( m_doc->tempDir() );
      emit infoMessage( i18n("Removed image file %1").arg(m_doc->tempDir()), K3bJob::SUCCESS );
    }
  }
}


bool K3bDvdJob::waitForDvd()
{
  emit infoMessage( i18n("Waiting for media") + "...", INFO );

  int mt = 0;
  if( m_doc->writingMode() == K3b::WRITING_MODE_INCR_SEQ || m_doc->writingMode() == K3b::DAO )
    mt = K3bCdDevice::MEDIA_DVD_RW_SEQ|K3bCdDevice::MEDIA_DVD_R_SEQ;
  else if( m_doc->writingMode() == K3b::WRITING_MODE_RES_OVWR ) // we treat DVD+R(W) as restricted overwrite media
    mt = K3bCdDevice::MEDIA_DVD_RW_OVWR|K3bCdDevice::MEDIA_DVD_PLUS_RW|K3bCdDevice::MEDIA_DVD_PLUS_R|K3bCdDevice::MEDIA_DVD_PLUS_R_DL;
  else
    mt = K3bCdDevice::MEDIA_WRITABLE_DVD;

  // double layer media
  if( m_doc->size() > 4700372992LL )
    mt = K3bCdDevice::MEDIA_DVD_PLUS_R_DL;

  int m = K3bEmptyDiscWaiter::wait( m_doc->burner(), 
				    m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
				    m_doc->multiSessionMode() == K3bDataDoc::FINISH,
				    mt );
  if( m == -1 ) {
    cancel();
    return false;
  }
  
  if( m == 0 ) {
    emit infoMessage( i18n("Forced by user. Growisofs will be called without further tests."), INFO );
  }

  else {
    if( m & (K3bCdDevice::MEDIA_DVD_PLUS_RW|K3bCdDevice::MEDIA_DVD_PLUS_R) ) {
      if( m_doc->dummy() ) {
	if( KMessageBox::warningYesNo( qApp->activeWindow(),
				       i18n("K3b does not support simulation with DVD+R(W) media. "
					    "Do you really want to continue? The media will be written "
					    "for real."),
				       i18n("No simulation with DVD+R(W)") ) == KMessageBox::No ) {
	  cancel();
	  return false;
	}

	m_doc->setDummy( false );
      }
      
      if( m_doc->writingMode() != K3b::WRITING_MODE_AUTO && m_doc->writingMode() != K3b::WRITING_MODE_RES_OVWR )
	emit infoMessage( i18n("Writing mode ignored when writing DVD+R(W) media."), INFO );

      if( m & K3bCdDevice::MEDIA_DVD_PLUS_RW ) {
	  if( m_doc->multiSessionMode() == K3bDataDoc::NONE ||
	      m_doc->multiSessionMode() == K3bDataDoc::START )
	    emit infoMessage( i18n("Writing DVD+RW."), INFO );
	  else
	    emit infoMessage( i18n("Growing Iso9660 filesystem on DVD+RW."), INFO );
      }
      else
	emit infoMessage( i18n("Writing DVD+R."), INFO );
    }
    else {
      if( m_doc->dummy() && !m_doc->burner()->dvdMinusTestwrite() ) {
	if( KMessageBox::warningYesNo( qApp->activeWindow(),
				       i18n("Your writer (%1 %2) does not support simulation with DVD-R(W) media. "
					    "Do you really want to continue? The media will be written "
					    "for real.")
				       .arg(m_doc->burner()->vendor())
				       .arg(m_doc->burner()->description()),
				       i18n("No simulation with DVD-R(W)") ) == KMessageBox::No ) {
	  cancel();
	  return false;
	}

	m_doc->setDummy( false );
      }


      if( m & K3bCdDevice::MEDIA_DVD_RW_OVWR ) {
	if( m_doc->multiSessionMode() == K3bDataDoc::NONE ||
	    m_doc->multiSessionMode() == K3bDataDoc::START )
	  emit infoMessage( i18n("Writing DVD-RW in restricted overwrite mode."), INFO );
	else
	  emit infoMessage( i18n("Growing Iso9660 filesystem on DVD-RW in restricted overwrite mode."), INFO );
      }
      else if( m & (K3bCdDevice::MEDIA_DVD_RW_SEQ|
		    K3bCdDevice::MEDIA_DVD_RW) ) {
	if( m_doc->writingMode() == K3b::DAO ||
	    ( m_doc->writingMode() == K3b::WRITING_MODE_AUTO &&
	      m_doc->multiSessionMode() == K3bDataDoc::NONE ) )
	  emit infoMessage( i18n("Writing DVD-RW in DAO mode."), INFO );
	else if( m_doc->multiSessionMode() == K3bDataDoc::START ||
		 m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ) {
	  // check if the writer supports writing sequential and thus multisession
	  if( !m_doc->burner()->supportsFeature( 0x21 ) ) {
	    if( KMessageBox::warningYesNo( qApp->activeWindow(),
					   i18n("Your writer (%1 %2) does not support Incremental Streaming with %3 "
						"media. Multisession will not be possible. Continue anyway?")
					   .arg(m_doc->burner()->vendor())
					   .arg(m_doc->burner()->description())
					   .arg( i18n("DVD-RW") ),
					   i18n("No Incremental Streaming") ) == KMessageBox::No ) {
	      cancel();
	      return false;
	    }
	  }
	  emit infoMessage( i18n("Writing DVD-RW in sequential mode."), INFO );	
	}
      }
      else if( m & (K3bCdDevice::MEDIA_DVD_R_SEQ|
		    K3bCdDevice::MEDIA_DVD_R) ) {
	if( m_doc->writingMode() == K3b::DAO ||
	    ( m_doc->writingMode() == K3b::WRITING_MODE_AUTO &&
	      m_doc->multiSessionMode() == K3bDataDoc::NONE ) )
	  emit infoMessage( i18n("Writing DVD-R in DAO mode."), INFO );
	else {
	  if( m_doc->multiSessionMode() == K3bDataDoc::START ||
	      m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ) {
	    // check if the writer supports writing sequential and thus multisession
	    if( !m_doc->burner()->supportsFeature( 0x21 ) ) {
	      if( KMessageBox::warningYesNo( qApp->activeWindow(),
					     i18n("Your writer (%1 %2) does not support Incremental Streaming with %3 "
						  "media. Multisession will not be possible. Continue anyway?")
					     .arg(m_doc->burner()->vendor())
					     .arg(m_doc->burner()->description())
					     .arg( i18n("DVD-R") ),
					     i18n("No Incremental Streaming") ) == KMessageBox::No ) {
		cancel();
		return false;
	      }
	    }
	  }
	
	  if( m_doc->writingMode() == K3b::WRITING_MODE_RES_OVWR )
	    emit infoMessage( i18n("Restricted Overwrite is not possible with DVD-R media."), INFO );
	  emit infoMessage( i18n("Writing DVD-R in sequential mode."), INFO );	
	}
      }
    }
  }

  return true;
}


QString K3bDvdJob::jobDescription() const
{
  if( m_doc->onlyCreateImages() ) {
    return i18n("Creating Data Image File");
  }
  else {
    if( m_doc->isoOptions().volumeID().isEmpty() ) {
      if( m_doc->multiSessionMode() == K3bDataDoc::NONE )
	return i18n("Writing Data DVD");
      else
	return i18n("Writing Multisession DVD");
    }
    else {
      if( m_doc->multiSessionMode() == K3bDataDoc::NONE )
	return i18n("Writing Data DVD (%1)").arg(m_doc->isoOptions().volumeID());
      else
	return i18n("Writing Multisession DVD (%1)").arg(m_doc->isoOptions().volumeID());
    }
  }
}


QString K3bDvdJob::jobDetails() const
{
  return i18n("Iso9660 Filesystem (Size: %1)").arg(KIO::convertSize( m_doc->size() ));
}

#include "k3bdvdjob.moc"
