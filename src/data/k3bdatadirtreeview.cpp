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

#include <qdragobject.h>
#include <qheader.h>


K3bDataDirTreeView::K3bDataDirTreeView( K3bDataDoc* doc, QWidget* parent )
  : KListView( parent )
{
  setAcceptDrops( true );
  setDropVisualizer( false );
  setDropHighlighter( true );
  setRootIsDecorated( true );
  setAlternateBackground( QColor() );  // disable alternate bg

  addColumn( "Dir" );
  header()->hide();
	
  setItemsRenameable( true );

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
  return ( e->source() == viewport() || QTextDrag::canDecode(e) );
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
	  if( !m_itemMap.contains(dirItem) )
	    m_itemMap.insert( dirItem, new K3bDataDirViewItem( dirItem, m_itemMap[dirItem->parent()] ) );
	}

      item = item->nextSibling();
    }
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
