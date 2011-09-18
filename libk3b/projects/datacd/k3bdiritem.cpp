/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdiritem.h"
#include "k3bdatadoc.h"
#include "k3bsessionimportitem.h"
#include "k3bfileitem.h"
#include "k3bisooptions.h"

#include <KDebug>
#include <KLocale>


K3b::DirItem::DirItem(const QString& name, K3b::DataDoc* doc, const ItemFlags& flags)
    : K3b::DataItem( doc, flags | DIR ),
      m_size(0),
      m_followSymlinksSize(0),
      m_blocks(0),
      m_followSymlinksBlocks(0),
      m_files(0),
      m_dirs(0)
{
    m_k3bName = name;
}


K3b::DirItem::DirItem( const K3b::DirItem& item )
    : K3b::DataItem( item ),
      m_size(0),
      m_followSymlinksSize(0),
      m_blocks(0),
      m_followSymlinksBlocks(0),
      m_files(0),
      m_dirs(0),
      m_localPath( item.m_localPath )
{
    Q_FOREACH( K3b::DataItem* _item, item.children() ) {
        addDataItem( _item->copy() );
    }
}

K3b::DirItem::~DirItem()
{
    // delete all children
    // doing this by hand is much saver than using the
    // auto-delete feature since some of the items' destructors
    // may change the list
    while( !m_children.isEmpty() ) {
        // it is important to use takeDataItem here to be sure
        // the size gets updated properly
        K3b::DataItem* item = m_children.first();
        takeDataItem( item );
        delete item;
    }

    // this has to be done after deleting the children
    // because the directory itself has a size of 0 in K3b
    // and all it's files' sizes have already been substracted
    take();
}


K3b::DataItem* K3b::DirItem::copy() const
{
    return new K3b::DirItem( *this );
}


K3b::DirItem* K3b::DirItem::getDirItem() const
{
    return const_cast<K3b::DirItem*>(this);
}


K3b::DirItem* K3b::DirItem::addDataItem( K3b::DataItem* item )
{
    if( canAddDataItem( item ) ) {

        // Detach item from its parent in case it's moved from elsewhere.
        // It is essential to do this before calling doc()->aboutToAddItemToDir()
        // avoid situation when beginRemoveRows() is called after beginInsertRows()
        // in DataProjectModel
        item->take();

        // inform the doc
        if( doc() )
            doc()->beginInsertItems( this, m_children.size(), m_children.size() );

        addDataItemImpl( item );

        if( doc() )
            doc()->endInsertItems( this, m_children.size()-1, m_children.size()-1 );
    }

    return this;
}


void K3b::DirItem::addDataItems( const Children& items )
{
    Children newItems;
    newItems.reserve( items.size() );
    Q_FOREACH( DataItem* item, items ) {
        if( canAddDataItem( item ) ) {
            // Detach item from its parent in case it's moved from elsewhere.
            // It is essential to do this before calling doc()->aboutToAddItemToDir()
            // avoid situation when beginRemoveRows() is called after beginInsertRows()
            // in DataProjectModel
            item->take();

            newItems.push_back( item );
        }
    }

    if( !newItems.empty() ) {
        const int start = m_children.size();
        const int end = m_children.size() + newItems.size() - 1;

        // inform the doc
        if( doc() )
            doc()->beginInsertItems( this, start, end );

        // pre-alloc space for items
        m_children.reserve( m_children.size() + newItems.size() );

        Q_FOREACH( DataItem* item, newItems ) {
            addDataItemImpl( item );
        }

        if( doc() )
            doc()->endInsertItems( this, start, end );
    }
}


