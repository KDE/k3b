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


#include "k3biso9660imagewritingjob.h"
#include "k3bisoimageverificationjob.h"

#include <k3bdevice.h>
#include <k3bdiskinfo.h>
#include <k3bdevicehandler.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>
#include <k3bgrowisofswriter.h>
#include <k3bglobals.h>
#include <k3bemptydiscwaiter.h>
#include <k3bcore.h>
#include <k3bversion.h>
#include <k3bexternalbinmanager.h>
#include <k3bglobals.h>

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <ktempfile.h>
#include <kmessagebox.h>
#include <kio/global.h>

#include <qstring.h>
#include <qtextstream.h>
#include <qfile.h>
#include <qapplication.h>


K3bIso9660ImageWritingJob::K3bIso9660ImageWritingJob()
  : K3bBurnJob(),
    m_writingMode(K3b::WRITING_MODE_AUTO),
    m_simulate(false),
    m_burnproof(false),
    m_device(0),
    m_noFix(false),
    m_speed(2),
    m_dataMode(K3b::DATA_MODE_AUTO),
    m_writer(0),
    m_tocFile(0),
    m_copies(1),
    m_verifyJob(0)
{
}

K3bIso9660ImageWritingJob::~K3bIso9660ImageWritingJob()
{
  delete m_tocFile;
}


void K3bIso9660ImageWritingJob::start()
{
  m_canceled = m_finished = false;
  m_currentCopy = 1;

  emit started();

  if( m_simulate )
    m_verifyData = false;

  if( !QFile::exists( m_imagePath ) ) {
    emit infoMessage( i18n("Could not find image %1").arg(m_imagePath), K3bJob::ERROR );
    emit finished( false );
    return;
  }

  startWriting();
}


void K3bIso9660ImageWritingJob::slotWriterJobFinished( bool success )
{
  if( m_canceled ) {
    m_finished = true;
    emit canceled();
    emit finished(false);
    return;
  }

  if( success ) {
    if( !m_simulate && m_verifyData ) {
      emit burning(false);

      // allright
      // the writerJob should have emited the "simulation/writing successful" signal

      if( !m_verifyJob ) {
	m_verifyJob = new K3bIsoImageVerificationJob( this );
	connectSubJob( m_verifyJob,
		       SLOT(slotVerificationFinished(bool)),
		       true,
		       SLOT(slotVerificationProgress(int)),
		       SIGNAL(subPercent(int)) );
      }
      m_verifyJob->setDevice( m_device );
      m_verifyJob->setImageFileName( m_imagePath );
      if( m_copies == 1 )
	emit newTask( i18n("Verifying written data") );
      else
	emit newTask( i18n("Verifying written copy %1 of %2").arg(m_currentCopy).arg(m_copies) );
      m_verifyJob->start();
    }
    else if( m_currentCopy >= m_copies ) {
      m_finished = true;
      emit finished(true);
    }
    else {
      m_currentCopy++;
      startWriting();
    }
  }
  else {
    m_finished = true;
    emit finished(false);
  }
}


void K3bIso9660ImageWritingJob::slotVerificationFinished( bool success )
{
  if( m_canceled ) {
    m_finished = true;
    emit canceled();
    emit finished(false);
    return;
  }

  if( success && m_currentCopy < m_copies ) {
    m_currentCopy++;
    connect( K3bCdDevice::eject( m_device ), SIGNAL(finished(bool)),
	     this, SLOT(startWriting()) );
    return;
  }

  k3bcore->config()->setGroup("General Options");
  if( !k3bcore->config()->readBoolEntry( "No cd eject", false ) )
    K3bCdDevice::eject( m_device );

  m_finished = true;
  emit finished( success );
}


void K3bIso9660ImageWritingJob::slotVerificationProgress( int p )
{
  emit percent( (int)(100.0 / (double)m_copies * ( (double)(m_currentCopy-1) + 0.5 + (double)p/200.0 )) );
}


void K3bIso9660ImageWritingJob::slotWriterPercent( int p )
{
  emit subPercent( p );

  if( m_verifyData )
    emit percent( (int)(100.0 / (double)m_copies * ( (double)(m_currentCopy-1) + ((double)p/200.0) )) );
  else
    emit percent( (int)(100.0 / (double)m_copies * ( (double)(m_currentCopy-1) + ((double)p/100.0) )) );
}


void K3bIso9660ImageWritingJob::slotNextTrack( int, int )
{
  if( m_copies == 1 )
    emit newSubTask( i18n("Writing image") );
  else
    emit newSubTask( i18n("Writing copy %1 of %2").arg(m_currentCopy).arg(m_copies) );
}


