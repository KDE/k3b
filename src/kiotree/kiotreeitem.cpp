/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

//#include <kparts/browserextension.h>

#include "kiotreeitem.h"
#include "kiotree.h"
//#include "konq_treepart.h"
#include "kiotreetoplevelitem.h"

KioTreeItem::KioTreeItem( KioTreeItem *parentItem, KioTreeTopLevelItem *topLevelItem )
    : QListViewItem( parentItem )
{
    initItem( topLevelItem );
}

KioTreeItem::KioTreeItem( KioTree *parent, KioTreeTopLevelItem *topLevelItem )
    : QListViewItem( parent )
{
    initItem( topLevelItem );
}

void KioTreeItem::initItem( KioTreeTopLevelItem *topLevelItem )
{
    m_topLevelItem = topLevelItem;
    m_bListable = true;
    m_bClickable = true;

    setExpandable( true );
}

void KioTreeItem::middleButtonPressed()
{
//     emit tree()->part()->extension()->createNewWindow( externalURL() );
}

KioTreeModule * KioTreeItem::module() const
{
    return m_topLevelItem->module();
}

KioTree * KioTreeItem::tree() const
{
    return static_cast<KioTree *>(listView());
}
