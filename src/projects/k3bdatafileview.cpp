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


#include "k3bdatafileview.h"
#include "k3bdataview.h"
#include <k3bdatadoc.h>
#include <k3bdataitem.h>
#include <k3bdiritem.h>
#include <k3bfileitem.h>
#include <k3bspecialdataitem.h>
#include <k3bsessionimportitem.h>
#include "k3bdataurladdingdialog.h"
#include <k3bvalidators.h>
#include "k3bdatapropertiesdialog.h"
#include "k3bdatadirtreeview.h"
#include "k3bdataviewitem.h"
#include <k3bview.h>


#include <q3dragobject.h>
#include <qpainter.h>
#include <qfontmetrics.h>
#include <qtimer.h>
#include <q3header.h>
#include <qfileinfo.h>
//Added by qt3to4:
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>
#include <Q3ValueList>
#include <Q3PtrList>

#include <klocale.h>
#include <kaction.h>
#include <kurldrag.h>
#include <kinputdialog.h>
#include <kdebug.h>
#include <kshortcut.h>
#include <krun.h>
#include <kdeversion.h>


K3bDataFileView::K3bDataFileView( K3bView* view, K3bDataDirTreeView* dirTreeView, K3bDataDoc* doc, QWidget* parent )
  : K3bListView( parent ),
    m_view(view),
    m_dropDirItem(0)
{
  m_treeView = dirTreeView;

  setAcceptDrops( true );
  setDropVisualizer( false );
  setDropHighlighter( true );
  setDragEnabled( true );
  setItemsMovable( false );
  setAllColumnsShowFocus( true );
  setShowSortIndicator( true );

  setNoItemText( i18n("Use drag'n'drop to add files and directories to the project.\n"
		      "To remove or rename files use the context menu.\n"
		      "After that press the burn button to write the CD.") );


  addColumn( i18n("Name") );
  addColumn( i18n("Type") );
  addColumn( i18n("Size") );
  addColumn( i18n("Local Path") );
  addColumn( i18n("Link") );

  setSelectionModeExt( KListView::Extended );

  m_doc = doc;
  m_currentDir = doc->root();
  checkForNewItems();

  connect( m_treeView, SIGNAL(dirSelected(K3bDirItem*)), this, SLOT(slotSetCurrentDir(K3bDirItem*)) );
  connect( m_doc, SIGNAL(itemRemoved(K3bDataItem*)), this, SLOT(slotDataItemRemoved(K3bDataItem*)) );
  connect( m_doc, SIGNAL(itemAdded(K3bDataItem*)), this, SLOT(slotItemAdded(K3bDataItem*)) );
  connect( this, SIGNAL(executed(Q3ListViewItem*)), this, SLOT(slotExecuted(Q3ListViewItem*)) );
  connect( this, SIGNAL(contextMenu(KListView*, Q3ListViewItem*, const QPoint&)),
	   this, SLOT(showPopupMenu(KListView*, Q3ListViewItem*, const QPoint&)) );
  connect( this, SIGNAL(dropped(QDropEvent*, Q3ListViewItem*, Q3ListViewItem*)),
	   this, SLOT(slotDropped(QDropEvent*, Q3ListViewItem*, Q3ListViewItem*)) );
  connect( this, SIGNAL(doubleClicked(Q3ListViewItem*, const QPoint&, int)),
	   this, SLOT(slotDoubleClicked(Q3ListViewItem*)) );

  setupActions();
}


K3bDataFileView::~K3bDataFileView()
{
}


