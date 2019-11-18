/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
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


#ifndef K3BDIRITEM_H
#define K3BDIRITEM_H

#include "k3bdataitem.h"
#include "k3b_export.h"

#include <KIO/Global>

#include <QList>
#include <QString>

namespace K3b {
    class DataDoc;

    class LIBK3B_EXPORT DirItem : public DataItem
    {
    public:
        typedef QList<DataItem*> Children;

    public:
        explicit DirItem( const QString& name, const ItemFlags& flags = ItemFlags() );

        /**
         * Default copy constructor. Copies the dir including all children. However, none of the
         * children will have set a doc and the copy dir will not have set a parent dir.
         */
        DirItem( const DirItem& );

        ~DirItem() override;

        DataItem* copy() const override;

        DirItem* getDirItem() const override;

        Children const& children() const { return m_children; }
        DirItem* addDataItem( DataItem* item );
        void addDataItems( Children const& items );
        void removeDataItems( int start, int count );
        DataItem* takeDataItem( DataItem* item );
        Children takeDataItems( int start, int count );

        DataItem* nextSibling() const override;
        DataItem* nextChild( DataItem* ) const;

        bool alreadyInDirectory( const QString& fileName ) const;
        DataItem* find( const QString& filename ) const;
        DataItem* findByPath( const QString& );

        long numFiles() const;
        long numDirs() const;

        bool isEmpty() const { return ( numDirs() + numFiles() == 0 ); }

        /**
         * returns true if item is a subItem of
         * this dir item
         * (returns also true if item == this
         */
        bool isSubItem( const DataItem* item ) const;

        bool isRemoveable() const override;

        /**
         * Recursively creates a directory.
         */
        bool mkdir( const QString& dir );

        void setLocalPath( const QString& p ) { m_localPath = p; }
        QString localPath() const override { return m_localPath; }

        QMimeType mimeType() const override;

        /**
         * \reimplemented
         */
        bool writeToCd() const override;

    protected:
        /**
         * Normally one does not use this method but DataItem::size()
         *
         * This method does not take into account the possibility to share the data
         * between files with the same inode in an iso9660 filesystem.
         * For that one has to use FileCompilationSizeHandler.
         */
        KIO::filesize_t itemSize( bool followSymlinks ) const override;

        /*
         * Normally one does not use this method but DataItem::blocks()
         */
        Msf itemBlocks( bool followSymlinks ) const override;

    private:
        /**
         * this recursively updates the size of the directories.
         * The size of this dir and the parent dir is updated.
         * These values are just used for user information.
         */
        void updateSize( DataItem*, bool removed = false );
        /**
         * Updates the number of files and directories. These values are
         * just used for user information.
         */
        void updateFiles( long files, long dirs );
        /**
         * Unsets OLD_SESSION flag when directory no longer has
         * children from previous sessions
         */
        void updateOldSessionFlag();

        bool canAddDataItem( DataItem* item ) const;
        void addDataItemImpl( DataItem* item );

        mutable Children m_children;

        // size of the items simply added
        KIO::filesize_t m_size;
        KIO::filesize_t m_followSymlinksSize;

        // number of blocks (2048 bytes) used by all the items
        long m_blocks;
        long m_followSymlinksBlocks;

        long m_files;
        long m_dirs;

        // HACK: store the original path to be able to use it's permissions
        //       remove this once we have a backup project
        QString m_localPath;
    };


    class RootItem : public DirItem
    {
    public:
        explicit RootItem( DataDoc& doc );
        ~RootItem() override;

        DataDoc* getDoc() const override;
        QString k3bName() const override;
        void setK3bName( const QString& ) override;

        bool isMoveable() const override { return false; }
        bool isRemoveable() const override { return false; }

    private:
        DataDoc& m_doc;
    };
}
#endif
