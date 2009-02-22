/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
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
#include <k3bdiritem.h>

#include <klocale.h>
#include <kdebug.h>
#include <kio/global.h>
#include <k3urldrag.h>

#include <q3dragobject.h>
#include <qlist.h>
#include <qevent.h>
#include <q3header.h>
#include <QDropEvent>


K3b::MovixListViewItem::MovixListViewItem( K3b::MovixDoc* doc,
					    K3b::MovixFileItem* item,
					    Q3ListView* parent,
					    Q3ListViewItem* after )
    : K3b::ListViewItem( parent, after ),
      m_doc(doc),
      m_fileItem(item)
{
}


K3b::MovixListViewItem::MovixListViewItem( K3b::MovixDoc* doc,
					    K3b::MovixFileItem* item,
					    Q3ListViewItem* parent )
    : K3b::ListViewItem( parent ),
      m_doc(doc),
      m_fileItem(item)
{
}


K3b::MovixListViewItem::~MovixListViewItem()
{
}


K3b::MovixFileViewItem::MovixFileViewItem( K3b::MovixDoc* doc,
					    K3b::MovixFileItem* item,
					    Q3ListView* parent,
					    Q3ListViewItem* after )
    : K3b::MovixListViewItem( doc, item, parent, after ),
      KFileItem( 0, 0, KUrl(item->localPath()) )
{
    setPixmap( 1, KFileItem::pixmap( 16, KIconLoader::DefaultState ) );
    setEditor( 1, LINE );
}


