/***************************************************************************
                          k3bdatadirview.cpp  -  description
                             -------------------
    begin                : Sat Oct 20 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bdatadirtreeview.h"
#include "k3bdataview.h"
#include "k3bdatadoc.h"
#include "k3bdataitem.h"
#include "k3bdiritem.h"

#include "../kdelibs_patched/kcharvalidator.h"

#include <qdragobject.h>
#include <qheader.h>


K3bDataDirTreeView::K3bDataDirTreeView( K3bDataView* view, K3bDataDoc* doc, QWidget* parent )
  : KListView( parent )
{
  m_view = view;

  setAcceptDrops( true );
  setDropVisualizer( false );
  setDropHighlighter( true );
  setRootIsDecorated( true );
  setAlternateBackground( QColor() );  // disable alternate bg
  setFullWidth();
  setDragEnabled( true );
  setItemsMovable( false );

  addColumn( "Dir" );
  header()->hide();
	
  setItemsRenameable( true );

  setValidator( new KCharValidator( this, "isoValidator", "\\/;:*$", KCharValidator::InvalidChars ) );

  m_doc = doc;	
  m_root = new K3bDataRootViewItem( doc, this );
  m_itemMap.insert( doc->root(), m_root );

  connect( this, SIGNAL(clicked(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
  connect( this, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
  connect( m_doc, SIGNAL(itemRemoved(K3bDataItem*)), this, SLOT(slotDataItemRemoved(K3bDataItem*)) );
}


void K3bDataDirTreeView::slotExecuted( QListViewItem* item )
{
  if( item )
    emit dirSelected( ((K3bDataDirViewItem*)item)->dirItem() );
}


bool K3bDataDirTreeView::acceptDrag(QDropEvent* e) const{
  return ( e->source() == viewport() || QTextDrag::canDecode(e) || m_view->acceptDrag(e) );
}


void K3bDataDirTreeView::updateContents()
{
  // check for removed items
  K3bDataItem* item;

  // this is very ugly but I don't know a better way so far...
//   K3bDataDirViewItem* viewItem = (K3bDataDirViewItem*)firstChild();
//   while( viewItem != 0 )
//     {
//       item = m_root->dirItem()->nextSibling();
//       while( item != 0 )
// 	{
// 	  if( item == viewItem->dirItem() )
// 	    break;

// 	  item = item->nextSibling();
// 	}

//       if( item == 0 )
// 	{
// 	  K3bDataDirViewItem* viewItem2 = viewItem;
// 	  viewItem = (K3bDataDirViewItem*)viewItem->nextSibling();
// 	  m_itemMap.remove( viewItem->dirItem() );
// 	  delete viewItem2;
// 	}
//       else
// 	viewItem = (K3bDataDirViewItem*)viewItem->nextSibling();
//     }



  // check for new items
  item = m_root->dirItem()->nextSibling();
  while( item != 0 )
    {
      // check if we have an entry and if not, create one
      // we can assume that a listViewItem for the parent exists
      // since we go top to bottom
      if( K3bDirItem* dirItem = dynamic_cast<K3bDirItem*>( item ) ) 
	{
	  if( !m_itemMap.contains(dirItem) ) {
	    K3bDataDirViewItem* parentViewItem = m_itemMap[dirItem->parent()];
	    m_itemMap.insert( dirItem, new K3bDataDirViewItem( dirItem, parentViewItem ) );
	  }
	  else {
	    // check if parent still correct (to get moved items)
	    K3bDataDirViewItem* dirViewItem = m_itemMap[dirItem];
	    K3bDataDirViewItem* parentViewItem = (K3bDataDirViewItem*)dirViewItem->parent();
	    if( dirItem->parent() != parentViewItem->dirItem() ) {
	      // get the new parent view item
	      K3bDataDirViewItem* newParentViewItem = m_itemMap[dirItem->parent()];
	      // reparent it
	      parentViewItem->takeItem( dirViewItem );
	      newParentViewItem->insertItem( dirViewItem );
	    }
	  }
	}

      item = item->nextSibling();
    }

  // always show the first level
  m_root->setOpen( true );
}


void K3bDataDirTreeView::slotDataItemRemoved( K3bDataItem* item )
{
  if( K3bDirItem* dirItem = dynamic_cast<K3bDirItem*>( item ) )
    if( m_itemMap.contains( dirItem ) )
      {
	K3bDataDirViewItem* viewItem = m_itemMap[dirItem];
	m_itemMap.remove( dirItem );
	delete viewItem;
      }
}


void K3bDataDirTreeView::setCurrentDir( K3bDirItem* dirItem )
{
  if( m_itemMap.contains( dirItem ) ) {
    setCurrentItem( m_itemMap[dirItem] );
    m_itemMap[dirItem]->setOpen(true);
    if( m_itemMap[dirItem] != root() )
      m_itemMap[dirItem]->parent()->setOpen(true);
  }
  else {
    qDebug("Tried to set unknown dirItem to current");
  }
}


#include "k3bdatadirtreeview.moc"
