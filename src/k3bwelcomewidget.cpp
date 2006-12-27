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
#include <qcursor.h>

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


K3bWelcomeWidget::Display::Display( K3bWelcomeWidget* parent )
  : QWidget( parent->viewport() )
{
  setWFlags( Qt::WNoAutoErase );

  QFont fnt(font());
  fnt.setBold(true);
  fnt.setPointSize( 16 );
  m_header = new QSimpleRichText( i18n("Welcome to K3b - The CD and DVD Kreator"), fnt );
  m_infoText = new QSimpleRichText( QString::fromUtf8("<qt align=\"center\">K3b %1 (c) 1999 - 2006 Sebastian Trüg")
				    .arg(kapp->aboutData()->version()), font() );

  // set a large width just to be sure no linebreak occurs
  m_header->setWidth( 800 );

  setAcceptDrops( true );
  setBackgroundMode( PaletteBase );
  m_rows = m_cols = 1;

  m_buttonMore = new K3bFlatButton( i18n("Further actions..."), this );
  connect( m_buttonMore, SIGNAL(pressed()), parent, SLOT(slotMoreActions()) );
}


K3bWelcomeWidget::Display::~Display()
{
  delete m_header;
  delete m_infoText;
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


static void calculateButtons( int width, int numActions, int buttonWidth, int& cols, int& rows )
{
  // always try to avoid horizontal scrollbars
  int wa = width - 40;
  cols = QMAX( 1, QMIN( wa / (buttonWidth+4), numActions ) );
  rows = numActions/cols;
  int over = numActions%cols;
  if( over ) {
    rows++;
    // try to avoid useless cols
    while( over && cols - over - 1 >= rows-1 ) {
      --cols;
      over = numActions%cols;
    }
  }
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

    repositionButtons();
  }
}


void K3bWelcomeWidget::Display::repositionButtons()
{
  // calculate rows and columns
  calculateButtons( width(), m_actions.count(), m_buttonSize.width(), m_cols, m_rows );

  int availHor = width() - 40;
  int availVert = height() - 20 - 10 - m_header->height() - 10;
  availVert -= m_infoText->height() - 10;
  int leftMargin = 20 + (availHor - (m_buttonSize.width()+4)*m_cols)/2;
  int topOffset = m_header->height() + 20 + ( availVert - (m_buttonSize.height()+4)*m_rows - m_buttonMore->height() )/2;

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
  if( col > 0 )
    ++row;

  m_buttonMore->setGeometry( QRect( QPoint( leftMargin + 2,
					    topOffset + (row*(m_buttonSize.height()+4)) + 2 ),
				    QSize( m_cols*(m_buttonSize.width()+4) - 4, m_buttonMore->height() ) ) );
}


QSizePolicy K3bWelcomeWidget::Display::sizePolicy () const
{
  return QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum, true );
}


int K3bWelcomeWidget::Display::heightForWidth( int w ) const
{
  int ow = m_infoText->width();
  m_infoText->setWidth( w );
  int h = m_infoText->height();
  m_infoText->setWidth( ow );

  int cols, rows;
  calculateButtons( w, m_actions.count(), m_buttonSize.width(), cols, rows );

  return (20 + m_header->height() + 20 + 10 + ((m_buttonSize.height()+4)*rows) + 4 + m_buttonMore->height() + 10 + h + 20);
}


QSize K3bWelcomeWidget::Display::minimumSizeHint() const
{
  QSize size( QMAX(40+m_header->widthUsed(), 40+m_buttonSize.width()),
	      20 + m_header->height() + 20 + 10 + m_buttonSize.height() + 10 + m_infoText->height() + 20 );

  return size;
}


void K3bWelcomeWidget::Display::resizeEvent( QResizeEvent* e )
{
  m_infoText->setWidth( width() - 20 );
  QWidget::resizeEvent(e);
  repositionButtons();
}


void K3bWelcomeWidget::Display::paintEvent( QPaintEvent* )
{
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    QPainter p( this );
    p.setPen( theme->foregroundColor() );

    // draw the background including first filling with the bg color for transparent images
    p.fillRect( rect(), theme->backgroundColor() );
    p.drawTiledPixmap( rect(), theme->pixmap( K3bTheme::WELCOME_BG ) );

    // rect around the header
    QRect rect( 10, 10, QMAX( m_header->widthUsed() + 20, width() - 20 ), m_header->height() + 20 );
    p.fillRect( rect, theme->backgroundColor() );
    p.drawRect( rect );

    // big rect around the whole thing
    p.drawRect( 10, 10, width()-20, height()-20 );

    // draw the header text
    QColorGroup grp( colorGroup() );
    grp.setColor( QColorGroup::Text, theme->foregroundColor() );
    int pos = 20;
    pos += QMAX( (width()-40-m_header->widthUsed())/2, 0 );
    m_header->draw( &p, pos, 20, QRect(), grp );

    // draw the info box
    //    int boxWidth = 20 + m_infoText->widthUsed();
    int boxHeight = 10 + m_infoText->height();
    QRect infoBoxRect( 10/*QMAX( (width()-20-m_infoText->widthUsed())/2, 10 )*/,
		       height()-10-boxHeight,
		       width()-20/*boxWidth*/,
		       boxHeight );
    p.fillRect( infoBoxRect, theme->backgroundColor() );
    p.drawRect( infoBoxRect );
    m_infoText->draw( &p, infoBoxRect.left()+5, infoBoxRect.top()+5, QRect(), grp );
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
  main = new Display( this );
  addChild( main );

  connect( main, SIGNAL(dropped(const KURL::List&)), m_mainWindow, SLOT(addUrls(const KURL::List&)) );

  connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), main, SLOT(update()) );
  connect( kapp, SIGNAL(appearanceChanged()), main, SLOT(update()) );
}


