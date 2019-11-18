/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_MOVIX_FILEITEM_H_
#define _K3B_MOVIX_FILEITEM_H_

#include "k3bfileitem.h"

namespace K3b {
    class MovixDoc;
    class MovixSubtitleItem;

    class LIBK3B_EXPORT MovixFileItem : public FileItem
    {
    public:
        MovixFileItem( const QString& fileName, MovixDoc& doc, const QString& k3bName = 0 );
        ~MovixFileItem() override;

        virtual bool isSubtitle() { return false; }

        MovixSubtitleItem* subTitleItem() const { return m_subTitleItem; }
        void setSubTitleItem( MovixSubtitleItem* i ) { m_subTitleItem = i; }

        /**
         * reimplemented from DataItem
         * also renames the subTitleItem
         */
        void setK3bName( const QString& ) override;

        /**
         * returns the name that the subtitle file must have in
         * order to work with mplayer
         */
        static QString subTitleFileName( const QString& );


    private:
        MovixSubtitleItem* m_subTitleItem;
    };

    class LIBK3B_EXPORT MovixSubtitleItem : public MovixFileItem
    {
    public:
        MovixSubtitleItem( const QString& fileName, MovixDoc& doc, MovixFileItem *parent, const QString& k3bName = 0 );
        ~MovixSubtitleItem() override;

        bool isSubtitle() override { return true; }
        MovixFileItem *parent() const { return m_parent; }


    private:
        MovixFileItem *m_parent;
    };


}

#endif
