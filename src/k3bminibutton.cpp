/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * K3b::MiniButton is based on KDockButton_Private
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

#include <QPainter>
#include <QEvent>


K3b::MiniButton::MiniButton( QWidget* parent )
    :QPushButton( parent ),
     m_mouseOver( false )
{
    setFocusPolicy( Qt::NoFocus );
}

K3b::MiniButton::~MiniButton()
{
}

void K3b::MiniButton::paintEvent( QPaintEvent* )
{
    QPainter painter(this);
    drawButton(&painter);
}

void K3b::MiniButton::drawButton( QPainter* p )
{
    p->fillRect( 0,0, width(), height(), parentWidget()->palette().color( parentWidget()->backgroundRole() ) );
    QPixmap pixmap = icon().pixmap( width(), height() );
    p->drawPixmap( (width()-pixmap.width()) / 2, (height()-pixmap.height()) / 2, pixmap );

    QPainterPath pathNW;
    pathNW.moveTo( 0, height() - 1 );
    pathNW.lineTo( 0, 0 );
    pathNW.lineTo( width() - 1, 0 );

    QPainterPath pathSE;
    pathSE.moveTo( width() - 1, 0 );
    pathSE.lineTo( width() - 1, height() - 1 );
    pathSE.lineTo( 0, height() - 1 );

    if( m_mouseOver && !isDown() ){
        p->setPen( Qt::white );
        p->drawPath( pathNW );
        p->setPen( palette().dark().color() );
        p->drawPath( pathSE );
    }
    if( isChecked() || isDown() ){
        p->setPen( palette().dark().color() );
        p->drawPath( pathNW );
        p->setPen( Qt::white );
        p->drawPath( pathSE );
    }
}

void K3b::MiniButton::enterEvent( QEvent* )
{
    m_mouseOver = true;
    repaint();
}


void K3b::MiniButton::leaveEvent( QEvent* )
{
    m_mouseOver = false;
    repaint();
}

#include "k3bminibutton.moc"