K3bDirItem* K3bDataFileView::currentDir() const
{
  if( !m_currentDir )
    m_currentDir = m_doc->root();
  return m_currentDir;
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


void K3bDataFileView::slotItemAdded( K3bDataItem* item )
{
  if( item->parent() == currentDir() ) {
    K3bDataViewItem* vi = 0;
    if( item->isDir() )
      vi = new K3bDataDirViewItem( static_cast<K3bDirItem*>(item), this );
    else if( item->isFile() )
      vi = new K3bDataFileViewItem( static_cast<K3bFileItem*>(item), this );
    else if( item->isSpecialFile() )
      vi = new K3bSpecialDataViewItem( static_cast<K3bSpecialDataItem*>(item), this );
    else if( item->isFromOldSession() )
      vi = new K3bSessionImportViewItem( static_cast<K3bSessionImportItem*>(item), this );
    else
      kdDebug() << "(K3bDataFileView) ERROR: unknown data item type" << endl;

    if( vi )
      m_itemMap[item] = vi;
  }
}


void K3bDataFileView::slotDataItemRemoved( K3bDataItem* item )
{
  if( item->isDir() ) {
    if( static_cast<K3bDirItem*>(item)->isSubItem( currentDir() ) ) {
      slotSetCurrentDir( m_doc->root() );
    }
  }

  if( m_itemMap.contains( item ) ) {
    delete m_itemMap[item];
    m_itemMap.remove(item);
  }
}


void K3bDataFileView::checkForNewItems()
{
  hideEditor();

  // add items that are not there yet
  for( Q3PtrListIterator<K3bDataItem> it( m_currentDir->children() ); it.current(); ++it ) {
    if( !m_itemMap.contains( it.current() ) ) {
      slotItemAdded( it.current() );
    }
  }

  // now check if some of the items have been moved out of the currently showing dir.
  for( Q3ListViewItemIterator it( this ); it.current(); ++it ) {
    K3bDataViewItem* dataViewItem = dynamic_cast<K3bDataViewItem*>( it.current() );
    if( dataViewItem && dataViewItem->dataItem()->parent() != currentDir() )
      delete dataViewItem;
  }
}


Q3DragObject* K3bDataFileView::dragObject()
{
  Q3PtrList<Q3ListViewItem> selectedViewItems = selectedItems();
  KURL::List urls;
  for( Q3PtrListIterator<Q3ListViewItem> it( selectedViewItems ); it.current(); ++it ) {
    K3bDataViewItem* dataViewItem = dynamic_cast<K3bDataViewItem*>( it.current() );
    if( dataViewItem ) {
      urls.append( KURL::fromPathOrURL(dataViewItem->dataItem()->localPath()) );
    }
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


void K3bDataFileView::contentsDragMoveEvent( QDragMoveEvent* e )
{
  K3bListView::contentsDragMoveEvent( e );

  // highlight the folder the items would be added to
  if( m_dropDirItem )
    m_dropDirItem->highlightIcon( false );

  m_dropDirItem = dynamic_cast<K3bDataDirViewItem*>( itemAt(contentsToViewport(e->pos())) );
  if( m_dropDirItem )
    m_dropDirItem->highlightIcon( true );
}


void K3bDataFileView::contentsDragLeaveEvent( QDragLeaveEvent* e )
{
  K3bListView::contentsDragLeaveEvent( e );

  // remove any highlighting
  if( m_dropDirItem ) {
    m_dropDirItem->highlightIcon( false );
    m_dropDirItem = 0;
  }
}


void K3bDataFileView::slotDropped( QDropEvent* e, Q3ListViewItem*, Q3ListViewItem* )
{
  // remove any highlighting
  if( m_dropDirItem ) {
    m_dropDirItem->highlightIcon( false );
    m_dropDirItem = 0;
  }

  if( !e->isAccepted() )
    return;

  // determine K3bDirItem to add the items to
  m_addParentDir = currentDir();

  if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( itemAt(contentsToViewport(e->pos())) ) ) {
    // only add to a dir if we drop directly on the name
    if( header()->sectionAt( e->pos().x() ) == 0 )
      m_addParentDir = dirViewItem->dirItem();
  }

  if( m_addParentDir ) {

    // check if items have been moved
    if( e->source() == viewport() ) {
      // move all selected items
      Q3PtrList<Q3ListViewItem> selectedViewItems = selectedItems();
      Q3ValueList<K3bDataItem*> selectedDataItems;
      Q3PtrListIterator<Q3ListViewItem> it( selectedViewItems );
      for( ; it.current(); ++it ) {
	K3bDataViewItem* dataViewItem = dynamic_cast<K3bDataViewItem*>( it.current() );
	if( dataViewItem )
	  selectedDataItems.append( dataViewItem->dataItem() );
	else
	  kdDebug() << "no dataviewitem" << endl;
      }

      K3bDataUrlAddingDialog::copyMoveItems( selectedDataItems, m_addParentDir, this, e->action() == QDropEvent::Copy );
    }
    else if( e->source() == m_treeView->viewport() ) {
      // move the selected dir
      if( K3bDataDirViewItem* dirItem = dynamic_cast<K3bDataDirViewItem*>( m_treeView->selectedItem() ) ) {
	Q3ValueList<K3bDataItem*> selectedDataItems;
	selectedDataItems.append( dirItem->dirItem() );
	K3bDataUrlAddingDialog::copyMoveItems( selectedDataItems, m_addParentDir, this, e->action() == QDropEvent::Copy );
      }
    }
    else {
      // seems that new items have been dropped
      m_addUrls.clear();
      if( KURLDrag::decode( e, m_addUrls ) ) {
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


void K3bDataFileView::slotAddUrls()
{
  K3bDataUrlAddingDialog::addUrls( m_addUrls, m_addParentDir, this );
}


void K3bDataFileView::slotExecuted( Q3ListViewItem* item )
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
  KShortcut renameShortCut( Key_F2 );
  renameShortCut.append( KShortcut(CTRL+Key_R) ); // backwards compatibility
  m_actionRename = new KAction( i18n("Rename"), "edit", renameShortCut, this, SLOT(slotRenameItem()),
				actionCollection(), "rename" );
  m_actionParentDir = new KAction( i18n("Parent Directory"), "up", 0, this, SLOT(slotParentDir()),
				   actionCollection(), "parent_dir" );
  m_actionOpen = new KAction( i18n("Open"), "fileopen", 0, this, SLOT(slotOpen()),
				   actionCollection(), "open" );

  m_popupMenu = new KActionMenu( m_actionCollection, "contextMenu" );
  m_popupMenu->insert( m_actionParentDir );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( m_actionRename );
  m_popupMenu->insert( m_actionRemove );
  m_popupMenu->insert( m_actionNewDir );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( m_actionOpen );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( m_actionProperties );
  m_popupMenu->insert( new KActionSeparator( this ) );
  m_popupMenu->insert( m_view->actionCollection()->action("project_burn") );
}


void K3bDataFileView::showPopupMenu( KListView*, Q3ListViewItem* item, const QPoint& point )
{
  if( item ) {
    K3bDataItem* di = static_cast<K3bDataViewItem*>(item)->dataItem();
    m_actionRemove->setEnabled( di->isRemoveable() );
    m_actionRename->setEnabled( di->isRenameable() );
    if( currentDir() == m_doc->root() )
      m_actionParentDir->setEnabled( false );
    else
      m_actionParentDir->setEnabled( true );
    m_actionOpen->setEnabled( di->isFile() );
  }
  else {
    m_actionRemove->setEnabled( false );
    m_actionRename->setEnabled( false );
    m_actionOpen->setEnabled( false );
  }

  m_popupMenu->popup( point );
}


void K3bDataFileView::slotNewDir()
{
  K3bDirItem* parent = currentDir();

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


void K3bDataFileView::slotRenameItem()
{
  if( currentItem() )
    showEditor( (K3bListViewItem*)currentItem(), 0 );
}


void K3bDataFileView::slotRemoveItem()
{
  Q3PtrList<Q3ListViewItem> items = selectedItems();
  Q3PtrListIterator<Q3ListViewItem> it( items );
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
  Q3ValueList<K3bDataItem*> dataItems;

  // get selected item
  Q3PtrList<Q3ListViewItem> viewItems = selectedItems();
  for ( Q3PtrListIterator<Q3ListViewItem> it( viewItems ); *it; ++it ) {
      if( K3bDataViewItem* viewItem = dynamic_cast<K3bDataViewItem*>( *it ) ) {
          dataItems.append( viewItem->dataItem() );
      }
  }

  if ( dataItems.isEmpty() && currentDir() ) {
    // default to current dir
    dataItems.append( currentDir() );
  }

  if( !dataItems.isEmpty() ) {
    K3bDataPropertiesDialog d( dataItems, this );
    d.exec();
  }
  else
    m_view->slotProperties();
}


void K3bDataFileView::slotOpen()
{
  if( K3bDataViewItem* viewItem = dynamic_cast<K3bDataViewItem*>( selectedItems().first() ) ) {
    K3bDataItem* item = viewItem->dataItem();
    if( item->isFile() ) {
      K3bDataFileViewItem* fvi = static_cast<K3bDataFileViewItem*>( viewItem );
      if( fvi->mimeType() &&
#if KDE_IS_VERSION(3,3,0)
	  !KRun::isExecutableFile( KURL::fromPathOrURL(item->localPath()),
				   fvi->mimeType()->name() )
#else
	  !QFileInfo( item->localPath() ).isExecutable()
#endif
	  )
	KRun::runURL( KURL::fromPathOrURL(item->localPath()),
		      fvi->mimeType()->name() );
      else
	KRun::displayOpenWithDialog( KURL::fromPathOrURL(item->localPath()) );
    }
  }
}


void K3bDataFileView::slotDoubleClicked( Q3ListViewItem* )
{
    slotProperties();
}

#include "k3bdatafileview.moc"
