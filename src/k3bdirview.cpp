/***************************************************************************
                          k3bdirview.cpp  -  description
                             -------------------
    begin                : Mon Mar 26 2001
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

#include "k3bdirview.h"

// QT-includes
#include <qvbox.h>
#include <qdir.h>
#include <qlistview.h>
#include <qstring.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qsplitter.h>
#include <qdragobject.h>
#include <qstrlist.h>
#include <qheader.h>

// KDE-includes
#include <kmimetype.h>
#include <kcursor.h>
#include <kfiledetailview.h>
#include <kfileviewitem.h>
#include <ktoolbar.h>
#include <kiconloader.h>


// PrivateDirItem
////////////////////////////////////////////////////////////////////////////////////

K3bDirView::PrivateDirItem::PrivateDirItem( PrivateDirItem* parent, const QString& filename, const QString& altName )
 : QListViewItem( parent ), m_altName(altName), f(filename), pix( 0 )
{
    p = parent;
    readable = QDir( absPath() ).isReadable();
	
	setPixmap( new QPixmap(KMimeType::pixmapForURL( KURL( absPath() ), 0, KIcon::Small ) ) );
}


K3bDirView::PrivateDirItem::PrivateDirItem( QListView * parent, const QString& filename, const QString& altName )
 : QListViewItem( parent ), m_altName(altName), f(filename), pix( 0 )
{
    p = 0;
    readable = QDir( absPath() ).isReadable();
    setPixmap( new QPixmap(KMimeType::pixmapForURL( KURL( absPath() ), 0, KIcon::Small ) ) );
}


void K3bDirView::PrivateDirItem::setPixmap( QPixmap *px )
{
    pix = px;
    setup();
    widthChanged( 0 );
    invalidateHeight();
    repaint();
}


const QPixmap* K3bDirView::PrivateDirItem::pixmap( int i ) const
{
	  if ( i )
			return 0;
    return pix;
}

void K3bDirView::PrivateDirItem::setOpen( bool o )
{
  if ( o && !childCount() ) {
		QString s( absPath() );
		QDir thisDir( s, QString::null, QDir::Name, QDir::Dirs );
		if ( !thisDir.isReadable() ) {
	    readable = FALSE;
	    setExpandable( FALSE );
	    return;
		}

		listView()->setUpdatesEnabled( FALSE );
		const QFileInfoList* files = thisDir.entryInfoList();
		if ( files ) {
	    QFileInfoListIterator it( *files );
	    QFileInfo * fi;
	    while( (fi=it.current()) != 0 ) {
				++it;
				if ( fi->fileName() == "." || fi->fileName() == ".." )
		    	; // nothing
				else if ( fi->isDir() ) {
		    	PrivateDirItem* _item = new PrivateDirItem( this, fi->fileName() );
		    	
		    	// check if the directory has subs
		    	QDir _newDir( _item->absPath(), QString::null, QDir::Name, QDir::Dirs );
		    	if( _newDir.count() > 2 )
		    		_item->setExpandable( true );
		    	else
		    		_item->setExpandable( false );
		    }
	    }
		}
		listView()->setUpdatesEnabled( TRUE );
  }
  QListViewItem::setOpen( o );
}


void K3bDirView::PrivateDirItem::expandDirItem()
{
	// remove all subs and reread them
	while( QListViewItem* _old = firstChild() )
		delete _old;
		
	QDir _dir( absPath(), QString::null, QDir::Name, QDir::Dirs );
	if( !_dir.isReadable() )
		return;
		
	QStringList _subs = _dir.entryList();
	_subs.remove(".");
	_subs.remove("..");
	
	 // add PrivateDirItems for all subsubdirectories
   for( QStringList::Iterator it = _subs.begin(); it != _subs.end(); it++ ) {
	   (void)new PrivateDirItem( this, *it );
   }
}


void K3bDirView::PrivateDirItem::setup()
{
    setExpandable( TRUE );
    QListViewItem::setup();
}


QString K3bDirView::PrivateDirItem::absPath()
{
  QString s;
  if ( p ) {
		s = p->absPath();
		s.append( f.name() );
		s.append( "/" );
    }
  else {
		s = f.name();
    }
  return s;
}


QString K3bDirView::PrivateDirItem::text( int i ) const
{
	if( i == 0 ) {
	  if ( m_altName.isEmpty() )
			return f.name();
		else
			return m_altName;
	}
	else
		return "directory";
}



// PrivateDirView
////////////////////////////////////////////////////////////////

K3bDirView::PrivateDirView::PrivateDirView( QWidget *parent, const char *name )
 : KListView( parent, name )
{
	setCursor( KCursor::handCursor() );

	addColumn("directory");
	setColumnWidthMode( 0, QListView::Maximum );
	setRootIsDecorated( true );
	header()->hide();
	
	// add "/" and home-dir as roots
	K3bDirView::PrivateDirItem* home = new K3bDirView::PrivateDirItem( this, QDir::home().absPath()+"/", "Home Directory" );
	(void)new K3bDirView::PrivateDirItem( this, "/", "Root Directory" );
	home->setOpen( true );
	
	connect( this, SIGNAL( executed(QListViewItem*) ), this, SLOT( slotFolderSelected(QListViewItem*) ) );
}





void K3bDirView::PrivateDirView::slotFolderSelected( QListViewItem *i )
{
  if ( !i )
		return;

  K3bDirView::PrivateDirItem* dir = (K3bDirView::PrivateDirItem*)i;
  emit folderSelected( dir->absPath() );
}


void K3bDirView::PrivateDirView::setDir( const QString &s )
{
	qDebug("setdir");
  QListViewItemIterator it( this );
  ++it;
  for ( ; it.current(); ++it ) {
		it.current()->setOpen( FALSE );
  }

  QStringList lst( QStringList::split( "/", s ) );
  QListViewItem *item = firstChild();
  QStringList::Iterator it2 = lst.begin();
  for ( ; it2 != lst.end(); ++it2 ) {
		while ( item ) {
	    if ( item->text(0) == *it2 ) {
				item->setOpen( TRUE );
				break;
	    }
	  item = item->itemBelow();
	  }
  }

  if ( item )
		setCurrentItem( item );
}



// K3bDirView::PrivateFileview
/////////////////////////////////////////////////////////////////////

K3bDirView::PrivateFileView::PrivateFileView( QWidget* parent, const char* name )
 : KFileDetailView( parent, name )
{
	setDragEnabled( true );
	setSelectionMode( KFile::Extended );
	setSelectionModeExt( Extended );
}

	
QDragObject* K3bDirView::PrivateFileView::dragObject() const
{
	if( !currentItem() )
		return 0;
	
	const KFileViewItemList* list = KFileView::selectedItems();
	QListIterator<KFileViewItem> it(*list);
	QStrList dragstr;
	
	for( ; it.current(); ++it )
		dragstr.append( it.current()->urlString() );
		
	return new QUriDrag( dragstr, viewport() );
}


// K3bDirView
////////////////////////////////////////////////////////////////////

K3bDirView::K3bDirView(QWidget *parent, const char *name )
 : QSplitter(parent, name)
{
	QVBox* box = new QVBox( this );
	QVBox* box2 = new QVBox( this );
	m_dirView = new PrivateDirView( box );
	
	KToolBar* toolbar = new KToolBar( box2, "filetoolbar", true, false );
	
	m_fileView = new KDirOperator( QDir::home().absPath(), box2 );
	m_fileView->setView( new PrivateFileView( m_fileView, "fileview" ) );	
	
	// add some actions to the toolbar
	m_fileView->actionCollection()->action("up")->plug( toolbar );
	m_fileView->actionCollection()->action("home")->plug( toolbar );
	m_fileView->actionCollection()->action("reload")->plug( toolbar );
	
	//((KListView*)(m_fileView->viewWidget()))->setDragEnabled( true );
	
  connect( m_dirView, SIGNAL(folderSelected(const QString&) ), this, SLOT(slotFolderSelected(const QString&) ) );
  connect( m_fileView, SIGNAL( dirActivated(const KFileViewItem*) ), this, SLOT( slotFileItemSelected(const KFileViewItem*) ) );
}


K3bDirView::~K3bDirView(){
}


void K3bDirView::slotFolderSelected( const QString& url )
{
	m_fileView->setURL( url, true );
}


void K3bDirView::slotFileItemSelected( const KFileViewItem* _item )
{
	qDebug("slotFileItemSelected()");
	if( _item && !_item->isFile() )
		m_dirView->setDir( _item->urlString() );
}
