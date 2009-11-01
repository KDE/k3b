/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 *           (C)      2009 Michal Malek <michalm@jabster.pl>
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

#include "k3bflatbutton.h"
#include "k3bthememanager.h"
#include "k3bapplication.h"

#include <KGlobal>
#include <KGlobalSettings>
#include <KIconLoader>

#include <QAction>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QToolTip>


namespace {
    
    const int FRAME_WIDTH = 1;
    const int FRAME_HEIGHT = 1;
    const int ICON_LABEL_SPACE = 5;
    const int MARGIN_SIZE = 5;
    
} // namespace


K3b::FlatButton::FlatButton( QWidget *parent)
    : QWidget( parent ),
      m_pressed(false)
{
    init();
}


K3b::FlatButton::FlatButton( const QString& text, QWidget *parent )
    : QWidget( parent ),
      m_pressed(false)
{
    init();
    setText( text );
}


K3b::FlatButton::FlatButton( QAction* a, QWidget *parent)
    : QWidget( parent ),
      m_pressed(false)
{
    init();

    setText( a->text() );
    setToolTip( a->toolTip() );
    setPixmap( a->icon().pixmap( KIconLoader::SizeMedium, KIconLoader::SizeMedium ) );

    connect( this, SIGNAL(clicked()), a, SLOT(trigger()) );
}


K3b::FlatButton::~FlatButton() {}


void K3b::FlatButton::init()
{
    setContentsMargins( MARGIN_SIZE + FRAME_WIDTH, MARGIN_SIZE + FRAME_HEIGHT,
                        MARGIN_SIZE + FRAME_WIDTH, MARGIN_SIZE + FRAME_HEIGHT );
    setHover(false);

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );
    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()), this, SLOT(slotThemeChanged()) );
    slotThemeChanged();
}


void K3b::FlatButton::setText( const QString& s )
{
    m_text = s;
    m_text.remove( '&' );

    update();
}


void K3b::FlatButton::setPixmap( const QPixmap& p )
{
    m_pixmap = p;
    update();
}


void K3b::FlatButton::enterEvent( QEvent* )
{
    setHover(true);
}


void K3b::FlatButton::leaveEvent( QEvent* )
{
    setHover(false);
}


void K3b::FlatButton::mousePressEvent( QMouseEvent* e )
{
    if( e->button() == Qt::LeftButton ) {
        emit pressed();
        m_pressed = true;
    }
    else
        e->ignore();
}


void K3b::FlatButton::mouseReleaseEvent( QMouseEvent* e )
{
    if( e->button() == Qt::LeftButton ) {
        if( m_pressed  )
            emit clicked();
        m_pressed = false;
    }
    else
        e->ignore();
}


void K3b::FlatButton::setHover( bool b )
{
    m_hover = b;
    update();
}


QSize K3b::FlatButton::sizeHint() const
{
    // height: pixmap + spacing + font height + frame width
    // width: max( pixmap, text) + frame width
    return QSize( qMax( m_pixmap.width(), fontMetrics().width( m_text ) ) + ( MARGIN_SIZE + FRAME_WIDTH )*2,
                  m_pixmap.height() + fontMetrics().height() + ICON_LABEL_SPACE + ( MARGIN_SIZE + FRAME_HEIGHT )*2 );
}


void K3b::FlatButton::paintEvent ( QPaintEvent* event )
{
    QPainter p(this);
    p.setPen( m_hover ? m_backColor : m_foreColor );
    p.fillRect( event->rect(), m_hover ? m_foreColor : m_backColor );
    p.drawRect( event->rect().x(), event->rect().y(), event->rect().width()-1, event->rect().height()-1 );
    
    QRect rect = contentsRect();

    if( !m_pixmap.isNull() ) {
        int pixX = rect.left() + qMax( 0, (rect.width() - m_pixmap.width()) / 2 );
        int pixY = rect.top();
        p.drawPixmap( pixX, pixY, m_pixmap );
        p.drawText( rect, Qt::AlignBottom | Qt::AlignHCenter, m_text );
    }
    else {
        p.drawText( rect, Qt::AlignCenter, m_text );
    }
}


void K3b::FlatButton::setColors( const QColor& fore, const QColor& back )
{
    m_foreColor = fore;
    m_backColor = back;

    setHover( m_hover );
}


void K3b::FlatButton::slotThemeChanged()
{
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
        setColors( theme->foregroundColor(), theme->backgroundColor() );
    }
}

#include "k3bflatbutton.moc"
