/***************************************************************************
                          k3bdatafileview.cpp  -  description
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

#include "k3bdatafileview.h"
#include "k3bdataview.h"
#include "k3bdatadoc.h"
#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include "k3bisovalidator.h"
#include "k3bdatapropertiesdialog.h"
#include "k3bdatadirtreeview.h"
#include "k3bdataviewitem.h"
#include "../k3b.h"
#include "../k3bview.h"

#include "../klistviewlineedit.h"


#include <qdragobject.h>
#include <qpainter.h>
#include <qfontmetrics.h>

#include <klocale.h>
#include <kaction.h>
#include <kurldrag.h>
#include <klineeditdlg.h>
#include <kdebug.h>


K3bDataFileView::K3bDataFileView( K3bView* view, K3bDataDirTreeView* dirTreeView, K3bDataDoc* doc, QWidget* parent )
  : K3bListView( parent ), m_view(view)
{
  m_treeView = dirTreeView;

  setAcceptDrops( true );
  setDropVisualizer( false );
  setDropHighlighter( true );
  setDragEnabled( true );
  setItemsMovable( false );
  setAllColumnsShowFocus( true );

  setNoItemText( i18n("Use drag'n'drop to add files and directories to the project.") +"\n"
		 + i18n("To remove or rename files use the context menu.") + "\n" 
		 + i18n("After that press the burn button to write the CD.") );

	
  addColumn( i18n("Name") );
  addColumn( i18n("Type") );
  addColumn( i18n("Size") );
  addColumn( i18n("Local Path") );
  addColumn( i18n("Link") );

  setItemsRenameable( true );
  setSelectionModeExt( KListView::Extended );

  m_editor = new KListViewLineEdit( this );
  m_editor->setValidator( new K3bIsoValidator( m_editor ) );
  
  m_doc = doc;
  m_currentDir = doc->root();
  updateContents();

  connect( m_treeView, SIGNAL(dirSelected(K3bDirItem*)), this, SLOT(slotSetCurrentDir(K3bDirItem*)) );
  connect( m_doc, SIGNAL(itemRemoved(K3bDataItem*)), this, SLOT(slotDataItemRemoved(K3bDataItem*)) );
  connect( m_doc, SIGNAL(newFileItems()), this, SLOT(updateContents()) );
  connect( this, SIGNAL(executed(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
  connect( m_editor, SIGNAL(done(QListViewItem*,int)), this, SLOT(doneEditing(QListViewItem*,int)) );
  connect( this, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(showPopupMenu(KListView*, QListViewItem*, const QPoint&)) );
  connect( this, SIGNAL(dropped(QDropEvent*, QListViewItem*, QListViewItem*)),
	   this, SLOT(slotDropped(QDropEvent*, QListViewItem*, QListViewItem*)) );

  setupActions();
}


K3bDataFileView::~K3bDataFileView()
{
}


void K3bDataFileView::rename( QListViewItem* item, int c )
{
  m_editor->load( item, c );
}


void K3bDataFileView::slotSetCurrentDir( K3bDirItem* dir )
{
  if( dir ) {
    m_currentDir = dir;
    updateContents();
  }
}


void K3bDataFileView::updateContents()
{
  // clear view
  clear();

  // perhaps we should check if the K3bDirItem m_currentDir still exists
	
  //  kdDebug() << "(K3bDataFileView) reloading current dir: " << m_currentDir->k3bName() << endl;
	
  for( QListIterator<K3bDataItem> it( *m_currentDir->children() ); it.current(); ++it ) {
    if( it.current()->isDir() )
      (void)new K3bDataDirViewItem( (K3bDirItem*)it.current(), this );
    else
      (void)new K3bDataFileViewItem( (K3bFileItem*)it.current(), this );

//     if( K3bDirItem* _item = dynamic_cast<K3bDirItem*>( _it.current() ) ) {
//       (void)new K3bDataDirViewItem( _item, this );
//     }
//     else if( K3bFileItem* _item = dynamic_cast<K3bFileItem*>( _it.current() ) ) {
//       (void)new K3bDataFileViewItem( _item, this );
//     }
  }
	
  //  kdDebug() << "(K3bDataFileView) reloading finished" << endl;
}


bool K3bDataFileView::acceptDrag(QDropEvent* e) const{
  return ( e->source() == viewport() || QUriDrag::canDecode(e) || e->source() == m_treeView->viewport() );
}


void K3bDataFileView::slotDropped( QDropEvent* e, QListViewItem*, QListViewItem* )
{
  if( !e->isAccepted() )
    return;

  // determine K3bDirItem to add the items to
  K3bDirItem* parent = 0;
  if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( itemAt(e->pos()) ) ) {
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
  if( item == currentDir() ) 
    {
      slotSetCurrentDir( item->parent() );
      
    }
  if( item->parent() == currentDir() ) 
    {
      QListViewItemIterator it(this);
      for( ; it.current(); ++it )
	{
	  if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>(it.current()) ) {
	    if( dirViewItem->dirItem() == item ) {
	      delete it.current();
	      break;
	    }
	  }
	  else if( K3bDataFileViewItem* fileViewItem = dynamic_cast<K3bDataFileViewItem*>(it.current()) ) {
	    if( fileViewItem->fileItem() == item ) {
	      delete it.current();
	      break;
	    }
	  }
	} // for it
    }
}


void K3bDataFileView::slotExecuted( QListViewItem* item )
{
  if( K3bDataDirViewItem* k = dynamic_cast<K3bDataDirViewItem*>( item ) ) {
    slotSetCurrentDir( k->dirItem() );
    emit dirSelected( currentDir() );
  }
}


void K3bDataFileView::setupActions()
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
  m_popupMenu->insert( k3bMain()->actionCollection()->action("file_burn") );
}


void K3bDataFileView::showPopupMenu( KListView*, QListViewItem* item, const QPoint& point )
{
  if( item ) {
    m_actionRemove->setEnabled( true );
    m_actionRename->setEnabled( true );
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


void K3bDataFileView::slotRenameItem()
{
  rename( currentItem(), 0 );
}


void K3bDataFileView::slotRemoveItem()
{
  QPtrList<QListViewItem> items = selectedItems();
  QListIterator<QListViewItem> it( items );
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
    m_view->burnDialog( false );
}


#include "k3bdatafileview.moc"
