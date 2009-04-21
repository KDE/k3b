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

#include "k3bsessionimportitem.h"
#include "k3bfileitem.h"
#include "k3bdiritem.h"

#include "k3biso9660.h"

#include <KLocale>


K3b::SessionImportItem::SessionImportItem( const K3b::Iso9660File* isoF, K3b::DataDoc* doc, K3b::DirItem* dir )
    : K3b::SpecialDataItem( doc, isoF->size(), dir, isoF->name() ),
      m_replaceItem(0)

{
    // add automagically like a qlistviewitem
    if( parent() )
        parent()->addDataItem( this );
}


K3b::SessionImportItem::SessionImportItem( const K3b::SessionImportItem& item )
    : K3b::SpecialDataItem( item ),
      m_replaceItem( item.m_replaceItem )
{
}


K3b::SessionImportItem::~SessionImportItem()
{
    if( m_replaceItem )
        m_replaceItem->setReplacedItemFromOldSession(0);

    // remove this from parentdir
    if( parent() )
        parent()->takeDataItem( this );
}


K3b::DataItem* K3b::SessionImportItem::copy() const
{
    return new K3b::SessionImportItem( *this );
}
