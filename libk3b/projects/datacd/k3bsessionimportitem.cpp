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

#include "k3bsessionimportitem.h"
#include "k3bfileitem.h"
#include "k3bdiritem.h"

#include "k3biso9660.h"

#include <KLocale>

namespace K3b {

SessionImportItem::SessionImportItem( const Iso9660File* isoF, DataDoc* doc )
    : SpecialDataItem( doc, isoF->size(), isoF->name(), OLD_SESSION ),
      m_replaceItem(0)
{
}


SessionImportItem::SessionImportItem( const SessionImportItem& item )
    : SpecialDataItem( item ),
      m_replaceItem( item.m_replaceItem )
{
}


SessionImportItem::~SessionImportItem()
{
    if( m_replaceItem )
        m_replaceItem->setReplacedItemFromOldSession(0);

    // remove this from parentdir
    if( parent() )
        parent()->takeDataItem( this );
}


DataItem* SessionImportItem::copy() const
{
    return new SessionImportItem( *this );
}

} // namespace K3b
