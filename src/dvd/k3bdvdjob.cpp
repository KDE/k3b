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


#include "k3bdvdjob.h"
#include "k3bdvddoc.h"

#include <k3bdvdrecordwriter.h>
#include <k3bisoimager.h>
#include <k3bemptydiscwaiter.h>

#include <klocale.h>


K3bDvdJob::K3bDvdJob( K3bDvdDoc* doc, QObject* parent )
  : K3bDataJob( doc, parent ),
    m_doc( doc )
{
}


K3bDvdJob::~K3bDvdJob()
{
}


bool K3bDvdJob::prepareWriterJob()
{
  if( m_writerJob )
    delete m_writerJob;

  K3bDvdrecordWriter* writer = new K3bDvdrecordWriter( m_doc->burner(), this );

  writer->setSimulate( m_doc->dummy() );
  writer->setBurnproof( m_doc->burnproof() );
  writer->setBurnSpeed( m_doc->speed() );

  // multisession
  if( m_doc->multiSessionMode() == K3bDataDoc::START ||
      m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ) {
    writer->addArgument("-multi");
  }

  if( m_doc->onTheFly() &&
      ( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
	m_doc->multiSessionMode() == K3bDataDoc::FINISH ) )
    writer->addArgument("-waiti");

  if( m_doc->onTheFly() ) {
    writer->addArgument( QString("-tsize=%1s").arg(m_isoImager->size()) )->addArgument("-");
    writer->setProvideStdin(true);
  }
  else {
    writer->addArgument( m_doc->tempDir() );
  }

  m_writerJob = writer;
    
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


void K3bDvdJob::waitForDisk()
{
  if( K3bEmptyDiscWaiter::wait( m_doc->burner(), 
				m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
				m_doc->multiSessionMode() == K3bDataDoc::FINISH, 
				true )
      == K3bEmptyDiscWaiter::CANCELED ) {
    cancel();
  }
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
