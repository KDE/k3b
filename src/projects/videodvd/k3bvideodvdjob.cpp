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


#include "k3bvideodvdjob.h"
#include "k3bvideodvddoc.h"
#include "k3bvideodvdimager.h"

#include <k3bcore.h>
#include <k3bisoimager.h>
#include <k3bdataverifyingjob.h>
#include <k3bgrowisofswriter.h>
#include <k3bglobals.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kconfig.h>



K3bVideoDvdJob::K3bVideoDvdJob( K3bVideoDvdDoc* doc, K3bJobHandler* jh, QObject* parent )
  : K3bDvdJob( doc, jh, parent ),
    m_doc(doc)
{
}


K3bVideoDvdJob::~K3bVideoDvdJob()
{
}


void K3bVideoDvdJob::start()
{
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
    prepareIsoImager();
    
    if( prepareWriterJob() ) {
      if( waitForDvd() ) {
	emit burning(true);
	m_writerJob->start();
	m_isoImager->writeToFd( m_writerJob->fd() );
	m_isoImager->start();
      }
      else
	emit finished(false);
    }
    else
      emit finished(false);
  }
}


void K3bVideoDvdJob::prepareIsoImager()
{
  if( !m_isoImager ) {
    m_isoImager = new K3bVideoDvdImager( m_doc, this );
    connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), 
	     this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
    connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
    connect( m_isoImager, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }
}


QString K3bVideoDvdJob::jobDescription() const
{
  if( m_doc->onlyCreateImages() ) {
    return i18n("Creating VideoDvd Image File");
  }
  else {
    if( m_doc->isoOptions().volumeID().isEmpty() )
      return i18n("Writing VideoDVD");
    else
      return i18n("Writing VideoDVD (%1)").arg(m_doc->isoOptions().volumeID());
  }
}


QString K3bVideoDvdJob::jobDetails() const
{
  return ( i18n("Iso9660/Udf Filesystem (Size: %1)").arg(KIO::convertSize( doc()->size() ))
	   + ( m_doc->copies() > 1 
	       ? i18n(" - %n copy", " - %n copies", m_doc->copies()) 
	       : QString::null ) );
}

#include "k3bvideodvdjob.moc"
