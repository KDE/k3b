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


#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bdatadoc.h"
#include "k3bisooptions.h"
#include <kdebug.h>

#include <math.h>


class K3b::DataItem::Private
{
public:
    K3b::DataItem::ItemFlags flags;
};


K3b::DataItem::DataItem( K3b::DataDoc* doc, const ItemFlags& flags )
    : m_parentDir(0),
      m_sortWeight(0),
      m_bHideOnRockRidge(false),
      m_bHideOnJoliet(false),
      m_bRemoveable(true),
      m_bRenameable(true),
      m_bMovable(true),
      m_bHideable(true),
      m_bWriteToCd(true)
{
    d = new Private;
    d->flags = flags;

    m_doc = doc;
    m_bHideOnRockRidge = m_bHideOnJoliet = false;
}


K3b::DataItem::DataItem( const K3b::DataItem& item )
    : m_k3bName( item.m_k3bName ),
      m_extraInfo( item.m_extraInfo ),
      m_doc( 0 ),
      m_parentDir( 0 ),
      m_sortWeight( item.m_sortWeight ),
      m_bHideOnRockRidge( item.m_bHideOnRockRidge ),
      m_bHideOnJoliet( item.m_bHideOnJoliet ),
      m_bRemoveable( item.m_bRemoveable ),
      m_bRenameable( item.m_bRenameable ),
      m_bMovable( item.m_bMovable ),
      m_bHideable( item.m_bHideable ),
      m_bWriteToCd( item.m_bWriteToCd )
{
    d = new Private;
    d->flags = item.d->flags;
}


K3b::DataItem::~DataItem()
{
    delete d;
}


const K3b::DataItem::ItemFlags& K3b::DataItem::flags() const
{
   return d->flags;
}


void K3b::DataItem::setFlags( const ItemFlags& flags )
{
    d->flags = flags;
}


bool K3b::DataItem::isDir() const
{
   return d->flags & DIR;
}


bool K3b::DataItem::isFile() const
{
   return d->flags & FILE;
}


bool K3b::DataItem::isSpecialFile() const
{
   return d->flags & SPECIALFILE;
}


bool K3b::DataItem::isSymLink() const
{
   return d->flags & SYMLINK;
}


bool K3b::DataItem::isFromOldSession() const
{
   return d->flags & OLD_SESSION;
}


bool K3b::DataItem::isBootItem() const
{
   return d->flags & BOOT_IMAGE;
}


KIO::filesize_t K3b::DataItem::size() const
{
    return itemSize( m_doc
                     ? m_doc->isoOptions().followSymbolicLinks() ||
                     !m_doc->isoOptions().createRockRidge()
                     : false );
}


K3b::Msf K3b::DataItem::blocks() const
{
    return itemBlocks( m_doc
                       ? m_doc->isoOptions().followSymbolicLinks() ||
                       !m_doc->isoOptions().createRockRidge()
                       : false );
}


K3b::Msf K3b::DataItem::itemBlocks( bool followSymbolicLinks ) const
{
    return (long)::ceil( (double)itemSize( followSymbolicLinks ) / 2048.0 );
}


void K3b::DataItem::setK3bName( const QString& name ) {
    if ( name != m_k3bName ) {
        // test for not-allowed characters
        if( name.contains('/') ) {
            kDebug() << "(K3b::DataItem) name contained invalid characters!";
            return;
        }

        if( parent() ) {
            K3b::DataItem* item = parent()->find( name );
            if( item && item != this ) {
                kDebug() << "(K3b::DataItem) item with that name already exists.";
                return;
            }
        }

        m_k3bName = name;
        m_doc->setModified();
    }
}


QString K3b::DataItem::k3bName() const
{
    return m_k3bName;
}


K3b::DataItem* K3b::DataItem::take()
{
    if( parent() )
        parent()->takeDataItem( this );

    return this;
}


QString K3b::DataItem::k3bPath() const
{
    if( !parent() )
        return QString();  // the root item is the only one not having a parent
    else if( isDir() )
        return parent()->k3bPath() + k3bName() + "/";
    else
        return parent()->k3bPath() + k3bName();
}


QString K3b::DataItem::writtenPath() const
{
    if( !parent() )
        return QString();  // the root item is the only one not having a parent
    else if( isDir() )
        return parent()->writtenPath() + writtenName() + "/";
    else
        return parent()->writtenPath() + writtenName();
}


QString K3b::DataItem::iso9660Path() const
{
    if( !parent() )
        return QString();  // the root item is the only one not having a parent
    else if( isDir() )
        return parent()->iso9660Path() + iso9660Name() + "/";
    else
        return parent()->iso9660Path() + iso9660Name();
}


K3b::DataItem* K3b::DataItem::nextSibling() const
{
    K3b::DataItem* item = const_cast<K3b::DataItem*>(this); // urg, but we know that we don't mess with it, so...
    K3b::DirItem* parentItem = parent();

    while( parentItem ) {
        if( K3b::DataItem* i = parentItem->nextChild( item ) )
            return i;

        item = parentItem;
        parentItem = item->parent();
    }

    return 0;
}


void K3b::DataItem::reparent( K3b::DirItem* newParent )
{
    // addDataItem will do all the stuff including taking this
    newParent->addDataItem( this );
}


bool K3b::DataItem::hideOnRockRidge() const
{
    if( !isHideable() )
        return false;
    if( parent() )
        return m_bHideOnRockRidge || parent()->hideOnRockRidge();
    else
        return m_bHideOnRockRidge;
}


bool K3b::DataItem::hideOnJoliet() const
{
    if( !isHideable() )
        return false;
    if( parent() )
        return m_bHideOnJoliet || parent()->hideOnJoliet();
    else
        return m_bHideOnJoliet;
}


void K3b::DataItem::setHideOnRockRidge( bool b )
{
    // there is no use in changing the value if
    // it is already set by the parent
    if( ( !parent() || !parent()->hideOnRockRidge() ) &&
        b != m_bHideOnRockRidge ) {
        m_bHideOnRockRidge = b;
        if ( m_doc )
            m_doc->setModified();
    }
}


void K3b::DataItem::setHideOnJoliet( bool b )
{
    // there is no use in changing the value if
    // it is already set by the parent
    if( ( !parent() || !parent()->hideOnJoliet() ) &&
        b != m_bHideOnJoliet ) {
        m_bHideOnJoliet = b;
        if ( m_doc )
            m_doc->setModified();
    }
}


int K3b::DataItem::depth() const
{
    if( parent() )
        return parent()->depth() + 1;
    else
        return 0;
}


KMimeType::Ptr K3b::DataItem::mimeType() const
{
    return KMimeType::defaultMimeTypePtr();
}
