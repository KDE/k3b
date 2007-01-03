/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bthemedlabel.h"
#include "k3bapplication.h"

K3bThemedLabel::K3bThemedLabel( QWidget* parent )
  : KCutLabel( parent ),
    m_themePixmapCode( -1 )
{
  slotThemeChanged();

  connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
	   this, SLOT(slotThemeChanged()) );
  connect( kapp, SIGNAL(appearanceChanged()),
	   this, SLOT(slotThemeChanged()) );
}


K3bThemedLabel::K3bThemedLabel( const QString& text, QWidget* parent )
  : KCutLabel( text, parent ),
    m_themePixmapCode( -1 )
{
  slotThemeChanged();

  connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
	   this, SLOT(slotThemeChanged()) );
  connect( kapp, SIGNAL(appearanceChanged()),
	   this, SLOT(slotThemeChanged()) );
}


K3bThemedLabel::K3bThemedLabel( K3bTheme::PixmapType pix, QWidget* parent )
  : KCutLabel( parent )
{
  setThemePixmap( pix );

  connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
	   this, SLOT(slotThemeChanged()) );
  connect( kapp, SIGNAL(appearanceChanged()),
	   this, SLOT(slotThemeChanged()) );
}


void K3bThemedLabel::setThemePixmap( K3bTheme::PixmapType pix )
{
  m_themePixmapCode = pix;
  slotThemeChanged();
}


void K3bThemedLabel::slotThemeChanged()
{
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    setPaletteBackgroundColor( theme->backgroundColor() );
    setPaletteForegroundColor( theme->foregroundColor() );
    if( m_themePixmapCode > -1 ) {
      setPixmap( theme->pixmap( (K3bTheme::PixmapType)m_themePixmapCode ) );
      setScaledContents( false );
    }
  }
}

#include "k3bthemedlabel.moc"
