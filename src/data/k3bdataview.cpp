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

#include <qpixmap.h>
#include <qsplitter.h>
#include <qlayout.h>
#include <qdragobject.h>
#include <qheader.h>
#include <qlistview.h>

#include <assert.h>


K3bDataView::K3bDataView(K3bDataDoc* doc, QWidget *parent, const char *name )
	: K3bView(doc, parent,name)
{
	m_doc = doc;
	m_burnDialog = 0;
	
	// --- setup GUI ---------------------------------------------------
	QSplitter* _main = new QSplitter( this );	
	m_dataDirTree = new K3bPrivateDataDirTree( doc, _main );
	m_dataFileView = new K3bPrivateDataFileView( doc, _main );
	m_fillStatusDisplay = new K3bFillStatusDisplay( doc, this );
	
	QVBoxLayout* _box = new QVBoxLayout( this );
	_box->addWidget( _main );
	_box->addWidget( m_fillStatusDisplay );

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
		m_dataFileView->reload();
	
	m_fillStatusDisplay->repaint();
}


void K3bDataView::slotAddDir( K3bDirItem* dirItem )
{
	K3bDirItem* parentDir = dirItem->parent();
	
	if( !parentDir ) {
		(void)new K3bPrivateDataDirViewItem( dirItem, m_dataDirTree->root() );
		parentDir = m_doc->root();
	}
	else {
		QListViewItemIterator _it(m_dataDirTree);
		for( ; _it.current(); ++_it )
		{
			K3bPrivateDataDirViewItem* _dirViewItem = (K3bPrivateDataDirViewItem*)_it.current();
			if( _dirViewItem->dirItem() == dirItem->parent() ) {
				(void)new K3bPrivateDataDirViewItem( dirItem, _it.current() );
				break;
			}
		}
			
		// add to root by default
		if( !_it.current() ) {
			qDebug("(K3bDataView) could not find corresponding listViewItem to %s", dirItem->parent()->k3bName().latin1() );
			(void)new K3bPrivateDataDirViewItem( dirItem, m_dataDirTree->root() );
			parentDir = m_doc->root();
		}
	}

	if( m_dataFileView->currentDir() == parentDir )
		m_dataFileView->reload();
		
	m_fillStatusDisplay->repaint();
}
	
	
void K3bDataView::slotDropped( KListView* listView, QDropEvent* e, QListViewItem* after )
{
	if( !e->isAccepted() )
		return;

	QString droppedText;
	QTextDrag::decode( e, droppedText );
	QStringList _urls = QStringList::split("\r\n", droppedText );
	
	// get directory from "after"
	if( dynamic_cast<K3bPrivateDataDirTree*>( listView ) ) {
		emit dropped( _urls, ((K3bPrivateDataDirViewItem*)after)->dirItem() );
	}
	else if( K3bPrivateDataFileView* f = dynamic_cast<K3bPrivateDataFileView*>( listView ) ) {
		emit dropped( _urls, f->currentDir() );
	}
}


