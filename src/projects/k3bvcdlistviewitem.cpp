/*
*
* $Id$
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
*/

#include <kio/global.h>
#include <kiconloader.h>


// K3b Includes
#include "k3bvcdlistviewitem.h"
#include "k3bvcdtrack.h"
#include <k3bglobals.h>

K3bVcdListViewItem::K3bVcdListViewItem( K3bVcdTrack* track, K3bListView* parent )
        : K3bListViewItem( parent ), m_track( track )
{
    setEditor( 1, LINE );
    animate();
}

K3bVcdListViewItem::K3bVcdListViewItem( K3bVcdTrack* track, K3bListView* parent, Q3ListViewItem* after )
        : K3bListViewItem( parent, after ), m_track( track )
{
    setEditor( 1, LINE );
    animate();
}


K3bVcdListViewItem::~K3bVcdListViewItem()
{}

QString K3bVcdListViewItem::text( int i ) const
{
    //
    // We add two spaces after all strings (except the once renamable)
    // to increase readability
    //

    switch ( i ) {
            case 0:
            return QString::number( m_track->index() + 1 ).rightJustified( 2, ' ' ) + "  ";
            case 1:
            return m_track->title();
            case 2:
            // track mpegtype
            return m_track->mpegTypeS() + "  ";
            case 3:
            // track mpegsize
            return m_track->resolution() + "  ";
            case 4:
            // track low mpegsize for MPEG1 Stills
            return m_track->highresolution() + "  ";
            case 5:
            // track mpegfps
            return m_track->video_frate() + "  ";
            case 6:
            // track mpegmbps
            return QString::number( m_track->muxrate() ) + "  ";
            case 7:
            // track mpegduration
            return m_track->duration() + "  ";
            case 8:
            // track size
            return KIO::convertSize( m_track->size() ) + "  ";
            case 9:
            // filename
            return m_track->fileName();

            default:
            return K3ListViewItem::text( i );
    }
}

void K3bVcdListViewItem::setText( int col, const QString& text )
{
    if ( col == 1 ) {
        // this is the title field
        m_track->setTitle( text );
    }

    K3ListViewItem::setText( col, text );
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

bool K3bVcdListViewItem::animate()
{
    bool animate = false;

    switch ( m_track->mpegType() ) {
            case 0: // MPEG_MOTION
            setPixmap( 2, ( SmallIcon( "video" ) ) );
            break;
            case 1: // MPEG_STILL
            setPixmap( 2, ( SmallIcon( "image" ) ) );
            break;
            case 2: // MPEG_AUDIO
            setPixmap( 2, ( SmallIcon( "sound" ) ) );
            break;

            default:
            setPixmap( 2, ( SmallIcon( "video" ) ) );
            break;
    }


    return animate;
}
