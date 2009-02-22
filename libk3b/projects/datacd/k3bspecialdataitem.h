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


#ifndef K3BSPECIALDATAITEM_H
#define K3BSPECIALDATAITEM_H

#include "k3bdataitem.h"
#include "k3bdiritem.h"

#include <kio/global.h>

namespace K3b {
    /**
     * This can be used to create fake items like the boot catalog
     * It's mainly a DataItem where everything has to be set manually
     */
    class SpecialDataItem : public DataItem
    {
    public:
        SpecialDataItem( DataDoc* doc, KIO::filesize_t size, DirItem* parent = 0, const QString& k3bName = QString() )
            : DataItem( doc, parent ),
              m_size( size ) {
            setK3bName( k3bName );

            // add automagically like a qlistviewitem
            if( parent )
                parent->addDataItem( this );
        }

        SpecialDataItem( const SpecialDataItem& item )
            : DataItem( item ),
              m_specialType( item.m_specialType ),
              m_size( item.m_size ) {
        }

        virtual ~SpecialDataItem() {
            // remove this from parentdir
            if( parent() )
                parent()->takeDataItem( this );
        }

        DataItem* copy() const {
            return new SpecialDataItem( *this );
        }

        void setSpecialType( const QString& s ) { m_specialType = s; }
        QString specialType() const { return m_specialType; }

        bool isSpecialFile() const { return true; }

    protected:
        /**
         * Normally one does not use this method but DataItem::size()
         */
        KIO::filesize_t itemSize( bool ) const { return m_size; }

    private:
        QString m_specialType;
        KIO::filesize_t m_size;
    };
}

#endif

