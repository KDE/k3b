/*
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3blistviewitemanimator.h"

#include <qtimer.h>
#include <q3listview.h>


K3b::ListViewItemAnimator::ListViewItemAnimator( QObject* parent )
    : QObject( parent )
{
    init();
}


K3b::ListViewItemAnimator::ListViewItemAnimator( Q3ListViewItem* item, int col, QObject* parent )
    : QObject( parent )
{
    init();
    setItem( item, col );
}


K3b::ListViewItemAnimator::~ListViewItemAnimator()
{
}


Q3ListViewItem* K3b::ListViewItemAnimator::item() const
{
    return m_item;
}


void K3b::ListViewItemAnimator::init()
{
    m_item = 0;
    m_column = 0;
    m_timer = new QTimer( this );
    connect( m_timer, SIGNAL(timeout()), this, SLOT(slotAnimate()) );
}


void K3b::ListViewItemAnimator::start()
{
    if( m_item && !m_pixmap.isNull() ) {
        m_animationStep = 0;
        m_animationBack = false;
        m_timer->start( 150 );
    }
    else
        stop();
}


void K3b::ListViewItemAnimator::stop()
{
    m_timer->stop();
}


void K3b::ListViewItemAnimator::setItem( Q3ListViewItem* item, int col )
{
    m_item = item;
    m_column = col;
    m_pixmap = *item->pixmap(col);
    m_fadeColor = item->listView()->palette().base().color();
    start();
}


void K3b::ListViewItemAnimator::setPixmap( const QPixmap& p )
{
    m_pixmap = p;
    start();
}


void K3b::ListViewItemAnimator::setColumn( int col )
{
    m_column = col;
    start();
}


void K3b::ListViewItemAnimator::setFadeColor( const QColor& c )
{
    m_fadeColor = c;
    start();
}


void K3b::ListViewItemAnimator::slotAnimate()
{
    if( m_item->isVisible() ) {
        double val = (double)m_animationStep;
        val /= 10.0;
#ifdef __GNUC__
#warning "Need to port qimageblitz"
#endif
#if 0
        // we need a temp pixmap since KPixmapEffect changes our pixmap
        KPixmap pix( m_pixmap );
        m_item->setPixmap( m_column, KPixmapEffect::fade( pix, val, m_fadeColor ) );;
#endif
    }

    if( m_animationBack ) {
        --m_animationStep;
        if( m_animationStep < 0 ) {
            // two steps full
            m_animationStep = 0;
            m_animationBack = false;
        }
    }
    else {
        ++m_animationStep;
        // do not fade it completely
        if( m_animationStep > 9 ) {
            m_animationStep = 8;
            m_animationBack = true;
        }
    }
}

#include "k3blistviewitemanimator.moc"
