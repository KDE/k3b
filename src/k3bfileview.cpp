/***************************************************************************
                          k3bfileview.cpp  -  description
                             -------------------
    begin                : Sun Oct 28 2001
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

#include "k3bfileview.h"

#include <qwidget.h>
#include <qdragobject.h>
#include <qlayout.h>
#include <qdir.h>
#include <qvbox.h>

#include <kfiledetailview.h>
#include <klistview.h>
#include <kaction.h>
#include <kdiroperator.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kurl.h>

#define UP_BUTTON_INDEX              0
#define HOME_BUTTON_INDEX        1
#define RELOAD_BUTTON_INDEX    2

// K3bFileView::PrivateFileview
/////////////////////////////////////////////////////////////////////

K3bFileView::PrivateFileView::PrivateFileView( QWidget* parent, const char* name )
  : KFileDetailView( parent, name )
{
  setDragEnabled( true );
  KFileDetailView::setSelectionMode( KFile::Extended );
  KListView::setSelectionModeExt( KListView::Extended );
}

	
QDragObject* K3bFileView::PrivateFileView::dragObject() const
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


K3bFileView::K3bFileView(QWidget *parent, const char *name ) : QVBox(parent,name) {

  KToolBar *toolBar = new KToolBar( this, "fileviewtoolbar" );

  m_fileView = new KDirOperator( QDir::home().absPath(), this );
  m_fileView->setView( new PrivateFileView( m_fileView, "fileview" ) );	

  // add some actions to the toolbar
  KAction* _actionUp = m_fileView->actionCollection()->action("up");
  KAction* _actionHome = m_fileView->actionCollection()->action("home");
  KAction* _actionReload = m_fileView->actionCollection()->action("reload");

  toolBar->insertButton(_actionUp->icon(), UP_BUTTON_INDEX);
  toolBar->insertButton(_actionHome->icon(), HOME_BUTTON_INDEX);
  toolBar->insertButton(_actionReload->icon(), RELOAD_BUTTON_INDEX);

  KToolBarButton *_buttonUp = toolBar->getButton(UP_BUTTON_INDEX);
  KToolBarButton *_buttonHome = toolBar->getButton(HOME_BUTTON_INDEX);
  KToolBarButton *_buttonReload = toolBar->getButton(RELOAD_BUTTON_INDEX);
	
  // the buttons should be square
  _buttonUp->setMaximumWidth( _buttonUp->height() );
  _buttonHome->setMaximumWidth( _buttonUp->height() );
  _buttonReload->setMaximumWidth( _buttonUp->height() );

  // connect to the actions
  connect( _buttonUp, SIGNAL(clicked()), _actionUp, SLOT(activate()) );
  connect( _buttonHome, SIGNAL(clicked()), _actionHome, SLOT(activate()) );
  connect( _buttonReload, SIGNAL(clicked()), _actionReload, SLOT(activate()) );

}
K3bFileView::~K3bFileView(){
}

void K3bFileView::setUrl(const KURL& url, bool forward){
	m_fileView->setURL( url, forward );
}

