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

#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>
#include <kapp.h>
#include <kpopupmenu.h>
#include <kaction.h>
#include <klineeditdlg.h>

#include <qpixmap.h>
#include <qsplitter.h>
#include <qlayout.h>
#include <qdragobject.h>
#include <qheader.h>

#include <assert.h>


K3bDataView::K3bDataView(K3bDataDoc* doc, QWidget *parent, const char *name )
  : K3bView(doc, parent,name)
{
  m_doc = doc;
  m_burnDialog = 0;
	
  // --- setup GUI ---------------------------------------------------
  QSplitter* _main = new QSplitter( this );	
  m_dataDirTree = new K3bDataDirTreeView( doc, _main );
  m_dataFileView = new K3bDataFileView( doc, _main );
  m_fillStatusDisplay = new K3bFillStatusDisplay( doc, this );
	
  QVBoxLayout* _box = new QVBoxLayout( this );
  _box->addWidget( _main );
  _box->addWidget( m_fillStatusDisplay );
  _box->setSpacing( 5 );
  _box->setMargin( 2 );

  setupPopupMenu();	
  connect( m_dataDirTree, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(showPopupMenu(QListViewItem*, const QPoint&)) );
  connect( m_dataFileView, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(showPopupMenu(QListViewItem*, const QPoint&)) );

  // connect to the dropped signals
  connect( m_dataDirTree, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*)),
	   this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*)) );
  connect( m_dataFileView, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*)),
	   this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*)) );

  connect( m_dataDirTree, SIGNAL(dirSelected(K3bDirItem*)), m_dataFileView, SLOT(slotSetCurrentDir(K3bDirItem*)) );

  connect( m_doc, SIGNAL(newFileItems()), m_dataDirTree, SLOT(updateContents()) );
  connect( m_doc, SIGNAL(newFileItems()), m_dataFileView, SLOT(updateContents()) );

  connect( m_doc, SIGNAL(itemRemoved(K3bDataItem*)), m_fillStatusDisplay, SLOT(update()) );
  connect( m_doc, SIGNAL(newFileItems()), m_fillStatusDisplay, SLOT(update()) );
}


K3bDataView::~K3bDataView(){
}


K3bProjectBurnDialog* K3bDataView::burnDialog()
{
  if( !m_burnDialog )
    m_burnDialog = new K3bDataBurnDialog( m_doc, k3bMain(), "databurndialog", true );
		
  return m_burnDialog;
}


void K3bDataView::slotAddFile( K3bFileItem* file )
{
  if( m_dataFileView->currentDir() == file->parent() )
    m_dataFileView->updateContents();
	
  m_fillStatusDisplay->repaint();
}


void K3bDataView::slotAddDir( K3bDirItem* dirItem )
{
  K3bDirItem* parentDir = dirItem->parent();
	
  if( !parentDir ) {
    (void)new K3bDataDirViewItem( dirItem, m_dataDirTree->root() );
    parentDir = m_doc->root();
  }
  else {
    QListViewItemIterator _it(m_dataDirTree);
    for( ; _it.current(); ++_it )
      {
	K3bDataDirViewItem* _dirViewItem = (K3bDataDirViewItem*)_it.current();
	if( _dirViewItem->dirItem() == dirItem->parent() ) {
	  (void)new K3bDataDirViewItem( dirItem, _it.current() );
	  break;
	}
      }
			
    // add to root by default
    if( !_it.current() ) {
      qDebug("(K3bDataView) could not find corresponding listViewItem to %s", dirItem->parent()->k3bName().latin1() );
      (void)new K3bDataDirViewItem( dirItem, m_dataDirTree->root() );
      parentDir = m_doc->root();
    }
  }

  if( m_dataFileView->currentDir() == parentDir )
    m_dataFileView->updateContents();
		
  m_fillStatusDisplay->repaint();
}
	
	
void K3bDataView::slotDropped( KListView* listView, QDropEvent* e, QListViewItem* after )
{
  if( !e->isAccepted() )
    return;

  QString droppedText;
  QTextDrag::decode( e, droppedText );
  QStringList _urls = QStringList::split("\r\n", droppedText );
	

  if( listView == m_dataFileView &&
      m_dataFileView->itemAt( e->pos() ) == 0 )
    emit dropped( _urls, m_dataFileView->currentDir() );

  // get directory from "after"
  else if( after ) {
    // if dropping on a dirItem: add it to this dir
    if( dynamic_cast<K3bDataDirViewItem*>( after ) ) {
      emit dropped( _urls, ((K3bDataDirViewItem*)after)->dirItem() );
    }
    else {
      // add to the parent of the item
      emit dropped( _urls, ((K3bDataFileViewItem*)after)->fileItem()->parent() );
    }
  }
  else {// after == 0
    if( listView == m_dataFileView )
      emit dropped( _urls, m_dataFileView->currentDir() );
  }
}


K3bDataDirViewItem::K3bDataDirViewItem( K3bDirItem* dir, QListView* parent )
  : KListViewItem( parent )
{
  assert( dir );

  m_dirItem = dir;
  setPixmap( 0, *(new QPixmap(KMimeType::pixmapForURL( KURL( "/" ), 0, KIcon::Small ) )) );
}


