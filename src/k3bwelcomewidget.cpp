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
#include <qmap.h>

#include <kurl.h>
#include <kurldrag.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kpopupmenu.h>


K3bWelcomeWidget::Display::Display( QWidget* parent )
  : QWidget( parent )
{
  QFont fnt(font());
  fnt.setBold(true);
  fnt.setPointSize( 16 );
  m_header = new QSimpleRichText( i18n("Welcome to K3b %1 - The CD and DVD Kreator")
				  .arg( k3bcore->version() ), fnt );
  // set a large width just to be sure no linebreak occurs
  m_header->setWidth( 800 );

  setAcceptDrops( true );
  setBackgroundMode( PaletteBase );
}


void K3bWelcomeWidget::Display::setHeaderBackgroundColor( const QColor& c )
{
  m_headerBgColor = c;
  update();
}


void K3bWelcomeWidget::Display::setHeaderForegroundColor( const QColor& c )
{
  m_headerFgColor = c;
  update();
}


void K3bWelcomeWidget::Display::addAction( KAction* action )
{
  if( action ) {
    m_actions.append(action);
    rebuildGui();
  }
}


void K3bWelcomeWidget::Display::removeAction( KAction* action )
{
  if( action ) {
    m_actions.removeRef( action );
    rebuildGui();
  }
}


void K3bWelcomeWidget::Display::removeButton( QToolButton* b )
{
  removeAction( m_buttonMap[b] );
}


void K3bWelcomeWidget::Display::rebuildGui( const QPtrList<KAction>& actions )
{
  m_actions = actions;
  rebuildGui();
}


void K3bWelcomeWidget::Display::rebuildGui()
{
  // step 1: delete all old buttons in the buttons QPtrList<QToolButton>
  m_buttonMap.clear();
  m_buttons.setAutoDelete(true);
  m_buttons.clear();

  int numActions = m_actions.count();
  if( numActions > 0 ) {
    // step 2: calculate rows and columns
    // for now we simply create 1-3 rows and as may cols as neccessary
    m_cols = 0;
    int rows = 0;
    if( numActions < 3 )
      rows = 1;
    else if( numActions > 6 )
      rows = 3;
    else
      rows = 2;

    m_cols = numActions/rows;
    if( numActions%rows )
      m_cols++;

    // step 3: create buttons
    for( QPtrListIterator<KAction> it( m_actions ); it.current(); ++it ) {
      KAction* a = it.current();

      QToolButton* b = new QToolButton( this );
      b->setTextLabel( a->toolTip(), true );
      b->setTextLabel( a->text(), false );
      b->setIconSet( a->iconSet(KIcon::Desktop) );
      b->setUsesTextLabel( true );
      b->setUsesBigPixmap( true );
      b->setAutoRaise( true );
      b->setTextPosition( QToolButton::Under );

      connect( b, SIGNAL(clicked()), a, SLOT(activate()) );

      m_buttons.append( b );
      m_buttonMap.insert( b, a );
    }

    // step 4: calculate button size
    // determine the needed button size (since all buttons should be equal in size
    // we use the max of all sizes)
    m_buttonSize = m_buttons.first()->sizeHint();
    for( QPtrListIterator<QToolButton> it( m_buttons ); it.current(); ++it ) {
      m_buttonSize = m_buttonSize.expandedTo( it.current()->sizeHint() );
    }

    // step 5: position buttons
    // starting rect
    repositionButtons();

    // step 6: calculate widget size
    m_size = QSize( QMAX(40+m_header->widthUsed(), 160+(m_buttonSize.width()*m_cols)),
		    160+(m_buttonSize.height()*rows) );
  }
}