K3b::DataItem* K3b::DirItem::takeDataItem( K3b::DataItem* item )
{
    int x = m_children.lastIndexOf( item );
    if( x > -1 ) {
        if ( doc() )
            doc()->beginRemoveItems( this, x, x );

        K3b::DataItem* item = m_children.takeAt(x);
        updateSize( item, true );
        if( item->isDir() )
            updateFiles( -1*((K3b::DirItem*)item)->numFiles(), -1*((K3b::DirItem*)item)->numDirs()-1 );
        else
            updateFiles( -1, 0 );

        item->setParentDir( 0 );
        
        // unset OLD_SESSION flag if it was the last child from previous sessions
        updateOldSessionFlag();

        // inform the doc
        if( doc() )
            doc()->endRemoveItems( this, x, x );

        if( item->isFile() ) {
            // restore the item imported from an old session
            if( static_cast<K3b::FileItem*>(item)->replaceItemFromOldSession() )
                addDataItem( static_cast<K3b::FileItem*>(item)->replaceItemFromOldSession() );
        }

        return item;
    }
    else
        return 0;
}


K3b::DataItem* K3b::DirItem::nextSibling() const
{
    if( !m_children.isEmpty() )
        return m_children.first();
    else
        return K3b::DataItem::nextSibling();
}


K3b::DataItem* K3b::DirItem::nextChild( K3b::DataItem* prev ) const
{
    // search for prev in children
    int index = m_children.lastIndexOf( prev );
    if( index < 0 || index+1 == m_children.count() ) {
        return 0;
    }
    else
        return m_children[index+1];
}


bool K3b::DirItem::alreadyInDirectory( const QString& filename ) const
{
    return (find( filename ) != 0);
}


K3b::DataItem* K3b::DirItem::find( const QString& filename ) const
{
    Q_FOREACH( K3b::DataItem* item, m_children ) {
        if( item->k3bName() == filename )
            return item;
    }
    return 0;
}


K3b::DataItem* K3b::DirItem::findByPath( const QString& p )
{
    if( p.isEmpty() || p == "/" )
        return this;

    QString path = p;
    if( path.startsWith("/") )
        path = path.mid(1);
    int pos = path.indexOf( "/" );
    if( pos < 0 )
        return find( path );
    else {
        // do it recursivly
        K3b::DataItem* item = find( path.left(pos) );
        if( item && item->isDir() )
            return ((K3b::DirItem*)item)->findByPath( path.mid( pos+1 ) );
        else
            return 0;
    }
}


bool K3b::DirItem::mkdir( const QString& dirPath )
{
    //
    // An absolut path always starts at the root item
    //
    if( dirPath[0] == '/' ) {
        if( parent() )
            return parent()->mkdir( dirPath );
        else
            return mkdir( dirPath.mid( 1 ) );
    }

    if( findByPath( dirPath ) )
        return false;

    QString restPath;
    QString dirName;
    int pos = dirPath.indexOf( '/' );
    if( pos == -1 ) {
        dirName = dirPath;
    }
    else {
        dirName = dirPath.left( pos );
        restPath = dirPath.mid( pos+1 );
    }

    K3b::DataItem* dir = find( dirName );
    if( !dir ) {
        dir = new K3b::DirItem( dirName, doc() );
        addDataItem( dir );
    } else if( !dir->isDir() ) {
        return false;
    }

    if( !restPath.isEmpty() )
        return static_cast<K3b::DirItem*>(dir)->mkdir( restPath );

    return true;
}


KIO::filesize_t K3b::DirItem::itemSize( bool followsylinks ) const
{
    if( followsylinks )
        return m_followSymlinksSize;
    else
        return m_size;
}


K3b::Msf K3b::DirItem::itemBlocks( bool followSymlinks ) const
{
    if( followSymlinks )
        return m_followSymlinksBlocks;
    else
        return m_blocks;
}


bool K3b::DirItem::isSubItem( const DataItem* item ) const
{
    for( const DirItem* dir = dynamic_cast<const DirItem*>(item); dir != 0; dir = dir->parent() ) {
        if( dir == this ) {
            return true;
        }
    }

    return false;
}


long K3b::DirItem::numFiles() const
{
    return m_files;
}


long K3b::DirItem::numDirs() const
{
    return m_dirs;
}


bool K3b::DirItem::isRemoveable() const
{
    if( !K3b::DataItem::isRemoveable() )
        return false;

    for( Children::const_iterator it = m_children.constBegin(), end = m_children.constEnd(); it != end; ++it ) {
        if( !( *it )->isRemoveable() )
            return false;
    }

    return true;
}


