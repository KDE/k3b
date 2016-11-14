/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bthemedlabel.h"
#include "k3bapplication.h"
#include <QDebug>
K3b::ThemedLabel::ThemedLabel( QWidget* parent )
    : KSqueezedTextLabel( parent ),
      m_themePixmapCode( -1 )
{
    slotThemeChanged();
    setTextElideMode( Qt::ElideRight );

    if (k3bappcore) {
        connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
                 this, SLOT(slotThemeChanged()) );
    }
}


K3b::ThemedLabel::ThemedLabel( const QString& text, QWidget* parent )
    : KSqueezedTextLabel( text, parent ),
      m_themePixmapCode( -1 )
{
    slotThemeChanged();
    setTextElideMode( Qt::ElideRight );

    if (k3bappcore) {
        connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
                 this, SLOT(slotThemeChanged()) );
    }
}


K3b::ThemedLabel::ThemedLabel( K3b::Theme::PixmapType pix, QWidget* parent )
    : KSqueezedTextLabel( parent )
{
    setThemePixmap( pix );
    setTextElideMode( Qt::ElideRight );

    if (k3bappcore) {
        connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
                 this, SLOT(slotThemeChanged()) );
    }
}


bool K3b::ThemedLabel::event( QEvent *event )
{
    if( event->type() == QEvent::StyleChange ) {
        slotThemeChanged();
    }
    return KSqueezedTextLabel::event( event );
}


void K3b::ThemedLabel::setThemePixmap( K3b::Theme::PixmapType pix )
{
    m_themePixmapCode = pix;
    slotThemeChanged();
}


void K3b::ThemedLabel::slotThemeChanged()
{
    setAutoFillBackground( true );
    if (k3bappcore == Q_NULLPTR)
        return;
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
        QPalette p = palette();
        p.setColor( backgroundRole(), theme->backgroundColor() );
        p.setColor( foregroundRole(), theme->foregroundColor() );
        setPalette( p );
        if( m_themePixmapCode > -1 ) {
            setPixmap( theme->pixmap( (K3b::Theme::PixmapType)m_themePixmapCode ) );
            setScaledContents( false );
            if ( pixmap() ) {
                setFixedSize( pixmap()->size() );
            }
        }
    }
}


