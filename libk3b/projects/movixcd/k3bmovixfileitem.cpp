/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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


#include "k3bmovixfileitem.h"
#include "k3bmovixdoc.h"

#include "k3bdiritem.h"

K3b::MovixSubtitleItem::MovixSubtitleItem( const QString& fileName,
                                           K3b::MovixDoc* doc,
                                           K3b::DirItem* dir,
                                           K3b::MovixFileItem* parent,
                                           const QString& k3bName )
    : K3b::MovixFileItem( fileName, doc, dir, k3bName ),
      m_parent( parent )
{
}

K3b::MovixSubtitleItem::~MovixSubtitleItem()
{
}

K3b::MovixFileItem::MovixFileItem( const QString& fileName,
                                   K3b::MovixDoc* doc,
                                   K3b::DirItem* dir,
                                   const QString& k3bName )
    : K3b::FileItem( fileName, doc, dir, k3bName ),
      m_doc(doc),
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
    // rely on the K3b::FileItem destructor becasue
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

