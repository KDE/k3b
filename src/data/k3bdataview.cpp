/***************************************************************************
                          k3bdataview.cpp  -  description
                             -------------------
    begin                : Thu May 10 2001
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

#include "k3bdataview.h"
#include "k3bdatadoc.h"
#include "../k3bfillstatusdisplay.h"
#include "k3bfileitem.h"
#include "k3bdiritem.h"
#include "../k3b.h"
#include "k3bdataburndialog.h"
#include "k3bdatapropertiesdialog.h"


#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>
#include <kapp.h>
#include <kpopupmenu.h>
#include <kaction.h>
#include <klineeditdlg.h>
#include <kio/global.h>

#include <qpixmap.h>
#include <qsplitter.h>
#include <qlayout.h>
#include <qdragobject.h>
#include <qheader.h>
#include <qlist.h>

#include <assert.h>


K3bDataView::K3bDataView(K3bDataDoc* doc, QWidget *parent, const char *name )
  : K3bView(doc, parent,name)
{
  m_doc = doc;
  m_burnDialog = 0;
	
  // --- setup GUI ---------------------------------------------------
  QSplitter* _main = new QSplitter( this );	
  m_dataDirTree = new K3bDataDirTreeView( this, doc, _main );
  m_dataFileView = new K3bDataFileView( this, doc, _main );
  m_fillStatusDisplay = new K3bFillStatusDisplay( doc, this );
	
  QVBoxLayout* _box = new QVBoxLayout( this );
  _box->addWidget( _main );
  _box->addWidget( m_fillStatusDisplay );
  _box->setSpacing( 5 );
  _box->setMargin( 2 );

  setupActions();
  setupPopupMenu();	

  connect( m_dataDirTree, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(showPopupMenu(QListViewItem*, const QPoint&)) );
  connect( m_dataFileView, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(showPopupMenu(QListViewItem*, const QPoint&)) );

  // connect to the dropped signals
  connect( m_dataDirTree, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*, QListViewItem*)),
	   this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*,QListViewItem*)) );
  connect( m_dataFileView, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*, QListViewItem*)),
	   this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*, QListViewItem*)) );

  connect( m_dataDirTree, SIGNAL(dirSelected(K3bDirItem*)), m_dataFileView, SLOT(slotSetCurrentDir(K3bDirItem*)) );
  connect( m_dataFileView, SIGNAL(dirSelected(K3bDirItem*)), m_dataDirTree, SLOT(setCurrentDir(K3bDirItem*)) );

  connect( m_doc, SIGNAL(newFileItems()), m_dataDirTree, SLOT(updateContents()) );
  connect( m_doc, SIGNAL(newFileItems()), m_dataFileView, SLOT(updateContents()) );

  connect( m_doc, SIGNAL(itemRemoved(K3bDataItem*)), m_fillStatusDisplay, SLOT(update()) );
  connect( m_doc, SIGNAL(newFileItems()), m_fillStatusDisplay, SLOT(update()) );

  m_dataDirTree->updateContents();
  m_dataFileView->updateContents();
}


K3bDataView::~K3bDataView(){
}


K3bProjectBurnDialog* K3bDataView::burnDialog()
{
  if( !m_burnDialog ) {
    m_burnDialog = new K3bDataBurnDialog( m_doc, k3bMain(), "databurndialog", true );
  }
		
  return m_burnDialog;
}


K3bDirItem* K3bDataView::currentDir() const
{
  return m_dataFileView->currentDir();
}


void K3bDataView::slotDropped( KListView* listView, QDropEvent* e, QListViewItem* after, QListViewItem* parentViewItem )
{
  if( !e->isAccepted() )
    return;

  // determine K3bDirItem to add the items to
  K3bDirItem* parent = 0;
  if( listView == m_dataFileView ) {
    if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( after ) ) {
      parent = dirViewItem->dirItem();
    }
    else {
      parent = m_dataFileView->currentDir();
    }
  }
  else if( listView == m_dataDirTree ) {
    if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( parentViewItem ) ) {
      parent = dirViewItem->dirItem();
    }
  }

  if( parent ) {

    // check if items have been moved
    if( e->source() == m_dataFileView->viewport() ) {
      // move all selected items
      QList<QListViewItem> selectedViewItems = m_dataFileView->selectedItems();
      QList<K3bDataItem> selectedDataItems;
      QListIterator<QListViewItem> it( selectedViewItems );
      for( ; it.current(); ++it ) {
	K3bDataViewItem* dataViewItem = dynamic_cast<K3bDataViewItem*>( it.current() );
	if( dataViewItem )
	  selectedDataItems.append( dataViewItem->dataItem() );
	else
	  qDebug("no dataviewitem");
      }

      m_doc->moveItems( selectedDataItems, parent );
    }
    else if( e->source() == m_dataDirTree->viewport() ) {
      // move the selected dir
      K3bDirItem* dirItem = ((K3bDataDirViewItem*)m_dataDirTree->selectedItem())->dirItem();
      m_doc->moveItem( dirItem, parent );
    }

    // seems that new items have been dropped
    else if( QTextDrag::canDecode( e ) ) {
      QString droppedText;
      QTextDrag::decode( e, droppedText );
      QStringList urls = QStringList::split("\r\n", droppedText );
    
      m_doc->slotAddUrlsToDir( urls, parent );
    }
  }
}


bool K3bDataView::acceptDrag( QDropEvent* e ) const
{
  return( e->source() == m_dataFileView->viewport() || e->source() == m_dataDirTree->viewport() );
}


void K3bDataView::setupActions()
{
  m_actionProperties = new KAction( i18n("Properties..."), "misc", 0, this, SLOT(slotProperties()), actionCollection() );
  m_actionCollection->insert( new KActionSeparator( this ) );
  m_actionNewDir = new KAction( i18n("New Directory..."), "folder_new", CTRL+Key_N, this, SLOT(slotNewDir()), actionCollection() );
  m_actionRemove = new KAction( i18n("Remove"), "editdelete", Key_Delete, this, SLOT(slotRemoveItem()), actionCollection() );
  m_actionRename = new KAction( i18n("Rename"), "edit", CTRL+Key_R, this, SLOT(slotRenameItem()), actionCollection() );
  m_actionCollection->insert( new KActionSeparator( this ) );
  m_actionParentDir = new KAction( i18n("Parent Directory"), "up", 0, this, SLOT(slotParentDir()), actionCollection() );

  //TODO: disable remove by default since the root item cannot be removed and enable when items are there
  //      we need to check if tree or fileview are active
  //      for example: connect to the selected signals of both and check if it is a root item
  //                   and check if items are left after removal

  // TODO: add properties-action
  //       for dir and files: local file/dir info, project filename change and info
  //       for root item: doc properties without write-button
}


void K3bDataView::setupPopupMenu()
{
  m_popupMenu = new KPopupMenu( this, "DataViewPopupMenu" );
  m_actionParentDir->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  m_actionRemove->plug( m_popupMenu );
  m_actionRename->plug( m_popupMenu);
  m_actionNewDir->plug( m_popupMenu);
  m_popupMenu->insertSeparator();
  m_actionProperties->plug( m_popupMenu );
}

void K3bDataView::showPopupMenu( QListViewItem* item, const QPoint& point )
{
  if( item ) {
    if( item == m_dataDirTree->root() ) {
      m_actionRemove->setEnabled( false );
    }
    else {
      m_actionRemove->setEnabled( true );
    }

    if( m_dataFileView->currentDir() == m_doc->root() ) {
      m_actionParentDir->setEnabled( false );
    }
    else {
      m_actionParentDir->setEnabled( true );
    }

    m_actionRemove->setEnabled( true );
    m_actionRename->setEnabled( true );

    m_popupMenu->popup( point );
  }
  else if( m_dataFileView->hasFocus() ) {
    m_actionRemove->setEnabled( false );
    m_actionRename->setEnabled( false );

    m_popupMenu->popup( point );
  }
}


void K3bDataView::slotNewDir()
{
  K3bDirItem* parent;
  if( m_dataDirTree->hasFocus() ) 
    {
      parent = ( (K3bDataDirViewItem*)m_dataDirTree->currentItem() )->dirItem();
    }
  else
    {
      parent = m_dataFileView->currentDir();
    }

  QString name;
  bool ok;

  name = KLineEditDlg::getText( i18n("Please insert the name for the new directory"),
				"New directory", &ok, k3bMain() );

  while( ok && K3bDataDoc::nameAlreadyInDir( name, parent ) ) {
    name = KLineEditDlg::getText( i18n("A file with that name already exists.\nPlease insert the name for the new directory"),
				  "New directory", &ok, k3bMain() );
  }

  if( !ok )
    return;


  m_doc->addEmptyDir( name, parent );
}

void K3bDataView::slotRenameItem()
{
  if( m_dataDirTree->hasFocus() ) {
    m_dataDirTree->rename( m_dataDirTree->currentItem(), 0 );
  }
  else if( m_dataFileView->hasFocus() ) {
    m_dataFileView->rename( m_dataFileView->currentItem(), 0 );
  }
  else
    qDebug("(K3bDataView) slotRenameItem() without selected item!");
}


void K3bDataView::slotRemoveItem()
{
  if( m_dataDirTree->hasFocus() && m_dataDirTree->currentItem() ) {
    if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( m_dataDirTree->currentItem() ) )
      m_doc->removeItem( dirViewItem->dirItem() );
  }
  else if( m_dataFileView->hasFocus() ) {
    QList<QListViewItem> items = m_dataFileView->selectedItems();
    QListIterator<QListViewItem> it( items );
    for(; it.current(); ++it ) {
      if( K3bDataViewItem* d = dynamic_cast<K3bDataViewItem*>( it.current() ) )
	m_doc->removeItem( d->dataItem() );
    }
  }
  else
    qDebug("(K3bDataView) slotRemoveItem() without selected item!");
}


void K3bDataView::slotParentDir()
{
  if( m_dataFileView->hasFocus() ) {
    if( m_dataFileView->currentDir() != m_doc->root() ) {
      m_dataFileView->slotSetCurrentDir( m_dataFileView->currentDir()->parent() );
      m_dataDirTree->setCurrentDir( m_dataFileView->currentDir() );
    }
  }
  else if( m_dataDirTree->hasFocus() ) {
    if( m_dataDirTree->currentItem() != m_dataDirTree->root() )
      if( K3bDataDirViewItem* k = dynamic_cast<K3bDataDirViewItem*>( m_dataDirTree->currentItem() ) ) {
	m_dataDirTree->setCurrentDir( k->dirItem()->parent() );
	m_dataFileView->slotSetCurrentDir( k->dirItem()->parent() );
      }
  }
}


void K3bDataView::slotProperties()
{
  K3bDataItem* dataItem = 0;

  // get selected item
  if( m_dataDirTree->hasFocus() ) {
    if( dynamic_cast<K3bDataRootViewItem*>( m_dataDirTree->currentItem() ) ) {
      burnDialog()->exec( false );
    }
    else if( K3bDataViewItem* viewItem = dynamic_cast<K3bDataViewItem*>( m_dataDirTree->currentItem() ) ) {
      dataItem = viewItem->dataItem();
    }
  }
  else {
    if( K3bDataViewItem* viewItem = dynamic_cast<K3bDataViewItem*>( m_dataFileView->currentItem() ) ) {
      dataItem = viewItem->dataItem();
    }
    else {
      // default to current dir
      dataItem = m_dataFileView->currentDir();
    }
  }

  if( dataItem ) {
    K3bDataPropertiesDialog d( dataItem, this );
    d.exec();
  }
}


// =================================================================================
// ViewItems
// =================================================================================

K3bDataViewItem::K3bDataViewItem( QListView* parent )
  : KListViewItem( parent )
{
}

K3bDataViewItem::K3bDataViewItem( QListViewItem* parent )
  : KListViewItem( parent )
{
}

K3bDataViewItem::~K3bDataViewItem()
{}

K3bDataDirViewItem::K3bDataDirViewItem( K3bDirItem* dir, QListView* parent )
  : K3bDataViewItem( parent )
{
  assert( dir );

  m_dirItem = dir;
  setPixmap( 0, KMimeType::pixmapForURL( KURL( "/" ), 0, KIcon::Small ) );
}


K3bDataDirViewItem::K3bDataDirViewItem( K3bDirItem* dir, QListViewItem* parent )
  : K3bDataViewItem( parent )
{
  assert( dir );

  m_dirItem = dir;
  setPixmap( 0, KMimeType::pixmapForURL( KURL( "/" ), 0, KIcon::Small ) );
}


K3bDataItem* K3bDataDirViewItem::dataItem() const
{
  return m_dirItem; 
}


K3bDataDirViewItem::~K3bDataDirViewItem()
{
}

	
QString K3bDataDirViewItem::text( int index ) const
{
  switch( index ) {
  case 0:
    return m_dirItem->k3bName();
  case 1:
    return i18n("Directory");
  default:
    return "";
  }
}


void K3bDataDirViewItem::setText(int col, const QString& text )
{
  if( col == 0 )
    dirItem()->setK3bName( text );

  KListViewItem::setText( col, text );
}


QString K3bDataDirViewItem::key( int col, bool a ) const
{
  if( a ) {
    return "0" + text(col);
  }
  else {
    return "1" + text(col);
  }
}

	
K3bDataFileViewItem::K3bDataFileViewItem( K3bFileItem* file, QListView* parent )
  : K3bDataViewItem( parent )
{
  assert( file );

  m_fileItem = file;
  setPixmap( 0, file->pixmap(16) );
}


K3bDataFileViewItem::K3bDataFileViewItem( K3bFileItem* file, QListViewItem* parent )
  : K3bDataViewItem( parent )
{
  assert( file );
	
  m_fileItem = file;
  setPixmap( 0, file->pixmap(16) );
}


K3bDataItem* K3bDataFileViewItem::dataItem() const
{
  return m_fileItem; 
}

	
QString K3bDataFileViewItem::text( int index ) const
{
  switch( index ) {
  case 0:
    return m_fileItem->k3bName();   // TODO: add the ability to show ISO, Joilet, or RockRidge
  case 1:
    return m_fileItem->mimeComment();
  case 2:
    return KIO::convertSize( m_fileItem->size() );
  default:
    return "";
  }
}


void K3bDataFileViewItem::setText(int col, const QString& text )
{
  if( col == 0 )
    fileItem()->setK3bName( text );
		
  KListViewItem::setText( col, text );
}


QString K3bDataFileViewItem::key( int col, bool a ) const
{
  if( a ) {
    return "1" + text(col);
  }
  else {
    return "0" + text(col);
  }
}


K3bDataRootViewItem::K3bDataRootViewItem( K3bDataDoc* doc, QListView* parent )
  : K3bDataDirViewItem( doc->root(), parent )
{
  m_doc = doc;
  setPixmap( 0, SmallIcon( "cdrom_unmount" ) );
}


QString K3bDataRootViewItem::text( int index ) const
{
  switch( index ) {
  case 0:
    return m_doc->volumeID();
  default:
    return "";
  }
}


void K3bDataRootViewItem::setText( int col, const QString& text )
{
  if( col == 0 )
    m_doc->setVolumeID( text );

  KListViewItem::setText( col, text );
}




// void K3bDataView::slotItemRemoved( K3bDataItem* item )
// {
//   // we only need to search in the fileView if it currently displays the corresponding directory
//   if( item == m_dataFileView->currentDir() ) {
//     qDebug( "(K3bDataView) fileView currently displays a deleted directory. Setting to parent.");
//     m_dataFileView->slotSetCurrentDir( item->parent() );
//   }
//   else if( item->parent() == m_dataFileView->currentDir() ) {
//     qDebug("(K3bDataView) seaching in fileView for viewItems to delete");
//     QListViewItemIterator _it2(m_dataFileView);
//     for( ; _it2.current(); ++_it2 )
//       {
// 	if( K3bDataDirViewItem* _dirViewItem = dynamic_cast<K3bDataDirViewItem*>(_it2.current()) ) {
// 	  qDebug("   found dirViewItem ... comparing ... ");
// 	  if( _dirViewItem->dirItem() == item ) {
// 	    delete _it2.current();
// 	    qDebug( "(K3bDataView) found listViewItem to remove in fileView: %s", item->k3bName().latin1() );
// 	    break;
// 	  }
// 	}
// 	else if( K3bDataFileViewItem* _fileViewItem = dynamic_cast<K3bDataFileViewItem*>(_it2.current()) ) {
// 	  qDebug("   found fileViewItem ... comparing ... ");
// 	  if( _fileViewItem->fileItem() == item ) {
// 	    delete _it2.current();
// 	    qDebug( "(K3bDataView) found listViewItem to remove in fileView: %s", item->k3bName().latin1() );
// 	    break;
// 	  }
// 	}
			
//       } // for _it2
//   }

//   m_fillStatusDisplay->repaint();	
// }


#include "k3bdataview.moc"
