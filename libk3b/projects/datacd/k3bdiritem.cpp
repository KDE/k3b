/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include <QString>
#include <QList>

#include <KDebug>
#include <KLocale>


K3b::DirItem::DirItem(const QString& name, K3b::DataDoc* doc, K3b::DirItem* parentDir)
    : K3b::DataItem( doc, parentDir ),
      m_size(0),
      m_followSymlinksSize(0),
      m_blocks(0),
      m_followSymlinksBlocks(0),
      m_files(0),
      m_dirs(0)
{
    m_k3bName = name;

    // add automagically like a qlistviewitem
    if( parent() )
        parent()->addDataItem( this );
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
    // check if we are a subdir of item
    if( K3b::DirItem* dirItem = dynamic_cast<K3b::DirItem*>(item) ) {
        if( dirItem->isSubItem( this ) ) {
            kDebug() << "(K3b::DirItem) trying to move a dir item down in it's own tree.";
            return this;
        }
    }

    if( m_children.lastIndexOf( item ) == -1 ) {
        if( item->isFile() ) {
            // do we replace an old item?
            QString name = item->k3bName();
            int cnt = 1;
            while( K3b::DataItem* oldItem = find( name ) ) {
                if( !oldItem->isDir() && oldItem->isFromOldSession() ) {
                    // in this case we remove this item from it's parent and save it in the new one
                    // to be able to recover it
                    oldItem->take();
                    static_cast<K3b::SessionImportItem*>(oldItem)->setReplaceItem( static_cast<K3b::FileItem*>(item) );
                    static_cast<K3b::FileItem*>(item)->setReplacedItemFromOldSession( oldItem );
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

        // inform the doc
        if( doc() )
            doc()->aboutToAddItemToDir( this, item );

        m_children.append( item->take() );
        updateSize( item, false );
        if( item->isDir() )
            updateFiles( ((K3b::DirItem*)item)->numFiles(), ((K3b::DirItem*)item)->numDirs()+1 );
        else
            updateFiles( 1, 0 );

        item->m_parentDir = this;

        // inform the doc
        if( doc() )
            doc()->itemAddedToDir( this, item );
    }

    return this;
}


K3b::DataItem* K3b::DirItem::takeDataItem( K3b::DataItem* item )
{
    int x = m_children.lastIndexOf( item );
    if( x > -1 ) {
        if ( doc() )
            doc()->aboutToRemoveItemFromDir( this, item );

        K3b::DataItem* item = m_children.takeAt(x);
        updateSize( item, true );
        if( item->isDir() )
            updateFiles( -1*((K3b::DirItem*)item)->numFiles(), -1*((K3b::DirItem*)item)->numDirs()-1 );
        else
            updateFiles( -1, 0 );

        item->m_parentDir = 0;

        // inform the doc
        if( doc() )
            doc()->itemRemovedFromDir( this, item );

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
    if( !dir )
        dir = new K3b::DirItem( dirName, doc(), this );
    else if( !dir->isDir() )
        return false;

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


bool K3b::DirItem::isSubItem( K3b::DataItem* item ) const
{
    if( dynamic_cast<K3b::DirItem*>(item) == this )
        return true;

    K3b::DirItem* d = item->parent();
    while( d ) {
        if( d == this ) {
            return true;
        }
        d = d->parent();
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

    QList<K3b::DataItem*>::const_iterator end( m_children.constEnd() );
    for( QList<K3b::DataItem*>::const_iterator it = m_children.constBegin(); it != end; ++it ) {
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


bool K3b::DirItem::isFromOldSession() const
{
    QList<K3b::DataItem*>::const_iterator end( m_children.constEnd() );
    for( QList<K3b::DataItem*>::const_iterator it = m_children.constBegin(); it != end; ++it ) {
        if( (*it)->isFromOldSession() )
            return true;
    }
    return false;
}


bool K3b::DirItem::writeToCd() const
{
    // check if this dir contains items to write
    QList<K3b::DataItem*>::const_iterator end( m_children.constEnd() );
    for( QList<K3b::DataItem*>::const_iterator it = m_children.constBegin(); it != end; ++it ) {
        if( (*it)->writeToCd() )
            return true;
    }
    return K3b::DataItem::writeToCd();
}


KMimeType::Ptr K3b::DirItem::mimeType() const
{
    return KMimeType::mimeType( "inode/directory" );
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
