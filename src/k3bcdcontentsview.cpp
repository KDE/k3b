/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bcdcontentsview.h"

#include <kcutlabel.h>
#include <k3bthememanager.h>
#include <k3bstdguiitems.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>



K3bCdContentsView::K3bCdContentsView( bool withHeader,
				      QWidget* parent, const char* name )
  : QWidget( parent, name ),
    m_centerWidget(0),
    m_leftPixmap( K3bTheme::MEDIA_LEFT ),
    m_rightPixmap( K3bTheme::MEDIA_NONE )
{
  if( withHeader ) {
    QGridLayout* mainGrid = new QGridLayout( this );
    mainGrid->setMargin( 2 );
    mainGrid->setSpacing( 0 );

    QFrame* headerFrame = K3bStdGuiItems::purpleFrame( this );
    QHBoxLayout* headerLayout = new QHBoxLayout( headerFrame ); 
    headerLayout->setMargin( 2 );  // to make sure the frame gets displayed
    m_pixmapLabelLeft = new QLabel( headerFrame, "pixmapLabel1" );
    m_pixmapLabelLeft->setScaledContents( FALSE );
    headerLayout->addWidget( m_pixmapLabelLeft );

    m_labelTitle = new KCutLabel( headerFrame, "m_labelTitle" );
    m_labelTitle->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 1, 0, m_labelTitle->sizePolicy().hasHeightForWidth() ) );
    QFont m_labelTitle_font( m_labelTitle->font() );
    m_labelTitle_font.setPointSize( 12 );
    m_labelTitle_font.setBold( TRUE );
    m_labelTitle->setFont( m_labelTitle_font ); 
    headerLayout->addWidget( m_labelTitle );

    m_pixmapLabelRight = new QLabel( headerFrame, "pixmapLabel2" );
    m_pixmapLabelRight->setScaledContents( FALSE );
    headerLayout->addWidget( m_pixmapLabelRight );

    mainGrid->addWidget( headerFrame, 0, 0 );

    connect( k3bthememanager, SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );

    slotThemeChanged();
  }
}


K3bCdContentsView::~K3bCdContentsView()
{
}


QWidget* K3bCdContentsView::mainWidget()
{
  if( !m_centerWidget )
    setMainWidget( new QWidget( this ) );
  return m_centerWidget;
}


void K3bCdContentsView::reload()
{
}


void K3bCdContentsView::setMainWidget( QWidget* w )
{
  m_centerWidget = w;
  ((QGridLayout*)layout())->addWidget( w, 1, 0 );
}


void K3bCdContentsView::setTitle( const QString& s )
{
  m_labelTitle->setText( s );
}


void K3bCdContentsView::setLeftPixmap( K3bTheme::PixmapType s )
{
  m_leftPixmap = s;
  slotThemeChanged();
}


void K3bCdContentsView::setRightPixmap( K3bTheme::PixmapType s )
{
  m_rightPixmap = s;
  slotThemeChanged();
}


void K3bCdContentsView::slotThemeChanged()
{
  if( K3bTheme* theme = k3bthememanager->currentTheme() ) {
    m_pixmapLabelLeft->setPixmap( theme->pixmap( m_leftPixmap ) );
    m_labelTitle->setPaletteBackgroundColor( theme->backgroundColor() );
    m_labelTitle->setPaletteForegroundColor( theme->foregroundColor() );
    m_pixmapLabelRight->setPixmap( theme->pixmap( m_rightPixmap ) );
  }
}

#include "k3bcdcontentsview.moc"
