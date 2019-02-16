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


#ifndef K3BFILEITEM_H
#define K3BFILEITEM_H


#include "k3bdataitem.h"
#include "k3bglobals.h"

#include <KIO/Global>
#include <QString>

#include "k3b_export.h"

namespace K3b {
    class DataDoc;
    class DirItem;

    class LIBK3B_EXPORT FileItem : public DataItem
    {
    public:
        /**
         * Creates a new FileItem
         */
        FileItem( const QString& filePath, DataDoc& doc, const QString& k3bName = 0, const ItemFlags& flags = ItemFlags() );

        /**
         * Constructor for optimized file item creation which does no additional stat.
         *
         * Used by K3b to speedup file item creation.
         */
        FileItem( const k3b_struct_stat* stat,
                  const k3b_struct_stat* followedStat,
                  const QString& filePath, DataDoc& doc, const QString& k3bName = 0, const ItemFlags& flags = ItemFlags() );

        /**
         * Default copy constructor
         * Creates a copy of the fileitem. The copy, however, is not an exact duplicate of this item.
         * The copy does not have a parent dir set and any old session items are set to 0.
         */
        FileItem( const FileItem& );

        ~FileItem() override;

        DataItem* copy() const override;

        bool exists() const;

        QString absIsoPath();

        /** reimplemented from DataItem */
        QString localPath() const override;

        /**
         * Identification of the files on the local device.
         */
        struct Id {
            dev_t device;
            ino_t inode;
        };

        /**
         * This is not the normal inode number but it also contains
         * the device number.
         */
        Id localId() const;

        /**
         * The id of the file the symlink is pointing to
         */
        Id localId( bool followSymlinks ) const;

        DirItem* getDirItem() const override;

        QString linkDest() const;

        QMimeType mimeType() const override;

        /** returns true if the item is not a link or
         *  if the link's destination is part of the compilation */
        bool isValid() const override;

        DataItem* replaceItemFromOldSession() const { return m_replacedItemFromOldSession; }
        void setReplacedItemFromOldSession( DataItem* item ) { m_replacedItemFromOldSession = item; }

        /**
         * Normally one does not use this method but DataItem::size()
         */
        KIO::filesize_t itemSize( bool followSymlinks ) const override;
        
    private:
        void init( const QString& filePath,
                   const QString& k3bName,
                   DataDoc& doc,
                   const k3b_struct_stat* stat,
                   const k3b_struct_stat* followedStat );

    private:
        DataItem* m_replacedItemFromOldSession;

        KIO::filesize_t m_size;
        KIO::filesize_t m_sizeFollowed;
        Id m_id;
        Id m_idFollowed;

        QString m_localPath;

        QMimeType m_mimeType;
    };

    bool operator==( const FileItem::Id&, const FileItem::Id& );
    bool operator<( const FileItem::Id&, const FileItem::Id& );
    bool operator>( const FileItem::Id&, const FileItem::Id& );
}

#endif
