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


#include "k3bvideodvddoc.h"
#include "k3bvideodvdjob.h"
//#include "k3bvideodvdview.h"
#include "k3bvideodvdburndialog.h"

#include <k3bdiritem.h>

#include <k3bisooptions.h>

#include <kconfig.h>


K3bVideoDvdDoc::K3bVideoDvdDoc( QObject* parent )
  : K3bDvdDoc( parent )
{
}

K3bVideoDvdDoc::~K3bVideoDvdDoc()
{
}


bool K3bVideoDvdDoc::newDocument()
{
  if( K3bDataDoc::newDocument() ) {
    K3bDirItem* videoTsDir = new K3bDirItem( "VIDEO_TS", this, root() );
    videoTsDir->setRemoveable(false);
    videoTsDir->setRenameable(false);
    videoTsDir->setMoveable(false);
    videoTsDir->setHideable(false);

    K3bDirItem* audioTsDir = new K3bDirItem( "AUDIO_TS", this, root() );
    audioTsDir->setRemoveable(false);
    audioTsDir->setRenameable(false);
    audioTsDir->setMoveable(false);
    audioTsDir->setHideable(false);

    return true;
  }
  else
    return false;
}


K3bBurnJob* K3bVideoDvdDoc::newBurnJob()
{
  return new K3bVideoDvdJob( this );
}


// K3bView* K3bVideoDvdDoc::newView( QWidget* parent )
// {
//   return new K3bVideoDvdView( this, parent );
// }


void K3bVideoDvdDoc::loadDefaultSettings( KConfig* c )
{
  K3bDvdDoc::loadDefaultSettings(c);
}


K3bProjectBurnDialog* K3bVideoDvdDoc::newBurnDialog( QWidget* parent, const char* name )
{
  return new K3bVideoDvdBurnDialog( this, parent, name, true );
}

//#include "k3bdvddoc.moc"
