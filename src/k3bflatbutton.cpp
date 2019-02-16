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

#include <KIconLoader>

#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QAction>
#include <QFrame>
#include <QToolTip>


namespace {
    
    const int FRAME_WIDTH = 1;
    const int FRAME_HEIGHT = 1;
    const int ICON_LABEL_SPACE = 5;
    const int MARGIN_SIZE = 5;
    
} // namespace


K3b::FlatButton::FlatButton( QWidget* parent)
    : QAbstractButton( parent )
{
    init();
}


K3b::FlatButton::FlatButton( const QString& text, QWidget* parent )
    : QAbstractButton( parent )
{
    setText( text );
    init();
}


K3b::FlatButton::FlatButton( QAction* action, QWidget* parent )
    : QAbstractButton( parent )
{
    setText( action->text() );
    setToolTip( action->toolTip() );
    setIcon( action->icon() );
    init();

    connect( this, SIGNAL(clicked(bool)), action, SLOT(trigger()) );
}


K3b::FlatButton::~FlatButton()
{
}


void K3b::FlatButton::init()
{
    setContentsMargins( MARGIN_SIZE + FRAME_WIDTH, MARGIN_SIZE + FRAME_HEIGHT,
                        MARGIN_SIZE + FRAME_WIDTH, MARGIN_SIZE + FRAME_HEIGHT );
    setIconSize( QSize( KIconLoader::SizeMedium, KIconLoader::SizeMedium ) );
    setHover(false);

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );
    slotThemeChanged();
}


bool K3b::FlatButton::event( QEvent *event )
{
    if( event->type() == QEvent::StyleChange ) {
        slotThemeChanged();
    }
    return QAbstractButton::event(event);
}


void K3b::FlatButton::enterEvent( QEvent* )
{
    setHover(true);
}


void K3b::FlatButton::leaveEvent( QEvent* )
{
    setHover(false);
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
    return QSize( qMax( iconSize().width(), fontMetrics().width( text() ) ) + ( MARGIN_SIZE + FRAME_WIDTH )*2,
                  iconSize().height() + fontMetrics().height() + ICON_LABEL_SPACE + ( MARGIN_SIZE + FRAME_HEIGHT )*2 );
}


void K3b::FlatButton::paintEvent( QPaintEvent* event )
{
    QPainter p( this );
    p.setPen( m_hover ? m_backColor : m_foreColor );
    p.fillRect( event->rect(), m_hover ? m_foreColor : m_backColor );
    p.drawRect( event->rect().x(), event->rect().y(), event->rect().width()-1, event->rect().height()-1 );
    
    QRect rect = contentsRect();

    if( !icon().isNull() ) {
        int pixX = rect.left() + qMax( 0, (rect.width() - iconSize().width()) / 2 );
        int pixY = rect.top();
        p.drawPixmap( pixX, pixY, icon().pixmap( iconSize() ) );
        p.drawText( rect, Qt::AlignBottom | Qt::AlignHCenter | Qt::TextHideMnemonic, text() );
    }
    else {
        p.drawText( rect, Qt::AlignCenter | Qt::TextHideMnemonic, text() );
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