K3bWelcomeWidget::~K3bWelcomeWidget()
{
}


void K3bWelcomeWidget::loadConfig( KConfigBase* c )
{
  QStringList sl = KConfigGroup( c, "Welcome Widget" ).readListEntry( "welcome_actions" );

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
  KConfigGroup grp( c, "Welcome Widget" );

  QStringList sl;
  for( QPtrListIterator<KAction> it( main->m_actions ); it.current(); ++it )
    sl.append( it.current()->name() );

  grp.writeEntry( "welcome_actions", sl );
}


void K3bWelcomeWidget::resizeEvent( QResizeEvent* e )
{
  QScrollView::resizeEvent( e );
  fixSize();
}


void K3bWelcomeWidget::showEvent( QShowEvent* e )
{
  QScrollView::showEvent( e );
  fixSize();
}


void K3bWelcomeWidget::fixSize()
{
  QSize s = contentsRect().size();
  s.setWidth( QMAX( main->minimumSizeHint().width(), s.width() ) );
  s.setHeight( QMAX( main->heightForWidth(s.width()), s.height() ) );

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
      if( aname != "file_new" &&
	  ( aname.startsWith( "tools" ) || aname.startsWith( "file_new" ) ) &&
	  !main->m_actions.containsRef(a) )
	map.insert( addPop.insertItem( a->iconSet(), a->text() ), a );
    }

    // menu identifiers in QT are always < 0 (when automatically generated)
    // and unique throughout the entire application!
    int r = 0;
    int removeAction = 0;

    QWidget* widgetAtPos = viewport()->childAt(e->pos());
    if( widgetAtPos && widgetAtPos->inherits( "K3bFlatButton" ) ) {
      KPopupMenu pop;
      removeAction = pop.insertItem( SmallIcon("remove"), i18n("Remove Button") );
      if ( addPop.count() > 0 )
          pop.insertItem( i18n("Add Button"), &addPop );
      pop.insertSeparator();
      r = pop.exec( e->globalPos() );
    }
    else {
      addPop.insertTitle( i18n("Add Button"), -1, 0 );
      addPop.insertSeparator();
      r = addPop.exec( e->globalPos() );
    }

    if( r != 0 ) {
      if( r == removeAction )
	main->removeButton( static_cast<K3bFlatButton*>(widgetAtPos) );
      else
	main->addAction( map[r] );
    }

    fixSize();
  }
}


void K3bWelcomeWidget::slotMoreActions()
{
  KPopupMenu popup;

  m_mainWindow->actionCollection()->action( "file_new_data" )->plug( &popup );
  m_mainWindow->actionCollection()->action( "file_new_dvd" )->plug( &popup );
  m_mainWindow->actionCollection()->action( "file_continue_multisession" )->plug( &popup );
  (new KActionSeparator( &popup ))->plug( &popup );
  m_mainWindow->actionCollection()->action( "file_new_audio" )->plug( &popup );
  (new KActionSeparator( &popup ))->plug( &popup );
  m_mainWindow->actionCollection()->action( "file_new_mixed" )->plug( &popup );
  (new KActionSeparator( &popup ))->plug( &popup );
  m_mainWindow->actionCollection()->action( "file_new_vcd" )->plug( &popup );
  m_mainWindow->actionCollection()->action( "file_new_video_dvd" )->plug( &popup );
  (new KActionSeparator( &popup ))->plug( &popup );
  m_mainWindow->actionCollection()->action( "file_new_movix" )->plug( &popup );
  m_mainWindow->actionCollection()->action( "file_new_movix_dvd" )->plug( &popup );
  (new KActionSeparator( &popup ))->plug( &popup );
  m_mainWindow->actionCollection()->action( "tools_copy_cd" )->plug( &popup );
  m_mainWindow->actionCollection()->action( "tools_copy_dvd" )->plug( &popup );
  (new KActionSeparator( &popup ))->plug( &popup );
  m_mainWindow->actionCollection()->action( "tools_blank_cdrw" )->plug( &popup );
  m_mainWindow->actionCollection()->action( "tools_format_dvd" )->plug( &popup );
  (new KActionSeparator( &popup ))->plug( &popup );
  m_mainWindow->actionCollection()->action( "tools_write_cd_image" )->plug( &popup );
  m_mainWindow->actionCollection()->action( "tools_write_dvd_iso" )->plug( &popup );
  (new KActionSeparator( &popup ))->plug( &popup );
  m_mainWindow->actionCollection()->action( "tools_cdda_rip" )->plug( &popup );
  m_mainWindow->actionCollection()->action( "tools_videodvd_rip" )->plug( &popup );
  m_mainWindow->actionCollection()->action( "tools_videocd_rip" )->plug( &popup );

//   KActionPtrList actions = m_mainWindow->actionCollection()->actions();
//   for( KActionPtrList::iterator it = actions.begin(); it != actions.end(); ++it ) {
//     KAction* a = *it;
//     // We only allow project and tools buttons but not the file_new action since this is
//     // the actionmenu containing all the other file_new actions and that would not make sense
//     // on a toolbutton
//     QString aname(a->name());
//     if( aname != "file_new" &&
// 	( aname.startsWith( "tools" ) || aname.startsWith( "file_new" ) ) &&
// 	!main->m_actions.containsRef(a) )
//       a->plug( &popup );
//   }

  popup.exec( QCursor::pos() );
}

#include "k3bwelcomewidget.moc"
