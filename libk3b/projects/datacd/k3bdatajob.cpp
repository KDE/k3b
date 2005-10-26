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


#include "k3bdatajob.h"
#include "k3bdatadoc.h"
#include "k3bisoimager.h"
#include "k3bmsinfofetcher.h"
#include "k3bdataverifyingjob.h"

#include <k3bcore.h>
#include <k3bglobals.h>
#include <k3bversion.h>
#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bdevicehandler.h>
#include <k3bexternalbinmanager.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>
#include <k3bglobalsettings.h>

#include <kprocess.h>
#include <kapplication.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kio/global.h>
#include <kio/job.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qdatastream.h>
#include <kdebug.h>



class K3bDataJob::Private
{
public:
  Private()
    : verificationJob(0) {
  }

  K3bDataDoc* doc;

  bool imageFinished;
  bool canceled;

  KTempFile* tocFile;

  int usedDataMode;
  int usedWritingApp;
  int usedWritingMode;
  K3bDataDoc::MultiSessionMode usedMultiSessionMode;

  int copies;
  int copiesDone;

  K3bDataVerifyingJob* verificationJob;
};


K3bDataJob::K3bDataJob( K3bDataDoc* doc, K3bJobHandler* hdl, QObject* parent )
  : K3bBurnJob( hdl, parent )
{
  d = new Private;

  d->doc = doc;
  m_writerJob = 0;
  d->tocFile = 0;

  m_isoImager = 0;

  m_msInfoFetcher = new K3bMsInfoFetcher( this, this );
  connect( m_msInfoFetcher, SIGNAL(finished(bool)), this, SLOT(slotMsInfoFetched(bool)) );
  connect( m_msInfoFetcher, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_msInfoFetcher, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  d->imageFinished = true;
}

K3bDataJob::~K3bDataJob()
{
  delete d->tocFile;
  delete d;
}


K3bDoc* K3bDataJob::doc() const
{
  return d->doc;
}


K3bDevice::Device* K3bDataJob::writer() const
{
  return doc()->burner();
}


void K3bDataJob::start()
{
  emit started();

  d->canceled = false;
  d->imageFinished = false;
  d->copies = d->doc->copies();
  d->copiesDone = 0;
  d->usedMultiSessionMode = d->doc->multiSessionMode();

  prepareImager();

  if( d->doc->dummy() ) {
    d->doc->setVerifyData( false );
    d->copies = 1;
  }

  emit newTask( i18n("Preparing data") );

  if( d->usedMultiSessionMode == K3bDataDoc::AUTO && !d->doc->onlyCreateImages() )
    determineMultiSessionMode();
  else
    prepareWriting();
}


void K3bDataJob::prepareWriting()
{
  if( !d->doc->onlyCreateImages() && 
      ( d->usedMultiSessionMode == K3bDataDoc::CONTINUE ||
	d->usedMultiSessionMode == K3bDataDoc::FINISH ) ) {
    // no sense continuing the same session twice
    // FIXME: why not?
    d->copies = 1;
    
    m_msInfoFetcher->setDevice( d->doc->burner() );
    waitForDisk();
    
    // just to be sure we did not get canceled during the async discWaiting
    if( d->canceled )
      return;
    
    if( !KIO::findDeviceMountPoint( d->doc->burner()->mountDevice() ).isEmpty() ) {
      emit infoMessage( i18n("Unmounting disk"), INFO );
      // unmount the cd
      connect( KIO::unmount( d->doc->burner()->mountPoint(), false ), SIGNAL(result(KIO::Job*)),
	       m_msInfoFetcher, SLOT(start()) );
    }
    else
      m_msInfoFetcher->start();
  }
  else {
    m_isoImager->setMultiSessionInfo( QString::null );
    determineWritingMode();
    writeImage();
  }
}


void K3bDataJob::slotMsInfoFetched(bool success)
{
  if( d->canceled )
    return;

  if( success ) {
    // we call this here since in ms mode we might want to check
    // the last track's datamode
    determineWritingMode();

    if( d->usedWritingApp == K3b::CDRECORD )
      m_isoImager->setMultiSessionInfo( m_msInfoFetcher->msInfo(), d->doc->burner() );
    else  // cdrdao seems to write a 150 blocks pregap that is not used by cdrecord
      m_isoImager->setMultiSessionInfo( QString("%1,%2").arg(m_msInfoFetcher->lastSessionStart()).arg(m_msInfoFetcher->nextSessionStart()+150), d->doc->burner() );

    writeImage();
  }
  else {
    // the MsInfoFetcher already emitted failure info
    cancelAll();
  }
}


void K3bDataJob::writeImage()
{
  if( d->doc->onTheFly() && !d->doc->onlyCreateImages() ) {
    m_isoImager->calculateSize();
  }
  else {
    emit burning(false);

    // get image file path
    if( d->doc->tempDir().isEmpty() )
      d->doc->setTempDir( K3b::findUniqueFilePrefix( d->doc->isoOptions().volumeID() ) + ".iso" );

    // TODO: check if the image file is part of the project and if so warn the user
    //       and append some number to make the path unique.

    emit newTask( i18n("Creating image file") );
    emit newSubTask( i18n("Track 1 of 1") );
    emit infoMessage( i18n("Creating image file in %1").arg(d->doc->tempDir()), INFO );

    m_isoImager->writeToImageFile( d->doc->tempDir() );
    m_isoImager->start();
  }
}


void K3bDataJob::slotSizeCalculationFinished( int status, int size )
{
  emit debuggingOutput( "K3b", QString( "Size of filesystem calculated: %1" ).arg(size) );

  //
  // this is only called in on-the-fly mode
  //
  if( status == ERROR || !startOnTheFlyWriting() )
    cancelAll();
}


bool K3bDataJob::startOnTheFlyWriting()
{
  if( prepareWriterJob() ) {
    if( startWriterJob() ) {
      // try a direct connection between the processes
      if( m_writerJob->fd() != -1 )
	m_isoImager->writeToFd( m_writerJob->fd() );
      m_isoImager->start();
      return true;
    }
  }
  return false;
}


void K3bDataJob::cancel()
{
  emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
  emit canceled();

  cancelAll();
}


void K3bDataJob::slotIsoImagerPercent( int p )
{
  if( d->doc->onlyCreateImages() ) {
    emit subPercent( p );
    emit percent( p );
  }
  else if( !d->doc->onTheFly() ) {
    double totalTasks = d->copies;
    double tasksDone = d->copiesDone; // =0 when creating an image
    if( d->doc->verifyData() ) {
      totalTasks*=2;
      tasksDone*=2;
    }
    if( !d->doc->onTheFly() ) {
      totalTasks+=1.0;
    }

    emit subPercent( p );
    emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
  }
}


void K3bDataJob::slotIsoImagerFinished( bool success )
{
  if( d->canceled )
    return;

  if( !d->doc->onTheFly() ||
      d->doc->onlyCreateImages() ) {

    if( success ) {
      emit infoMessage( i18n("Image successfully created in %1").arg(d->doc->tempDir()), K3bJob::SUCCESS );
      d->imageFinished = true;

      if( d->doc->onlyCreateImages() ) {
	emit finished( true );
      }
      else {
	if( prepareWriterJob() )
	  startWriterJob();
      }
    }
    else {
      if( m_isoImager->hasBeenCanceled() )
	emit canceled();
      else
	emit infoMessage( i18n("Error while creating ISO image"), ERROR );

      cancelAll();
    }
  }

  // in case we are writing on the fly we leave the error handling to slotWriterJobFinished
}


bool K3bDataJob::startWriterJob()
{
  if( d->doc->dummy() )
    emit newTask( i18n("Simulating") );
  else if( d->copies > 1 )
    emit newTask( i18n("Writing Copy %1").arg(d->copiesDone+1) );
  else
    emit newTask( i18n("Writing") );

  // if we append a new session we asked for an appendable cd already
  if( d->usedMultiSessionMode == K3bDataDoc::NONE ||
      d->usedMultiSessionMode == K3bDataDoc::START ) {
	  
    waitForDisk();

    // just to be sure we did not get canceled during the async discWaiting
    if( d->canceled )
      return false;
  }

  emit burning(true);
  m_writerJob->start();
  return true;
}


void K3bDataJob::slotWriterJobPercent( int p )
{
  double totalTasks = d->copies;
  double tasksDone = d->copiesDone;
  if( d->doc->verifyData() ) {
    totalTasks*=2;
    tasksDone*=2;
  }
  if( !d->doc->onTheFly() ) {
    totalTasks+=1.0;
    tasksDone+=1.0;
  }

  emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3bDataJob::slotWriterNextTrack( int t, int tt )
{
  emit newSubTask( i18n("Writing Track %1 of %2").arg(t).arg(tt) );
}


void K3bDataJob::slotWriterJobFinished( bool success )
{
  if( d->canceled )
    return;


  if( success ) {
    // allright
    // the writerJob should have emited the "simulation/writing successful" signal

    if( d->doc->verifyData() ) {
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
      d->verificationJob->setDoc( d->doc );
      d->verificationJob->setDevice( d->doc->burner() );

      emit newTask( i18n("Verifying written data") );
      emit burning(false);

      d->verificationJob->start();
    }
    else {
      d->copiesDone++;

      if( d->copiesDone < d->copies ) {
	K3bDevice::eject( d->doc->burner() );

	bool failed = false;
	if( d->doc->onTheFly() )
	  failed = !startOnTheFlyWriting();
	else
	  failed = !startWriterJob();

	if( failed )
	  cancelAll();
      }
      else {
	if( !d->doc->onTheFly() && d->doc->removeImages() ) {
	  if( QFile::exists( d->doc->tempDir() ) ) {
	    QFile::remove( d->doc->tempDir() );
	    emit infoMessage( i18n("Removed image file %1").arg(d->doc->tempDir()), K3bJob::SUCCESS );
	  }
	}
	
	if( d->tocFile ) {
	  delete d->tocFile;
	  d->tocFile = 0;
	}
	
	emit finished(true);
      }
    }
  }
  else {
    cancelAll();
  }
}


void K3bDataJob::slotVerificationProgress( int p )
{
  double totalTasks = d->copies*2;
  double tasksDone = d->copiesDone*2 + 1; // the writing of the current copy has already been finished

  if( !d->doc->onTheFly() ) {
    totalTasks+=1.0;
    tasksDone+=1.0;
  }
  
  emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3bDataJob::slotVerificationFinished( bool success )
{
  if( d->canceled )
    return;

  d->copiesDone++;

  if( !k3bcore->globalSettings()->ejectMedia() || d->copiesDone < d->copies )
    K3bDevice::eject( d->doc->burner() );
  
  if( d->copiesDone < d->copies ) {
    bool failed = false;
    if( d->doc->onTheFly() )
      failed = !startOnTheFlyWriting();
    else
      failed = !startWriterJob();
    
    if( failed )
      cancelAll();
  }
  else
    emit finished( success );
}


void K3bDataJob::setWriterJob( K3bAbstractWriter* writer )
{
  // FIXME: progressedsize for multible copies
  m_writerJob = writer;
  connect( m_writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
  connect( m_writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_writerJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_writerJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_writerJob, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
  connect( m_writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writerJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
  connect( m_writerJob, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
  connect( m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
  connect( m_writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
}


void K3bDataJob::setImager( K3bIsoImager* imager )
{
  m_isoImager = imager;
  connect( m_isoImager, SIGNAL(sizeCalculated(int, int)), this, SLOT(slotSizeCalculationFinished(int, int)) );
  connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
  connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
  connect( m_isoImager, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
}


void K3bDataJob::prepareImager()
{
  if( !m_isoImager )
    setImager( new K3bIsoImager( d->doc, this, this ) );
}


bool K3bDataJob::prepareWriterJob()
{
  if( m_writerJob )
    return true;

  // It seems as if cdrecord is not able to append sessions in dao mode whereas cdrdao is
  if( d->usedWritingApp == K3b::CDRECORD )  {
    K3bCdrecordWriter* writer = new K3bCdrecordWriter( d->doc->burner(), this, this );

    // cdrecord manpage says that "not all" writers are able to write
    // multisession disks in dao mode. That means there are writers that can.

    // Does it really make sence to write DAta ms cds in DAO mode since writing the
    // first session of a cd-extra in DAO mode is no problem with my writer while
    // writing the second data session is only possible in TAO mode.
    if( d->usedWritingMode == K3b::DAO &&
	d->usedMultiSessionMode != K3bDataDoc::NONE )
      emit infoMessage( i18n("Most writers do not support writing "
			     "multisession CDs in DAO mode."), INFO );

    writer->setWritingMode( d->usedWritingMode );
    writer->setSimulate( d->doc->dummy() );
    writer->setBurnSpeed( d->doc->speed() );

    // multisession
    if( d->usedMultiSessionMode == K3bDataDoc::START ||
	d->usedMultiSessionMode == K3bDataDoc::CONTINUE ) {
      writer->addArgument("-multi");
    }

    if( d->doc->onTheFly() &&
	( d->usedMultiSessionMode == K3bDataDoc::CONTINUE ||
	  d->usedMultiSessionMode == K3bDataDoc::FINISH ) )
      writer->addArgument("-waiti");

    if( d->usedDataMode == K3b::MODE1 )
      writer->addArgument( "-data" );
    else {
      if( k3bcore->externalBinManager()->binObject("cdrecord") && 
	  k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "xamix" ) )
	writer->addArgument( "-xa" );
      else
	writer->addArgument( "-xa1" );
    }

    if( d->doc->onTheFly() ) {
      writer->addArgument( QString("-tsize=%1s").arg(m_isoImager->size()) )->addArgument("-");
    }
    else {
      writer->addArgument( d->doc->tempDir() );
    }

    setWriterJob( writer );
  }
  else {
    // create cdrdao job
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( d->doc->burner(), this, this );
    writer->setCommand( K3bCdrdaoWriter::WRITE );
    writer->setSimulate( d->doc->dummy() );
    writer->setBurnSpeed( d->doc->speed() );
    // multisession
    writer->setMulti( d->usedMultiSessionMode == K3bDataDoc::START ||
		      d->usedMultiSessionMode == K3bDataDoc::CONTINUE );

    // now write the tocfile
    if( d->tocFile ) delete d->tocFile;
    d->tocFile = new KTempFile( QString::null, "toc" );
    d->tocFile->setAutoDelete(true);

    if( QTextStream* s = d->tocFile->textStream() ) {
      if( d->usedDataMode == K3b::MODE1 ) {
	*s << "CD_ROM" << "\n";
	*s << "\n";
	*s << "TRACK MODE1" << "\n";
      }
      else {
	*s << "CD_ROM_XA" << "\n";
	*s << "\n";
	*s << "TRACK MODE2_FORM1" << "\n";
      }
      if( d->doc->onTheFly() )
	*s << "DATAFILE \"-\" " << m_isoImager->size()*2048 << "\n";
      else
	*s << "DATAFILE \"" << d->doc->tempDir() << "\"\n";

      d->tocFile->close();
    }
    else {
      kdDebug() << "(K3bDataJob) could not write tocfile." << endl;
      emit infoMessage( i18n("IO Error"), ERROR );
      cancelAll();
      return false;
    }

    writer->setTocFile( d->tocFile->name() );

    setWriterJob( writer );
  }

  return true;
}


void K3bDataJob::determineWritingMode()
{
  // we don't need this when only creating image and it is possible
  // that the burn device is null
  if( d->doc->onlyCreateImages() )
    return;

  // first of all we determine the data mode
  if( d->doc->dataMode() == K3b::DATA_MODE_AUTO ) {
    if( !d->doc->onlyCreateImages() &&
	( d->usedMultiSessionMode == K3bDataDoc::CONTINUE ||
	  d->usedMultiSessionMode == K3bDataDoc::FINISH ) ) {

      // try to get the last track's datamode
      // we already asked for an appendable cdr when fetching
      // the ms info
      kdDebug() << "(K3bDataJob) determining last track's datamode..." << endl;

      // FIXME: use a devicethread
      K3bDevice::Toc toc = d->doc->burner()->readToc();
      if( toc.isEmpty() ) {
	kdDebug() << "(K3bDataJob) could not retrieve toc." << endl;
	emit infoMessage( i18n("Unable to determine the last track's datamode. Using default."), ERROR );
	d->usedDataMode = K3b::MODE2;
      }
      else {
	if( toc[toc.count()-1].mode() == K3bDevice::Track::MODE1 )
	  d->usedDataMode = K3b::MODE1;
	else
	  d->usedDataMode = K3b::MODE2;

	kdDebug() << "(K3bDataJob) using datamode: "
		  << (d->usedDataMode == K3b::MODE1 ? "mode1" : "mode2")
		  << endl;
      }
    }
    else if( d->usedMultiSessionMode == K3bDataDoc::NONE )
      d->usedDataMode = K3b::MODE1;
    else
      d->usedDataMode = K3b::MODE2;
  }
  else
    d->usedDataMode = d->doc->dataMode();


  // determine the writing mode
  if( d->doc->writingMode() == K3b::WRITING_MODE_AUTO ) {
    // TODO: put this into the cdreocrdwriter and decide based on the size of the
    // track
    if( writer()->dao() && d->usedDataMode == K3b::MODE1 &&
	d->usedMultiSessionMode == K3bDataDoc::NONE )
      d->usedWritingMode = K3b::DAO;
    else
      d->usedWritingMode = K3b::TAO;
  }
  else
    d->usedWritingMode = d->doc->writingMode();


  // cdrecord seems to have problems writing xa 1 disks in dao mode? At least on my system!
  if( writingApp() == K3b::DEFAULT ) {
    if( d->usedWritingMode == K3b::DAO ) {
      if( d->usedMultiSessionMode != K3bDataDoc::NONE )
	d->usedWritingApp = K3b::CDRDAO;
      else if( d->usedDataMode == K3b::MODE2 )
	d->usedWritingApp = K3b::CDRDAO;
      else
	d->usedWritingApp = K3b::CDRECORD;
    }
    else
      d->usedWritingApp = K3b::CDRECORD;
  }
  else
    d->usedWritingApp = writingApp();
}


void K3bDataJob::determineMultiSessionMode()
{
  if( d->doc->writingMode() == K3b::WRITING_MODE_AUTO ||
      d->doc->writingMode() == K3b::TAO ) {
    emit newSubTask( i18n("Searching for old session") );
    
    // 1. size of the project
    // 2. size of the media
    // 3. space left on appendable media >= project size (otherwise no ms)
    // 4. size of the project >= size of the media*90% (fill up?)
    
    int wantedMediaState = K3bDevice::STATE_INCOMPLETE|K3bDevice::STATE_EMPTY;
    if( d->doc->size() != d->doc->burningSize() ) // this means that an old session has been imported.
      wantedMediaState = K3bDevice::STATE_INCOMPLETE;
    
    int m = waitForMedia( d->doc->burner(), 
			  wantedMediaState,
			  K3bDevice::MEDIA_WRITABLE_CD );
    
    if( m < 0 )
      cancel();
    else {
      // now we need to determine the media's size
      connect( K3bDevice::sendCommand( K3bDevice::DeviceHandler::NG_DISKINFO, d->doc->burner() ), 
	       SIGNAL(finished(K3bDevice::DeviceHandler*)),
	       this, 
	       SLOT(slotDetermineMultiSessionMode(K3bDevice::DeviceHandler*)) );
    }
  }
  else {
    // we need TAO for multisession
    d->usedMultiSessionMode = K3bDataDoc::NONE;

    // carry on with the writing
    prepareWriting();
  }
}


void K3bDataJob::slotDetermineMultiSessionMode( K3bDevice::DeviceHandler* dh )
{
  const K3bDevice::DiskInfo& info = dh->diskInfo();

  if( info.appendable() ) {
    //
    // 3 cases:
    //  1. the project does not fit -> no multisession (resulting in asking for another media above)
    //     Except if an old session has been imported.
    //  2. the project does fit and fills up the CD -> finish multisession
    //  3. the project does fit and does not fill up the CD -> continue multisession
    //
    if( d->doc->size() > info.remainingSize().mode1Bytes() && d->doc->size() == d->doc->burningSize() )
      d->usedMultiSessionMode = K3bDataDoc::NONE;
    else if( d->doc->size() >= info.remainingSize().mode1Bytes()*9/10 )
      d->usedMultiSessionMode = K3bDataDoc::FINISH;
    else
      d->usedMultiSessionMode = K3bDataDoc::CONTINUE;
  }

  else if( info.empty() ) {
    //
    // We only close the CD if the project fills up the CD almost completely (90%)
    //
    if( d->doc->size() >= info.capacity().mode1Bytes()*9/10 ||
	d->doc->writingMode() == K3b::DAO )
      d->usedMultiSessionMode = K3bDataDoc::NONE;
    else
      d->usedMultiSessionMode = K3bDataDoc::START;
  }

  else { // complete (WE ACTUALLY NEVER GET HERE SINCE WE WAIT FOR AN EMPTY/APPENDABLE CD ABOVE!)
    //
    // Now we decide only based on the project size.
    // let's just use a 680 MB CD as our reference
    //
    if( d->doc->size()/1024/1024 >= 680*9/10 ||
	d->doc->writingMode() == K3b::DAO )
      d->usedMultiSessionMode = K3bDataDoc::NONE;
    else
      d->usedMultiSessionMode = K3bDataDoc::START;
  }

  // carry on with the writing
  prepareWriting();
}


void K3bDataJob::cancelAll()
{
  d->canceled = true;

  m_isoImager->cancel();
  m_msInfoFetcher->cancel();
  if( m_writerJob )
    m_writerJob->cancel();
  if( d->verificationJob )
    d->verificationJob->cancel();

  // remove iso-image if it is unfinished or the user selected to remove image
  if( QFile::exists( d->doc->tempDir() ) ) {
    if( (!d->doc->onTheFly() || d->doc->onlyCreateImages() ) && (d->doc->removeImages() || !d->imageFinished) ) {
      emit infoMessage( i18n("Removing ISO image %1").arg(d->doc->tempDir()), K3bJob::SUCCESS );
      QFile::remove( d->doc->tempDir() );
    }
  }

  if( d->tocFile ) {
    delete d->tocFile;
    d->tocFile = 0;
  }

  // TODO: wait for the subjobs to be finished

  emit finished(false);
}


void K3bDataJob::waitForDisk()
{
  emit newSubTask( i18n("Waiting for media") );
  if( waitForMedia( d->doc->burner(), 
		    d->usedMultiSessionMode == K3bDataDoc::CONTINUE ||
		    d->usedMultiSessionMode == K3bDataDoc::FINISH ?
		    K3bDevice::STATE_INCOMPLETE :
		    K3bDevice::STATE_EMPTY,
		    K3bDevice::MEDIA_WRITABLE_CD ) < 0 ) {
    cancel();
  }
}


QString K3bDataJob::jobDescription() const
{
  if( d->doc->onlyCreateImages() ) {
    return i18n("Creating Data Image File");
  }
  else if( d->doc->multiSessionMode() == K3bDataDoc::NONE ||
	   d->doc->multiSessionMode() == K3bDataDoc::AUTO ) {
    return i18n("Writing Data CD")
      + ( d->doc->isoOptions().volumeID().isEmpty()
	  ? QString::null
	  : QString( " (%1)" ).arg(d->doc->isoOptions().volumeID()) );
  }
  else {
    return i18n("Writing Multisession CD")
      + ( d->doc->isoOptions().volumeID().isEmpty()
	  ? QString::null
	  : QString( " (%1)" ).arg(d->doc->isoOptions().volumeID()) );
  }
}


QString K3bDataJob::jobDetails() const
{
  if( d->doc->copies() > 1 && 
      !d->doc->dummy() &&
      !(d->doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
	d->doc->multiSessionMode() == K3bDataDoc::FINISH) )
    return i18n("ISO9660 Filesystem (Size: %1) - %n copy", 
		"ISO9660 Filesystem (Size: %1) - %n copies",
		d->doc->copies() )
      .arg(KIO::convertSize( d->doc->size() ));
  else
    return i18n("ISO9660 Filesystem (Size: %1)")
      .arg(KIO::convertSize( d->doc->size() ));
}

#include "k3bdatajob.moc"
