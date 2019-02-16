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

#ifndef _K3B_BOOT_ITEM_H_
#define _K3B_BOOT_ITEM_H_

#include "k3bfileitem.h"
#include "k3b_export.h"

namespace K3b {
    class LIBK3B_EXPORT BootItem : public FileItem
    {
    public:
        BootItem( const QString& fileName, DataDoc& doc, const QString& k3bName = 0 );
        BootItem( const BootItem& );
        ~BootItem() override;

        DataItem* copy() const override;

        bool isHideable() const override { return false; }

        enum ImageType { FLOPPY, HARDDISK, NONE };

        void setNoBoot( bool b ) { m_noBoot = b; }
        void setBootInfoTable( bool b ) { m_bootInfoTable = b; }
        void setLoadSegment( int s ) { m_loadSegment = s; }
        void setLoadSize( int s ) { m_loadSize = s; }
        void setImageType( int t ) { m_imageType = t; }

        void setTempPath( const QString& p ) { m_tempPath = p; }

        bool noBoot() const { return m_noBoot; }
        bool bootInfoTable() const { return m_bootInfoTable; }
        int loadSegment() const { return m_loadSegment; }
        int loadSize() const { return m_loadSize; }
        int imageType() const { return m_imageType; }

        /**
         * mkisofs changes boot images on disk. That is why the iso imager
         * buffers them and saves the path to the buffered copy here.
         */
        QString tempPath() const { return m_tempPath; }

    private:
        bool m_noBoot;
        bool m_bootInfoTable;
        int m_loadSegment;
        int m_loadSize;
        int m_imageType;

        QString m_tempPath;
    };
}

#endif
