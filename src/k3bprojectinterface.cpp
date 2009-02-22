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


#include "k3bprojectinterface.h"
#include "k3bburnprogressdialog.h"
#include <k3bdoc.h>
#include <k3bview.h>
#include <k3bmsf.h>
#include <k3bcore.h>
#include <k3bdevicemanager.h>
#include <k3bjob.h>

#include <qtimer.h>
//Added by qt3to4:
#include <Q3CString>


//static
Q3CString K3b::ProjectInterface::newIfaceName()
{
  static int s_docIFNumber = 0;
  Q3CString name;
  name.setNum( s_docIFNumber++ );
  name.prepend("K3b::Project-");
  return name;
}


K3b::ProjectInterface::ProjectInterface( K3b::Doc* doc )
  : DCOPObject( name ? Q3CString(name) : newIfaceName() ),
    m_doc( doc )
{
}


K3b::ProjectInterface::~ProjectInterface()
{
}

void K3b::ProjectInterface::addUrls( const QStringList& urls )
{
  m_doc->addUrls( KUrl::List(urls) );
}

void K3b::ProjectInterface::addUrl( const QString& url )
{
  m_doc->addUrl( KUrl(url) );
}

void K3b::ProjectInterface::burn()
{
  // we want to return this method immediately
  QTimer::singleShot( 0, m_doc->view(), SLOT(slotBurn()) );
}


bool K3b::ProjectInterface::directBurn()
{
  if( m_doc->burner() ) {
    K3b::JobProgressDialog* dlg = 0;
    if( m_doc->onlyCreateImages() )
      dlg = new K3b::JobProgressDialog( m_doc->view() );
    else
      dlg = new K3b::BurnProgressDialog( m_doc->view() );

    K3b::Job* job = m_doc->newBurnJob( dlg );

    dlg->startJob( job );

    delete job;
    delete dlg;

    return true;
  }
  else
    return false;
}


void K3b::ProjectInterface::setBurnDevice( const QString& name )
{
  if( K3b::Device::Device* dev = k3bcore->deviceManager()->findDevice( name ) )
    m_doc->setBurner( dev );
}


int K3b::ProjectInterface::length() const
{
  return m_doc->length().lba();
}


KIO::filesize_t K3b::ProjectInterface::size() const
{
  return m_doc->size();
}


const QString& K3b::ProjectInterface::imagePath() const
{
  return m_doc->tempDir();
}


QString K3b::ProjectInterface::projectType() const
{
  switch( m_doc->type() ) {
  case K3b::Doc::AUDIO:
    return "audiocd";
  case K3b::Doc::DATA:
    return "data";
  case K3b::Doc::MIXED:
    return "mixedcd";
  case K3b::Doc::VCD:
    return "videocd";
  case K3b::Doc::MOVIX:
    return "emovix";
  case K3b::Doc::VIDEODVD:
    return "videodvd";
  default:
    return "unknown";
  }
}
