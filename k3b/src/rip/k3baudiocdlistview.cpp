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

#include "k3baudiocdlistview.h"
#include "k3baudiocdview.h"

#include <klocale.h>

#include <qheader.h>
#include <qtooltip.h>


K3bAudioCdListView::K3bAudioCdListView( K3bAudioCdView* view, QWidget* parent, const char* name )
  : K3bListView( parent, name ),
    m_view(view)
{
  setFullWidth(true);
  setSorting(-1);
  setAllColumnsShowFocus( true );
  setSelectionModeExt( Extended );
  setDragEnabled( true );
  addColumn( "" );
  addColumn( "" );
  addColumn( i18n("Artist") );
  addColumn( i18n("Title") );
  addColumn( i18n("Length") );
  addColumn( i18n("Size") );

  setDoubleClickForEdit( true );

  header()->setClickEnabled(false);
  setColumnWidthMode( 0, QListView::Manual );
  setColumnWidth( 0, 20 );
  header()->setResizeEnabled( false,0 );

  setColumnAlignment( 4, Qt::AlignHCenter );

  QToolTip::add( viewport(), i18n("Check the tracks that should be ripped") );
}


K3bAudioCdListView::~K3bAudioCdListView()
{
}


QDragObject* K3bAudioCdListView::dragObject()
{
  return m_view->dragObject();
}


#include "k3baudiocdlistview.moc"

