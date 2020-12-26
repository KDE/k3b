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
    addAudioVideoTsDirs();

    setMultiSessionMode( NONE );

    setModified( false );

    return true;
  }
  else
    return false;
}

void K3b::VideoDvdDoc::clear()
{
    bool addTsDirs = false;
    if (m_videoTsDir) {
        // K3b::DataDoc::clear() needs the items to be removeable or clear loops forever
        // so change them to removeable if they exist, so they can be deleted and add them back again later
        m_videoTsDir->setRemoveable(true);
        m_audioTsDir->setRemoveable(true);
        addTsDirs = true;
    }
    K3b::DataDoc::clear();
    if (addTsDirs) {
        addAudioVideoTsDirs();
    }
}

void K3b::VideoDvdDoc::addAudioVideoTsDirs()
{
    m_videoTsDir = new K3b::DirItem( "VIDEO_TS" );
    m_videoTsDir->setRemoveable(false);
    m_videoTsDir->setRenameable(false);
    m_videoTsDir->setMoveable(false);
    m_videoTsDir->setHideable(false);
    root()->addDataItem( m_videoTsDir );

    m_audioTsDir = new K3b::DirItem( "AUDIO_TS" );
    m_audioTsDir->setRemoveable(false);
    m_audioTsDir->setRenameable(false);
    m_audioTsDir->setMoveable(false);
    m_audioTsDir->setHideable(false);
    root()->addDataItem( m_audioTsDir );
}


K3b::BurnJob* K3b::VideoDvdDoc::newBurnJob( K3b::JobHandler* hdl, QObject* parent )
{
  return new K3b::VideoDvdJob( this, hdl, parent );
}


K3b::Device::MediaTypes K3b::VideoDvdDoc::supportedMediaTypes() const
{
    return K3b::Device::MEDIA_WRITABLE_DVD | K3b::Device::MEDIA_WRITABLE_BD;
}

bool K3b::VideoDvdDoc::saveDocumentData(QDomElement*)
{
    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__;
    return true;
}

//#include "k3bdvddoc.moc"
