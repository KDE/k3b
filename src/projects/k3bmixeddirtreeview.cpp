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


#include "k3bmixeddirtreeview.h"

#include "k3bmixeddoc.h"
#include <k3blistview.h>
#include <k3baudiodoc.h>
#include <k3bdataviewitem.h>

#include <qevent.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <kurldrag.h>
#include <klocale.h>


class K3bMixedDirTreeView::PrivateAudioRootViewItem : public K3bListViewItem
{
public:
  PrivateAudioRootViewItem( K3bMixedDoc* doc, QListView* parent, QListViewItem* after )
    : K3bListViewItem( parent, after ),
      m_doc(doc)
  {
    setPixmap( 0, SmallIcon("sound") );
  }

  QString text( int col ) const {
    if( col == 0 )
      return i18n("Audio Tracks") + QString(" (%1)").arg(m_doc->audioDoc()->numOfTracks());
    else
      return QString::null;
  }

  private:
    K3bMixedDoc* m_doc;
};


K3bMixedDirTreeView::K3bMixedDirTreeView( K3bView* view, K3bMixedDoc* doc, QWidget* parent, const char* )
  : K3bDataDirTreeView( view, doc->dataDoc(), parent ), m_doc(doc)
{
  m_audioRootItem = new PrivateAudioRootViewItem( doc, this, root() );

  connect( this, SIGNAL(selectionChanged(QListViewItem*)),
	   this, SLOT(slotSelectionChanged(QListViewItem*)) );
  connect( m_doc->audioDoc(), SIGNAL(changed()), this, SLOT(slotNewAudioTracks()) );
}


K3bMixedDirTreeView::~K3bMixedDirTreeView()
{
}


void K3bMixedDirTreeView::slotDropped( QDropEvent* e, QListViewItem* parent, QListViewItem* after )
{
  if( !e->isAccepted() )
    return;

  QListViewItem* droppedItem = itemAt(e->pos());
  if( droppedItem == m_audioRootItem ) {
    KURL::List urls;
    if( KURLDrag::decode( e, urls ) )
      m_doc->audioDoc()->addUrls( urls );
  }
  else
    K3bDataDirTreeView::slotDropped( e, parent, after );
}


void K3bMixedDirTreeView::slotSelectionChanged( QListViewItem* i )
{
  if( i == m_audioRootItem )
    emit audioTreeSelected();
  else
    emit dataTreeSelected();
}


void K3bMixedDirTreeView::slotNewAudioTracks()
{
  // update the tracknumber
  m_audioRootItem->repaint();
}

#include "k3bmixeddirtreeview.moc"