void K3bWelcomeWidget::Display::repositionButtons()
{
  int leftMargin = QMAX( 80, (width() - (m_buttonSize.width()*m_cols))/2 );

  int row = 0;
  int col = 0;
  for( QPtrListIterator<QToolButton> it( m_buttons ); it.current(); ++it ) {
    QToolButton* b = it.current();
    
    b->setGeometry( QRect( QPoint( leftMargin+(col*m_buttonSize.width()), 80+(row*m_buttonSize.height()) ), 
			   m_buttonSize ) );
    b->show();
    
    col++;
    if( col == m_cols ) {
      col = 0;
      row++;
    }
  }
}


void K3bWelcomeWidget::Display::resizeEvent( QResizeEvent* e )
{
  QWidget::resizeEvent(e);
  repositionButtons();
}


void K3bWelcomeWidget::Display::paintEvent( QPaintEvent* e )
{
  QWidget::paintEvent( e );

  QPainter p( this );
  QRect rect( 10, 10, QMAX( m_header->widthUsed() + 20, width() - 20 ), m_header->height() + 20 );
  p.fillRect( rect, m_headerBgColor );
  p.setPen( m_headerFgColor );
  p.drawRect( rect );
  p.drawRect( 10, 10, width()-20, height()-20 );
  QColorGroup grp( colorGroup() );
  grp.setColor( QColorGroup::Text, m_headerFgColor );
  int pos = 20;
  pos += QMAX( (width()-40-m_header->widthUsed())/2, 0 );
  m_header->draw( &p, pos, 20, QRect(), grp );
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

  QPtrList<KAction> actions;
  for( QStringList::const_iterator it = sl.begin(); it != sl.end(); ++it )
    if( KAction* a = m_mainWindow->actionCollection()->action( (*it).latin1() ) )
      actions.append(a);

  main->rebuildGui( actions );
}


void K3bWelcomeWidget::saveConfig( KConfig* c )
{
  c->setGroup( "Welcome Widget" );

  QStringList sl;
  for( QPtrListIterator<KAction> it( main->m_actions ); it.current(); ++it )
    sl.append( it.current()->name() );

  c->writeEntry( "welcome_actions", sl );
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


void K3bWelcomeWidget::contentsMousePressEvent( QMouseEvent* e )
{
  if( e->button() == QMouseEvent::RightButton ) {
    QMap<int, KAction*> map;
    KPopupMenu pop;
    KPopupMenu addPop;

    KActionPtrList actions = m_mainWindow->actionCollection()->actions();
    for( KActionPtrList::iterator it = actions.begin(); it != actions.end(); ++it ) {
      KAction* a = *it;
      // We only allow project and tools buttons but not the file_new action since this is
      // the actionmenu containing all the other file_new actions and that would not make sense
      // on a toolbutton
      QString aname(a->name());
      if( aname != "file_new"  &&
	  ( aname.startsWith( "tools" ) || aname.startsWith( "file_new" ) ) )
	map.insert( addPop.insertItem( a->iconSet(), a->text() ), a );
    }
    
    int r = -1;

    int removeAction = -1;
    if( viewport()->childAt(e->pos())->inherits( "QToolButton" ) ) {
      removeAction = pop.insertItem( SmallIcon("remove"), i18n("Remove Button") );
      pop.insertItem( i18n("Add Button"), &addPop );
      r = pop.exec( e->globalPos() );
    }
    else {
      addPop.insertTitle( i18n("Add Button"), -1, 0 );
      r = addPop.exec( e->globalPos() );
    }

    if( r != -1 ) {
      if( r == removeAction )
	main->removeButton( (QToolButton*)viewport()->childAt(e->pos()) );
      else
	main->addAction( map[r] );
    }
  }
}


void K3bWelcomeWidget::slotThemeChanged()
{
  if( K3bTheme* theme = k3bthememanager->currentTheme() ) {
    main->setPaletteBackgroundPixmap( theme->pixmap( K3bTheme::WELCOME_BG ) );
    main->setHeaderBackgroundColor( theme->backgroundColor() );
    main->setHeaderForegroundColor( theme->foregroundColor() );
  }
}


#include "k3bwelcomewidget.moc"
