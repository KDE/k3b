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


#include "k3bdatafileview.h"
#include "k3bdataview.h"
#include "k3bdatadoc.h"
#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include "k3bspecialdataitem.h"
#include <k3bvalidators.h>
#include "k3bdatapropertiesdialog.h"
#include "k3bdatadirtreeview.h"
#include "k3bdataviewitem.h"
#include <k3bview.h>


#include <qdragobject.h>
#include <qpainter.h>
#include <qfontmetrics.h>

#include <klocale.h>
#include <kaction.h>
#include <kurldrag.h>
#include <klineeditdlg.h>
#include <kdebug.h>


K3bDataFileView::K3bDataFileView( K3bView* view, K3bDataDirTreeView* dirTreeView, K3bDataDoc* doc, QWidget* parent )
  : K3bListView( parent ), 
    m_view(view)
{
  m_treeView = dirTreeView;

  setAcceptDrops( true );
  setDropVisualizer( false );
  setDropHighlighter( true );
  setDragEnabled( true );
  setItemsMovable( false );
  setAllColumnsShowFocus( true );

  setNoItemText( i18n("Use drag'n'drop to add files and directories to the project.\n"
		 "To remove or rename files use the context menu.\n"
		 "After that press the burn button to write the CD.") );


  addColumn( i18n("Name") );
  addColumn( i18n("Type") );
  addColumn( i18n("Size") );
  addColumn( i18n("Local Path") );
  addColumn( i18n("Link") );

  setSelectionModeExt( KListView::Extended );

  setValidator( K3bValidators::iso9660Validator( false, this ) );

  m_doc = doc;
  m_currentDir = doc->root();
  checkForNewItems();

  connect( m_treeView, SIGNAL(dirSelected(K3bDirItem*)), this, SLOT(slotSetCurrentDir(K3bDirItem*)) );
  connect( m_doc, SIGNAL(itemRemoved(K3bDataItem*)), this, SLOT(slotDataItemRemoved(K3bDataItem*)) );
  connect( m_doc, SIGNAL(newFileItems()), this, SLOT(checkForNewItems()) );
  connect( m_doc, SIGNAL(changed()), this, SLOT(checkForNewItems()) );
  connect( this, SIGNAL(executed(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
  connect( this, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(showPopupMenu(KListView*, QListViewItem*, const QPoint&)) );
  connect( this, SIGNAL(dropped(QDropEvent*, QListViewItem*, QListViewItem*)),
	   this, SLOT(slotDropped(QDropEvent*, QListViewItem*, QListViewItem*)) );

  setupActions();
}


K3bDataFileView::~K3bDataFileView()
{
}


void K3bDataFileView::slotSetCurrentDir( K3bDirItem* dir )
{
  if( dir ) {
    m_currentDir = dir;
    clearItems();
    checkForNewItems();
  }
}


void K3bDataFileView::clearItems()
{
  m_itemMap.clear();
  K3bListView::clear();
}


void K3bDataFileView::checkForNewItems()
{
  kdDebug() << "(K3bDataFileView::checkForNewItems()" << endl;
  hideEditor();

  // add items that are not there yet
  for( QPtrListIterator<K3bDataItem> it( m_currentDir->children() ); it.current(); ++it ) {
    if( !m_itemMap.contains( it.current() ) ) {
      K3bDataViewItem* vi = 0;
      if( it.current()->isDir() )
	vi = new K3bDataDirViewItem( (K3bDirItem*)it.current(), this );
      else if( it.current()->isFile() )
	vi = new K3bDataFileViewItem( (K3bFileItem*)it.current(), this );
      else if( it.current()->isSpecialFile() )
	vi = new K3bSpecialDataViewItem( (K3bSpecialDataItem*)it.current(), this );
      else if( it.current()->isFromOldSession() )
	vi = new K3bSessionImportViewItem( (K3bSessionImportItem*)it.current(), this );
      else
	kdDebug() << "(K3bDataFileView) ERROR: unknown data item type" << endl;

      if( vi )
	m_itemMap[it.current()] = vi;
    }
  }
  kdDebug() << "(K3bDataFileView::checkForNewItems finished." << endl;
}


QDragObject* K3bDataFileView::dragObject()
{
  QPtrList<QListViewItem> selectedViewItems = selectedItems();
  KURL::List urls;
  for( QPtrListIterator<QListViewItem> it( selectedViewItems ); it.current(); ++it ) {
    K3bDataViewItem* dataViewItem = dynamic_cast<K3bDataViewItem*>( it.current() );
    if( dataViewItem )
      if( dataViewItem->dataItem()->isFile()
	  &&  !dataViewItem->dataItem()->localPath().isEmpty() )
	urls.append( KURL::fromPathOrURL(dataViewItem->dataItem()->localPath()) );
    else
      kdDebug() << "no dataviewitem" << endl;
  }

  if( urls.isEmpty() )
    return 0;

  return KURLDrag::newDrag( urls, viewport() );
}


bool K3bDataFileView::acceptDrag(QDropEvent* e) const
{
  return ( e->source() == viewport() || 
	   KURLDrag::canDecode(e) || 
	   e->source() == m_treeView->viewport() );
}


void K3bDataFileView::slotDropped( QDropEvent* e, QListViewItem*, QListViewItem* )
{
  if( !e->isAccepted() )
    return;

  // determine K3bDirItem to add the items to
  K3bDirItem* parent = 0;
  if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( itemAt(contentsToViewport(e->pos())) ) ) {
    parent = dirViewItem->dirItem();
  }
  else {
    parent = currentDir();
  }

  if( parent ) {

    // check if items have been moved
    if( e->source() == viewport() ) {
      // move all selected items
      QPtrList<QListViewItem> selectedViewItems = selectedItems();
      QPtrList<K3bDataItem> selectedDataItems;
      QPtrListIterator<QListViewItem> it( selectedViewItems );
      for( ; it.current(); ++it ) {
	K3bDataViewItem* dataViewItem = dynamic_cast<K3bDataViewItem*>( it.current() );
	if( dataViewItem )
	  selectedDataItems.append( dataViewItem->dataItem() );
	else
	  kdDebug() << "no dataviewitem" << endl;
      }

      m_doc->moveItems( selectedDataItems, parent );
    }
    else if( e->source() == m_treeView->viewport() ) {
      // move the selected dir
      if( K3bDataDirViewItem* dirItem = dynamic_cast<K3bDataDirViewItem*>( m_treeView->selectedItem() ) )
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


void K3bDataFileView::slotDataItemRemoved( K3bDataItem* item )
{
  kdDebug() << "(K3bDataFileView::slotDataItemRemoved) " << item->k3bName() << endl;

  if( item->isDir() ) {
    if( ((K3bDirItem*)item)->isSubItem( currentDir() ) ) {
      slotSetCurrentDir( item->parent() );
    }
  }
  
  if( m_itemMap.contains( item ) ) {
    kdDebug() << "(K3bDataFileView::slotDataItemRemoved) removing " << item->k3bName() << endl;
    delete m_itemMap[item];
    m_itemMap.remove(item);
  }
  kdDebug() << "(K3bDataFileView::slotDataItemRemoved finished" << endl;
}


void K3bDataFileView::slotExecuted( QListViewItem* item )
{
  if( K3bDataDirViewItem* k = dynamic_cast<K3bDataDirViewItem*>( item ) ) {
    hideEditor();  // disable the K3bListView Editor
    slotSetCurrentDir( k->dirItem() );
    emit dirSelected( currentDir() );
  }
}


void K3bDataFileView::setupActions()
{
  m_actionCollection = new KActionCollection( this );

  m_actionProperties = new KAction( i18n("Properties"), "misc", 0, this, SLOT(slotProperties()),
				    actionCollection(), "properties" );
  m_actionNewDir = new KAction( i18n("New Directory..."), "folder_new", CTRL+Key_N, this, SLOT(slotNewDir()),
				actionCollection(), "new_dir" );
  m_actionRemove = new KAction( i18n("Remove"), "editdelete", Key_Delete, this, SLOT(slotRemoveItem()),
				actionCollection(), "remove" );
  m_actionRename = new KAction( i18n("Rename"), "edit", CTRL+Key_R, this, SLOT(slotRenameItem()),
				actionCollection(), "rename" );
  m_actionParentDir = new KAction( i18n("Parent Directory"), "up", 0, this, SLOT(slotParentDir()),
				   actionCollection(), "parent_dir" );

  m_popupMenu = new KActionMenu( m_actionCollection, "contextMenu" );
  m_popupMenu->insert( m_actionParentDir );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( m_actionRename );
  m_popupMenu->insert( m_actionRemove );
  m_popupMenu->insert( m_actionNewDir );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( m_actionProperties );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( m_doc->actionCollection()->action("project_burn") );
}


void K3bDataFileView::showPopupMenu( KListView*, QListViewItem* item, const QPoint& point )
{
  if( item ) {
    K3bDataItem* di = ((K3bDataViewItem*)item)->dataItem();
    m_actionRemove->setEnabled( di->isRemoveable() );
    m_actionRename->setEnabled( di->isRenameable() );
    if( currentDir() == m_doc->root() )
      m_actionParentDir->setEnabled( false );
    else
      m_actionParentDir->setEnabled( true );
  }
  else {
    m_actionRemove->setEnabled( false );
    m_actionRename->setEnabled( false );
  }

  m_popupMenu->popup( point );
}


void K3bDataFileView::slotNewDir()
{
  K3bDirItem* parent = currentDir();

  QString name;
  bool ok;

  name = KLineEditDlg::getText( i18n("Please insert the name for the new directory:"),
				i18n("New Directory"), &ok, this );

  while( ok && K3bDataDoc::nameAlreadyInDir( name, parent ) ) {
    name = KLineEditDlg::getText( i18n("A file with that name already exists."
				       "Please insert the name for the new directory:"),
				  i18n("New Directory"), &ok, this );
  }

  if( !ok )
    return;


  m_doc->addEmptyDir( name, parent );
}


void K3bDataFileView::slotRenameItem()
{
  showEditor( (K3bListViewItem*)currentItem(), 0 );
}


void K3bDataFileView::slotRemoveItem()
{
  QPtrList<QListViewItem> items = selectedItems();
  QPtrListIterator<QListViewItem> it( items );
  for(; it.current(); ++it ) {
    if( K3bDataViewItem* d = dynamic_cast<K3bDataViewItem*>( it.current() ) )
      m_doc->removeItem( d->dataItem() );
  }
}


void K3bDataFileView::slotParentDir()
{
  if( currentDir() != m_doc->root() ) {
    slotSetCurrentDir( currentDir()->parent() );

    emit dirSelected( currentDir() );
  }
}


void K3bDataFileView::slotProperties()
{
  K3bDataItem* dataItem = 0;

  // get selected item
  if( K3bDataViewItem* viewItem = dynamic_cast<K3bDataViewItem*>( selectedItems().first() ) ) {
    dataItem = viewItem->dataItem();
  }
  else {
    // default to current dir
    dataItem = currentDir();
  }

  if( dataItem ) {
    K3bDataPropertiesDialog d( dataItem, this );
    d.exec();
  }
  else
    m_doc->slotProperties();
}


#include "k3bdatafileview.moc"
