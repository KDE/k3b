/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvideodvdview.h"
#include "k3bvideodvddoc.h"
#include "k3bvideodvdburndialog.h"
#include <k3bfillstatusdisplay.h>
#include <k3bdatafileview.h>

#include <klocale.h>


K3bVideoDvdView::K3bVideoDvdView( K3bVideoDvdDoc* doc, QWidget *parent, const char *name )
  : K3bDvdView( doc, parent, name ),
    m_doc(doc)
{
}


K3bVideoDvdView::~K3bVideoDvdView()
{
}


K3bProjectBurnDialog* K3bVideoDvdView::newBurnDialog( QWidget* parent, const char* name )
{
  return new K3bVideoDvdBurnDialog( m_doc, parent, name, true );
}

//#include "k3bvideodvdview.moc"
