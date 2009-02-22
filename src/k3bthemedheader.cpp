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

#include "k3bthemedheader.h"
#include "k3bthememanager.h"
#include "k3bapplication.h"

#include <k3btitlelabel.h>

#include <KGlobalSettings>
#include <qlabel.h>
#include <qlayout.h>
#include <QHBoxLayout>


K3b::ThemedHeader::ThemedHeader( QWidget* parent )
    : QFrame( parent )
{
    init();
}


K3b::ThemedHeader::ThemedHeader( const QString& title, const QString& subtitle, QWidget* parent )
    : QFrame( parent )
{
    setTitle( title );
    setSubTitle( subtitle );

    init();
}


K3b::ThemedHeader::~ThemedHeader()
{
}


void K3b::ThemedHeader::setTitle( const QString& title, const QString& subtitle )
{
    m_titleLabel->setTitle( title, subtitle );
}


void K3b::ThemedHeader::setSubTitle( const QString& subtitle )
{
    m_titleLabel->setSubTitle( subtitle );
}


void K3b::ThemedHeader::setLeftPixmap( K3b::Theme::PixmapType p )
{
    m_leftPix = p;
    slotThemeChanged();
}


void K3b::ThemedHeader::setRightPixmap( K3b::Theme::PixmapType p )
{
    m_rightPix = p;
    slotThemeChanged();
}


void K3b::ThemedHeader::setAlignment( int align )
{
    m_titleLabel->setAlignment( align );
}


void K3b::ThemedHeader::init()
{
    setFrameShape( QFrame::StyledPanel );
    setFrameShadow( QFrame::Sunken );
    setLineWidth( 1 );
    //setMargin( 1 );

    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setMargin( 2 ); // to make sure the frame gets displayed
    layout->setSpacing( 0 );

    m_leftLabel = new QLabel( this );
    m_leftLabel->setScaledContents( false );
    m_leftLabel->setAutoFillBackground( true );

    m_titleLabel = new K3b::TitleLabel( this );

    m_rightLabel = new QLabel( this );
    m_rightLabel->setScaledContents( false );
    m_rightLabel->setAutoFillBackground( true );

    layout->addWidget( m_leftLabel );
    layout->addWidget( m_titleLabel );
    layout->setStretchFactor( m_titleLabel, 1 );
    layout->addWidget( m_rightLabel );

    m_leftPix = K3b::Theme::DIALOG_LEFT;
    m_rightPix = K3b::Theme::DIALOG_RIGHT;

    slotThemeChanged();

    connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
             this, SLOT(slotThemeChanged()) );
    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()),
             this, SLOT(slotThemeChanged()) );
}


void K3b::ThemedHeader::slotThemeChanged()
{
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
        m_leftLabel->setPalette( theme->palette() );
        m_leftLabel->setPixmap( theme->pixmap( m_leftPix ) );

        m_rightLabel->setPalette( theme->palette() );
        m_rightLabel->setPixmap( theme->pixmap( m_rightPix ) );

        m_titleLabel->setPalette( theme->palette() );
    }
}

#include "k3bthemedheader.moc"
