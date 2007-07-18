/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bvideodvddoc.h"
#include "k3bvideodvdjob.h"

#include <k3bdiritem.h>

#include <k3bisooptions.h>

#include <kconfig.h>


K3bVideoDvdDoc::K3bVideoDvdDoc( QObject* parent )
  : K3bDataDoc( parent )
{
}


K3bVideoDvdDoc::~K3bVideoDvdDoc()
{
}


bool K3bVideoDvdDoc::newDocument()
{
  if( K3bDataDoc::newDocument() ) {

    // K3bDataDoc::newDocument already deleted m_videoTsDir (again: bad design!)
    m_videoTsDir = new K3bDirItem( "VIDEO_TS", this, root() );
    m_videoTsDir->setRemoveable(false);
    m_videoTsDir->setRenameable(false);
    m_videoTsDir->setMoveable(false);
    m_videoTsDir->setHideable(false);

    K3bDirItem* audioTsDir = new K3bDirItem( "AUDIO_TS", this, root() );
    audioTsDir->setRemoveable(false);
    audioTsDir->setRenameable(false);
    audioTsDir->setMoveable(false);
    audioTsDir->setHideable(false);

    setMultiSessionMode( NONE );

    setModified( false );

    return true;
  }
  else
    return false;
}


K3bBurnJob* K3bVideoDvdDoc::newBurnJob( K3bJobHandler* hdl, QObject* parent )
{
  return new K3bVideoDvdJob( this, hdl, parent );
}


int K3bVideoDvdDoc::supportedMediaTypes() const
{
    return K3bDevice::MEDIA_WRITABLE_DVD;
}

//#include "k3bdvddoc.moc"
