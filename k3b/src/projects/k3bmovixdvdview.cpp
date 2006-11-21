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

#include "k3bmovixdvdview.h"
#include "k3bmovixdvddoc.h"
#include "k3bmovixdvdburndialog.h"
#include <k3bmovixlistview.h>
#include <k3bfillstatusdisplay.h>

#include <klocale.h>


K3bMovixDvdView::K3bMovixDvdView( K3bMovixDvdDoc* doc, QWidget *parent, const char *name )
  : K3bMovixView( doc, parent, name )
{
  m_doc = doc;

  fillStatusDisplay()->showDvdSizes(true);

  m_listView->setNoItemText( i18n("Use drag'n'drop to add files to the project.") +"\n"
			     + i18n("To remove or rename files use the context menu.") + "\n"
			     + i18n("After that press the burn button to write the DVD.") );
}


K3bMovixDvdView::~K3bMovixDvdView()
{
}


K3bProjectBurnDialog* K3bMovixDvdView::newBurnDialog( QWidget* parent, const char* name )
{
  return new K3bMovixDvdBurnDialog( m_doc, parent, name, true );
}

#include "k3bmovixdvdview.moc"