QString K3b::MovixFileViewItem::text( int col ) const
{
    //
    // We add two spaces after all strings (except the once renamable)
    // to increase readability
    //

    switch( col ) {
    case 0:
        // allowing 999 files to be added.
        return QString::number( doc()->indexOf( fileItem() ) ).rightJustified( 3, ' ' );
    case 1:
        return fileItem()->k3bName();
    case 2:
    {
        if( fileItem()->isSymLink() )
            return i18n("Link to %1",const_cast<K3b::MovixFileViewItem*>(this)->mimeComment()) + "  ";
        else
            return const_cast<K3b::MovixFileViewItem*>(this)->mimeComment() + "  ";
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


void K3b::MovixFileViewItem::setText( int col, const QString& text )
{
    if( col == 1 )
        fileItem()->setK3bName( text );

    K3b::MovixListViewItem::setText( col, text );
}


QString K3b::MovixFileViewItem::key( int, bool ) const
{
    return QString::number( doc()->indexOf( fileItem() ) ).rightJustified( 10, '0' );
}




K3b::MovixSubTitleViewItem::MovixSubTitleViewItem( K3b::MovixDoc* doc,
						    K3b::MovixFileItem* item,
						    K3b::MovixListViewItem* parent )
    : K3b::MovixListViewItem( doc, item, parent ),
      KFileItem( 0, 0, KUrl(item->subTitleItem()->localPath()) )
{
}


K3b::MovixSubTitleViewItem::~MovixSubTitleViewItem()
{
}


QString K3b::MovixSubTitleViewItem::text( int c ) const
{
    switch( c ) {
    case 1:
        return fileItem()->subTitleItem()->k3bName();
    case 2:
    {
        if( fileItem()->subTitleItem()->isSymLink() )
            return i18n("Link to %1",const_cast<K3b::MovixSubTitleViewItem*>(this)->mimeComment());
        else
            return const_cast<K3b::MovixSubTitleViewItem*>(this)->mimeComment();
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











K3b::MovixListView::MovixListView( K3b::MovixDoc* doc, QWidget* parent )
    : K3b::ListView( parent ),
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
    setSelectionModeExt( K3ListView::Extended );
    setSorting(0);

    setNoItemText( i18n("Use drag'n'drop to add files to the project.") +"\n"
                   + i18n("To remove or rename files use the context menu.") + "\n"
                   + i18n("After that press the burn button to write the CD.") );

    connect( m_doc, SIGNAL(changed()), this, SLOT(slotChanged()) );
    connect( m_doc, SIGNAL(newMovixFileItems()), this, SLOT(slotNewFileItems()) );
    connect( m_doc, SIGNAL(movixItemRemoved(K3b::MovixFileItem*)), this, SLOT(slotFileItemRemoved(K3b::MovixFileItem*)) );
    connect( m_doc, SIGNAL(subTitleItemRemoved(K3b::MovixFileItem*)), this, SLOT(slotSubTitleItemRemoved(K3b::MovixFileItem*)) );
    connect( this, SIGNAL(dropped(K3ListView*, QDropEvent*, Q3ListViewItem*)),
             this, SLOT(slotDropped(K3ListView*, QDropEvent*, Q3ListViewItem*)) );

    // let's see what the doc already has
    slotNewFileItems();
    slotChanged();
}


K3b::MovixListView::~MovixListView()
{
}


bool K3b::MovixListView::acceptDrag(QDropEvent* e) const
{
    // the first is for built-in item moving, the second for dropping urls
    return ( K3b::ListView::acceptDrag(e) || K3URLDrag::canDecode(e) );
}


void K3b::MovixListView::slotNewFileItems()
{
    K3b::MovixFileItem* lastItem = 0;
    Q_FOREACH( K3b::MovixFileItem* item, m_doc->movixFileItems() ) {
        if( !m_itemMap.contains( item ) )
            m_itemMap.insert( item, new K3b::MovixFileViewItem( m_doc, item, this, lastItem ? m_itemMap[lastItem] : 0L ) );

        if( item->subTitleItem() ) {
            K3b::MovixFileViewItem* vi = m_itemMap[item];
            if( vi->childCount() <= 0 ) {
                (void)new K3b::MovixSubTitleViewItem( m_doc, item, vi );
                vi->setOpen(true);
            }
        }

        lastItem = item;
    }

    // arghhh
    sort();
}


void K3b::MovixListView::slotFileItemRemoved( K3b::MovixFileItem* item )
{
    if( m_itemMap.contains( item ) ) {
        K3b::MovixFileViewItem* vi = m_itemMap[item];
        m_itemMap.remove(item);
        delete vi;
    }
}


void K3b::MovixListView::slotSubTitleItemRemoved( K3b::MovixFileItem* item )
{
    if( m_itemMap.contains( item ) ) {
        K3b::MovixFileViewItem* vi = m_itemMap[item];
        if( vi->childCount() >= 1 )
            delete vi->firstChild();
    }
}


void K3b::MovixListView::slotDropped( K3ListView*, QDropEvent* e, Q3ListViewItem* after )
{
    if( !e->isAccepted() )
        return;

    int pos;
    if( after == 0L )
        pos = 0;
    else
        pos = m_doc->indexOf( ((K3b::MovixListViewItem*)after)->fileItem() );

    if( e->source() == viewport() ) {
        QList<Q3ListViewItem*> sel = selectedItems();
        QList<Q3ListViewItem*>::iterator it = sel.begin();
        K3b::MovixFileItem* itemAfter = ( after ? ((K3b::MovixListViewItem*)after)->fileItem() : 0 );
        while( it != sel.end() ) {
            K3b::MovixListViewItem* vi = (K3b::MovixListViewItem*)*it;
            if( vi->isMovixFileItem() ) {
                K3b::MovixFileItem* item = vi->fileItem();
                m_doc->moveMovixItem( item, itemAfter );
                itemAfter = item;
            }
            else
                kDebug() << "(K3b::MovixListView) I don't move subtitle items!";

            ++it;
        }

        sort();  // This is so lame!
    }
    else {
        KUrl::List urls;
        K3URLDrag::decode( e, urls );

        for( KUrl::List::ConstIterator it = urls.constBegin(); it != urls.constEnd(); ++it ) {
            m_doc->addMovixFile( *it, pos++ );
        }
    }

    // now grab that focus
    setFocus();
}


Q3DragObject* K3b::MovixListView::dragObject()
{
    QList<Q3ListViewItem*> list = selectedItems();

    if( list.isEmpty() )
        return 0;

    KUrl::List urls;

    Q_FOREACH( Q3ListViewItem* item,list )
        urls.append( KUrl( ((K3b::MovixListViewItem*)item)->fileItem()->localPath() ) );

    return K3URLDrag::newDrag( urls, viewport() );
}


void K3b::MovixListView::slotChanged()
{
    header()->setVisible( m_doc->root()->numFiles() > 0 );
}

#include "k3bmovixlistview.moc"
