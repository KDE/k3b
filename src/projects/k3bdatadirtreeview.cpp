/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdatadirtreeview.h"
#include "k3bdatafileview.h"
#include "k3bdataview.h"
#include "k3bdatadoc.h"
#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bdatapropertiesdialog.h"
#include "k3bdataviewitem.h"
#include "k3bdataurladdingdialog.h"
#include <k3bview.h>
#include <k3bvalidators.h>

#include <qdragobject.h>
#include <qheader.h>
#include <qtimer.h>

#include <klocale.h>
#include <kaction.h>
#include <kurldrag.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <kshortcut.h>

#include <kdebug.h>


class K3bDataDirTreeView::Private
{
public:
  Private()
    : animatedDirItem(0),
      dropDirItem(0) {
  }

  K3bDataDirViewItem* animatedDirItem;
  K3bDataDirViewItem* dropDirItem;
  int animationCounter;
  QPixmap beforeAniPixmap;

  // used for the urladdingdialog hack
  KURL::List addUrls;
  K3bDirItem* addParentDir;

  QString lastUpdateVolumeId;

  QValidator* iso9660Validator;
  QValidator* asciiValidator;
};


K3bDataDirTreeView::K3bDataDirTreeView( K3bView* view, K3bDataDoc* doc, QWidget* parent )
  : K3bListView( parent ), m_view(view)
{
  d = new Private();

  m_fileView = 0;

  setAcceptDrops( true );
  setDropVisualizer( false );
  setDropHighlighter( true );
  setRootIsDecorated( false );
  setFullWidth( true );
  setDragEnabled( true );
  setItemsMovable( false );
  setAlternateBackground( QColor() );
  //  setSorting(-1);

  addColumn( i18n("Directory") );
  header()->hide();

  m_doc = doc;

  m_root = new K3bDataRootViewItem( doc, this );
  m_itemMap.insert( doc->root(), m_root );

  connect( m_doc, SIGNAL(changed()), this, SLOT(slotDocChanged()) );
  connect( this, SIGNAL(clicked(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
  connect( this, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
  connect( m_doc, SIGNAL(itemRemoved(K3bDataItem*)), this, SLOT(slotDataItemRemoved(K3bDataItem*)) );
  connect( m_doc, SIGNAL(itemAdded(K3bDataItem*)), this, SLOT(slotItemAdded(K3bDataItem*)) );
  connect( this, SIGNAL(contextMenu(KListView*,QListViewItem*, const QPoint&)),
	   this, SLOT(showPopupMenu(KListView*,QListViewItem*, const QPoint&)) );
  connect( this, SIGNAL(dropped(QDropEvent*, QListViewItem*, QListViewItem*)),
	   this, SLOT(slotDropped(QDropEvent*, QListViewItem*, QListViewItem*)) );

  setupActions();
}


K3bDataDirTreeView::~K3bDataDirTreeView()
{
  delete d;
}


void K3bDataDirTreeView::slotExecuted( QListViewItem* item )
{
  if( K3bDataDirViewItem* viewItem = dynamic_cast<K3bDataDirViewItem*>(item) )
    emit dirSelected( viewItem->dirItem() );
}


bool K3bDataDirTreeView::acceptDrag(QDropEvent* e) const{
  return ( e->source() == viewport() || KURLDrag::canDecode(e) ||
	   ( m_fileView && e->source() == m_fileView->viewport() ) );
}


void K3bDataDirTreeView::contentsDragMoveEvent( QDragMoveEvent* e )
{
  K3bListView::contentsDragMoveEvent( e );

  // highlight the folder the items would be added to
  if( d->dropDirItem )
    d->dropDirItem->highlightIcon( false );

  d->dropDirItem = dynamic_cast<K3bDataDirViewItem*>( itemAt(contentsToViewport(e->pos())) );
  if( !d->dropDirItem )
    d->dropDirItem = m_root;

  d->dropDirItem->highlightIcon( true );
}


void K3bDataDirTreeView::contentsDragLeaveEvent( QDragLeaveEvent* e )
{
  K3bListView::contentsDragLeaveEvent( e );

  // remove any highlighting
  if( d->dropDirItem ) {
    d->dropDirItem->highlightIcon( false );
    d->dropDirItem = 0;
  }
}


void K3bDataDirTreeView::slotDropped( QDropEvent* e, QListViewItem*, QListViewItem* )
{
  // remove any highlighting
  if( d->dropDirItem ) {
    d->dropDirItem->highlightIcon( false );
    d->dropDirItem = 0;
  }

  if( !e->isAccepted() )
    return;

  // determine K3bDirItem to add the items to
  if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( itemAt(contentsToViewport(e->pos())) ) ) {
    d->addParentDir = dirViewItem->dirItem();
  }
  else {
    d->addParentDir = m_doc->root();
  }

  if( d->addParentDir ) {

    //    startDropAnimation( parent );

    // check if items have been moved
    if( m_fileView &&
	e->source() == m_fileView->viewport() ) {
      // move all selected items
      QPtrList<QListViewItem> selectedViewItems = m_fileView->selectedItems();
      QValueList<K3bDataItem*> selectedDataItems;
      QPtrListIterator<QListViewItem> it( selectedViewItems );
      for( ; it.current(); ++it ) {
	K3bDataViewItem* dataViewItem = dynamic_cast<K3bDataViewItem*>( it.current() );
	if( dataViewItem )
	  selectedDataItems.append( dataViewItem->dataItem() );
	else
	  kdDebug() << "no dataviewitem" << endl;
      }

      K3bDataUrlAddingDialog::copyMoveItems( selectedDataItems, d->addParentDir, this, e->action() == QDropEvent::Copy );
    }
    else if( e->source() == viewport() ) {
      // move the selected dir
      if( K3bDataDirViewItem* dirItem = dynamic_cast<K3bDataDirViewItem*>( selectedItem() ) ) {
	QValueList<K3bDataItem*> selectedDataItems;
	selectedDataItems.append( dirItem->dirItem() );
	K3bDataUrlAddingDialog::copyMoveItems( selectedDataItems, d->addParentDir, this, e->action() == QDropEvent::Copy );
      }
    }
    else {
      // seems that new items have been dropped
      d->addUrls.clear();
      if( KURLDrag::decode( e, d->addUrls ) ) {
	//
	// This is a small (not to ugly) hack to circumvent problems with the
	// event queues: the url adding dialog will be non-modal regardless of
	// the settings in case we open it directly.
	//
	QTimer::singleShot( 0, this, SLOT(slotAddUrls()) );
      }
    }
  }

  // now grab that focus
  setFocus();
}


void K3bDataDirTreeView::slotAddUrls()
{
  K3bDataUrlAddingDialog::addUrls( d->addUrls, d->addParentDir, this );
}


void K3bDataDirTreeView::slotItemAdded( K3bDataItem* item )
{
  if( item->isDir() ) {
    //
    // We assume that we do not already have an item for the dir since the itemAdded signal
    // should only be emitted once for every item
    //
    K3bDirItem* dirItem = static_cast<K3bDirItem*>( item );
    K3bDataDirViewItem* parentViewItem = m_itemMap[dirItem->parent()];
    K3bDataDirViewItem* newDirItem = new K3bDataDirViewItem( dirItem, parentViewItem );
    m_itemMap.insert( dirItem, newDirItem );
  }
}


void K3bDataDirTreeView::slotDataItemRemoved( K3bDataItem* item )
{
  if( item->isDir() ) {
    K3bDirItem* dirItem = static_cast<K3bDirItem*>( item );
    QMapIterator<K3bDirItem*, K3bDataDirViewItem*> it = m_itemMap.find( dirItem );
    if( it != m_itemMap.end() ) {
      K3bDataDirViewItem* viewItem = it.data();
      m_itemMap.remove( it );

      // we don't get removedInfo for the child items
      // so we need to remove them here
      QPtrListIterator<K3bDataItem> it( dirItem->children() );
      for( ; it.current(); ++it ) {
	if( it.current()->isDir() )
	  slotDataItemRemoved( it.current() );
      }

      delete viewItem;
    }
  }
}


void K3bDataDirTreeView::setCurrentDir( K3bDirItem* dirItem )
{
  QMapIterator<K3bDirItem*, K3bDataDirViewItem*> it = m_itemMap.find( dirItem );
  if( it != m_itemMap.end() ) {
    setCurrentItem( it.data() );
    it.data()->setOpen(true);
    if( it.data() != root() )
      it.data()->parent()->setOpen(true);
  }
  else {
    kdDebug() << "Tried to set unknown dirItem to current" << endl;
  }
}


void K3bDataDirTreeView::setupActions()
{
  m_actionCollection = new KActionCollection( this );

  m_actionProperties = new KAction( i18n("Properties"), "misc", 0, this, SLOT(slotProperties()),
				    actionCollection(), "properties" );
  m_actionNewDir = new KAction( i18n("New Directory..."), "folder_new", CTRL+Key_N, this, SLOT(slotNewDir()),
				actionCollection(), "new_dir" );
  m_actionRemove = new KAction( i18n("Remove"), "editdelete", Key_Delete, this, SLOT(slotRemoveItem()),
				actionCollection(), "remove" );
  KShortcut renameShortCut( Key_F2 );
  renameShortCut.append( KShortcut(CTRL+Key_R) ); // backwards compatibility
  m_actionRename = new KAction( i18n("Rename"), "edit", renameShortCut, this, SLOT(slotRenameItem()),
				actionCollection(), "rename" );

  m_popupMenu = new KActionMenu( m_actionCollection, "contextMenu" );
  m_popupMenu->insert( m_actionRename );
  m_popupMenu->insert( m_actionRemove );
  m_popupMenu->insert( m_actionNewDir );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( m_actionProperties );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( m_view->actionCollection()->action("project_burn") );
}


void K3bDataDirTreeView::showPopupMenu( KListView*, QListViewItem* item, const QPoint& point )
{
  if( item ) {
    if( K3bDataViewItem* di = dynamic_cast<K3bDataViewItem*>(item) ) {
      m_actionRemove->setEnabled( di->dataItem()->isRemoveable() );
      m_actionRename->setEnabled( di->dataItem()->isRenameable() );
    }
    else {
      m_actionRemove->setEnabled( false );
      m_actionRename->setEnabled( false );
    }
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

    name = KInputDialog::getText( i18n("New Directory"),
				  i18n("Please insert the name for the new directory:"),
				  i18n("New Directory"), &ok, this );

    while( ok && K3bDataDoc::nameAlreadyInDir( name, parent ) ) {
      name = KInputDialog::getText( i18n("New Directory"),
                                    i18n("A file with that name already exists. "
					 "Please insert the name for the new directory:"),
				    i18n("New Directory"), &ok, this );
    }

    if( !ok )
      return;


    m_doc->addEmptyDir( name, parent );
  }
}


void K3bDataDirTreeView::slotRenameItem()
{
  showEditor( (K3bListViewItem*)currentItem(), 0 );
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
  if( viewItem && currentItem() != root() ) {
      QValueList<K3bDataItem*> dataItems;
      dataItems.append( viewItem->dataItem() );
      K3bDataPropertiesDialog d( dataItems, this );
      if( d.exec() ) {
          repaint();
          if( m_fileView )
              m_fileView->repaint();
      }
  }
  else
      m_view->slotProperties();
}


void K3bDataDirTreeView::startDropAnimation( K3bDirItem* dir )
{
  stopDropAnimation();

  K3bDataDirViewItem* vI = m_itemMap[dir];
  if( vI ) {
    d->animationCounter = 0;
    d->animatedDirItem = vI;
    d->beforeAniPixmap = QPixmap( *vI->pixmap(0) );
    QTimer::singleShot( 0, this, SLOT(slotDropAnimate()) );
  }
}


void K3bDataDirTreeView::slotDropAnimate()
{
  if( d->animatedDirItem ) {
    if( d->animationCounter > 5 )
      stopDropAnimation();
    else {
      switch(d->animationCounter) {
      case 0:
	d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_cyan" ) );
	break;
      case 1:
	d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_green" ) );
	break;
      case 2:
	d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_yellow" ) );
	break;
      case 3:
	d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_orange" ) );
	break;
      case 4:
	d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_red" ) );
	break;
      case 5:
	d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_violet" ) );
	break;
      }

      d->animationCounter++;
      QTimer::singleShot( 300, this, SLOT(slotDropAnimate()) );
    }
  }
}


void K3bDataDirTreeView::stopDropAnimation()
{
  if( d->animatedDirItem ) {
    d->animatedDirItem->setPixmap( 0, d->beforeAniPixmap );
    d->animatedDirItem = 0;
  }
}


// FIXME: remove this
void K3bDataDirTreeView::checkForNewItems()
{
  K3bDataItem* item = m_root->dirItem()->nextSibling();
  while( item != 0 )
    {
      // check if we have an entry and if not, create one
      // we can assume that a listViewItem for the parent exists
      // since we go top to bottom
      if( item->isDir() )
	{
	  K3bDirItem* dirItem = dynamic_cast<K3bDirItem*>( item );

	  QMapIterator<K3bDirItem*, K3bDataDirViewItem*> itDirItem = m_itemMap.find( dirItem );
	  if( itDirItem == m_itemMap.end() ) {
	    K3bDataDirViewItem* parentViewItem = m_itemMap[dirItem->parent()];
	    K3bDataDirViewItem* newDirItem = new K3bDataDirViewItem( dirItem, parentViewItem );
	    m_itemMap.insert( dirItem, newDirItem );
	  }
	  else {
	    // check if parent still correct (to get moved items)
	    K3bDataDirViewItem* dirViewItem = itDirItem.data();
	    K3bDataDirViewItem* parentViewItem = (K3bDataDirViewItem*)dirViewItem->parent();
	    K3bDataDirViewItem* dirParentViewItem = m_itemMap[dirItem->parent()];
	    if( dirParentViewItem != parentViewItem ) {
	      // reparent it
	      parentViewItem->takeItem( dirViewItem );
	      dirParentViewItem->insertItem( dirViewItem );
	    }
	  }
	}

      item = item->nextSibling();
    }


  // check the directory depth
  QListViewItemIterator it(root());
  while( it.current() != 0 ) {
    if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>(it.current()) )
      if( it.current() != m_root ) {
	K3bDirItem* dirItem = dirViewItem->dirItem();
	dirViewItem->setPixmap( 0, dirItem->depth() > 7 ? SmallIcon( "folder_red" ) : SmallIcon( "folder" ) );
      }

    ++it;
  }

  // always show the first level
  m_root->setOpen( true );
}


void K3bDataDirTreeView::slotDocChanged()
{
  // avoid flicker
  if( d->lastUpdateVolumeId != m_doc->isoOptions().volumeID() ) {
    d->lastUpdateVolumeId = m_doc->isoOptions().volumeID();
    root()->repaint();
  }
}

#include "k3bdatadirtreeview.moc"
