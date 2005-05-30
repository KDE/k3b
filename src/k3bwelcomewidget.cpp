/* 
 *
 * $Id$
 * Copyright (C) 2003-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bwelcomewidget.h"
#include "k3b.h"
#include "k3bflatbutton.h"
#include <k3bstdguiitems.h>
#include "k3bapplication.h"
#include <k3bversion.h>
#include "k3bthememanager.h"

#include <qpixmap.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qsimplerichtext.h>
#include <qptrlist.h>
#include <qmap.h>
#include <qtooltip.h>

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
#include <kaboutdata.h>
#include <kactionclasses.h>


K3bWelcomeWidget::Display::Display( QWidget* parent )
  : QWidget( parent )
{
  QFont fnt(font());
  fnt.setBold(true);
  fnt.setPointSize( 16 );
  m_header = new QSimpleRichText( i18n("Welcome to K3b - The CD and DVD Kreator"), fnt );
  m_infoText = new QSimpleRichText( i18n("<p align=\"center\">Change the welcome screen buttons with a "
					 "right mouse click.<br>"
					 "Every other project type and the tools "
					 "like Image writing or Formatting are accessible via the K3b menu."), font() );

  // set a large width just to be sure no linebreak occurs
  m_header->setWidth( 800 );

  setAcceptDrops( true );
  setBackgroundMode( PaletteBase );
  m_infoTextVisible = true;
}


K3bWelcomeWidget::Display::~Display()
{
  delete m_header;
  delete m_infoText;
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


void K3bWelcomeWidget::Display::removeButton( K3bFlatButton* b )
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
  // step 1: delete all old buttons in the buttons QPtrList<K3bFlatButton>
  m_buttonMap.clear();
  m_buttons.setAutoDelete(true);
  m_buttons.clear();

  int numActions = m_actions.count();
  if( numActions > 0 ) {

    // create buttons
    for( QPtrListIterator<KAction> it( m_actions ); it.current(); ++it ) {
      KAction* a = it.current();

      K3bFlatButton* b = new K3bFlatButton( a, this );

      m_buttons.append( b );
      m_buttonMap.insert( b, a );
    }

    // determine the needed button size (since all buttons should be equal in size
    // we use the max of all sizes)
    m_buttonSize = m_buttons.first()->sizeHint();
    for( QPtrListIterator<K3bFlatButton> it( m_buttons ); it.current(); ++it ) {
      m_buttonSize = m_buttonSize.expandedTo( it.current()->sizeHint() );
    }

    // calculate rows and columns
    m_cols = 0;
    m_rows = 0;
    if( numActions < 3 )
      m_rows = 1;
    else if( numActions > 6 )
      m_rows = 3;
    else
      m_rows = 2;

    m_cols = numActions/m_rows;
    if( numActions%m_rows )
      m_cols++;

    repositionButtons();

    // calculate widget size
    m_size = QSize( QMAX(40+m_header->widthUsed(), 160+((m_buttonSize.width()+4)*m_cols)),
		    160+((m_buttonSize.height()+4)*m_rows) );
  }
}


void K3bWelcomeWidget::Display::repositionButtons()
{
  int leftMargin = QMAX( 80, (width() - ((m_buttonSize.width()+4)*m_cols))/2 );
  int topOffset = m_header->height() + 40 + 10;//(height() - m_header->height() - 60 - (m_buttonSize.height()*m_rows))/2;

  int row = 0;
  int col = 0;

  for( QPtrListIterator<K3bFlatButton> it( m_buttons ); it.current(); ++it ) {
    K3bFlatButton* b = it.current();
    
    b->setGeometry( QRect( QPoint( leftMargin + (col*(m_buttonSize.width()+4) + 2 ), 
				   topOffset + (row*(m_buttonSize.height()+4)) + 2 ),
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
  m_infoText->setWidth( width() - 20 );
  QWidget::resizeEvent(e);
  repositionButtons();
}


void K3bWelcomeWidget::Display::paintEvent( QPaintEvent* e )
{
  QWidget::paintEvent( e );

  QPainter p( this );
  p.setPen( m_headerFgColor );

  // rect around the header
  QRect rect( 10, 10, QMAX( m_header->widthUsed() + 20, width() - 20 ), m_header->height() + 20 );
  p.fillRect( rect, m_headerBgColor );
  p.drawRect( rect );

  // big rect around the whole thing
  p.drawRect( 10, 10, width()-20, height()-20 );

  // draw the header text
  QColorGroup grp( colorGroup() );
  grp.setColor( QColorGroup::Text, m_headerFgColor );
  int pos = 20;
  pos += QMAX( (width()-40-m_header->widthUsed())/2, 0 );
  m_header->draw( &p, pos, 20, QRect(), grp );

  if( m_infoTextVisible ) {
    // draw the info box
    //    int boxWidth = 20 + m_infoText->widthUsed();
    int boxHeight = 20 + m_infoText->height();
    QRect infoBoxRect( 10/*QMAX( (width()-20-m_infoText->widthUsed())/2, 10 )*/,
		       height()-10-boxHeight,
		       width()-20/*boxWidth*/, 
		       boxHeight );
    p.fillRect( infoBoxRect, m_headerBgColor );
    p.drawRect( infoBoxRect );
    m_infoText->draw( &p, infoBoxRect.left()+10, infoBoxRect.top()+10, QRect(), grp );
  }
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

  connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );

  slotThemeChanged();
}


