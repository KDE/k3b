/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudioconverterview.h"

#include <kurldrag.h>
#include <klocale.h>


K3bAudioConverterView::K3bAudioConverterView( QWidget* parent, const char* name )
  : K3bListView( parent, name )
{
  setAcceptDrops(true);
  addColumn( i18n("Filename") );
  addColumn( i18n("Filetype") );
  addColumn( i18n("Length") );
  setColumnAlignment( 1, Qt::AlignHCenter );
}


K3bAudioConverterView::~K3bAudioConverterView()
{
}


bool K3bAudioConverterView::acceptDrag( QDropEvent* event ) const
{
  bool b = KURLDrag::canDecode( event ) ;
  event->accept( b );
  return b;
}


#include "k3baudioconverterview.moc"
