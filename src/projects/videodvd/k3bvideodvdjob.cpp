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

#include <k3bcore.h>
#include <k3bisoimager.h>
#include <k3bdataverifyingjob.h>
#include <k3bgrowisofswriter.h>
#include <k3bemptydiscwaiter.h>
#include <k3bglobals.h>
#include <k3bemptydiscwaiter.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kconfig.h>



K3bVideoDvdJob::K3bVideoDvdJob( K3bVideoDvdDoc* doc, QObject* parent )
  : K3bDvdJob( doc, parent ),
    m_doc(doc)
{
}


K3bVideoDvdJob::~K3bVideoDvdJob()
{
}


void K3bVideoDvdJob::start()
{
  setVideoDvd( true );

  // TODO: configure the isoimager properly

  K3bDvdJob::start();
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
  return i18n("Iso9660/Udf Filesystem (Size: %1)").arg(KIO::convertSize( doc()->size() ));
}

#include "k3bvideodvdjob.moc"
