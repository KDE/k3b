/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdvdview.h"
#include "k3bdvddoc.h"
#include "k3bdvdburndialog.h"
#include <k3bfillstatusdisplay.h>


K3bDvdView::K3bDvdView( K3bDvdDoc* doc, QWidget *parent, const char *name )
  : K3bDataView( doc, parent, name )
{
  m_doc = doc;

  fillStatusDisplay()->showDvdSizes(true);
}


K3bDvdView::~K3bDvdView()
{
}


void K3bDvdView::burnDialog( bool withWriting )
{
  K3bDvdBurnDialog d( m_doc, this, "databurndialog", true );
  d.exec( withWriting );
}

#include "k3bdvdview.moc"
