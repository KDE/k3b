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
#include "k3bdoc.h"



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
  m_doc->slotBurn();
}
