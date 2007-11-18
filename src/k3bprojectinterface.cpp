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
Q3CString K3bProjectInterface::newIfaceName()
{
  static int s_docIFNumber = 0;
  Q3CString name;
  name.setNum( s_docIFNumber++ );
  name.prepend("K3bProject-");
  return name;
}


K3bProjectInterface::K3bProjectInterface( K3bDoc* doc )
  : DCOPObject( name ? Q3CString(name) : newIfaceName() ),
    m_doc( doc )
{
}


K3bProjectInterface::~K3bProjectInterface()
{
}

void K3bProjectInterface::addUrls( const QStringList& urls )
{
  m_doc->addUrls( KUrl::List(urls) );
}

void K3bProjectInterface::addUrl( const QString& url )
{
  m_doc->addUrl( KUrl(url) );
}

void K3bProjectInterface::burn()
{
  // we want to return this method immediately
  QTimer::singleShot( 0, m_doc->view(), SLOT(slotBurn()) );
}


bool K3bProjectInterface::directBurn()
{
  if( m_doc->burner() ) {
    K3bJobProgressDialog* dlg = 0;
    if( m_doc->onlyCreateImages() )
      dlg = new K3bJobProgressDialog( m_doc->view() );
    else
      dlg = new K3bBurnProgressDialog( m_doc->view() );

    K3bJob* job = m_doc->newBurnJob( dlg );

    dlg->startJob( job );

    delete job;
    delete dlg;

    return true;
  }
  else
    return false;
}


void K3bProjectInterface::setBurnDevice( const QString& name )
{
  if( K3bDevice::Device* dev = k3bcore->deviceManager()->findDevice( name ) )
    m_doc->setBurner( dev );
}


int K3bProjectInterface::length() const
{
  return m_doc->length().lba();
}


KIO::filesize_t K3bProjectInterface::size() const
{
  return m_doc->size();
}


const QString& K3bProjectInterface::imagePath() const
{
  return m_doc->tempDir();
}


QString K3bProjectInterface::projectType() const
{
  switch( m_doc->type() ) {
  case K3bDoc::AUDIO:
    return "audiocd";
  case K3bDoc::DATA:
    return "data";
  case K3bDoc::MIXED:
    return "mixedcd";
  case K3bDoc::VCD:
    return "videocd";
  case K3bDoc::MOVIX:
    return "emovix";
  case K3bDoc::VIDEODVD:
    return "videodvd";
  default:
    return "unknown";
  }
}
