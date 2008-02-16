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

#include <KGlobalSettings>

K3bThemedLabel::K3bThemedLabel( QWidget* parent )
    : KSqueezedTextLabel( parent ),
      m_themePixmapCode( -1 )
{
    slotThemeChanged();
    setTextElideMode( Qt::ElideRight );

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
             this, SLOT(slotThemeChanged()) );
    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()),
             this, SLOT(slotThemeChanged()) );
}


K3bThemedLabel::K3bThemedLabel( const QString& text, QWidget* parent )
    : KSqueezedTextLabel( text, parent ),
      m_themePixmapCode( -1 )
{
    slotThemeChanged();
    setTextElideMode( Qt::ElideRight );

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
             this, SLOT(slotThemeChanged()) );
    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()),
             this, SLOT(slotThemeChanged()) );
}


K3bThemedLabel::K3bThemedLabel( K3bTheme::PixmapType pix, QWidget* parent )
    : KSqueezedTextLabel( parent )
{
    setThemePixmap( pix );
    setTextElideMode( Qt::ElideRight );

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
             this, SLOT(slotThemeChanged()) );
    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()),
             this, SLOT(slotThemeChanged()) );
}


void K3bThemedLabel::setThemePixmap( K3bTheme::PixmapType pix )
{
    m_themePixmapCode = pix;
    slotThemeChanged();
}


void K3bThemedLabel::slotThemeChanged()
{
    setAutoFillBackground( true );
    if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
        QPalette p = palette();
        p.setColor( backgroundRole(), theme->backgroundColor() );
        p.setColor( foregroundRole(), theme->foregroundColor() );
        setPalette( p );
        if( m_themePixmapCode > -1 ) {
            setPixmap( theme->pixmap( (K3bTheme::PixmapType)m_themePixmapCode ) );
            setScaledContents( false );
            if ( pixmap() ) {
                setFixedSize( pixmap()->size() );
            }
        }
    }
}

#include "k3bthemedlabel.moc"
