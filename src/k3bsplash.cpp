/* 
 *
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

#include "k3bsplash.h"

#include <k3bthememanager.h>
#include <k3bapplication.h>

#include <qapplication.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qevent.h>
#include <qstring.h>
#include <qfontmetrics.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QDesktopWidget>

#include <kstandarddirs.h>
#include <kapplication.h>
#include <kaboutdata.h>


K3bSplash::K3bSplash( QWidget* parent )
  : Q3VBox( parent,"", 
	   Qt::WStyle_Customize|
	   Qt::WDestructiveClose|
	   /*	   Qt::WStyle_Splash|*/
	   Qt::X11BypassWindowManagerHint|
	   Qt::WStyle_NoBorder|
	   Qt::WStyle_StaysOnTop )
{
  setMargin( 0 );
  setSpacing( 0 );

  QLabel* copyrightLabel = new QLabel( KGlobal::mainComponent().aboutData()->copyrightStatement(), this );
  copyrightLabel->setMargin( 5 );
  copyrightLabel->setPaletteBackgroundColor( Qt::black );
  copyrightLabel->setPaletteForegroundColor( Qt::white );
  copyrightLabel->setAlignment( Qt::AlignRight );

  QLabel* picLabel = new QLabel( this );
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    picLabel->setPaletteBackgroundColor( theme->backgroundColor() );
    picLabel->setPixmap( theme->pixmap( K3bTheme::SPLASH ) );
  }

  m_infoBox = new QLabel( this );
  m_infoBox->setMargin( 5 );
  m_infoBox->setPaletteBackgroundColor( Qt::black );
  m_infoBox->setPaletteForegroundColor( Qt::white );

  // Set geometry, with support for Xinerama systems
  QRect r;
  r.setSize(sizeHint());
  int ps = QApplication::desktop()->primaryScreen();
  r.moveCenter( QApplication::desktop()->screenGeometry(ps).center() );
  setGeometry(r);
}


K3bSplash::~K3bSplash()
{
}


void K3bSplash::mousePressEvent( QMouseEvent* )
{
  close();
}


void K3bSplash::show()
{
  Q3VBox::show();
  // make sure the splash screen is shown immediately
  qApp->processEvents();
}


void K3bSplash::addInfo( const QString& s )
{
  m_infoBox->setText( s );

  qApp->processEvents();
}


// void K3bSplash::paintEvent( QPaintEvent* e )
// {
//   // first let the window paint the background and the child widget
//   QWidget::paintEvent( e );

//   // now create the text we want to display
//   // find the lower left corner and paint it on top of the pixmap
//   QPainter p( this );
//   p.setPen( Qt::blue );

//   QFontMetrics fm = p.fontMetrics();

//   QString line1 = QString( "K3b version %1" ).arg(VERSION);
//   QString line2( "(c) 2001 by Sebastian Trueg" );
//   QString line3( "licenced under the GPL" );

//   QRect rect1 = fm.boundingRect( line1 );
//   QRect rect2 = fm.boundingRect( line2 );
//   QRect rect3 = fm.boundingRect( line3 );

//   int textH = rect1.height() + rect2.height() + rect3.height() + 2 * fm.leading() + 2 + rect2.height() /*hack because the boundingRect method seems not to work properly! :-(*/;
//   int textW = qMax( rect1.width(), qMax( rect2.width(), rect3.width() ) ) + 2;

//   int startX = 10;
//   int startY = height() - 10 - textH;

//   p.drawText( startX, startY, textW, textH, 0, QString("%1\n%2\n%3").arg(line1).arg(line2).arg(line3) );
// }


#include "k3bsplash.moc"