K3bWelcomeWidget::~K3bWelcomeWidget()
{
}


void K3bWelcomeWidget::loadConfig( KConfigBase* c )
{
  c->setGroup( "Welcome Widget" );

  main->m_infoTextVisible = c->readBoolEntry( "show info text", true );

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

  fixSize();
}


void K3bWelcomeWidget::saveConfig( KConfigBase* c )
{
  c->setGroup( "Welcome Widget" );

  c->writeEntry( "show info text", main->m_infoTextVisible );

  QStringList sl;
  for( QPtrListIterator<KAction> it( main->m_actions ); it.current(); ++it )
    sl.append( it.current()->name() );

  c->writeEntry( "welcome_actions", sl );
}


void K3bWelcomeWidget::resizeEvent( QResizeEvent* e )
{
  QScrollView::resizeEvent( e );
  fixSize();
}


void K3bWelcomeWidget::fixSize()
{
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
    KPopupMenu addPop;

    KActionPtrList actions = m_mainWindow->actionCollection()->actions();
    for( KActionPtrList::iterator it = actions.begin(); it != actions.end(); ++it ) {
      KAction* a = *it;
      // We only allow project and tools buttons but not the file_new action since this is
      // the actionmenu containing all the other file_new actions and that would not make sense
      // on a toolbutton
      QString aname(a->name());
      if( aname != "file_new"  && aname != "file_new_cd" && aname != "file_new_dvd" &&
	  ( aname.startsWith( "tools" ) || aname.startsWith( "file_new" ) ) &&
	  !main->m_actions.containsRef(a) )
	map.insert( addPop.insertItem( a->iconSet(), a->text() ), a );
    }
    
    // menu identifiers in QT are always < 0 (when automatically generated)
    // and unique throughout the entire application!
    int r = 0;
    int removeAction = 0;
    int infoTextAction = 0;
    QString infoTextActionText;
    if( main->m_infoTextVisible )
      infoTextActionText = i18n("Hide Info Text");
    else
      infoTextActionText = i18n("Show Info Text");

    QWidget* widgetAtPos = viewport()->childAt(e->pos());
    if( widgetAtPos && widgetAtPos->inherits( "K3bFlatButton" ) ) {
      KPopupMenu pop;
      removeAction = pop.insertItem( SmallIcon("remove"), i18n("Remove Button") );
      pop.insertItem( i18n("Add Button"), &addPop );
      pop.insertSeparator();
      infoTextAction = pop.insertItem( infoTextActionText );
      r = pop.exec( e->globalPos() );
    }
    else {
      addPop.insertTitle( i18n("Add Button"), -1, 0 );
      addPop.insertSeparator();
      infoTextAction = addPop.insertItem( infoTextActionText );
      r = addPop.exec( e->globalPos() );
    }

    if( r != 0 ) {
      if( r == removeAction )
	main->removeButton( static_cast<K3bFlatButton*>(widgetAtPos) );
      else if( r == infoTextAction ) {
	main->m_infoTextVisible = !main->m_infoTextVisible;
	main->update();
      }
      else
	main->addAction( map[r] );
    }

    fixSize();
  }
}


void K3bWelcomeWidget::slotThemeChanged()
{
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    main->setPaletteBackgroundPixmap( theme->pixmap( K3bTheme::WELCOME_BG ) );
    main->setHeaderBackgroundColor( theme->backgroundColor() );
    main->setHeaderForegroundColor( theme->foregroundColor() );
  }
}


#include "k3bwelcomewidget.moc"
