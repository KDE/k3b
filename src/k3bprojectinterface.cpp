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


#include "k3bprojectinterface.h"
#include <k3bdoc.h>
#include <k3bview.h>
#include <k3bmsf.h>

#include <qtimer.h>


//static
QCString K3bProjectInterface::newIfaceName()
{
  static int s_docIFNumber = 0;
  QCString name;
  name.setNum( s_docIFNumber++ ); 
  name.prepend("K3bProject-");
  return name;
}


K3bProjectInterface::K3bProjectInterface( K3bDoc* doc, const char* name )
  : DCOPObject( name ? QCString(name) : newIfaceName() ),
    m_doc( doc )
{
}


K3bProjectInterface::~K3bProjectInterface()
{
}

void K3bProjectInterface::addUrls( const KURL::List& urls )
{
  m_doc->addUrls( urls );
}

void K3bProjectInterface::addUrl( const KURL& url )
{
  m_doc->addUrl( url );
}

void K3bProjectInterface::burn()
{
  // we want to return this method immediately
  QTimer::singleShot( 0, m_doc->view(), SLOT(slotBurn()) );
}


int K3bProjectInterface::length()
{
  return m_doc->length().lba();
}


KIO::filesize_t K3bProjectInterface::size()
{
  return m_doc->size();
}


QString K3bProjectInterface::projectType() const
{
  switch( m_doc->type() ) {
  case K3bDoc::AUDIO:
    return "audiocd";
  case K3bDoc::DATA:
    return "datacd";
  case K3bDoc::MIXED:
    return "mixedcd";
  case K3bDoc::VCD:
    return "videocd";
  case K3bDoc::MOVIX:
    return "emovixcd";
  case K3bDoc::MOVIX_DVD:
    return "emovixdvd";
  case K3bDoc::DVD:
    return "datadvd";
  case K3bDoc::VIDEODVD:
    return "videodvd";
  default:
    return "unknown";
  }
}
