/*
 *
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
#include "k3bdiritem.h"
#include "k3bisooptions.h"

#include <KConfig>


K3b::VideoDvdDoc::VideoDvdDoc( QObject* parent )
  : K3b::DataDoc( parent )
{
}


K3b::VideoDvdDoc::~VideoDvdDoc()
{
}


bool K3b::VideoDvdDoc::newDocument()
{
  if( K3b::DataDoc::newDocument() ) {

    // K3b::DataDoc::newDocument already deleted m_videoTsDir (again: bad design!)
    m_videoTsDir = new K3b::DirItem( "VIDEO_TS" );
    m_videoTsDir->setRemoveable(false);
    m_videoTsDir->setRenameable(false);
    m_videoTsDir->setMoveable(false);
    m_videoTsDir->setHideable(false);
    root()->addDataItem( m_videoTsDir );

    K3b::DirItem* audioTsDir = new K3b::DirItem( "AUDIO_TS" );
    audioTsDir->setRemoveable(false);
    audioTsDir->setRenameable(false);
    audioTsDir->setMoveable(false);
    audioTsDir->setHideable(false);
    root()->addDataItem( audioTsDir );

    setMultiSessionMode( NONE );

    setModified( false );

    return true;
  }
  else
    return false;
}


K3b::BurnJob* K3b::VideoDvdDoc::newBurnJob( K3b::JobHandler* hdl, QObject* parent )
{
  return new K3b::VideoDvdJob( this, hdl, parent );
}


K3b::Device::MediaTypes K3b::VideoDvdDoc::supportedMediaTypes() const
{
    return K3b::Device::MEDIA_WRITABLE_DVD;
}

//#include "k3bdvddoc.moc"