K3bDataDirViewItem::K3bDataDirViewItem( K3bDirItem* dir, QListViewItem* parent )
  : KListViewItem( parent )
{
  assert( dir );

  m_dirItem = dir;
  setPixmap( 0, *(new QPixmap(KMimeType::pixmapForURL( KURL( "/" ), 0, KIcon::Small ) )) );
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

	
K3bDataFileViewItem::K3bDataFileViewItem( K3bFileItem* file, QListView* parent )
  : KListViewItem( parent )
{
  assert( file );

  m_fileItem = file;
  setPixmap( 0, file->pixmap(16) );
}


K3bDataFileViewItem::K3bDataFileViewItem( K3bFileItem* file, QListViewItem* parent )
  : KListViewItem( parent )
{
  assert( file );
	
  m_fileItem = file;
  setPixmap( 0, file->pixmap(16) );
}

	
QString K3bDataFileViewItem::text( int index ) const
{
  switch( index ) {
  case 0:
    return m_fileItem->k3bName();   // TODO: add the ability to show ISO, Joilet, or RockRidge
  case 1:
    return m_fileItem->mimeComment();
  case 2:
    return QString::number( m_fileItem->size() );
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


K3bDataRootViewItem::K3bDataRootViewItem( K3bDataDoc* doc, QListView* parent )
  : K3bDataDirViewItem( doc->root(), parent )
{
  m_doc = doc;
  setPixmap( 0, kapp->iconLoader()->loadIcon( "cdrom_unmount", KIcon::Small, 16 ) );
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


void K3bDataView::setupPopupMenu()
{
  m_popupMenu = new KPopupMenu( this, "DataViewPopupMenu" );
  //	m_popupMenu->insertTitle( i18n( "File Options" ) );
  actionRename = new KAction( i18n("&Rename"), SmallIcon( "rename" ), CTRL+Key_R, this, SLOT(slotRenameItem()), this );
  actionRemove = new KAction( i18n( "&Remove" ), SmallIcon( "editdelete" ), Key_Delete, this, SLOT(slotRemoveItem()), this );
  actionNewDir = new KAction( i18n( "&New Directory" ), SmallIcon( "foler_new" ), CTRL+Key_N, this, SLOT(slotNewDir()), this );
  actionRemove->plug( m_popupMenu );
  actionRename->plug( m_popupMenu);
  actionNewDir->plug( m_popupMenu);
}

void K3bDataView::showPopupMenu( QListViewItem* _item, const QPoint& _point )
{
  if( _item->listView() == m_dataDirTree )
    actionRemove->setEnabled(false);   // needed since removing directories in treeView causes K3b to crash! FIXME!!!
  else
    actionRemove->setEnabled(true);

  if( _item )
    m_popupMenu->popup( _point );
}


void K3bDataView::slotNewDir()
{
  bool ok;
  QString name = KLineEditDlg::getText( i18n("Please insert the name for the new directory"),
					"New directory", &ok, k3bMain() );
  if( !ok )
    return;

  K3bDirItem* parent;
  if( m_dataDirTree->hasFocus() ) 
    {
      parent = ( (K3bDataDirViewItem*)m_dataDirTree->currentItem() )->dirItem();
    }
  else
    {
      parent = m_dataFileView->currentDir();
    }

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


void K3bDataView::slotItemRemoved( K3bDataItem* item )
{
  // we only need to search in the fileView if it currently displays the corresponding directory
  if( item == m_dataFileView->currentDir() ) {
    qDebug( "(K3bDataView) fileView currently displays a deleted directory. Setting to parent.");
    m_dataFileView->slotSetCurrentDir( item->parent() );
  }
  else if( item->parent() == m_dataFileView->currentDir() ) {
    qDebug("(K3bDataView) seaching in fileView for viewItems to delete");
    QListViewItemIterator _it2(m_dataFileView);
    for( ; _it2.current(); ++_it2 )
      {
	if( K3bDataDirViewItem* _dirViewItem = dynamic_cast<K3bDataDirViewItem*>(_it2.current()) ) {
	  qDebug("   found dirViewItem ... comparing ... ");
	  if( _dirViewItem->dirItem() == item ) {
	    delete _it2.current();
	    qDebug( "(K3bDataView) found listViewItem to remove in fileView: %s", item->k3bName().latin1() );
	    break;
	  }
	}
	else if( K3bDataFileViewItem* _fileViewItem = dynamic_cast<K3bDataFileViewItem*>(_it2.current()) ) {
	  qDebug("   found fileViewItem ... comparing ... ");
	  if( _fileViewItem->fileItem() == item ) {
	    delete _it2.current();
	    qDebug( "(K3bDataView) found listViewItem to remove in fileView: %s", item->k3bName().latin1() );
	    break;
	  }
	}
			
      } // for _it2
  }

  m_fillStatusDisplay->repaint();	
}



void K3bDataView::slotRemoveItem()
{
  if( m_dataDirTree->hasFocus() && m_dataDirTree->currentItem() ) {
    m_doc->removeItem( ((K3bDataDirViewItem*)m_dataDirTree->currentItem())->dirItem() );
  }
  else if( m_dataFileView->hasFocus() && m_dataFileView->currentItem() ) {
    if( K3bDataDirViewItem* d = dynamic_cast<K3bDataDirViewItem*>( m_dataFileView->currentItem() ) )
      m_doc->removeItem( d->dirItem() );
    else if ( K3bDataFileViewItem* f = dynamic_cast<K3bDataFileViewItem*>( m_dataFileView->currentItem() ) )
      m_doc->removeItem( f->fileItem() );
  }
  else
    qDebug("(K3bDataView) slotRemoveItem() without selected item!");
}
