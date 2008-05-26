/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * K3bMiniButton is based on KDockButton_Private
 * Copyright (C) 2000 Max Judin <novaprint@mtu-net.ru>
 * Copyright (C) 2002,2003 Joseph Wenninger <jowenn@kde.org>
 * Copyright (C) 2005 Dominik Haumann <dhdev@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bminibutton.h"

#include <qpainter.h>
//Added by qt3to4:
#include <QEvent>


K3bMiniButton::K3bMiniButton( QWidget *parent )
    :QPushButton( parent ),
     m_mouseOver( false )
{
    setFocusPolicy( Qt::NoFocus );
}

K3bMiniButton::~K3bMiniButton()
{
}

void K3bMiniButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    drawButton(&painter);
}

void K3bMiniButton::drawButton( QPainter* p )
{
    p->fillRect( 0,0, width(), height(), palette().base() );
    p->drawPixmap( (width() - pixmap()->width()) / 2, (height() - pixmap()->height()) / 2, *pixmap() );
    if( m_mouseOver && !isDown() ){
        p->setPen( Qt::white );
        QPainterPath path;
        path.moveTo( 0, height() - 1 );
        path.lineTo( 0, 0 );
        path.lineTo( width() - 1, 0 );

        p->setPen( palette().dark().color() );
        path.lineTo( width() - 1, height() - 1 );
        path.lineTo( 0, height() - 1 );
        p->drawPath(path);
    }
    if( isChecked() || isDown() ){
        p->setPen( palette().dark().color() );
        QPainterPath path;
        path.moveTo( 0, height() - 1 );
        path.lineTo( 0, 0 );
        path.lineTo( width() - 1, 0 );
        p->drawPath(path);

        QPainterPath pathWhite;
        p->setPen( Qt::white );
        pathWhite.lineTo( width() - 1, height() - 1 );
        pathWhite.lineTo( 0, height() - 1 );
        p->drawPath(pathWhite);
    }
}

void K3bMiniButton::enterEvent( QEvent * )
{
    m_mouseOver = true;
    repaint();
}


void K3bMiniButton::leaveEvent( QEvent * )
{
    m_mouseOver = false;
    repaint();
}

#include "k3bminibutton.moc"