void K3bIso9660ImageWritingJob::cancel()
{
  if( !m_finished ) {
    m_canceled = true;

    if( m_writer )
      m_writer->cancel();
    if( m_verifyData && m_verifyJob )
      m_verifyJob->cancel();
  }
}


void K3bIso9660ImageWritingJob::startWriting()
{
  // we wait for the following:
  // 1. if writing mode auto and writing app auto: all writable media types
  // 2. if writing mode auto and writing app not growisofs: all writable cd types
  // 3. if writing mode auto and writing app growisofs: all writable dvd types
  // 4. if writing mode TAO or RAW: all writable cd types
  // 5. if writing mode DAO and writing app auto: all writable cd types and DVD-R(W)
  // 6. if writing mode DAO and writing app GROWISOFS: DVD-R(W)
  // 7. if writing mode DAO and writing app CDRDAO or CDRECORD: all writable cd types
  // 8. if writing mode WRITING_MODE_INCR_SEQ: DVD-R(W)
  // 9. if writing mode WRITING_MODE_RES_OVWR: DVD-RW or DVD+RW

  int mt = 0;
  if( m_writingMode == K3b::WRITING_MODE_AUTO ) {
    if( writingApp() == K3b::DEFAULT )
      mt = K3bCdDevice::MEDIA_WRITABLE_DVD|K3bCdDevice::MEDIA_WRITABLE_CD;
    else if( writingApp() != K3b::GROWISOFS )
      mt = K3bCdDevice::MEDIA_WRITABLE_CD;
    else
      mt = K3bCdDevice::MEDIA_WRITABLE_DVD;
  }
  else if( m_writingMode == K3b::TAO || m_writingMode == K3b::RAW )
    mt = K3bCdDevice::MEDIA_WRITABLE_CD;
  else if( m_writingMode == K3b::DAO ) {
    if( writingApp() == K3b::DEFAULT )
      mt = K3bCdDevice::MEDIA_WRITABLE_CD|K3bCdDevice::MEDIA_DVD_RW_SEQ|K3bCdDevice::MEDIA_DVD_R_SEQ;
    else if( writingApp() == K3b::GROWISOFS )
      mt = K3bCdDevice::MEDIA_DVD_RW_SEQ|K3bCdDevice::MEDIA_DVD_R_SEQ;
    else
      mt = K3bCdDevice::MEDIA_WRITABLE_CD;
  }
  else if( m_writingMode == K3b::WRITING_MODE_INCR_SEQ )
    mt = K3bCdDevice::MEDIA_DVD_RW_SEQ|K3bCdDevice::MEDIA_DVD_R_SEQ;
  else
    mt = K3bCdDevice::MEDIA_DVD_PLUS_R|K3bCdDevice::MEDIA_DVD_PLUS_RW|K3bCdDevice::MEDIA_DVD_RW_OVWR;


  // wait for the media
  int media = K3bEmptyDiscWaiter::wait( m_device, false, mt );
  if( media == K3bEmptyDiscWaiter::CANCELED ) {
    m_finished = true;
    emit canceled();
    emit finished(false);
    return;
  }

  if( prepareWriter( media ) ) {
    emit burning(true);
    m_writer->start();
  }
  else {
    m_finished = true;
    emit finished(false);
  }
}


