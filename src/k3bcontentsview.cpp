/* 
 *
 * $Id: k3bcdcontentsview.cpp 582796 2006-09-10 15:31:38Z trueg $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bcontentsview.h"

#include <k3bthemedheader.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>


K3bContentsView::K3bContentsView( bool withHeader,
				  QWidget* parent, 
				  const char* name )
  : QWidget( parent, name ),
    m_header(0),
    m_centerWidget(0)
{
  if( withHeader ) {
    QVBoxLayout* lay = new QVBoxLayout( this );
    lay->setMargin( 2 );
    lay->setSpacing( 0 );

    m_header = new K3bThemedHeader( this );
    lay->addWidget( m_header );

    m_header->setLeftPixmap( K3bTheme::MEDIA_LEFT );
    m_header->setRightPixmap( K3bTheme::MEDIA_NONE );
  }
}


K3bContentsView::~K3bContentsView()
{
}


void K3bContentsView::setMainWidget( QWidget* w )
{
  m_centerWidget = w;
  ((QVBoxLayout*)layout())->addWidget( w );
}


QWidget* K3bContentsView::mainWidget()
{
  if( !m_centerWidget )
    setMainWidget( new QWidget( this ) );
  return m_centerWidget;
}


void K3bContentsView::setTitle( const QString& s )
{
  if( m_header )
    m_header->setTitle( s );
}


void K3bContentsView::setLeftPixmap( K3bTheme::PixmapType s )
{
  if( m_header )
    m_header->setLeftPixmap( s );
}


void K3bContentsView::setRightPixmap( K3bTheme::PixmapType s )
{
  if( m_header )
    m_header->setRightPixmap( s );
}

#include "k3bcontentsview.moc"
