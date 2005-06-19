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
#include <k3bglobals.h>
#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bdiskinfo.h>
#include <k3bdeviceglobals.h>
#include <k3bglobalsettings.h>
#include <k3biso9660.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kapplication.h>


class K3bDvdJob::Private
{
public:
  Private()
    : verificationJob(0) {
  }

  K3bDataVerifyingJob* verificationJob;
  bool imageError;

  int copies;
  int copiesDone;

  int foundMedia;

  K3bDataDoc::MultiSessionMode usedMultiSessionMode;
};


K3bDvdJob::K3bDvdJob( K3bDataDoc* doc, K3bJobHandler* hdl, QObject* parent )
  : K3bBurnJob( hdl, parent ),
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


K3bDevice::Device* K3bDvdJob::writer() const
{
  return m_doc->burner();
}


void K3bDvdJob::start()
{
  //
  // When writing multisession the design of growisofs does not allow creating an image
  // before writing which is basicly because of the "growing" part.
  // In this case we use the K3bGrowisofsImager, in all other cases the K3bGrowisofsWriter.
  // 

  // TODO: detemrine if we have tracksize option to write onthefly with the growisofswriter in dao mode
  //       that is if version (>= .17 or (>= .15 and DAO))

  emit started();
  emit newTask( i18n("Preparing data") );

  m_canceled = false;
  m_writingStarted = false;
  d->copies = m_doc->copies();
  d->copiesDone = 0;
  d->usedMultiSessionMode = m_doc->multiSessionMode();

  if( m_doc->dummy() ) {
    m_doc->setVerifyData( false );
    d->copies = 1;
  }

  if( !m_doc->onTheFly() || m_doc->onlyCreateImages() ) {
    emit newTask( i18n("Writing data") );
    emit burning(false);
    writeImage();
  }
  else if( d->usedMultiSessionMode == K3bDataDoc::AUTO ) {
    determineMultiSessionMode();
  }
  else if( d->usedMultiSessionMode != K3bDataDoc::NONE ) {
    if( !startWriting() ) {
      cleanup();
      emit finished( false );
    }
  }
  else {
    prepareIsoImager();
    m_isoImager->calculateSize();    
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
    m_isoImager = new K3bIsoImager( m_doc, this, this );
    connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), 
	     this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
    connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
    connect( m_isoImager, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
    connect( m_isoImager, SIGNAL(sizeCalculated(int, int)), 
	     this, SLOT(slotSizeCalculationFinished(int, int)) );
  }
}


