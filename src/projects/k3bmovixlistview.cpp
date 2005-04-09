/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bmovixlistview.h"
#include "k3bmovixdoc.h"
#include "k3bmovixfileitem.h"

#include <klocale.h>
#include <kdebug.h>
#include <kio/global.h>
#include <kurldrag.h>

#include <qdragobject.h>
#include <qptrlist.h>
#include <qevent.h>


K3bMovixListViewItem::K3bMovixListViewItem( K3bMovixDoc* doc, 
					    K3bMovixFileItem* item, 
					    QListView* parent, 
					    QListViewItem* after )
  : K3bListViewItem( parent, after ),
    m_doc(doc),
    m_fileItem(item)
{
}


K3bMovixListViewItem::K3bMovixListViewItem( K3bMovixDoc* doc, 
					    K3bMovixFileItem* item, 
					    QListViewItem* parent )
  : K3bListViewItem( parent ),
    m_doc(doc),
    m_fileItem(item)
{
}


K3bMovixListViewItem::~K3bMovixListViewItem()
{
}


K3bMovixFileViewItem::K3bMovixFileViewItem( K3bMovixDoc* doc, 
					    K3bMovixFileItem* item, 
					    QListView* parent, 
					    QListViewItem* after )
  : K3bMovixListViewItem( doc, item, parent, after ),
    KFileItem( 0, 0, KURL::fromPathOrURL(item->localPath()) )
{
  setPixmap( 1, KFileItem::pixmap(16) );
  setEditor( 1, LINE );
}


QString K3bMovixFileViewItem::text( int col ) const
{
  //
  // We add two spaces after all strings (except the once renamable)
  // to increase readability
  //

  switch( col ) {
  case 0:
    // allowing 999 files to be added. 
    return QString::number( doc()->indexOf( fileItem() ) ).rightJustify( 3, ' ' );
  case 1:
    return fileItem()->k3bName();
  case 2:
    {
      if( fileItem()->isSymLink() )
	return i18n("Link to %1").arg(const_cast<K3bMovixFileViewItem*>(this)->mimeComment()) + "  ";
      else
	return const_cast<K3bMovixFileViewItem*>(this)->mimeComment() + "  ";
    }
  case 3:
    return KIO::convertSize( fileItem()->size() ) + "  ";
  case 4:
    return fileItem()->localPath() + "  ";
  case 5:
    return ( fileItem()->isValid() ? fileItem()->linkDest() : fileItem()->linkDest() + i18n(" (broken)") );
  default:
    return "";
  }
}


void K3bMovixFileViewItem::setText( int col, const QString& text )
{
  if( col == 1 )
    fileItem()->setK3bName( text );

  K3bMovixListViewItem::setText( col, text );
}


QString K3bMovixFileViewItem::key( int, bool ) const
{
  return QString::number( doc()->indexOf( fileItem() ) ).rightJustify( 10, '0' );
}




K3bMovixSubTitleViewItem::K3bMovixSubTitleViewItem( K3bMovixDoc* doc, 
						    K3bMovixFileItem* item, 
						    K3bMovixListViewItem* parent )
  : K3bMovixListViewItem( doc, item, parent ),
    KFileItem( 0, 0, KURL::fromPathOrURL(item->subTitleItem()->localPath()) )
{
}


K3bMovixSubTitleViewItem::~K3bMovixSubTitleViewItem()
{
}
  
  
QString K3bMovixSubTitleViewItem::text( int c ) const
{
  switch( c ) {
  case 1:
    return fileItem()->subTitleItem()->k3bName();
  case 2:
    {
      if( fileItem()->subTitleItem()->isSymLink() )
	return i18n("Link to %1").arg(const_cast<K3bMovixSubTitleViewItem*>(this)->mimeComment());
      else
	return const_cast<K3bMovixSubTitleViewItem*>(this)->mimeComment();
    }
  case 3:
    return KIO::convertSize( fileItem()->subTitleItem()->size() );
  case 4:
    return fileItem()->subTitleItem()->localPath();
  case 5:
    return ( fileItem()->subTitleItem()->isValid() ? 
	     fileItem()->subTitleItem()->linkDest() : 
	     fileItem()->subTitleItem()->linkDest() + i18n(" (broken)") );
  default:
    return "";
  }
}











