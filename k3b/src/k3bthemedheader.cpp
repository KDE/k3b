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

#include "k3bthemedheader.h"
#include "k3bthememanager.h"
#include "k3bapplication.h"

#include <k3btitlelabel.h>

#include <qlabel.h>
#include <qlayout.h>


K3bThemedHeader::K3bThemedHeader( QWidget* parent )
  : QFrame( parent )
{
  init();
}


K3bThemedHeader::K3bThemedHeader( const QString& title, const QString& subtitle, QWidget* parent )
  : QFrame( parent )
{
  setTitle( title );
  setSubTitle( subtitle );

  init();
}


K3bThemedHeader::~K3bThemedHeader()
{
}

 
void K3bThemedHeader::setTitle( const QString& title, const QString& subtitle )
{
  m_titleLabel->setTitle( title, subtitle );
}


void K3bThemedHeader::setSubTitle( const QString& subtitle )
{
  m_titleLabel->setSubTitle( subtitle );
}


void K3bThemedHeader::setLeftPixmap( K3bTheme::PixmapType p )
{
  m_leftPix = p;
  slotThemeChanged();
}


void K3bThemedHeader::setRightPixmap( K3bTheme::PixmapType p )
{
  m_rightPix = p;
  slotThemeChanged();
}


void K3bThemedHeader::setAlignment( int align )
{
  m_titleLabel->setAlignment( align );
}


void K3bThemedHeader::init()
{
  setFrameShape( QFrame::StyledPanel );
  setFrameShadow( QFrame::Sunken );
  setLineWidth( 1 );
  setMargin( 1 );

  QHBoxLayout* layout = new QHBoxLayout( this );
  layout->setMargin( 2 ); // to make sure the frame gets displayed
  layout->setSpacing( 0 );

  m_leftLabel = new QLabel( this );
  m_leftLabel->setScaledContents( false );
  m_titleLabel = new K3bTitleLabel( this );
  m_rightLabel = new QLabel( this );
  m_rightLabel->setScaledContents( false );

  layout->addWidget( m_leftLabel );
  layout->addWidget( m_titleLabel );
  layout->setStretchFactor( m_titleLabel, 1 );
  layout->addWidget( m_rightLabel );

  m_leftPix = K3bTheme::DIALOG_LEFT;
  m_rightPix = K3bTheme::DIALOG_RIGHT;

  slotThemeChanged();

  connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
	   this, SLOT(slotThemeChanged()) );
  connect( kapp, SIGNAL(appearanceChanged()),
	   this, SLOT(slotThemeChanged()) );
}


void K3bThemedHeader::slotThemeChanged()
{
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
//     setPaletteBackgroundColor( theme->backgroundColor() );
//     setPaletteForegroundColor( theme->foregroundColor() );

    m_leftLabel->setPaletteBackgroundColor( theme->backgroundColor() );
    m_leftLabel->setPixmap( theme->pixmap( m_leftPix ) );
    m_rightLabel->setPaletteBackgroundColor( theme->backgroundColor() );
    m_rightLabel->setPixmap( theme->pixmap( m_rightPix ) );
    m_titleLabel->setPaletteBackgroundColor( theme->backgroundColor() );
    m_titleLabel->setPaletteForegroundColor( theme->foregroundColor() );
  }
}

#include "k3bthemedheader.moc"
