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
#include <k3bthememanager.h>

#include <qpixmap.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qsimplerichtext.h>
#include <qptrlist.h>

#include <kurl.h>
#include <kurldrag.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kconfig.h>



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
}


void K3bWelcomeWidget::Display::rebuildGui( const KActionPtrList& actions )
{
  // step 1: delete all old buttons in the buttons QPtrList<QToolButton>
  m_buttons.setAutoDelete(true);
  m_buttons.clear();

  int numActions = actions.count();
  if( numActions > 0 ) {
    // step 2: calculate rows and columns
    // for now we simply create 1-3 rows and as may cols as neccessary
    int cols = 0, rows = 0;
    if( numActions < 3 )
      rows = 1;
    else if( numActions > 6 )
      rows = 3;
    else
      rows = 2;

    cols = numActions/rows;
    if( numActions%rows )
      cols++;

    // step 3: create buttons
    for( KActionPtrList::const_iterator it = actions.begin(); it != actions.end(); ++it ) {
      KAction* a = *it;

      QToolButton* b = new QToolButton( this );
      b->setTextLabel( a->toolTip(), true );
      b->setTextLabel( a->text() );
      b->setIconSet( a->iconSet(KIcon::Desktop) );
      b->setUsesTextLabel( true );
      b->setUsesBigPixmap( true );
      b->setAutoRaise( true );
      b->setTextPosition( QToolButton::Under );

      connect( b, SIGNAL(clicked()), a, SLOT(activate()) );

      m_buttons.append( b );
    }

    // step 4: calculate button size
    // determine the needed button size (since all buttons should be equal in size
    // we use the max of all sizes)
    QSize buttonSize = m_buttons.first()->sizeHint();
    for( QPtrListIterator<QToolButton> it( m_buttons ); it.current(); ++it ) {
      buttonSize.expandedTo( it.current()->sizeHint() );
    }

    // step 5: position buttons
    // starting rect
    int row = 0;
    int col = 0;
    for( QPtrListIterator<QToolButton> it( m_buttons ); it.current(); ++it ) {
      QToolButton* b = it.current();

      b->setGeometry( QRect( QPoint( 80+(col*buttonSize.width()), 80+(row*buttonSize.height()) ), 
			     buttonSize ) );
      col++;
      if( col == cols ) {
	col = 0;
	row++;
      }
    }

    // step 6: calculate widget size
    m_size = QSize( QMAX(20+m_header->widthUsed(), 80+(buttonSize.width()*cols)),
		    80+(buttonSize.height()*rows) );
  }
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

  connect( main, SIGNAL(dropped(const KURL::List&)), m_mainWindow, SLOT(addUrls(const KURL::List&)) );

  connect( k3bthememanager, SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );

  slotThemeChanged();
}


K3bWelcomeWidget::~K3bWelcomeWidget()
{
}


void K3bWelcomeWidget::loadConfig( KConfig* c )
{
  c->setGroup( "Welcome Widget" );
  QStringList sl = c->readListEntry( "welcome_actions" );

  if( sl.isEmpty() ) {
    sl.append( "file_new_audio" );
    sl.append( "file_new_data" );
    sl.append( "file_new_dvd" );
    sl.append( "tools_copy_cd" );
  }

  KActionPtrList actions;
  for( QStringList::const_iterator it = sl.begin(); it != sl.end(); ++it )
    if( KAction* a = m_mainWindow->actionCollection()->action( *it ) )
      actions.append(a);

  main->rebuildGui( actions );
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


void K3bWelcomeWidget::slotThemeChanged()
{
  if( K3bTheme* theme = k3bthememanager->currentTheme() ) {
    main->setPaletteBackgroundPixmap( theme->pixmap( "k3b_3d_logo" ) );
  }
}


#include "k3bwelcomewidget.moc"
