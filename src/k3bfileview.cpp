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
#include "k3b.h"

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




K3bFileView::PrivateFileView::PrivateFileView( QWidget* parent, const char* name )
  : KFileDetailView( parent, name )
{
  setDragEnabled( true );
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




K3bFileView::K3bFileView(QWidget *parent, const char *name ) 
  : QVBox(parent,name) 
{
  setupGUI();
}


K3bFileView::~K3bFileView()
{
}


void K3bFileView::setupGUI()
{
  KToolBar* toolBar = new KToolBar( k3bMain(), this, "fileviewtoolbar" );

  m_dirOp = new KDirOperator( QDir::home().absPath(), this );
  PrivateFileView* fileView = new PrivateFileView( m_dirOp, "fileview" );
  m_dirOp->setView( fileView );
  fileView->setSelectionMode( KFile::Extended );

  // add some actions to the toolbar
  m_dirOp->actionCollection()->action("up")->plug( toolBar );
  m_dirOp->actionCollection()->action("home")->plug( toolBar );
  m_dirOp->actionCollection()->action("reload")->plug( toolBar );

  // this has to be disabled since the user must NEVER change the fileview because
  // that would disable the dragging support! (obsolete in KDE3)
  m_dirOp->actionCollection()->action("view menu")->setEnabled( false );
}


void K3bFileView::setUrl(const KURL& url, bool forward)
{
  m_dirOp->setURL( url, forward );
}


#include "k3bfileview.moc"
