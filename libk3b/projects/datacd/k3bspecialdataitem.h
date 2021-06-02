/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef K3BSPECIALDATAITEM_H
#define K3BSPECIALDATAITEM_H

#include "k3bdataitem.h"
#include "k3bdiritem.h"

#include <KIO/Global>

namespace K3b {
    /**
     * This can be used to create fake items like the boot catalog
     * It's mainly a DataItem where everything has to be set manually
     */
    class SpecialDataItem : public DataItem
    {
    public:
        explicit SpecialDataItem( KIO::filesize_t size, const QString& k3bName = QString(), const ItemFlags& flags = ItemFlags() )
            : DataItem( flags | SPECIALFILE ),
              m_size( size ) {
            setK3bName( k3bName );
        }

        SpecialDataItem( const SpecialDataItem& item )
            : DataItem( item ),
              m_specialType( item.m_specialType ),
              m_size( item.m_size ) {
        }

        ~SpecialDataItem() override {
            // remove this from parentdir
            if( parent() )
                parent()->takeDataItem( this );
        }

        DataItem* copy() const override {
            return new SpecialDataItem( *this );
        }

        void setSpecialType( const QString& s ) { m_specialType = s; }
        QString specialType() const { return m_specialType; }

        bool isSpecialFile() const { return true; }

    protected:
        /**
         * Normally one does not use this method but DataItem::size()
         */
        KIO::filesize_t itemSize( bool ) const override { return m_size; }

    private:
        QString m_specialType;
        KIO::filesize_t m_size;
    };
}

#endif

