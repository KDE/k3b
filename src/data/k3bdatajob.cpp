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


#include "k3bdatajob.h"
#include "k3bdatadoc.h"
#include "k3bisoimager.h"
#include "k3bmsinfofetcher.h"
#include <k3b.h>
#include <tools/k3bglobals.h>
#include <device/k3bdevice.h>
#include <k3bemptydiscwaiter.h>
#include <tools/k3bexternalbinmanager.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>


#include <kprocess.h>
#include <kapplication.h>
#include <kconfig.h>
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




K3bDataJob::K3bDataJob( K3bDataDoc* doc, QObject* parent )
  : K3bBurnJob(parent)
{
  m_doc = doc;
  m_writerJob = 0;
  m_tocFile = 0;

  m_isoImager = new K3bIsoImager( m_doc, this );
  connect( m_isoImager, SIGNAL(sizeCalculated(int, int)), this, SLOT(slotSizeCalculationFinished(int, int)) );
  connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_isoImager, SIGNAL(data(char*, int)), this, SLOT(slotReceivedIsoImagerData(char*, int)) );
  connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
  connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
  connect( m_isoImager, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  m_msInfoFetcher = new K3bMsInfoFetcher( this );
  connect( m_msInfoFetcher, SIGNAL(finished(bool)), this, SLOT(slotMsInfoFetched(bool)) );
  connect( m_msInfoFetcher, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );

  m_imageFinished = true;
}

K3bDataJob::~K3bDataJob()
{
  if( m_tocFile )
    delete m_tocFile;
}


K3bDoc* K3bDataJob::doc() const
{
  return m_doc;
}


K3bDevice* K3bDataJob::writer() const
{
  return doc()->burner();
}


void K3bDataJob::start()
{
  emit started();

  m_canceled = false;
  m_imageFinished = false;


  if( writingApp() == K3b::CDRECORD || 
      (writingApp() == K3b::DEFAULT && !(m_doc->dao() && !m_doc->multiSessionMode() == K3bDataDoc::NONE)) )
    m_usedWritingApp = K3b::CDRECORD;
  else 
    m_usedWritingApp = K3b::CDRDAO;


  if( !m_doc->onlyCreateImage() && 
      ( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
	m_doc->multiSessionMode() == K3bDataDoc::FINISH ) ) {
    m_msInfoFetcher->setDevice( m_doc->burner() );
    K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );
    if( waiter.waitForEmptyDisc( true ) == K3bEmptyDiscWaiter::CANCELED ) {
      cancel();
      return;
    }

    if( !KIO::findDeviceMountPoint( m_doc->burner()->mountDevice() ).isEmpty() ) {
      emit infoMessage( i18n("Unmounting disk"), INFO );
      // unmount the cd
      connect( KIO::unmount( m_doc->burner()->mountPoint(), false ), SIGNAL(result(KIO::Job*)),
	       m_msInfoFetcher, SLOT(start()) );
    }
    else
      m_msInfoFetcher->start();
  }
  else {
    m_isoImager->setMultiSessionInfo( QString::null );
    writeImage();
  }
}


void K3bDataJob::slotMsInfoFetched(bool success)
{
  if( m_canceled )
    return;

  if( success ) {
    if( m_usedWritingApp == K3b::CDRECORD )
      m_isoImager->setMultiSessionInfo( m_msInfoFetcher->msInfo(), m_doc->burner() );
    else  // cdrdao seems to write a 150 blocks pregap that is not used by cdrecord
      m_isoImager->setMultiSessionInfo( QString("%1,%2").arg(m_msInfoFetcher->lastSessionStart()).arg(m_msInfoFetcher->nextSessionStart()+150), m_doc->burner() );

    writeImage();
  }
  else {
    // the MsInfoFetcher already emitted failure info
    cancelAll();
  }
}


void K3bDataJob::writeImage()
{
  emit newTask( i18n("Writing data") );

  if( m_doc->onTheFly() && !m_doc->onlyCreateImage() ) {
    m_isoImager->calculateSize();
  }
  else {
    // get image file path
    if( m_doc->isoImage().isEmpty() )
      m_doc->setIsoImage( k3bMain()->findTempFile( "iso" ) );
    
    // open the file for writing
    m_imageFile.setName( m_doc->isoImage() );
    if( !m_imageFile.open( IO_WriteOnly ) ) {
      emit infoMessage( i18n("Could not open %1 for writing").arg(m_doc->isoImage()), ERROR );
      cancelAll();
      emit finished(false);
      return;
    }

    emit infoMessage( i18n("Writing image file to %1").arg(m_doc->isoImage()), INFO );
    emit newSubTask( i18n("Creating image file") );

    m_imageFileStream.setDevice( &m_imageFile );
    m_isoImager->start();
  }
}


