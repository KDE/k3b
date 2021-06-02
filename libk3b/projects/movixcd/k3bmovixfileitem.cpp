/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
    SPDX-FileCopyrightText: 2010-2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#include "k3bmovixfileitem.h"
#include "k3bmovixdoc.h"
#include "k3bdiritem.h"

K3b::MovixSubtitleItem::MovixSubtitleItem( const QString& fileName,
                                           K3b::MovixDoc& doc,
                                           K3b::MovixFileItem* parent,
                                           const QString& k3bName )
    : K3b::MovixFileItem( fileName, doc, k3bName ),
      m_parent( parent )
{
}

K3b::MovixSubtitleItem::~MovixSubtitleItem()
{
}

K3b::MovixFileItem::MovixFileItem( const QString& fileName,
                                   K3b::MovixDoc& doc,
                                   const QString& k3bName )
    : K3b::FileItem( fileName, doc, k3bName ),
      m_subTitleItem(0)
{
}


K3b::MovixFileItem::~MovixFileItem()
{
    if( m_subTitleItem ) {
        delete m_subTitleItem;
        m_subTitleItem = 0;
    }

    // remove this from parentdir
    // it is important to do it here and not
    // rely on the K3b::FileItem destructor because
    // otherwise the doc is not informed early enough
    if( parent() )
        parent()->takeDataItem( this );
}


void K3b::MovixFileItem::setK3bName( const QString& newName )
{
    K3b::FileItem::setK3bName( newName );

    // take care of the subTitle file
    if( m_subTitleItem ) {
        m_subTitleItem->setK3bName( subTitleFileName(k3bName()) );
    }
}

QString K3b::MovixFileItem::subTitleFileName( const QString& name )
{
    // remove ending from k3bName
    QString subName = name;
    int pos = subName.lastIndexOf('.');
    if( pos > 0 )
        subName.truncate( pos );
    subName += ".sub";
    return subName;
}

