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


#include "kdndfileview.h"

#include <kurldrag.h>
#include <kglobalsettings.h>
#include <kapplication.h>

#define XK_MISCELLANY
#include <X11/keysymdef.h>
#undef XK_MISCELLANY


KDndFileDetailView::KDndFileDetailView( QWidget* parent, const char* name )
  : KFileDetailView( parent, name )
{
  setDragEnabled( true );

  // HACK to not enter dirs when shift or control is pressed
  disconnect( this, SIGNAL(clicked(QListViewItem *, const QPoint&, int)), this, 0 );
  connect( this, SIGNAL(clicked(QListViewItem *, const QPoint&, int)),
	   SLOT(slotSelected(QListViewItem*)) );
}


KDndFileDetailView::~KDndFileDetailView()
{
}


void KDndFileDetailView::slotSelected( QListViewItem* item )
{
  if( !item )
    return;

  uint kb = KApplication::keyboardModifiers();
  bool notEnterDir = ( kb & XK_Control_L ||
		       kb & XK_Control_R ||
		       kb & XK_Alt_L ||
		       kb & XK_Alt_R );

  if ( KGlobalSettings::singleClick() ) {
    const KFileItem *fi = ( (KFileListViewItem*)item )->fileInfo();
    if ( fi && ( (fi->isDir() && !notEnterDir) || 
		 (!fi->isDir() && !onlyDoubleClickSelectsFiles()) ) ) {
      sig->activate( fi );
    }
  }
}


QDragObject* KDndFileDetailView::dragObject()
{
  const KFileItemList* list = KFileView::selectedItems();
  if( list->isEmpty() )
    return 0;

  QPtrListIterator<KFileItem> it(*list);
  KURL::List urls;
	
  for( ; it.current(); ++it )
    urls.append( it.current()->url() );

  return KURLDrag::newDrag( urls, viewport() );
}






KDndFileIconView::KDndFileIconView( QWidget* parent, const char* name )
  : KFileIconView( parent, name )
{
  // HACK to not enter dirs when shift or control is pressed
  disconnect( this, SIGNAL(clicked(QIconViewItem*, const QPoint&)), this, 0 );
  connect( this, SIGNAL(clicked(QIconViewItem*, const QPoint&)),
	   SLOT(slotSelected(QIconViewItem*)) );
}


KDndFileIconView::~KDndFileIconView()
{
}


void KDndFileIconView::slotSelected( QIconViewItem* item )
{
  if( !item )
    return;

  uint kb = KApplication::keyboardModifiers();
  bool notEnterDir = ( kb & XK_Control_L ||
		       kb & XK_Control_R ||
		       kb & XK_Alt_L ||
		       kb & XK_Alt_R );

  if ( KGlobalSettings::singleClick() ) {
    const KFileItem *fi = ( (KFileIconViewItem*)item )->fileInfo();
    if ( fi && ( (fi->isDir() && !notEnterDir) || 
		 (!fi->isDir() && !onlyDoubleClickSelectsFiles()) ) ) {
      sig->activate( fi );
    }
  }
}


QDragObject* KDndFileIconView::dragObject()
{
  const KFileItemList* list = KFileView::selectedItems();
  if( list->isEmpty() )
    return 0;

  QPtrListIterator<KFileItem> it(*list);
  KURL::List urls;
	
  for( ; it.current(); ++it )
    urls.append( it.current()->url() );

  return KURLDrag::newDrag( urls, viewport() );
}


#include "kdndfileview.moc"
