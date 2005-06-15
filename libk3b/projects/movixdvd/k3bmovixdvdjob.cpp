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


#include "k3bmovixdvdjob.h"
#include "k3bmovixdvddoc.h"
#include "k3bmovixfileitem.h"
#include "k3bmovixdocpreparer.h"

#include <k3bcore.h>
#include <k3bdvdjob.h>
#include <k3bdevice.h>

#include <klocale.h>
#include <kdebug.h>


K3bMovixDvdJob::K3bMovixDvdJob( K3bMovixDvdDoc* doc, K3bJobHandler* jh, QObject* parent )
  : K3bBurnJob( jh, parent ),
    m_doc(doc)
{
  m_dvdJob = new K3bDvdJob( doc, this, this );
  m_movixDocPreparer = new K3bMovixDocPreparer( doc, this, this );

  // pipe signals
  connect( m_dvdJob, SIGNAL(percent(int)), this, SIGNAL(percent(int)) );
  connect( m_dvdJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_dvdJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_dvdJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_dvdJob, SIGNAL(bufferStatus(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_dvdJob, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
  connect( m_dvdJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_dvdJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_dvdJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  connect( m_dvdJob, SIGNAL(infoMessage(const QString&, int)),
	   this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_dvdJob, SIGNAL(burning(bool)), this, SIGNAL(burning(bool)) );

  // we need to clean up here
  connect( m_dvdJob, SIGNAL(finished(bool)), this, SLOT(slotDvdJobFinished(bool)) );

  connect( m_movixDocPreparer, SIGNAL(infoMessage(const QString&, int)),
	   this, SIGNAL(infoMessage(const QString&, int)) );
}


K3bMovixDvdJob::~K3bMovixDvdJob()
{
}


K3bDevice::Device* K3bMovixDvdJob::writer() const
{
  return m_dvdJob->writer();
}


K3bDoc* K3bMovixDvdJob::doc() const
{
  return m_doc; 
}


void K3bMovixDvdJob::start()
{
  jobStarted();

  m_canceled = false;
  m_dvdJob->setWritingApp( writingApp() );

  if( m_movixDocPreparer->createMovixStructures() ) {
    m_dvdJob->start();
  }
  else {
    m_movixDocPreparer->removeMovixStructures();
    jobFinished(false);
  }
}


void K3bMovixDvdJob::cancel()
{
  m_canceled = true;
  m_dvdJob->cancel();
}


void K3bMovixDvdJob::slotDvdJobFinished( bool success )
{
  m_movixDocPreparer->removeMovixStructures();

  if( m_canceled )
    emit canceled();

  jobFinished( success );
}


QString K3bMovixDvdJob::jobDescription() const
{
  if( m_doc->isoOptions().volumeID().isEmpty() )
    return i18n("Writing eMovix DVD");
  else
    return i18n("Writing eMovix DVD (%1)").arg(m_doc->isoOptions().volumeID());
}


QString K3bMovixDvdJob::jobDetails() const
{
  return ( i18n("1 file (%1) and about 8 MB eMovix data", 
		"%n files (%1) and about 8 MB eMovix data", 
		m_doc->movixFileItems().count()).arg(KIO::convertSize(m_doc->size()))
	   + ( m_doc->copies() > 1 
	       ? i18n(" - %n copy", " - %n copies", m_doc->copies()) 
	       : QString::null ) );
}

#include "k3bmovixdvdjob.moc"