K3bMovixListView::K3bMovixListView( K3bMovixDoc* doc, QWidget* parent, const char* name )
  : K3bListView( parent, name ),
    m_doc(doc)
{
  addColumn( i18n("No.") );
  addColumn( i18n("Name") );
  addColumn( i18n("Type") );
  addColumn( i18n("Size") );
  addColumn( i18n("Local Path") );
  addColumn( i18n("Link") );

  setAcceptDrops( true );
  setDropVisualizer( true );
  setAllColumnsShowFocus( true );
  setDragEnabled( true );
  setItemsMovable( false );
  setSelectionModeExt( KListView::Extended );
  setSorting(0);

  setNoItemText( i18n("Use drag'n'drop to add files to the project.") +"\n"
		 + i18n("To remove or rename files use the context menu.") + "\n"
		 + i18n("After that press the burn button to write the CD.") );

  connect( m_doc, SIGNAL(newMovixFileItems()), this, SLOT(slotNewFileItems()) );
  connect( m_doc, SIGNAL(movixItemRemoved(K3bMovixFileItem*)), this, SLOT(slotFileItemRemoved(K3bMovixFileItem*)) );
  connect( m_doc, SIGNAL(subTitleItemRemoved(K3bMovixFileItem*)), this, SLOT(slotSubTitleItemRemoved(K3bMovixFileItem*)) );
  connect( this, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*)),
	   this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*)) );

  // let's see what the doc already has
  slotNewFileItems();
}


K3bMovixListView::~K3bMovixListView()
{
}


bool K3bMovixListView::acceptDrag(QDropEvent* e) const
{
  // the first is for built-in item moving, the second for dropping urls
  return ( K3bListView::acceptDrag(e) || KURLDrag::canDecode(e) );
}


void K3bMovixListView::slotNewFileItems()
{
  K3bMovixFileItem* lastItem = 0;
  for( QPtrListIterator<K3bMovixFileItem> it( m_doc->movixFileItems() ); it.current(); ++it ) {
    K3bMovixFileItem* item = it.current();
    if( !m_itemMap.contains( item ) )
      m_itemMap.insert( item, new K3bMovixFileViewItem( m_doc, item, this, lastItem ? m_itemMap[lastItem] : 0L ) );

    if( item->subTitleItem() ) {
      K3bMovixFileViewItem* vi = m_itemMap[item];
      if( vi->childCount() <= 0 ) {
	(void)new K3bMovixSubTitleViewItem( m_doc, item, vi );
	vi->setOpen(true);
      }
    }

    lastItem = item;
  }

  // arghhh
  sort();
}


void K3bMovixListView::slotFileItemRemoved( K3bMovixFileItem* item )
{
  if( m_itemMap.contains( item ) ) {
    K3bMovixFileViewItem* vi = m_itemMap[item];
    m_itemMap.erase(item);
    delete vi;
  }
}


void K3bMovixListView::slotSubTitleItemRemoved( K3bMovixFileItem* item )
{
  if( m_itemMap.contains( item ) ) {
    K3bMovixFileViewItem* vi = m_itemMap[item];
    if( vi->childCount() >= 1 )
      delete vi->firstChild();
  }
}


void K3bMovixListView::slotDropped( KListView*, QDropEvent* e, QListViewItem* after )
{
  if( !e->isAccepted() )
    return;

  int pos;
  if( after == 0L )
    pos = 0;
  else
    pos = m_doc->indexOf( ((K3bMovixListViewItem*)after)->fileItem() );

  if( e->source() == viewport() ) {
    QPtrList<QListViewItem> sel = selectedItems();
    QPtrListIterator<QListViewItem> it(sel);
    K3bMovixFileItem* itemAfter = ( after ? ((K3bMovixListViewItem*)after)->fileItem() : 0 );
    while( it.current() ) {
      K3bMovixListViewItem* vi = (K3bMovixListViewItem*)it.current();
      if( vi->isMovixFileItem() ) {
	K3bMovixFileItem* item = vi->fileItem();
	m_doc->moveMovixItem( item, itemAfter );
	itemAfter = item;
      }
      else
	kdDebug() << "(K3bMovixListView) I don't move subtitle items!" << endl;

      ++it;
    }

    sort();  // This is so lame!
  }
  else {
    KURL::List urls;
    KURLDrag::decode( e, urls );

    for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); ++it ) {
      m_doc->addMovixFile( *it, pos++ );
    }
  }
}


QDragObject* K3bMovixListView::dragObject()
{
  QPtrList<QListViewItem> list = selectedItems();

  if( list.isEmpty() )
    return 0;

  QPtrListIterator<QListViewItem> it(list);
  KURL::List urls;

  for( ; it.current(); ++it )
    urls.append( KURL( ((K3bMovixListViewItem*)it.current())->fileItem()->localPath() ) );

  return KURLDrag::newDrag( urls, viewport() );
}

#include "k3bmovixlistview.moc"
