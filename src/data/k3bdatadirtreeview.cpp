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
#include "k3bisovalidator.h"
#include "k3bdatapropertiesdialog.h"
#include "k3bdataviewitem.h"
#include "../k3b.h"
#include "../k3bview.h"

#include <qdragobject.h>
#include <qheader.h>

#include <klocale.h>
#include <kaction.h>
#include <kurldrag.h>
#include <klineeditdlg.h>
#include <kiconloader.h>

#include "../klistviewlineedit.h"
#include <kdebug.h>


K3bDataDirTreeView::K3bDataDirTreeView( K3bView* view, K3bDataDoc* doc, QWidget* parent )
  : KListView( parent ), m_view(view)
{
  m_fileView = 0;

  setAcceptDrops( true );
  setDropVisualizer( false );
  setDropHighlighter( true );
  setRootIsDecorated( false );
  setFullWidth();
  setDragEnabled( true );
  setItemsMovable( false );
  setAlternateBackground( QColor() );

  addColumn( i18n("Dir") );
  header()->hide();
	
  setItemsRenameable( true );

  m_editor = new KListViewLineEdit( this );
  m_editor->setValidator( new K3bIsoValidator( m_editor ) );

  m_doc = doc;	
  m_root = new K3bDataRootViewItem( doc, this );
  m_itemMap.insert( doc->root(), m_root );

  connect( this, SIGNAL(clicked(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
  connect( this, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
  connect( m_doc, SIGNAL(itemRemoved(K3bDataItem*)), this, SLOT(slotDataItemRemoved(K3bDataItem*)) );
  connect( m_doc, SIGNAL(newFileItems()), this, SLOT(updateContents()) );
  connect( m_editor, SIGNAL(done(QListViewItem*,int)), this, SLOT(doneEditing(QListViewItem*,int)) );
  connect( this, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(showPopupMenu(QListViewItem*, const QPoint&)) );
  connect( this, SIGNAL(dropped(QDropEvent*, QListViewItem*, QListViewItem*)),
	   this, SLOT(slotDropped(QDropEvent*, QListViewItem*, QListViewItem*)) );

  setupActions();
}


K3bDataDirTreeView::~K3bDataDirTreeView()
{
}


void K3bDataDirTreeView::rename( QListViewItem* item, int c )
{
  m_editor->load( item, c );
}


void K3bDataDirTreeView::slotExecuted( QListViewItem* item )
{
  if( item )
    emit dirSelected( ((K3bDataDirViewItem*)item)->dirItem() );
}


bool K3bDataDirTreeView::acceptDrag(QDropEvent* e) const{
  return ( e->source() == viewport() || QUriDrag::canDecode(e) || 
	   ( m_fileView && e->source() == m_fileView->viewport() ) );
}


void K3bDataDirTreeView::slotDropped( QDropEvent* e, QListViewItem*, QListViewItem* parentViewItem )
{
  if( !e->isAccepted() )
    return;

  // determine K3bDirItem to add the items to
  K3bDirItem* parent = 0;
  if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( parentViewItem ) ) {
    parent = dirViewItem->dirItem();
  }

  if( parent ) {

    // check if items have been moved
    if( m_fileView &&
	e->source() == m_fileView->viewport() ) {
      // move all selected items
      QPtrList<QListViewItem> selectedViewItems = m_fileView->selectedItems();
      QPtrList<K3bDataItem> selectedDataItems;
      QListIterator<QListViewItem> it( selectedViewItems );
      for( ; it.current(); ++it ) {
	K3bDataViewItem* dataViewItem = dynamic_cast<K3bDataViewItem*>( it.current() );
	if( dataViewItem )
	  selectedDataItems.append( dataViewItem->dataItem() );
	else
	  kdDebug() << "no dataviewitem" << endl;
      }
	
      m_doc->moveItems( selectedDataItems, parent );
    }
    else if( e->source() == viewport() ) {
      // move the selected dir
      if( K3bDataDirViewItem* dirItem = dynamic_cast<K3bDataDirViewItem*>( selectedItem() ) )
	m_doc->moveItem( dirItem->dirItem(), parent );
    }
    else {
      // seems that new items have been dropped
      KURL::List urls;
      if( KURLDrag::decode( e, urls ) )
	m_doc->slotAddUrlsToDir( urls, parent );
    }
  }
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


  // check the directory depth
  QListViewItemIterator it(this);
  while( it.current() != 0 ) {
    if( it.current() != m_root ) {
      K3bDirItem* dirItem = ((K3bDataDirViewItem*)it.current())->dirItem();
      it.current()->setPixmap( 0, dirItem->depth() > 7 ? SmallIcon( "folder_red" ) : SmallIcon( "folder" ) );
    }
    
    ++it;
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
    kdDebug() << "Tried to set unknown dirItem to current" << endl;
  }
}


void K3bDataDirTreeView::setupActions()
{
  m_actionCollection = new KActionCollection( this );

  m_actionProperties = new KAction( i18n("Properties..."), "misc", 0, this, SLOT(slotProperties()), 
				    actionCollection(), "properties" );
  m_actionNewDir = new KAction( i18n("New Directory..."), "folder_new", CTRL+Key_N, this, SLOT(slotNewDir()), 
				actionCollection(), "new_dir" );
  m_actionRemove = new KAction( i18n("Remove"), "editdelete", Key_Delete, this, SLOT(slotRemoveItem()), 
				actionCollection(), "remove" );
  m_actionRename = new KAction( i18n("Rename"), "edit", CTRL+Key_R, this, SLOT(slotRenameItem()), 
				actionCollection(), "rename" );

  m_popupMenu = new KActionMenu( m_actionCollection, "contextMenu" );
  m_popupMenu->insert( m_actionRename );
  m_popupMenu->insert( m_actionRemove );
  m_popupMenu->insert( m_actionNewDir );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( m_actionProperties );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( k3bMain()->actionCollection()->action("file_burn") );
}


void K3bDataDirTreeView::showPopupMenu( QListViewItem* item, const QPoint& point )
{
  if( item ) {
    m_actionRemove->setEnabled( true );
    m_actionRename->setEnabled( true );
    m_actionProperties->setEnabled( true );
  }
  else {
    m_actionRemove->setEnabled( false );
    m_actionRename->setEnabled( false );
    m_actionProperties->setEnabled( false );
  }

  m_popupMenu->popup( point );
}


void K3bDataDirTreeView::slotNewDir()
{
  if( K3bDataDirViewItem* vI = dynamic_cast<K3bDataDirViewItem*>(currentItem()) ) {
    K3bDirItem* parent = vI->dirItem();
    
    QString name;
    bool ok;
    
    name = KLineEditDlg::getText( i18n("Please insert the name for the new directory"),
				  "New directory", &ok, this );
    
    while( ok && K3bDataDoc::nameAlreadyInDir( name, parent ) ) {
      name = KLineEditDlg::getText( i18n("A file with that name already exists. ")
				    + i18n("Please insert the name for the new directory"),
				    "New directory", &ok, this );
    }
    
    if( !ok )
      return;
    
    
    m_doc->addEmptyDir( name, parent );
  }
}


void K3bDataDirTreeView::slotRenameItem()
{
  rename( currentItem(), 0 );
}


void K3bDataDirTreeView::slotRemoveItem()
{
  if( currentItem() ) {
    if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( currentItem() ) )
      m_doc->removeItem( dirViewItem->dirItem() );
  }
}


void K3bDataDirTreeView::slotProperties()
{
  K3bDataViewItem* viewItem = dynamic_cast<K3bDataViewItem*>( currentItem() );
  if(  viewItem && currentItem() != root() ) {
    K3bDataPropertiesDialog d( viewItem->dataItem(), this );
    if( d.exec() ) {
      repaint();
      if( m_fileView )
	m_fileView->repaint();
    }
  }
  else
    m_view->burnDialog( false );
}

#include "k3bdatadirtreeview.moc"
