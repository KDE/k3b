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

#include "k3bmovixdvddoc.h"
#include "k3bmovixdvdjob.h"
#include "k3bmovixdvdview.h"
#include "k3bmovixdvdburndialog.h"

#include <kconfig.h>


K3bMovixDvdDoc::K3bMovixDvdDoc( QObject* parent )
  : K3bMovixDoc( parent )
{
}

K3bMovixDvdDoc::~K3bMovixDvdDoc()
{
}

K3bBurnJob* K3bMovixDvdDoc::newBurnJob( K3bJobHandler* hdl, QObject* parent )
{
  return new K3bMovixDvdJob( this, hdl, parent );
}


K3bView* K3bMovixDvdDoc::newView( QWidget* parent )
{
  return new K3bMovixDvdView( this, parent );
}


void K3bMovixDvdDoc::loadDefaultSettings( KConfig* c )
{
  K3bMovixDoc::loadDefaultSettings(c);

  // K3bDoc defaults to 1, but for DVDs we need default 0 :(
  setSpeed( c->readNumEntry( "writing_speed", 0 ) );
}

#include "k3bmovixdvddoc.moc"
