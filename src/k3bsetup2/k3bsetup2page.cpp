/* 
 *
 * $Id: $
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

/***************************************************************************
                          k3bsetup2page.cpp
                                   -
                       Abstract base class for all pages
                             -------------------
    begin                : Sun Aug 25 13:19:59 CEST 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "k3bsetup2page.h"

#include <tools/k3blistview.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qevent.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qfont.h>

#include <kstandarddirs.h>
#include <kdialog.h>


class K3bSetup2PixmapHeader : public QWidget
{
public:
  K3bSetup2PixmapHeader( const QString& text, QWidget* parent = 0 )
    : QWidget( parent ) {
    setFixedWidth( 175 ); // width of the setup pixmaps
    setPaletteBackgroundColor( QColor(201, 208, 255) );  // background color of the setup pixmaps

    m_label = new QLabel( text, this );
    m_label->setPaletteBackgroundColor( QColor(201, 208, 255) );
    m_label->setAlignment( AlignCenter | WordBreak );
    m_label->setIndent( 10 );
    QFont f(m_label->font());
    f.setBold(true);
    m_label->setFont(f);

    m_label->setFixedWidth( 166 - 2*block().width() );
    m_label->move( block().width() + 5, 15 );
  }

  QSize sizeHint() const {
    int tileHeight = 15 + m_label->height();
    if( tileHeight%block().height() )
      tileHeight += block().height() - (tileHeight%block().height());

    return QSize( 175, tileHeight );
  }

protected:
  void paintEvent( QPaintEvent* ) {
    QPainter p( this );

    p.drawTiledPixmap( 5, 
		       1, 
		       166, 
		       block().height(), 
		       block() );

    p.drawTiledPixmap( 6, 
		       1, 
		       block().width(), 
		       sizeHint().height(), 
		       block() );

    p.drawTiledPixmap( 171-block().width(), 
		       1, 
		       block().width(), 
		       sizeHint().height(), 
		       block() );
  }

  const QPixmap& block() const {
    static const QPixmap block( locate( "data", "k3b/pics/k3bsetup_header_block.png" ) );
    return block;
  }

private:
  QLabel* m_label;
};


K3bSetup2Page::K3bSetup2Page( K3bListView* taskView, const QString& header, QWidget* parent, const char* name )
  : QWidget( parent, name ), m_taskView( taskView )
{
  m_headerLabel = new K3bSetup2PixmapHeader( header, this );
//   m_headerLabel->setAlignment( AlignCenter | WordBreak );
//   m_headerLabel->setIndent( 10 );
//   m_headerLabel->setPaletteBackgroundColor( QColor(201, 208, 255) );

  m_pixmapLabel = new QLabel( this );
  m_pixmapLabel->setScaledContents( false );

  static const QPixmap setupLogo( locate( "data", "k3b/pics/k3bsetup_1.png" ) );
  m_pixmapLabel->setPixmap( setupLogo );

  m_mainWidget = new QWidget( this );

  QGridLayout* mainGrid = new QGridLayout( this );
  mainGrid->setMargin( 0 );
  mainGrid->setSpacing( 0 );

  mainGrid->addWidget( m_headerLabel, 0, 0 );
  mainGrid->addWidget( m_pixmapLabel, 1, 0 );
  mainGrid->addColSpacing( 1, KDialog::marginHint() );
  mainGrid->addMultiCellWidget( m_mainWidget, 0, 1, 2, 2 );
}


K3bSetup2Page::~K3bSetup2Page()
{
}


void K3bSetup2Page::setPixmap( const QPixmap& p )
{
  m_pixmapLabel->setPixmap( p );
}


void K3bSetup2Page::load( KConfig* )
{
}

bool K3bSetup2Page::save( KConfig* )
{
  return true;
}


#include "k3bsetup2page.moc"