bool K3bIso9660ImageWritingJob::prepareWriter( int mediaType )
{
  if( mediaType == 0 ) { // media forced
    // just to get it going...
    if( writingApp() != K3b::GROWISOFS )
      mediaType = K3bCdDevice::MEDIA_CD_R;
    else
      mediaType = K3bCdDevice::MEDIA_DVD_R;
  }

  delete m_writer;

  if( mediaType == K3bCdDevice::MEDIA_CD_R || mediaType == K3bCdDevice::MEDIA_CD_RW ) {
    int usedWriteMode = m_writingMode;
    if( usedWriteMode == K3b::WRITING_MODE_AUTO ) {
      // cdrecord seems to have problems when writing in mode2 in dao mode
      // so with cdrecord we use TAO
      if( m_noFix || m_dataMode == K3b::MODE2 || !m_device->dao() )
	usedWriteMode = K3b::TAO;
      else
	usedWriteMode = K3b::DAO;
    }

    // and many writers fail to write DAO with cdrecord. With this release we try cdrdao ;)
    int usedApp = writingApp();
    if( usedApp == K3b::DEFAULT ) {
      if( k3bcore->externalBinManager()->binObject( "cdrdao" ) && 
	  ( k3bcore->externalBinManager()->binObject( "cdrdao" )->version >= K3bVersion( 1, 1, 7 ) ||
	   ( usedWriteMode == K3b::DAO && ( m_dataMode == K3b::MODE2 || m_noFix ) ) ) )
	usedApp = K3b::CDRDAO;
      else
	usedApp = K3b::CDRECORD;
    }


    if( usedApp == K3b::CDRECORD ) {
      K3bCdrecordWriter* writer = new K3bCdrecordWriter( m_device, this );

      writer->setWritingMode( usedWriteMode );
      writer->setSimulate( m_simulate );
      writer->setBurnproof( m_burnproof );
      writer->setBurnSpeed( m_speed );

      if( m_noFix ) {
	writer->addArgument("-multi");
      }

      if( (m_dataMode == K3b::DATA_MODE_AUTO && m_noFix) ||
	  m_dataMode == K3b::MODE2 ) {
	if( k3bcore->externalBinManager()->binObject("cdrecord") && 
	    k3bcore->externalBinManager()->binObject("cdrecord")->version >= K3bVersion( 2, 1, -1, "a12" ) )
	  writer->addArgument( "-xa" );
	else
	  writer->addArgument( "-xa1" );
      }
      else
	writer->addArgument("-data");

      writer->addArgument( m_imagePath );

      m_writer = writer;
    }
    else {
      // create cdrdao job
      K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_device, this );
      writer->setCommand( K3bCdrdaoWriter::WRITE );
      writer->setSimulate( m_simulate );
      writer->setBurnSpeed( m_speed );
      // multisession
      writer->setMulti( m_noFix );

      // now write the tocfile
      delete m_tocFile;
      m_tocFile = new KTempFile( QString::null, "toc" );
      m_tocFile->setAutoDelete(true);

      if( QTextStream* s = m_tocFile->textStream() ) {
	if( (m_dataMode == K3b::DATA_MODE_AUTO && m_noFix) ||
	    m_dataMode == K3b::MODE2 ) {
	  *s << "CD_ROM_XA" << "\n";
	  *s << "\n";
	  *s << "TRACK MODE2_FORM1" << "\n";
	}
	else {
	  *s << "CD_ROM" << "\n";
	  *s << "\n";
	  *s << "TRACK MODE1" << "\n";
	}
	*s << "DATAFILE \"" << m_imagePath << "\" 0 \n";

	m_tocFile->close();
      }
      else {
	kdDebug() << "(K3bDataJob) could not write tocfile." << endl;
	emit infoMessage( i18n("IO Error"), ERROR );
	return false;
      }

      writer->setTocFile( m_tocFile->name() );

      m_writer = writer;
    }
  }
  else {  // DVD
    if( mediaType & (K3bCdDevice::MEDIA_DVD_PLUS_RW|K3bCdDevice::MEDIA_DVD_PLUS_R) ) {
      if( m_simulate ) {
	if( KMessageBox::warningYesNo( qApp->activeWindow(),
				       i18n("K3b does not support simulation with DVD+R(W) media. "
					    "Do you really want to continue? The media will be written "
					    "for real."),
				       i18n("No simulation with DVD+R(W)") ) == KMessageBox::No ) {
	  return false;
	}
      }
      
      if( m_speed > 0 ) {
	emit infoMessage( i18n("DVD+R(W) writers do take care of the writing speed themselves."), INFO );
	emit infoMessage( i18n("The K3b writing speed setting is ignored for DVD+R(W) media."), INFO );
      }
    }

    K3bGrowisofsWriter* writer = new K3bGrowisofsWriter( m_device, this );
    writer->setSimulate( m_simulate );
    writer->setBurnSpeed( m_speed );
    writer->setWritingMode( m_writingMode == K3b::DAO ? K3b::DAO : 0 );
    writer->setImageToWrite( m_imagePath );

    m_writer = writer;
  }


  connect( m_writer, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writer, SIGNAL(nextTrack(int, int)), this, SLOT(slotNextTrack(int, int)) );
  connect( m_writer, SIGNAL(percent(int)), this, SLOT(slotWriterPercent(int)) );
  connect( m_writer, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_writer, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writer, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
  connect( m_writer, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
  connect( m_writer, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_writer, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writer, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}


QString K3bIso9660ImageWritingJob::jobDescription() const
{
  if( m_simulate )
    return i18n("Simulating ISO9660 image");
  else if( m_copies == 1 )
    return i18n("Burning ISO9660 image");
  else
    return i18n("Burning ISO9660 image (%1 copies)").arg(m_copies);
}


QString K3bIso9660ImageWritingJob::jobDetails() const
{
  return m_imagePath.section("/", -1) + QString( " (%1)" ).arg(KIO::convertSize(K3b::filesize(m_imagePath)));
}



#include "k3biso9660imagewritingjob.moc"
