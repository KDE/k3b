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

#include "../klistviewlineedit.h"

#include <qvalidator.h>
#include <qdragobject.h>
#include <qregexp.h>

#include <klocale.h>


K3bDataFileView::K3bDataFileView( K3bDataView* view, K3bDataDoc* doc, QWidget* parent )
  : KListView( parent )
{
  m_view = view;

  setAcceptDrops( true );
  setDropVisualizer( false );
  setDropHighlighter( true );
  setDragEnabled( true );
  setItemsMovable( false );
  setAllColumnsShowFocus( true );
	
  addColumn( i18n("Name") );
  addColumn( i18n("Type") );
  addColumn( i18n("Size") );

  setItemsRenameable( true );
  setSelectionModeExt( KListView::Extended );

  m_editor = new KListViewLineEdit( this );
  m_editor->setValidator( new QRegExpValidator( QRegExp("([a-z]|[A-Z]|[0-9]|_)*"), m_editor ) );
  
  m_doc = doc;
  m_currentDir = doc->root();
  updateContents();

  connect( m_doc, SIGNAL(itemRemoved(K3bDataItem*)), this, SLOT(slotDataItemRemoved(K3bDataItem*)) );
  connect( this, SIGNAL(executed(QListViewItem*)), this, SLOT(slotExecuted(QListViewItem*)) );
  connect( m_editor, SIGNAL(done(QListViewItem*,int)), this, SLOT(doneEditing(QListViewItem*,int)) );
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
	
  //  qDebug( "(K3bDataFileView) reloading current dir: " + m_currentDir->k3bName() );
	
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
	
  //  qDebug( "(K3bDataFileView) reloading finished" );
}


bool K3bDataFileView::acceptDrag(QDropEvent* e) const{
  return ( e->source() == viewport() || QUriDrag::canDecode(e) || m_view->acceptDrag(e) );
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


#include "k3bdatafileview.moc"
