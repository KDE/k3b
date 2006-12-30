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

#include "k3bmediacontentsview.h"

#include <k3bmediacache.h>
#include <k3bapplication.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>


class K3bMediaContentsView::Private
{
public:
  K3bMedium medium;
  int supportedMediumContent;
  int supportedMediumTypes;
  int supportedMediumStates;

  bool autoReload;
};


K3bMediaContentsView::K3bMediaContentsView( bool withHeader,
					    int mediumContent,
					    int mediumTypes,
					    int mediumState,
					    QWidget* parent,
					    const char* name )
  : K3bContentsView( withHeader, parent, name )
{
  d = new Private;
  d->supportedMediumContent = mediumContent;
  d->supportedMediumTypes = mediumTypes;
  d->supportedMediumStates = mediumState;
  d->autoReload = true;

  connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3bDevice::Device*)),
	   this, SLOT(slotMediumChanged(K3bDevice::Device*)) );
}


K3bMediaContentsView::~K3bMediaContentsView()
{
  delete d;
}


void K3bMediaContentsView::setAutoReload( bool b )
{
  d->autoReload = b;
}


int K3bMediaContentsView::supportedMediumContent() const
{
  return d->supportedMediumContent;
}


int K3bMediaContentsView::supportedMediumTypes() const
{
  return d->supportedMediumTypes;
}


int K3bMediaContentsView::supportedMediumStates() const
{
  return d->supportedMediumStates;
}


const K3bMedium& K3bMediaContentsView::medium() const
{
  return d->medium;
}


K3bDevice::Device* K3bMediaContentsView::device() const
{
  return medium().device();
}


void K3bMediaContentsView::setMedium( const K3bMedium& m )
{
  d->medium = m;
}


void K3bMediaContentsView::reload( K3bDevice::Device* dev )
{
  reload( k3bappcore->mediaCache()->medium( dev ) );
}


void K3bMediaContentsView::reload( const K3bMedium& m )
{
  setMedium( m );
  reload();
}


void K3bMediaContentsView::reload()
{
  enableInteraction( true );
  reloadMedium();
}


void K3bMediaContentsView::enableInteraction( bool enable )
{
  mainWidget()->setEnabled( enable );
}


void K3bMediaContentsView::slotMediumChanged( K3bDevice::Device* dev )
{
  kdDebug() << k_funcinfo << endl;
  if( !d->autoReload || !isVisible() )
    return;

  if( dev == device() ) {
    kdDebug() << k_funcinfo << " checking medium in " << dev->blockDeviceName() << endl;
    K3bMedium m = k3bappcore->mediaCache()->medium( dev );

    // no need to reload if the medium did not change (this is even
    // important since K3b blocks the devices in action and after
    // the release they are signalled as changed)
    if( m == medium() ) {
      kdDebug() << k_funcinfo << " medium did not change" << endl;
      enableInteraction( true );
    }
    else if( m.content() & supportedMediumContent() &&
	     m.diskInfo().mediaType() & supportedMediumTypes() &&
	     m.diskInfo().diskState() & supportedMediumStates() ) {
      kdDebug() << k_funcinfo << " new supported medium found" << endl;
      reload( m );
    }
    else {
      kdDebug() << k_funcinfo << " unsupported medium found" << endl;
      enableInteraction( false );
    }
  }
}

#include "k3bmediacontentsview.moc"
