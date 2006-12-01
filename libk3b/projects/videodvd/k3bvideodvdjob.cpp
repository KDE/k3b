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


void K3bVideoDvdJob::prepareImager()
{
  setImager( new K3bVideoDvdImager( m_doc, this ) );
}


bool K3bVideoDvdJob::prepareWriterJob()
{
  K3bGrowisofsWriter* writer = new K3bGrowisofsWriter( m_doc->burner(), this, this );
  
  // these do only make sense with DVD-R(W)
  writer->setSimulate( m_doc->dummy() );
  writer->setBurnSpeed( m_doc->speed() );

  // DAO seems to be the better default for Video DVD... !?
  if( m_doc->writingMode() == K3b::DAO || m_doc->writingMode() == K3b::WRITING_MODE_AUTO )
    writer->setWritingMode( K3b::DAO );

  writer->setMultiSession( false );
  writer->setCloseDvd( true );

  if( m_doc->onTheFly() ) {
    writer->setImageToWrite( QString::null );  // read from stdin
    writer->setTrackSize( m_isoImager->size() );
  }
  else
    writer->setImageToWrite( m_doc->tempDir() );

  setWriterJob( writer );

  return true;
}


QString K3bVideoDvdJob::jobDescription() const
{
  if( m_doc->onlyCreateImages() ) {
    return i18n("Creating Video DVD Image File");
  }
  else {
    return i18n("Writing Video DVD")
      + ( m_doc->isoOptions().volumeID().isEmpty()
	  ? QString::null
	  : QString( " (%1)" ).arg(m_doc->isoOptions().volumeID()) );
  }
}


QString K3bVideoDvdJob::jobDetails() const
{
  return ( i18n("ISO9660/Udf Filesystem (Size: %1)").arg(KIO::convertSize( doc()->size() ))
	   + ( m_doc->copies() > 1 
	       ? i18n(" - %n copy", " - %n copies", m_doc->copies()) 
	       : QString::null ) );
}

#include "k3bvideodvdjob.moc"
