/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
*/

#include <kio/global.h>

// K3b Includes
#include "k3bvcdlistviewitem.h"
#include "k3bvcdtrack.h"
#include <k3bglobals.h>

K3bVcdListViewItem::K3bVcdListViewItem( K3bVcdTrack* track, K3bListView* parent )
        : K3bListViewItem( parent ), m_track( track )
{
    setEditor( 1, LINE );
}

K3bVcdListViewItem::K3bVcdListViewItem( K3bVcdTrack* track, K3bListView* parent, QListViewItem* after )
        : K3bListViewItem( parent, after ), m_track( track )
{
    setEditor( 1, LINE );
}


K3bVcdListViewItem::~K3bVcdListViewItem()
{}

QString K3bVcdListViewItem::text( int i ) const
{
    switch ( i ) {
        case 0:
            return QString::number( m_track->index() + 1 ).rightJustify( 2, ' ' );
        case 1:
            return m_track->title();
        case 2:
            // track mpegtype
            return m_track->mpegVersion();
        case 3:
            // track mpegsize
            return m_track->mpegSize();
        case 4:
            // track mpegdisplaysize
            return m_track->mpegDisplaySize();
        case 5:
            // track mpegfps
            return m_track->mpegFps();
        case 6:
            // track mpegmbps
            return m_track->mpegMbps();
        case 7:
            // track mpegduration
            return m_track->mpegDuration();
        case 8:
            // track size
            return KIO::convertSize( m_track->size() );
        case 9:
            // filename
            return m_track->fileName();

        default:
            return KListViewItem::text( i );
    }
}

void K3bVcdListViewItem::setText( int col, const QString& text )
{
    if ( col == 1 ) {
        // this is the title field
        m_track->setTitle( text );
    }

    KListViewItem::setText( col, text );
}


QString K3bVcdListViewItem::key( int, bool ) const
{
    QString num = QString::number( m_track->index() );
    if ( num.length() == 1 )
        return "00" + num;
    else if ( num.length() == 2 )
        return "0" + num;

    return num;
}
