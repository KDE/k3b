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


#include "kdndfileview.h"

#include <kurldrag.h>
#include <kglobalsettings.h>
#include <kapplication.h>

#include <qevent.h>


KDndFileDetailView::KDndFileDetailView( QWidget* parent, const char* name )
  : KFileDetailView( parent, name ),
    m_notEnterDir(false)
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

  // this at least works for the shift key
  // But better use it since otherwise none of the keys
  // works if we do not have the focus :(
  uint kb = KApplication::keyboardModifiers();
  bool notEnterDir = ( (kb & Qt::Key_Shift) || 
		       (kb & Qt::Key_Control) );

  m_notEnterDir = notEnterDir || m_notEnterDir;

  if ( KGlobalSettings::singleClick() ) {
    const KFileItem *fi = ( (KFileListViewItem*)item )->fileInfo();
    if ( fi && ( (fi->isDir() && !m_notEnterDir) || 
		 (!fi->isDir() && !onlyDoubleClickSelectsFiles()) ) ) {
      sig->activate( fi );
    }
  }
}


void KDndFileDetailView::keyPressEvent( QKeyEvent* e )
{
  m_notEnterDir = e->key() == Key_Shift || e->key() == Key_Control;
  KFileDetailView::keyPressEvent(e);
}


void KDndFileDetailView::keyReleaseEvent( QKeyEvent* e )
{
  m_notEnterDir = false;
  KFileDetailView::keyReleaseEvent(e);
}


QDragObject* KDndFileDetailView::dragObject()
{
  const KFileItemList* list = KFileView::selectedItems();
  if( list->isEmpty() )
    return 0;

  QListIterator<KFileItem> it(*list);
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

  // this at least works for the shift key
  // But better use it since otherwise none of the keys
  // works if we do not have the focus :(
  uint kb = KApplication::keyboardModifiers();
  bool notEnterDir = ( (kb & Qt::Key_Shift) || 
		       (kb & Qt::Key_Control) );

  m_notEnterDir = notEnterDir || m_notEnterDir;

  if ( KGlobalSettings::singleClick() ) {
    const KFileItem *fi = ( (KFileIconViewItem*)item )->fileInfo();
    if ( fi && ( (fi->isDir() && !m_notEnterDir) || 
		 (!fi->isDir() && !onlyDoubleClickSelectsFiles()) ) ) {
      sig->activate( fi );
    }
  }
}


void KDndFileIconView::keyPressEvent( QKeyEvent* e )
{
  m_notEnterDir = e->key() == Key_Shift || e->key() == Key_Control;
  KFileIconView::keyPressEvent(e);
}


void KDndFileIconView::keyReleaseEvent( QKeyEvent* e )
{
  m_notEnterDir = false;
  KFileIconView::keyReleaseEvent(e);
}


QDragObject* KDndFileIconView::dragObject()
{
  const KFileItemList* list = KFileView::selectedItems();
  if( list->isEmpty() )
    return 0;

  QListIterator<KFileItem> it(*list);
  KURL::List urls;
	
  for( ; it.current(); ++it )
    urls.append( it.current()->url() );

  return KURLDrag::newDrag( urls, viewport() );
}


#include "kdndfileview.moc"
