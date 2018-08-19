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

#include "k3bfilecompilationsizehandler.h"
#include "k3bfileitem.h"

#include <QDebug>
#include <QFile>
#include <QMap>
#include <QList>


// TODO: remove the items from the project if the savedSize differs
// with some info-widget: "Files xxx have changed on disk. Removing them from the project."
// or we just update the sizes!


static long usedBlocks( const KIO::filesize_t& bytes )
{
    if( bytes % 2048 )
        return bytes/2048 + 1;
    else
        return bytes/2048;
}


class InodeInfo
{
public:
    InodeInfo() {
        number = 0;
        savedSize = 0;
    }

    /**
     * How often has the file with
     * the corresponding inode been added
     */
    int number;

    /**
     * The size of the first added file. This has to be saved
     * to check further addings and to avoid the following situation:
     * A file with inode 1 is added, then deleted. Another file is created
     * at inode 1 and added to the project. Now the first file gets
     * removed and then the second. If we had not saved the size we would
     * have added the size of the first and removed the size of the second
     * file resulting in a corrupted project size.
     * This way we always use the size of the first added file and may
     * warn the user if sizes differ.
     */
    KIO::filesize_t savedSize;

    KIO::filesize_t completeSize() const { return savedSize*number; }

    /**
     * In an iso9660 filesystem a file occupies complete blocks of 2048 bytes.
     */
    K3b::Msf blocks() const { return K3b::Msf( usedBlocks(savedSize) ); }

    QList<K3b::DataItem*> items;
};


class K3b::FileCompilationSizeHandler::Private
{
public:
    Private()
        : size(0) {
    }

    void clear() {
        inodeMap.clear();
        size = 0;
        blocks = 0;
    }

    void addFile( K3b::FileItem* item, bool followSymlinks ) {
        InodeInfo& inodeInfo = inodeMap[item->localId(followSymlinks)];

        inodeInfo.items.append( item );

        if( inodeInfo.number == 0 ) {
            inodeInfo.savedSize = item->itemSize( followSymlinks );

            size += inodeInfo.savedSize;
            blocks += inodeInfo.blocks();
        }

        inodeInfo.number++;
    }

    void addSpecialItem( K3b::DataItem* item ) {
        // special files do not have a corresponding local file
        // so we just add their k3bSize
        size += item->size();
        blocks += usedBlocks(item->size());
        specialItems.append( item );
    }

    void removeFile( K3b::FileItem* item, bool followSymlinks ) {
        InodeInfo& inodeInfo = inodeMap[item->localId(followSymlinks)];

        if( !inodeInfo.items.contains( item ) ) {
            qCritical() << "(K3b::FileCompilationSizeHandler) "
                     << item->localPath()
                     << " has been removed without being added!" << endl;
        }
        else {
            if( item->itemSize(followSymlinks) != inodeInfo.savedSize ) {
                qCritical() << "(K3b::FileCompilationSizeHandler) savedSize differs!" << endl;
            }

            inodeInfo.items.removeOne( item );
            inodeInfo.number--;
            if( inodeInfo.number == 0 ) {
                size -= inodeInfo.savedSize;
                blocks -= inodeInfo.blocks();
            }
        }
    }

    void removeSpecialItem( K3b::DataItem* item ) {
        // special files do not have a corresponding local file
        // so we just subtract their k3bSize
        if( !specialItems.contains( item ) ) {
            qCritical() << "(K3b::FileCompilationSizeHandler) Special item "
                     << item->k3bName()
                     << " has been removed without being added!" << endl;
        }
        else {
            specialItems.removeOne( item );
            size -= item->size();
            blocks -= usedBlocks(item->size());
        }
    }


    /**
     * This maps from inodes to the number of occurrences of the inode.
     */
    QMap<K3b::FileItem::Id, InodeInfo> inodeMap;

    KIO::filesize_t size;
    K3b::Msf blocks;

    QList<K3b::DataItem*> specialItems;
};



K3b::FileCompilationSizeHandler::FileCompilationSizeHandler()
{
    d_symlinks = new Private;
    d_noSymlinks = new Private;
}

K3b::FileCompilationSizeHandler::~FileCompilationSizeHandler()
{
    delete d_symlinks;
    delete d_noSymlinks;
}


const KIO::filesize_t& K3b::FileCompilationSizeHandler::size( bool followSymlinks ) const
{
    if( followSymlinks )
        return d_noSymlinks->size;
    else
        return d_symlinks->size;
}


const K3b::Msf& K3b::FileCompilationSizeHandler::blocks( bool followSymlinks ) const
{
    if( followSymlinks )
        return d_noSymlinks->blocks;
    else
        return d_symlinks->blocks;
}


void K3b::FileCompilationSizeHandler::addFile( K3b::DataItem* item )
{
    if( item->isSpecialFile() ) {
        d_symlinks->addSpecialItem( item );
        d_noSymlinks->addSpecialItem( item );
    }
    else if( item->isFile() ) {
        K3b::FileItem* fileItem = static_cast<K3b::FileItem*>( item );
        d_symlinks->addFile( fileItem, false );
        d_noSymlinks->addFile( fileItem, true );
    }
}


void K3b::FileCompilationSizeHandler::removeFile( K3b::DataItem* item )
{
    if( item->isSpecialFile() ) {
        d_symlinks->removeSpecialItem( item );
        d_noSymlinks->removeSpecialItem( item );
    }
    else if( item->isFile() ) {
        K3b::FileItem* fileItem = static_cast<K3b::FileItem*>( item );
        d_symlinks->removeFile( fileItem, false );
        d_noSymlinks->removeFile( fileItem, true );
    }
}


void K3b::FileCompilationSizeHandler::clear()
{
    d_symlinks->clear();
    d_noSymlinks->clear();
}
