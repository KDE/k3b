/***************************************************************************
                          k3bdatadoc.cpp  -  description
                             -------------------
    begin                : Sun Apr 22 2001
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

#include "k3bdatadoc.h"
#include "k3bfileitem.h"
#include "k3bdiritem.h"
#include "k3bdataview.h"

#include <qdir.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qtextstream.h>

#include <kstddirs.h>


K3bDataDoc::K3bDataDoc( QObject* parent )
	: K3bDoc( parent )
{
	m_docType = DATA;
	m_root = 0;
		
//	connect( this, SIGNAL(signalAddDirectory(const QString&, K3bDirItem*)),
//					this, SLOT(slotAddDirectory( const QString&, K3bDirItem*)) );
}

K3bDataDoc::~K3bDataDoc()
{
	delete m_root;	
}

bool K3bDataDoc::newDocument()
{
	if( m_root )
		delete m_root;
		
	m_root = new K3bRootItem( this );
	
	m_name = "Dummyname";
	
	return K3bDoc::newDocument();
}


K3bView* K3bDataDoc::newView( QWidget* parent )
{
	return new K3bDataView( this, parent );
}


void K3bDataDoc::addView(K3bView* view)
{
	K3bDataView* v = (K3bDataView*)view;
	connect( v, SIGNAL(dropped(const QStringList&, K3bDirItem*)), this, SLOT(slotAddURLs(const QStringList&, K3bDirItem*)) );
	connect( this, SIGNAL(newDir(K3bDirItem*)), v, SLOT(slotAddDir(K3bDirItem*)) );
	connect( this, SIGNAL(newFile(K3bFileItem*)), v, SLOT(slotAddFile(K3bFileItem*)) );
	connect( this, SIGNAL(itemRemoved(K3bDataItem*)), v, SLOT(slotItemRemoved(K3bDataItem*)) );
	
	K3bDoc::addView( view );
}

void K3bDataDoc::slotAddURLs( const QStringList& urls, K3bDirItem* dirItem )
{
	if( !dirItem )
		dirItem = m_root;

	qDebug( "(K3bDataDoc) adding urls to %s", dirItem->k3bName().latin1() );
				
	for( QStringList::ConstIterator _it = urls.begin(); _it != urls.end(); ++_it ) {
		// test if url directory or file
		if( (*_it).right(1) == "/" ) {
			qDebug("       -dir-");
			KURL k( *_it );
			slotAddDirectory( k.path(), dirItem );
		}
		else {
			qDebug("       -file-");
			// TODO: test if already in compilation (shall we allow equal files with different names?)
			emit newFile( new K3bFileItem( *_it, this, dirItem ) );
		}
	}
}


void K3bDataDoc::slotAddDirectory( const QString& url, K3bDirItem* parent )
{
	qDebug( "(K3bDataDoc) slotAddDirectory: %s", url.latin1() );

	QFileInfo _info( url );
	if( !_info.isDir() ) {
		qDebug( "(K3bDataDoc) tried to add url %s as directory which is no directory!", url.latin1() );
		return;
	}
		
	// TODO: test if dir already exists!!
	
	K3bDirItem* _newDirItem = new K3bDirItem( QDir(url).dirName(), this, parent );
	emit newDir( _newDirItem );
	
	QDir _d( url );
	QStringList _dlist = _d.entryList();
	_dlist.remove(".");
	_dlist.remove("..");
	
	for( QStringList::Iterator _it = _dlist.begin(); _it != _dlist.end(); ++_it ) {
		if( QFileInfo( _d.absPath() + "/" + *_it ).isDir() )
			slotAddDirectory(  _d.absPath() + "/" + *_it, _newDirItem );
		else
			emit newFile( new K3bFileItem( _d.absPath() + "/" + *_it, this, _newDirItem ) );
	}
}


int K3bDataDoc::size()
{
	return 650;
}


bool K3bDataDoc::loadDocumentData( QFile& )
{
	// TODO: so what? load the shit! ;-)
	return true;
}


bool K3bDataDoc::saveDocumentData( QFile& )
{
	// TODO: some saving work...
	return true;
}


void K3bDataDoc::removeItem( K3bDataItem* item )
{
	qDebug( "(K3bDataDoc) remove item " + item->k3bName() );
	
	if( item == root() )
		qDebug( "(K3bDataDoc) tried to remove root-entry!");
	else {
		emit itemRemoved( item );
	
		// the item takes care about it's parent!
		qDebug( "(K3bDataDoc) now it should be save to delete the item!");
		delete item;
		qDebug( "(K3bDataDoc) now the item has been deleted!");
	}
}


QString K3bDataDoc::writePathSpec( const QString& filename )
{
	QFile file( filename );
	if( !file.open( IO_WriteOnly ) ) {
		qDebug( "(K3bDataDoc) Could not open path-spec-file %s", filename.latin1() );
		return QString::null;
	}
	
	QTextStream t(&file);

	// start writing the path-specs
	// iterate over all the dataItems
	K3bDataItem* item = root()->nextSibling();
	
	while( item ) {
		t << item->k3bPath() << "=" << item->localPath() << "\n";
		
		item = item->nextSibling();
	}
	
	file.close();
	return filename;
}


QString K3bDataDoc::dummyDir()
{
	QDir _appDir( locateLocal( "appdata", "temp/" ) );
	if( !_appDir.cd( "dummydir" ) ) {
		_appDir.mkdir( "dummydir" );
	}
	m_dummyDir = _appDir.absPath();
	
	// TODO: test if dummy dir is empty
	
	return m_dummyDir + "/";
}
