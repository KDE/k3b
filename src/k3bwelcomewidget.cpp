/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bwelcomewidget.h"
#include "k3b.h"
#include <k3bstdguiitems.h>
#include <k3bcore.h>
#include <k3bversion.h>

#include <qpixmap.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qsimplerichtext.h>

#include <kurl.h>
#include <kurldrag.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kglobal.h>



K3bWelcomeWidget::Display::Display( QWidget* parent )
  : QWidget( parent )
{
  QFont fnt(font());
  fnt.setBold(true);
  fnt.setPointSize( 16 );
  m_header = new QSimpleRichText( i18n("Welcome to K3b %1 - The CD/DVD Burning Facility")
				  .arg( k3bcore->version() ), fnt );
  // set a large width just to be sure no linebreak occurs
  m_header->setWidth( 800 );

  setAcceptDrops( true );
  setBackgroundMode( PaletteBase );
  setPaletteBackgroundPixmap( locate( "appdata", "pics/k3b_3d_logo.png" ) );

  // now we add the buttons with fixed coordinates
  audioDocButton = new QToolButton( this );
  dataDocButton = new QToolButton( this );
  dataDvdDocButton = new QToolButton( this );
  copyCdButton = new QToolButton( this );

  audioDocButton->setTextLabel( i18n("Create Audio CD Project") );
  dataDocButton->setTextLabel( i18n("Create Data CD Project") );
  dataDvdDocButton->setTextLabel( i18n("Create Data DVD Project") );
  copyCdButton->setTextLabel( i18n("Copy CD") );

  audioDocButton->setIconSet( DesktopIcon( "sound" ) );
  dataDocButton->setIconSet( DesktopIcon( "tar" ) );
  dataDvdDocButton->setIconSet( DesktopIcon( "dvd_unmount" ) );
  copyCdButton->setIconSet( DesktopIcon( "cdcopy" ) );

  audioDocButton->setUsesTextLabel( true );
  dataDocButton->setUsesTextLabel( true );
  dataDvdDocButton->setUsesTextLabel( true );
  copyCdButton->setUsesTextLabel( true );

  audioDocButton->setUsesBigPixmap( true );
  dataDocButton->setUsesBigPixmap( true );
  dataDvdDocButton->setUsesBigPixmap( true );
  copyCdButton->setUsesBigPixmap( true );

  audioDocButton->setAutoRaise( true );
  dataDocButton->setAutoRaise( true );
  dataDvdDocButton->setAutoRaise( true );
  copyCdButton->setAutoRaise( true );

  audioDocButton->setTextPosition( QToolButton::Under );
  dataDocButton->setTextPosition( QToolButton::Under );
  dataDvdDocButton->setTextPosition( QToolButton::Under );
  copyCdButton->setTextPosition( QToolButton::Under );

  // determine the needed button size (since all buttons should be equal in size
  // we use the max of all sizes)
  QSize buttonSize = audioDocButton->sizeHint();
  buttonSize = buttonSize.expandedTo( dataDocButton->sizeHint() );
  buttonSize = buttonSize.expandedTo( dataDvdDocButton->sizeHint() );
  buttonSize = buttonSize.expandedTo( copyCdButton->sizeHint() );

  // position the buttons
  QRect r( QPoint(80, 80), buttonSize );
  audioDocButton->setGeometry( r );
  r.moveBy( buttonSize.width(), 0 );
  dataDvdDocButton->setGeometry( r );
  r.moveBy( 0, buttonSize.height() );
  copyCdButton->setGeometry( r );
  r.moveBy( -1 * buttonSize.width(), 0 );
  dataDocButton->setGeometry( r );

  m_size = QSize( QMAX(20+m_header->widthUsed(), 80+(audioDocButton->width()*2)),
		  80+(audioDocButton->height()*2) );
}


void K3bWelcomeWidget::Display::paintEvent( QPaintEvent* e )
{
  QWidget::paintEvent( e );

  QPainter p( this );
  m_header->draw( &p, 20, 20, QRect(), colorGroup() );
}


void K3bWelcomeWidget::Display::dragEnterEvent( QDragEnterEvent* event )
{
  event->accept( KURLDrag::canDecode(event) );
}


void K3bWelcomeWidget::Display::dropEvent( QDropEvent* e )
{
  KURL::List urls;
  KURLDrag::decode( e, urls );
  emit dropped( urls );
}



K3bWelcomeWidget::K3bWelcomeWidget( K3bMainWindow* mw, QWidget* parent, const char* name )
  : QScrollView( parent, name ),
    m_mainWindow( mw )
{
  main = new Display( viewport() );
  addChild( main );

//   viewport()->resize( 800, 400 );
//   main->resize( 800, 400 );
 
  // connect the buttons
  connect( main->audioDocButton, SIGNAL(clicked()), m_mainWindow, SLOT(slotNewAudioDoc()) );
  connect( main->dataDocButton, SIGNAL(clicked()), m_mainWindow, SLOT(slotNewDataDoc()) );
  connect( main->dataDvdDocButton, SIGNAL(clicked()), m_mainWindow, SLOT(slotNewDvdDoc()) );
  connect( main->copyCdButton, SIGNAL(clicked()), m_mainWindow, SLOT(slotCdCopy()) );

  connect( main, SIGNAL(dropped(const KURL::List&)), m_mainWindow, SLOT(addUrls(const KURL::List&)) );
}


K3bWelcomeWidget::~K3bWelcomeWidget()
{
}


void K3bWelcomeWidget::resizeEvent( QResizeEvent* e )
{
  QScrollView::resizeEvent( e );

  QRect r( contentsRect() );
  QSize s = r.size();
  if( s.width() < main->sizeHint().width() )
    s.setWidth( main->sizeHint().width() );
  if( s.height() < main->sizeHint().height() )
    s.setHeight( main->sizeHint().height() );

  main->resize( s );
  viewport()->resize( s );
}


// void K3bWelcomeWidget::slotMailClick( const QString& address, const QString& )
// {
//   kapp->invokeMailer( address, "K3b: " );
// }


#include "k3bwelcomewidget.moc"