void K3bDataJob::slotSizeCalculationFinished( int status, int size )
{
  emit infoMessage( i18n("Size calculated:") + i18n("%1 (1 Byte)", "%1 (%n bytes)", size*2048).arg(size), INFO );
  if( status != ERROR ) {
    // this only happens in on-the-fly mode
    if( prepareWriterJob() ) {
      startWriting();
      m_isoImager->start();
    }
  }
  else {
    cancelAll();
  }

}


void K3bDataJob::cancel()
{
  emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
  emit canceled();

  cancelAll();
}


void K3bDataJob::slotReceivedIsoImagerData( char* data, int len )
{
  if( !m_doc->onlyCreateImage() && m_doc->onTheFly() ) {
    if( !m_writerJob ) {
      kdError() << "(K3bDataJob) ERROR: no writer job" << endl;
      cancelAll();
      return;
    }
    if( !m_writerJob->write( data, len ) )
      kdError() << "(K3bDataJob) Error while writing data to Writer" << endl;
  }
  else {
    m_imageFileStream.writeRawBytes( data, len );
    m_isoImager->resume();
  }
}


void K3bDataJob::slotIsoImagerPercent( int p )
{
  if( m_doc->onlyCreateImage() ) {
    emit percent( p  );
    emit subPercent( p );
  }
  else if( !m_doc->onTheFly() ) {
    emit subPercent( p );
    emit percent( p/2 );
  }
}


void K3bDataJob::slotIsoImagerFinished( bool success )
{
  if( m_canceled )
    return;

  if( !m_doc->onTheFly() ||
      m_doc->onlyCreateImage() ) {
    m_imageFile.close();
    if( success ) {
      emit infoMessage( i18n("Image successfully created in %1").arg(m_doc->isoImage()), K3bJob::STATUS );
      m_imageFinished = true;
    
      if( m_doc->onlyCreateImage() ) {
	emit finished( true );
      }
      else {
	if( prepareWriterJob() )
	  startWriting();
      }
    }
  }

  if( !success ) {
    emit infoMessage( i18n("Error while creating iso image"), ERROR );
    cancelAll();
  }
}


void K3bDataJob::startWriting()
{
  // if we append a new session we asked for an appendable cd already
  if( m_doc->multiSessionMode() == K3bDataDoc::NONE ||
      m_doc->multiSessionMode() == K3bDataDoc::START ) {
	  
    K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );
    if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
      cancel();
      return;
    }
  }
	
  m_writerJob->start();
}


void K3bDataJob::slotWriterJobPercent( int p )
{
  if( m_doc->onTheFly() )
    emit percent( p );
  else {
    emit percent( 50 + p/2 );
  }
}


void K3bDataJob::slotWriterNextTrack( int t, int tt )
{
  emit newSubTask( i18n("Writing Track %1 of %2").arg(t).arg(tt) );
}


void K3bDataJob::slotDataWritten()
{
  m_isoImager->resume();
}


void K3bDataJob::slotWriterJobFinished( bool success )
{
  if( m_canceled )
    return;

  if( !m_doc->onTheFly() && m_doc->deleteImage() ) {
    QFile::remove( m_doc->isoImage() );
    m_doc->setIsoImage("");
    emit infoMessage( i18n("Removed image file %1").arg(m_doc->isoImage()), K3bJob::STATUS );
  }

  if( m_tocFile ) {
    delete m_tocFile;
    m_tocFile = 0;
  }

  if( success ) {
    // allright
    // the writerJob should have emited the "simulation/writing successful" signal
    emit finished(true);
  }
  else {
    cancelAll();
  }
}


