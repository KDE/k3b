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

#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>

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

	// --- setup GUI ---------------------------------------------------
	QSplitter* _main = new QSplitter( this );	
	m_dataDirTree = new K3bPrivateDataDirTree( doc, _main );
	m_dataFileView = new K3bPrivateDataFileView( doc, _main );
	m_fillStatusDisplay = new K3bFillStatusDisplay( doc, this );
	
	QVBoxLayout* _box = new QVBoxLayout( this );
	_box->addWidget( _main );
	_box->addWidget( m_fillStatusDisplay );
	
	
	// connect to the dropped signals
	connect( m_dataDirTree, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*)),
					this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*)) );
	connect( m_dataFileView, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*)),
					this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*)) );

	connect( m_dataDirTree, SIGNAL(dirSelected(K3bDirItem*)), m_dataFileView, SLOT(slotSetCurrentDir(K3bDirItem*)) );
}

K3bDataView::~K3bDataView(){
}


void K3bDataView::slotAddFile( K3bFileItem* file )
{
	if( m_dataFileView->currentDir() == file->parent() )
		m_dataFileView->reload();
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
			qDebug("(K3bDataView) could not find corresponding listViewItem to %s", dirItem->parent()->name().latin1() );
			(void)new K3bPrivateDataDirViewItem( dirItem, m_dataDirTree->root() );
			parentDir = m_doc->root();
		}
	}

	if( m_dataFileView->currentDir() == parentDir )
		m_dataFileView->reload();
}
	
	
void K3bDataView::slotDropped( KListView*, QDropEvent* e, QListViewItem* after )
{
	if( !e->isAccepted() )
		return;

	QString droppedText;
	QTextDrag::decode( e, droppedText );
	QStringList _urls = QStringList::split("\r\n", droppedText );
	
	// get directory from "after"
	if( K3bPrivateDataDirViewItem* _dirViewItem = dynamic_cast<K3bPrivateDataDirViewItem*>( after ) ) {
		emit dropped( _urls, _dirViewItem->dirItem() );
	}
	else if( K3bPrivateDataFileViewItem* _fileViewItem = dynamic_cast<K3bPrivateDataFileViewItem*>( after ) ) {
		emit dropped( _urls, _fileViewItem->fileItem()->parent() );
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
	
	m_root = new K3bDataView::K3bPrivateDataDirViewItem( doc->root(), this );
	
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
	setDropVisualizer( true );
	
	addColumn( i18n("Name") );
	addColumn( i18n("Type") );
	addColumn( i18n("Size") );
	
	m_currentDir = doc->root();
	reload();
}


void K3bDataView::K3bPrivateDataFileView::slotSetCurrentDir( K3bDirItem* dir )
{
	if( dir ) {
		m_currentDir = dir;
		reload();
	}
}


void K3bDataView::K3bPrivateDataFileView::reload()
{
	// clear view
	clear();
	
	for( QListIterator<K3bDataItem> _it( *m_currentDir->children() ); _it.current(); ++_it ) {
		if( K3bDirItem* _item = dynamic_cast<K3bDirItem*>( _it.current() ) )
			(void)new K3bDataView::K3bPrivateDataDirViewItem( _item, this );
		
		else if( K3bFileItem* _item = dynamic_cast<K3bFileItem*>( _it.current() ) )
			(void)new K3bDataView::K3bPrivateDataFileViewItem( _item, this );
	}
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
			return m_dirItem->name();
		case 1:
			return i18n("Directory");
		default:
			return "";
	}
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
			return m_fileItem->name();   // TODO: add the ability to show ISO, Joilet, or RockRidge
		case 1:
			return m_fileItem->mimetype();
		case 2:
			return QString::number( (double)(m_fileItem->size()) / 1024.0, 'g', 2 );
		default:
			return "";
	}
}
