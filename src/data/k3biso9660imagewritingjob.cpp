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


#include "k3biso9660imagewritingjob.h"

#include <device/k3bdevice.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>
#include <tools/k3bglobals.h>
#include <k3bemptydiscwaiter.h>

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <ktempfile.h>

#include <qstring.h>
#include <qtextstream.h>
#include <qfile.h>


K3bIso9660ImageWritingJob::K3bIso9660ImageWritingJob()
  : K3bBurnJob(),
    m_writingMode(K3b::WRITING_MODE_AUTO),
    m_simulate(false),
    m_burnproof(false),
    m_device(0),
    m_noFix(false),
    m_speed(2),
    m_dataMode(K3b::AUTO),
    m_writer(0),
    m_tocFile(0)
{
}

K3bIso9660ImageWritingJob::~K3bIso9660ImageWritingJob()
{
  if( m_tocFile )
    delete m_tocFile;
}


void K3bIso9660ImageWritingJob::start()
{
  emit started();

  if( !QFile::exists( m_imagePath ) ) {
    emit infoMessage( i18n("Could not find image %1").arg(m_imagePath), K3bJob::ERROR );
    emit finished( false );
    return;
  }

  if( prepareWriter() ) {
    if( K3bEmptyDiscWaiter::wait( m_device ) == K3bEmptyDiscWaiter::CANCELED ) {
      emit canceled();
      emit finished(false);
    }
    else {
      m_writer->start();
    }
  }
}


void K3bIso9660ImageWritingJob::slotWriterJobFinished( bool success )
{
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
    emit finished(false);
  }
}


void K3bIso9660ImageWritingJob::cancel()
{
  if( m_writer ) {
    emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
    emit canceled();
    
    m_writer->cancel();
  }
}


bool K3bIso9660ImageWritingJob::prepareWriter()
{
  if( m_writer )
    delete m_writer;

  int usedWriteMode = m_writingMode;
  if( usedWriteMode == K3b::WRITING_MODE_AUTO ) {
    // cdrecord seems to have problems when writing in mode2 in dao mode
    // so with cdrecord we use TAO
    if( m_noFix || m_dataMode == K3b::MODE2 )
      usedWriteMode = K3b::TAO;
    else
      usedWriteMode = K3b::DAO;
  }

  int usedApp = writingApp();
  if( usedApp == K3b::DEFAULT ) {
    if( usedWriteMode == K3b::DAO && 
	( m_dataMode == K3b::MODE2 || m_noFix ) )
      usedApp = K3b::CDRDAO;
    else
      usedApp = K3b::CDRECORD;
  }


  if( usedApp == K3b::CDRECORD ) {
    K3bCdrecordWriter* writer = new K3bCdrecordWriter( m_device, this );

    writer->setDao( false );
    writer->setSimulate( m_simulate );
    writer->setBurnproof( m_burnproof );
    writer->setBurnSpeed( m_speed );
    writer->prepareArgumentList();

    if( m_noFix ) {
      writer->addArgument("-multi");
    }

    if( usedWriteMode == K3b::DAO )
      writer->addArgument( "-dao" );
    else if( usedWriteMode == K3b::RAW )
      writer->addArgument( "-raw" );

    if( (m_dataMode == K3b::AUTO && m_noFix) ||
	m_dataMode == K3b::MODE2 )
      writer->addArgument("-xa1");
    else
      writer->addArgument("-data");

    writer->addArgument( m_imagePath );

    m_writer = writer;
  }
  else {
    // create cdrdao job
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_device, this );
    writer->setSimulate( m_simulate );
    writer->setBurnSpeed( m_speed );
    // multisession
    writer->setMulti( m_noFix );

    // now write the tocfile
    if( m_tocFile ) delete m_tocFile;
    m_tocFile = new KTempFile( QString::null, "toc" );
    m_tocFile->setAutoDelete(true);

    if( QTextStream* s = m_tocFile->textStream() ) {
      if( (m_dataMode == K3b::AUTO && m_noFix) ||
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
      emit finished(false);
      return false;
    }

    writer->setTocFile( m_tocFile->name() );

    m_writer = writer;
  }
    
  connect( m_writer, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writer, SIGNAL(percent(int)), this, SIGNAL(percent(int)) );
  //  connect( m_writer, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_writer, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_writer, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writer, SIGNAL(writeSpeed(int)), this, SIGNAL(writeSpeed(int)) );
  connect( m_writer, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
  connect( m_writer, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_writer, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writer, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}


QString K3bIso9660ImageWritingJob::jobDescription() const
{
  return i18n("Writing Iso9660 image");
}


QString K3bIso9660ImageWritingJob::jobDetails() const
{
  return m_imagePath.section("/", -1);
}
		


#include "k3biso9660imagewritingjob.moc"
