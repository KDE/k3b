/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bsessionimportitem.h"
#include "k3bfileitem.h"
#include "k3bdiritem.h"
#include "k3b_i18n.h"

#include "k3biso9660.h"


namespace K3b {

SessionImportItem::SessionImportItem( const Iso9660File* isoF )
    : SpecialDataItem( isoF->size(), isoF->name(), OLD_SESSION ),
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