void K3bDvdJob::prepareGrowisofsImager()
{
  if( !m_growisofsImager ) {
    m_growisofsImager = new K3bGrowisofsImager( m_doc, this, this );
    connect( m_growisofsImager, SIGNAL(infoMessage(const QString&, int)), 
	     this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_growisofsImager, SIGNAL(percent(int)), this, SLOT(slotGrowisofsImagerPercent(int)) );
    connect( m_growisofsImager, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
    connect( m_growisofsImager, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
    connect( m_growisofsImager, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( m_growisofsImager, SIGNAL(finished(bool)), this, SLOT(slotWritingFinished(bool)) );
    connect( m_growisofsImager, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
    connect( m_growisofsImager, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_growisofsImager, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }
}


void K3bDvdJob::slotSizeCalculationFinished( int status, int size )
{
  emit debuggingOutput( "K3b", QString( "Size of filesystem calculated: %1" ).arg(size) );

  //
  // this is only called in on-the-fly mode
  //
  if( status == ERROR || !startWriting() ) {
    cleanup();
    emit finished(false);
  }
}


void K3bDvdJob::slotIsoImagerPercent( int p )
{
  if( m_doc->onlyCreateImages() ) {
    emit subPercent( p );
    emit percent( p );
  }
  else if( !m_doc->onTheFly() ) {
    double totalTasks = d->copies;
    double tasksDone = d->copiesDone; // =0 when creating an image
    if( m_doc->verifyData() ) {
      totalTasks*=2;
      tasksDone*=2;
    }
    if( !m_doc->onTheFly() ) {
      totalTasks+=1.0;
    }

    emit subPercent( p );
    emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
  }
}


void K3bDvdJob::slotGrowisofsImagerPercent( int p )
{
  emit subPercent( p );

  double totalTasks = d->copies;
  double tasksDone = d->copiesDone;
  if( m_doc->verifyData() ) {
    totalTasks*=2;
    tasksDone*=2;
  }
  if( !m_doc->onTheFly() ) {
    totalTasks+=1.0;
    tasksDone+=1.0;
  }

  emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );

  if( !m_writingStarted ) {
    m_writingStarted = true;
    emit newSubTask( i18n("Writing data") );
  }
}


void K3bDvdJob::slotIsoImagerFinished( bool success )
{
  if( m_canceled ) {
    if( !numRunningSubJobs() ||
	( numRunningSubJobs() == 1 && runningSubJobs().containsRef(m_isoImager) ) ||
	( numRunningSubJobs() == 1 && runningSubJobs().containsRef(m_growisofsImager) ) ) {
      emit canceled();
      emit finished(false);
    }
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
      else if( !startWriting() ) {
	cleanup();
	emit finished(false);
      }
    }
  }
  
  else {
    emit infoMessage( i18n("Error while creating ISO image"), ERROR );
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
  if( m_writerJob )
    m_writerJob->cancel();
  if( d->verificationJob )
    d->verificationJob->cancel();

  cleanup();
}


bool K3bDvdJob::startWriting()
{
  if( m_doc->dummy() )
    emit newTask( i18n("Simulating") );
  else if( d->copies > 1 )
    emit newTask( i18n("Writing Copy %1").arg(d->copiesDone+1) );
  else
    emit newTask( i18n("Writing") );

  emit burning(true);

  if( d->usedMultiSessionMode != K3bDataDoc::NONE ) {
    prepareGrowisofsImager();
    
    if( waitForDvd() ) {
      m_growisofsImager->start();
      return true;
    }
  }
  else if( prepareWriterJob() && waitForDvd() ) {
    m_writerJob->start();

    if( m_doc->onTheFly() ) {
      m_isoImager->writeToFd( m_writerJob->fd() );
      m_isoImager->start();
    }
    return true;
  }
  
  return false;
}


bool K3bDvdJob::prepareWriterJob()
{
  if( m_writerJob )
    delete m_writerJob;

  K3bGrowisofsWriter* writer = new K3bGrowisofsWriter( m_doc->burner(), this, this );
  
  // these do only make sense with DVD-R(W)
  writer->setSimulate( m_doc->dummy() );
  writer->setBurnSpeed( m_doc->speed() );

  // Andy said incremental sequential is the default mode and it seems uses have more problems with DAO anyway
  if( m_doc->writingMode() == K3b::DAO )
//     || ( m_doc->writingMode() == K3b::WRITING_MODE_AUTO &&
// 	 d->usedMultiSessionMode == K3bDataDoc::NONE ) )
    writer->setWritingMode( K3b::DAO );

  writer->setCloseDvd( d->usedMultiSessionMode == K3bDataDoc::NONE ||
		       d->usedMultiSessionMode == K3bDataDoc::FINISH );

  if( m_doc->onTheFly() ) {
    writer->setImageToWrite( QString::null );  // read from stdin
    writer->setTrackSize( m_isoImager->size() );
  }
  else
    writer->setImageToWrite( m_doc->tempDir() );

  m_writerJob = writer;


  connect( m_writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
  connect( m_writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_writerJob, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writerJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
  connect( m_writerJob, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
  connect( m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotWritingFinished(bool)) );
  connect( m_writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}


void K3bDvdJob::slotWriterJobPercent( int p )
{
  double totalTasks = d->copies;
  double tasksDone = d->copiesDone;
  if( m_doc->verifyData() ) {
    totalTasks*=2;
    tasksDone*=2;
  }
  if( !m_doc->onTheFly() ) {
    totalTasks+=1.0;
    tasksDone+=1.0;
  }

  emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3bDvdJob::slotWritingFinished( bool success )
{
  if( m_canceled ) {
    if( !numRunningSubJobs() ||
	( numRunningSubJobs() == 1 && runningSubJobs().containsRef(m_isoImager) ) ) {
      emit canceled();
      emit finished(false);
    }
    return;
  }

  if( success ) {
    // allright
    // the writerJob should have emited the "simulation/writing successful" signal

    if( m_doc->verifyData() ) {
      if( !d->verificationJob ) {
	d->verificationJob = new K3bDataVerifyingJob( this, this );
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
      d->copiesDone++;

      if( d->copiesDone < d->copies ) {
	K3bDevice::eject( m_doc->burner() );

	if( !startWriting() ) {
	  cleanup();
	  emit finished(false);
	}
      }
      else {
	cleanup();
	emit finished(true);
      }
    }
  }
  else {
    cleanup();
    emit finished( false );
  }
}


void K3bDvdJob::slotVerificationProgress( int p )
{
  double totalTasks = d->copies*2;
  double tasksDone = d->copiesDone*2 + 1; // the writing of the current copy has already been finished

  if( !m_doc->onTheFly() ) {
    totalTasks+=1.0;
    tasksDone+=1.0;
  }
  
  emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3bDvdJob::slotVerificationFinished( bool success )
{
  if( m_canceled ) {
    if( !numRunningSubJobs() ||
	( numRunningSubJobs() == 1 && runningSubJobs().containsRef(m_isoImager) ) ) {
      emit canceled();
      emit finished(false);
    }
    return;
  }

  d->copiesDone++;
  
  if( success && d->copiesDone < d->copies ) {
    K3bDevice::eject( m_doc->burner() );
    
    if( !startWriting() ) {
      cleanup();
      emit finished(false);
    }
  }
  else {
    cleanup();
    
    if( k3bcore->globalSettings()->ejectMedia() )
      K3bDevice::eject( m_doc->burner() );
  
    emit finished( success );
  }
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


void K3bDvdJob::determineMultiSessionMode()
{
  int m = requestMedia( K3bDevice::STATE_INCOMPLETE|K3bDevice::STATE_EMPTY );

  if( m < 0 ) {
    cancel();
  }
  else {
     connect( K3bDevice::sendCommand( K3bDevice::DeviceHandler::NG_DISKINFO, m_doc->burner() ), 
	     SIGNAL(finished(K3bDevice::DeviceHandler*)),
	     this, 
	     SLOT(slotDetermineMultiSessionMode(K3bDevice::DeviceHandler*)) );
  }
}


void K3bDvdJob::slotDetermineMultiSessionMode( K3bDevice::DeviceHandler* dh )
{
  const K3bDevice::DiskInfo& info = dh->diskInfo();

  if( info.mediaType() & (K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR) ) {
    //
    // we need to handle DVD+RW and DVD-RW overwrite media differently since remainingSize() is not valid
    // in both cases
    // Since one never closes a DVD+RW we only differ between CONTINUE and NONE
    //

    // try to check the filesystem size
    K3bIso9660 iso( m_doc->burner() );
    if( iso.open() && info.capacity() - iso.primaryDescriptor().volumeSpaceSize >= m_doc->length() ) {
      d->usedMultiSessionMode = K3bDataDoc::CONTINUE;
    }
    else {
      d->usedMultiSessionMode = K3bDataDoc::NONE;
    }
  }
  else if( info.appendable() ) {
    //
    // 3 cases:
    //  1. the project does not fit -> no multisession (resulting in asking for another media above)
    //  2. the project does fit and fills up the CD -> finish multisession
    //  3. the project does fit and does not fill up the CD -> continue multisession
    //
    if( m_doc->size() > info.remainingSize().mode1Bytes() )
      d->usedMultiSessionMode = K3bDataDoc::NONE;
    else if( m_doc->size() >= info.remainingSize().mode1Bytes()*9/10 )
      d->usedMultiSessionMode = K3bDataDoc::FINISH;
    else
      d->usedMultiSessionMode = K3bDataDoc::CONTINUE;
  }
  else {
    d->usedMultiSessionMode = K3bDataDoc::NONE;
  }

  if( d->usedMultiSessionMode != K3bDataDoc::NONE ) {
    if( !startWriting() ) {
      cleanup();
      emit finished( false );
    }
  }
  else {
    prepareIsoImager();
    m_isoImager->calculateSize();    
  }
}


int K3bDvdJob::requestMedia( int state )
{
  int mt = 0;
  if( m_doc->writingMode() == K3b::WRITING_MODE_INCR_SEQ || m_doc->writingMode() == K3b::DAO )
    mt = K3bDevice::MEDIA_DVD_RW_SEQ|K3bDevice::MEDIA_DVD_R_SEQ;
  else if( m_doc->writingMode() == K3b::WRITING_MODE_RES_OVWR ) // we treat DVD+R(W) as restricted overwrite media
    mt = K3bDevice::MEDIA_DVD_RW_OVWR|K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_PLUS_R|K3bDevice::MEDIA_DVD_PLUS_R_DL;
  else
    mt = K3bDevice::MEDIA_WRITABLE_DVD;

  // double layer media
  if( m_doc->size() > 4700372992LL )
    mt = K3bDevice::MEDIA_WRITABLE_DVD_DL;

  return waitForMedia( m_doc->burner(),
		       state, 
		       mt );
}


bool K3bDvdJob::waitForDvd()
{
  emit infoMessage( i18n("Waiting for media") + "...", INFO );

  d->foundMedia = requestMedia( d->usedMultiSessionMode == K3bDataDoc::CONTINUE ||
				d->usedMultiSessionMode == K3bDataDoc::FINISH ?
				K3bDevice::STATE_INCOMPLETE :
				K3bDevice::STATE_EMPTY );

  if( d->foundMedia < 0 ) {
    cancel();
    return false;
  }
  
  if( d->foundMedia == 0 ) {
    emit infoMessage( i18n("Forced by user. Growisofs will be called without further tests."), INFO );
  }

  else {
    // -------------------------------
    // DVD Plus
    // -------------------------------
    if( d->foundMedia & K3bDevice::MEDIA_DVD_PLUS_ALL ) {
      if( m_doc->dummy() ) {
	if( KMessageBox::warningYesNo( qApp->activeWindow(),
				       i18n("K3b does not support simulation with DVD+R(W) media. "
					    "Do you really want to continue? The media will be written "
					    "for real."),
				       i18n("No Simulation with DVD+R(W)") ) == KMessageBox::No ) {
	  cancel();
	  return false;
	}

	m_doc->setDummy( false );
      }
      
      if( m_doc->writingMode() != K3b::WRITING_MODE_AUTO && m_doc->writingMode() != K3b::WRITING_MODE_RES_OVWR )
	emit infoMessage( i18n("Writing mode ignored when writing DVD+R(W) media."), INFO );

      if( d->foundMedia & K3bDevice::MEDIA_DVD_PLUS_RW ) {
	  if( d->usedMultiSessionMode == K3bDataDoc::NONE ||
	      d->usedMultiSessionMode == K3bDataDoc::START )
	    emit infoMessage( i18n("Writing DVD+RW."), INFO );
	  else
	    emit infoMessage( i18n("Growing ISO9660 filesystem on DVD+RW."), INFO );
      }
      else if( d->foundMedia & K3bDevice::MEDIA_DVD_PLUS_R_DL )
	emit infoMessage( i18n("Writing Double Layer DVD+R."), INFO );
      else
	emit infoMessage( i18n("Writing DVD+R."), INFO );
    }

    // -------------------------------
    // DVD Minus
    // -------------------------------
    else {
      if( m_doc->dummy() && !m_doc->burner()->dvdMinusTestwrite() ) {
	if( KMessageBox::warningYesNo( qApp->activeWindow(),
				       i18n("Your writer (%1 %2) does not support simulation with DVD-R(W) media. "
					    "Do you really want to continue? The media will be written "
					    "for real.")
				       .arg(m_doc->burner()->vendor())
				       .arg(m_doc->burner()->description()),
				       i18n("No Simulation with DVD-R(W)") ) == KMessageBox::No ) {
	  cancel();
	  return false;
	}

	m_doc->setDummy( false );
      }

      // RESTRICTED OVERWRITE
      // --------------------
      if( d->foundMedia & K3bDevice::MEDIA_DVD_RW_OVWR ) {
	if( d->usedMultiSessionMode == K3bDataDoc::NONE ||
	    d->usedMultiSessionMode == K3bDataDoc::START )
	  emit infoMessage( i18n("Writing DVD-RW in restricted overwrite mode."), INFO );
	else
	  emit infoMessage( i18n("Growing ISO9660 filesystem on DVD-RW in restricted overwrite mode."), INFO );
      }

      // NORMAL
      // ------
      else {

	// FIXME: DVD-R DL jump and stuff

	if( m_doc->writingMode() == K3b::DAO ||
	    ( m_doc->writingMode() == K3b::WRITING_MODE_AUTO &&
	      d->usedMultiSessionMode == K3bDataDoc::NONE ) )
	  emit infoMessage( i18n("Writing %1 in DAO mode.").arg( K3bDevice::mediaTypeString(d->foundMedia, true) ), INFO );

	else {
	  // check if the writer supports writing sequential and thus multisession
	  if( !m_doc->burner()->featureCurrent( K3bDevice::FEATURE_INCREMENTAL_STREAMING_WRITABLE ) ) {
	    if( KMessageBox::warningYesNo( qApp->activeWindow(),
					   i18n("Your writer (%1 %2) does not support Incremental Streaming with %3 "
						"media. Multisession will not be possible. Continue anyway?")
					   .arg(m_doc->burner()->vendor())
					   .arg(m_doc->burner()->description())
					   .arg( K3bDevice::mediaTypeString(d->foundMedia, true) ),
					   i18n("No Incremental Streaming") ) == KMessageBox::No ) {
	      cancel();
	      return false;
	    }
	    else {
	      emit infoMessage( i18n("Writing %1 in DAO mode.").arg( K3bDevice::mediaTypeString(d->foundMedia, true) ), INFO );
	    }
	  }
	  else {
	    if( !(d->foundMedia & (K3bDevice::MEDIA_DVD_RW|K3bDevice::MEDIA_DVD_RW_OVWR|K3bDevice::MEDIA_DVD_RW_SEQ)) &&
		m_doc->writingMode() == K3b::WRITING_MODE_RES_OVWR )
	      emit infoMessage( i18n("Restricted Overwrite is not possible with DVD-R media."), INFO );
	    
	    emit infoMessage( i18n("Writing %1 in incremental mode.").arg( K3bDevice::mediaTypeString(d->foundMedia, true) ), INFO );
	  }
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
  else if( m_doc->multiSessionMode() == K3bDataDoc::NONE ||
	   m_doc->multiSessionMode() == K3bDataDoc::AUTO ) {
    return i18n("Writing Data DVD")
      + ( m_doc->isoOptions().volumeID().isEmpty()
	  ? QString::null
	  : QString( " (%1)" ).arg(m_doc->isoOptions().volumeID()) );
  }
  else {
    return i18n("Writing Multisession DVD")
      + ( m_doc->isoOptions().volumeID().isEmpty()
	  ? QString::null
	  : QString( " (%1)" ).arg(m_doc->isoOptions().volumeID()) );
  }
}


QString K3bDvdJob::jobDetails() const
{
  if( m_doc->copies() > 1 && 
      !m_doc->dummy() &&
      !(m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
	m_doc->multiSessionMode() == K3bDataDoc::FINISH) )
    return i18n("ISO9660 Filesystem (Size: %1) - %n copy",
		"ISO9660 Filesystem (Size: %1) - %n copies",
		m_doc->copies())
      .arg(KIO::convertSize( m_doc->size() ));
  else
    return i18n("ISO9660 Filesystem (Size: %1)")
      .arg(KIO::convertSize( m_doc->size() ));
}

#include "k3bdvdjob.moc"
