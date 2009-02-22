/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include <kaction.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <KGlobalSettings>

#include <qpainter.h>
#include <qtooltip.h>
#include <qfontmetrics.h>
#include <qpixmap.h>
#include <QEvent>
#include <QMouseEvent>
#include <QFrame>


K3b::FlatButton::FlatButton( QWidget *parent)
    : QFrame( parent/*, WNoAutoErase*/ ),
      m_pressed(false)
{
    init();
}


K3b::FlatButton::FlatButton( const QString& text, QWidget *parent )
    : QFrame( parent/*, WNoAutoErase*/ ),
      m_pressed(false)
{
    init();
    setText( text );
}


K3b::FlatButton::FlatButton( QAction* a, QWidget *parent)
    : QFrame( parent/*, WNoAutoErase*/ ),
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
    setHover(false);
    setMargin(5);
    setFrameStyle( QFrame::Box|QFrame::Plain );
    setAutoFillBackground( true );

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );
    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()), this, SLOT(slotThemeChanged()) );
    slotThemeChanged();
}


void K3b::FlatButton::setMargin( int margin )
{
    m_margin = margin;
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
    QPalette pal( palette() );
    if( b ) {
        pal.setColor( QPalette::WindowText, m_backColor );
        pal.setColor( QPalette::Window, m_foreColor );
    } else {
        pal.setColor( QPalette::WindowText, m_foreColor );
        pal.setColor( QPalette::Window, m_backColor );
    }
    setPalette( pal );

    m_hover = b;

    update();
}


QSize K3b::FlatButton::sizeHint() const
{
    // height: pixmap + 5 spacing + font height + frame width
    // width: max( pixmap, text) + frame width
    return QSize( qMax( m_pixmap.width(), fontMetrics().width( m_text ) ) + ( m_margin + frameWidth() )*2,
                  m_pixmap.height() + fontMetrics().height() + 5 + ( m_margin + frameWidth() )*2 );
}


void K3b::FlatButton::paintEvent ( QPaintEvent * event )
{
    QFrame::paintEvent( event );
    QRect rect = contentsRect();
    rect.setLeft( rect.left() + m_margin );
    rect.setRight( rect.right() - m_margin );
    rect.setTop( rect.top() + m_margin );
    rect.setBottom( rect.bottom() - m_margin );

    QPainter p(this);
    QRect textRect = fontMetrics().boundingRect( m_text );
    int textX = qMax( 0, ( rect.width() - textRect.width() ) / 2 );
    int textY = textRect.height();

    if( !m_pixmap.isNull() ) {
        p.translate( rect.left(), rect.top() );
        textX = qMax( textX, (m_pixmap.width() - textRect.width()) / 2 );
        textY += 5 + m_pixmap.height();

        int pixX = qMax( qMax( 0, (textRect.width() - m_pixmap.width()) / 2 ),
                         ( rect.width() - m_pixmap.width() ) / 2 );
        p.drawPixmap( pixX, 0, m_pixmap );
        p.drawText( textX, textY, m_text );
    }
    else
        p.drawText( rect, Qt::AlignCenter, m_text );

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