bool K3bDataJob::prepareWriterJob()
{
  if( m_writerJob )
    delete m_writerJob;

  // It seems as if cdrecord is not able to append sessions in dao mode whereas cdrdao is
  if( m_usedWritingApp == K3b::CDRECORD )  {
    K3bCdrecordWriter* writer = new K3bCdrecordWriter( m_doc->burner(), this );

    // cdrecord manpage says that "not all" writers are able to write
    // multisession disks in dao mode. That means there are writers that can.

    // Does it really make sence to write DAta ms cds in DAO mode since writing the
    // first session of a cd-extra in DAO mode is no problem with my writer while
    // writing the second data session is only possible in TAO mode.
    writer->setDao( m_doc->dao() );
    writer->setSimulate( m_doc->dummy() );
    writer->setBurnproof( m_doc->burnproof() );
    writer->setBurnSpeed( m_doc->speed() );
    writer->prepareArgumentList();

    // multisession
    if( m_doc->multiSessionMode() == K3bDataDoc::START ||
	m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ) {
      writer->addArgument("-multi");
    }

    if( m_doc->onTheFly() &&
	( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
	  m_doc->multiSessionMode() == K3bDataDoc::FINISH ) )
      writer->addArgument("-waiti");

    if( m_doc->multiSessionMode() == K3bDataDoc::NONE )
      writer->addArgument( "-data" );  // default to mode1
    else
      writer->addArgument( "-xa1" );

    if( m_doc->onTheFly() ) {
      writer->addArgument( QString("-tsize=%1s").arg(m_isoImager->size()) )->addArgument("-");
      writer->setProvideStdin(true);
    }
    else {
      writer->addArgument( m_doc->isoImage() );
    }

    m_writerJob = writer;
  }
  else {
    // create cdrdao job
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_doc->burner(), this );
    writer->setSimulate( m_doc->dummy() );
    writer->setBurnSpeed( m_doc->speed() );
    // multisession
    writer->setMulti( m_doc->multiSessionMode() == K3bDataDoc::START ||
		      m_doc->multiSessionMode() == K3bDataDoc::CONTINUE );

    if( m_doc->onTheFly() ) {
      writer->setProvideStdin(true);
    }

    // now write the tocfile
    if( m_tocFile ) delete m_tocFile;
    m_tocFile = new KTempFile( QString::null, "toc" );
    m_tocFile->setAutoDelete(true);

    if( QTextStream* s = m_tocFile->textStream() ) {
      if( m_doc->multiSessionMode() == K3bDataDoc::NONE ) {
	*s << "CD_ROM" << "\n";
	*s << "\n";
	*s << "TRACK MODE1" << "\n";
      }
      else {
	*s << "CD_ROM_XA" << "\n";
	*s << "\n";
	*s << "TRACK MODE2_FORM1" << "\n";
      }
      if( m_doc->onTheFly() )
	*s << "DATAFILE \"-\" " << m_isoImager->size()*2048 << "\n";
      else
	*s << "DATAFILE \"" << m_doc->isoImage() << "\"\n";

      m_tocFile->close();
    }
    else {
      kdDebug() << "(K3bDataJob) could not write tocfile." << endl;
      emit infoMessage( i18n("IO Error"), ERROR );
      cancelAll();
      return false;
    }

    writer->setTocFile( m_tocFile->name() );

    m_writerJob = writer;
  }
    
  connect( m_writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
  connect( m_writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_writerJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_writerJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_writerJob, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
  connect( m_writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writerJob, SIGNAL(writeSpeed(int)), this, SIGNAL(writeSpeed(int)) );
  connect( m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
  connect( m_writerJob, SIGNAL(dataWritten()), this, SLOT(slotDataWritten()) );
  connect( m_writerJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}


void K3bDataJob::cancelAll()
{
  m_canceled = true;

  m_isoImager->cancel();
  m_msInfoFetcher->cancel();
  if( m_writerJob )
    m_writerJob->cancel();

  // remove iso-image if it is unfinished or the user selected to remove image
  if( QFile::exists( m_doc->isoImage() ) ) {
    if( !m_doc->onTheFly() && m_doc->deleteImage() || !m_imageFinished ) {
      emit infoMessage( i18n("Removing ISO image %1").arg(m_doc->isoImage()), K3bJob::STATUS );
      QFile::remove( m_doc->isoImage() );
      m_doc->setIsoImage("");
    }
  }

  if( m_tocFile ) {
    delete m_tocFile;
    m_tocFile = 0;
  }

  emit finished(false);
}


#include "k3bdatajob.moc"