void K3b::DirItem::updateSize( K3b::DataItem* item, bool removed )
{
    if ( !item->isFromOldSession() ) {
        if( removed ) {
            m_followSymlinksSize -= item->itemSize( true );
            m_size -= item->itemSize( false );
            m_followSymlinksBlocks -= item->itemBlocks( true ).lba();
            m_blocks -= item->itemBlocks( false ).lba();
        }
        else {
            m_followSymlinksSize += item->itemSize( true );
            m_size += item->itemSize( false );
            m_followSymlinksBlocks += item->itemBlocks( true ).lba();
            m_blocks += item->itemBlocks( false ).lba();
        }
    }

    if( parent() )
        parent()->updateSize( item, removed );
}

void K3b::DirItem::updateFiles( long files, long dirs )
{
    m_files += files;
    m_dirs += dirs;
    if( parent() )
        parent()->updateFiles( files, dirs );
}


void K3b::DirItem::updateOldSessionFlag()
{
    if( flags().testFlag( OLD_SESSION ) ) {
        for( Children::const_iterator it = m_children.constBegin(), end = m_children.constEnd(); it != end; ++it ) {
            if( (*it)->isFromOldSession() ) {
                return;
            }
        }
        setFlags( flags() & ~OLD_SESSION );
    }
}


bool K3b::DirItem::writeToCd() const
{
    // check if this dir contains items to write
    Children::const_iterator end( m_children.constEnd() );
    for( Children::const_iterator it = m_children.constBegin(); it != end; ++it ) {
        if( (*it)->writeToCd() )
            return true;
    }
    return K3b::DataItem::writeToCd();
}


KMimeType::Ptr K3b::DirItem::mimeType() const
{
    return KMimeType::mimeType( "inode/directory" );
}


bool K3b::DirItem::canAddDataItem( DataItem* item ) const
{
    // check if we are a subdir of item
    DirItem* dirItem = dynamic_cast<DirItem*>( item );
    if( dirItem && dirItem->isSubItem( this ) ) {
        kDebug() << "(K3b::DirItem) trying to move a dir item down in it's own tree.";
        return false;
    } else if( !item || m_children.contains( item ) ) {
        return false;
    } else {
        return true;
    }
}


void K3b::DirItem::addDataItemImpl( DataItem* item )
{
    if( item->isFile() ) {
        // do we replace an old item?
        QString name = item->k3bName();
        int cnt = 1;
        while( DataItem* oldItem = find( name ) ) {
            if( !oldItem->isDir() && oldItem->isFromOldSession() ) {
                // in this case we remove this item from it's parent and save it in the new one
                // to be able to recover it
                oldItem->take();
                static_cast<SessionImportItem*>(oldItem)->setReplaceItem( static_cast<FileItem*>(item) );
                static_cast<FileItem*>(item)->setReplacedItemFromOldSession( oldItem );
                break;
            }
            else {
                //
                // add a counter to the filename
                //
                if( item->k3bName()[item->k3bName().length()-4] == '.' )
                    name = item->k3bName().left( item->k3bName().length()-4 ) + QString::number(cnt++) + item->k3bName().right(4);
                else
                    name = item->k3bName() + QString::number(cnt++);
            }
        }
        item->setK3bName( name );
    }

    m_children.append( item );
    updateSize( item, false );
    if( item->isDir() )
        updateFiles( ((DirItem*)item)->numFiles(), ((DirItem*)item)->numDirs()+1 );
    else
        updateFiles( 1, 0 );

    item->setParentDir( this );

    // If item is from previous session,flag this directory as such also
    if( !isFromOldSession() && item->isFromOldSession() ) {
        setFlags( flags() | OLD_SESSION );
    }
}


K3b::RootItem::RootItem( K3b::DataDoc* doc )
    : K3b::DirItem( "root", doc, 0 )
{
}


K3b::RootItem::~RootItem()
{
}


QString K3b::RootItem::k3bName() const
{
    return doc()->isoOptions().volumeID();
}


void K3b::RootItem::setK3bName( const QString& text )
{
    doc()->setVolumeID( text );
}