K3bDataView::K3bPrivateDataDirTree::K3bPrivateDataDirTree( K3bDataDoc* doc, QWidget* parent )
	: KListView( parent )
{
 	setAcceptDrops( true );
	setDropVisualizer( true );
	setRootIsDecorated( true );
	
	addColumn( "Dir" );
	header()->hide();
	
	setItemsRenameable( true );
	
	m_root = new K3bDataView::K3bPrivateDataRootViewItem( doc, this );
	
	// TODO: read further items... (nessessary when loading a document...)
	
	connect( this, SIGNAL(clicked(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
	connect( this, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
}


void K3bDataView::K3bPrivateDataDirTree::slotExecuted( QListViewItem* item )
{
	if( item )
		emit dirSelected( ((K3bDataView::K3bPrivateDataDirViewItem*)item)->dirItem() );
}


bool K3bDataView::K3bPrivateDataDirTree::acceptDrag(QDropEvent* e) const{
	return ( e->source() == viewport() || QTextDrag::canDecode(e) );
}


K3bDataView::K3bPrivateDataFileView::K3bPrivateDataFileView( K3bDataDoc* doc, QWidget* parent )
	: KListView( parent )
{
 	setAcceptDrops( true );
	setDropVisualizer( false );
	
	addColumn( i18n("Name") );
	addColumn( i18n("Type") );
	addColumn( i18n("Size") );

	setItemsRenameable( true );
		
	m_currentDir = doc->root();
	reload();
}


void K3bDataView::K3bPrivateDataFileView::slotSetCurrentDir( K3bDirItem* dir )
{
	if( dir ) {
		qDebug( "(K3bPrivateDataFileView) setting current dir to " + dir->k3bName() );
		m_currentDir = dir;
		reload();
	}
}


void K3bDataView::K3bPrivateDataFileView::reload()
{
	// clear view
	clear();
	
	qDebug( "(K3bPrivateDataFileView) reloading current dir: " + m_currentDir->k3bName() );
	
	for( QListIterator<K3bDataItem> _it( *m_currentDir->children() ); _it.current(); ++_it ) {
		if( K3bDirItem* _item = dynamic_cast<K3bDirItem*>( _it.current() ) ) {
			(void)new K3bDataView::K3bPrivateDataDirViewItem( _item, this );
		}
		else if( K3bFileItem* _item = dynamic_cast<K3bFileItem*>( _it.current() ) ) {
			(void)new K3bDataView::K3bPrivateDataFileViewItem( _item, this );
		}
	}
	
	qDebug( "(K3bPrivateDataFileView) reloading finished" );
}


bool K3bDataView::K3bPrivateDataFileView::acceptDrag(QDropEvent* e) const{
	return ( e->source() == viewport() || QTextDrag::canDecode(e) );
}


K3bDataView::K3bPrivateDataDirViewItem::K3bPrivateDataDirViewItem( K3bDirItem* dir, QListView* parent )
	: QListViewItem( parent )
{
	assert( dir );

	m_dirItem = dir;
	setPixmap( 0, *(new QPixmap(KMimeType::pixmapForURL( KURL( "/" ), 0, KIcon::Small ) )) );
}


K3bDataView::K3bPrivateDataDirViewItem::K3bPrivateDataDirViewItem( K3bDirItem* dir, QListViewItem* parent )
	: QListViewItem( parent )
{
	assert( dir );

	m_dirItem = dir;
	setPixmap( 0, *(new QPixmap(KMimeType::pixmapForURL( KURL( "/" ), 0, KIcon::Small ) )) );
}

	
QString K3bDataView::K3bPrivateDataDirViewItem::text( int index ) const
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


void K3bDataView::K3bPrivateDataDirViewItem::setText(int col, const QString& text )
{
	if( col == 0 )
		dirItem()->setK3bName( text );

	QListViewItem::setText( col, text );
}

	
K3bDataView::K3bPrivateDataFileViewItem::K3bPrivateDataFileViewItem( K3bFileItem* file, QListView* parent )
	: QListViewItem( parent )
{
	assert( file );

	m_fileItem = file;
	setPixmap( 0, file->pixmap(16) );
}


K3bDataView::K3bPrivateDataFileViewItem::K3bPrivateDataFileViewItem( K3bFileItem* file, QListViewItem* parent )
	: QListViewItem( parent )
{
	assert( file );
	
	m_fileItem = file;
	setPixmap( 0, file->pixmap(16) );
}

	
QString K3bDataView::K3bPrivateDataFileViewItem::text( int index ) const
{
	switch( index ) {
		case 0:
			return m_fileItem->k3bName();   // TODO: add the ability to show ISO, Joilet, or RockRidge
		case 1:
			return m_fileItem->mimetype();
		case 2:
			return QString::number( m_fileItem->size() );
		default:
			return "";
	}
}


void K3bDataView::K3bPrivateDataFileViewItem::setText(int col, const QString& text )
{
	if( col == 0 )
		fileItem()->setK3bName( text );
		
	QListViewItem::setText( col, text );
}


K3bDataView::K3bPrivateDataRootViewItem::K3bPrivateDataRootViewItem( K3bDataDoc* doc, QListView* parent )
	: K3bPrivateDataDirViewItem( doc->root(), parent )
{
	m_doc = doc;
	setPixmap( 0, kapp->iconLoader()->loadIcon( "cdrom_unmount", KIcon::Small, 16 ) );
}


QString K3bDataView::K3bPrivateDataRootViewItem::text( int index ) const
{
	switch( index ) {
		case 0:
			return m_doc->name();
		case 1:
			return i18n("ISO-CD");
		default:
			return "";
	}
}


void K3bDataView::K3bPrivateDataRootViewItem::setText( int col, const QString& text )
{
	if( col == 0 )
		m_doc->setName( text );

	QListViewItem::setText( col, text );
}


void K3bDataView::setupPopupMenu()
{
	m_popupMenu = new KPopupMenu( this, "DataViewPopupMenu" );
//	m_popupMenu->insertTitle( i18n( "File Options" ) );
	actionRename = new KAction( i18n("&Rename"), SmallIcon( "rename" ), CTRL+Key_R, this, SLOT(slotRenameItem()), this );
	actionRemove = new KAction( i18n( "&Remove" ), SmallIcon( "editdelete" ), Key_Delete, this, SLOT(slotRemoveItem()), this );
	actionRemove->plug( m_popupMenu );
	actionRename->plug( m_popupMenu);
}

void K3bDataView::showPopupMenu( QListViewItem* _item, const QPoint& _point )
{
	// disable popupmenu on dirTree since the removing causes the app to crash !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if( m_dataFileView->hasFocus() )
		if( _item )
			m_popupMenu->popup( _point );
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
	qDebug("(K3bDataView) slotItemRemoved");

	// search for deleted item in tree view
	QListViewItemIterator _it(m_dataDirTree);
	qDebug("(K3bDataView) seaching in dirTree for viewItems to delete");
	for( ; _it.current(); ++_it )
	{
		K3bPrivateDataDirViewItem* _dirViewItem = (K3bPrivateDataDirViewItem*)_it.current();
		if( _dirViewItem->dirItem() == item ) {
			delete _it.current();
			qDebug( "(K3bDataView) found listViewItem to remove in dirTree: %s", item->k3bName().latin1() );
			break;
		}
	}
	
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
			if( K3bPrivateDataDirViewItem* _dirViewItem = dynamic_cast<K3bPrivateDataDirViewItem*>(_it2.current()) ) {
				qDebug("   found dirViewItem ... comparing ... ");
				if( _dirViewItem->dirItem() == item ) {
					delete _it2.current();
					qDebug( "(K3bDataView) found listViewItem to remove in fileView: %s", item->k3bName().latin1() );
					break;
				}
			}
			else if( K3bPrivateDataFileViewItem* _fileViewItem = dynamic_cast<K3bPrivateDataFileViewItem*>(_it2.current()) ) {
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
		m_doc->removeItem( ((K3bPrivateDataDirViewItem*)m_dataDirTree->currentItem())->dirItem() );
	}
	else if( m_dataFileView->hasFocus() && m_dataFileView->currentItem() ) {
		if( K3bPrivateDataDirViewItem* d = dynamic_cast<K3bPrivateDataDirViewItem*>( m_dataFileView->currentItem() ) )
			m_doc->removeItem( d->dirItem() );
		else if ( K3bPrivateDataFileViewItem* f = dynamic_cast<K3bPrivateDataFileViewItem*>( m_dataFileView->currentItem() ) )
			m_doc->removeItem( f->fileItem() );
	}
	else
		qDebug("(K3bDataView) slotRemoveItem() without selected item!");
}
