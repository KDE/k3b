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
#include <qsplitter.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qiconset.h>

// KDE-includes
#include <kmimetype.h>
#include <kcursor.h>
#include <kfiledetailview.h>
#include <kfileviewitem.h>
#include <ktoolbar.h>
#include <kiconloader.h>


#include "kiotree/kiotree.h"



// K3bDirView::PrivateFileview
/////////////////////////////////////////////////////////////////////

K3bDirView::PrivateFileView::PrivateFileView( QWidget* parent, const char* name )
  : KFileDetailView( parent, name )
{
  setDragEnabled( true );
  KFileDetailView::setSelectionMode( KFile::Extended );
  KListView::setSelectionModeExt( KListView::Extended );
}

	
QDragObject* K3bDirView::PrivateFileView::dragObject() const
{
  if( !currentItem() )
    return 0;
	
  const KFileViewItemList* list = KFileView::selectedItems();
  QListIterator<KFileViewItem> it(*list);
  QStrList dragstr;
	
  for( ; it.current(); ++it )
    dragstr.append( it.current()->url().path(-1) );
		
  return new QUriDrag( dragstr, viewport() );
}


// K3bDirView
////////////////////////////////////////////////////////////////////

K3bDirView::K3bDirView(QWidget *parent, const char *name )
  : QVBox(parent, name)
{
  m_mainSplitter = new QSplitter( this );
  QVBox* box = new QVBox( m_mainSplitter );
  QWidget* box2 = new QWidget( m_mainSplitter );
  QGridLayout* _box2Layout = new QGridLayout( box2 );
  _box2Layout->setSpacing( 0 );
	
  m_kiotree = new KioTree( box );
  m_kiotree->addTopLevelDir( KURL( QDir::homeDirPath() ), "Home" );
  m_kiotree->addTopLevelDir( KURL( "/" ), "Root" );

  m_fileView = new KDirOperator( QDir::home().absPath(), box2 );
  m_fileView->setView( new PrivateFileView( m_fileView, "fileview" ) );	
	
  // add some actions to the toolbar
  KAction* _actionUp = m_fileView->actionCollection()->action("up");
  KAction* _actionHome = m_fileView->actionCollection()->action("home");
  KAction* _actionReload = m_fileView->actionCollection()->action("reload");
	
  QPushButton* _buttonUp = new QPushButton( _actionUp->iconSet(), "", box2 );
  QPushButton* _buttonHome = new QPushButton( _actionHome->iconSet(), "", box2 );
  QPushButton* _buttonReload = new QPushButton( _actionReload->iconSet(), "", box2 );
	
  // add the buttons and the fileview in the layout
  _box2Layout->addWidget( _buttonUp, 0, 0 );
  _box2Layout->addWidget( _buttonHome, 0, 1 );
  _box2Layout->addWidget( _buttonReload, 0, 2 );
  _box2Layout->addMultiCellWidget( m_fileView, 2, 2, 0, 3 );
  _box2Layout->setRowStretch( 2, 1 );
	
  // the buttons should be square
  _buttonUp->setMaximumWidth( _buttonUp->height() );
  _buttonHome->setMaximumWidth( _buttonUp->height() );
  _buttonReload->setMaximumWidth( _buttonUp->height() );
	
  // connect to the actions
  connect( _buttonUp, SIGNAL(clicked()), _actionUp, SLOT(activate()) );
  connect( _buttonHome, SIGNAL(clicked()), _actionHome, SLOT(activate()) );
  connect( _buttonReload, SIGNAL(clicked()), _actionReload, SLOT(activate()) );

  connect( m_kiotree, SIGNAL(urlActivated(const KURL&)), this, SLOT(slotDirActivated(const KURL&)) );	
  connect( m_fileView, SIGNAL(urlEntered(const KURL&)), m_kiotree, SLOT(followURL(const KURL&)) );
}


K3bDirView::~K3bDirView(){
}


void K3bDirView::slotViewChanged( KFileView* newView )
{
  newView->setSelectionMode( KFile::Extended );
  if( KListView* _x = dynamic_cast<KListView*>( newView->widget() ) )
    _x->setDragEnabled( true );
}


void K3bDirView::slotDirActivated( const KURL& url )
{
  m_fileView->setURL( url, true );
}
