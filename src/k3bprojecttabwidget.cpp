/* 
 *
 * $Id$
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



#include "k3bprojecttabwidget.h"
#include "k3bapplication.h"
#include "k3bprojectmanager.h"

#include <k3bview.h>
#include <k3bdoc.h>

#include <kaction.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kurldrag.h>
#include <klocale.h>

#include <qevent.h>
#include <qtabbar.h>
//Added by qt3to4:
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragMoveEvent>


class K3bProjectTabWidget::ProjectData
{
public:
  ProjectData()
    : doc(0),
      modified(false) {
  }

  ProjectData( K3bDoc* doc_ )
    : doc(doc_),
      modified(false) {
  }

  // the project
  K3bDoc* doc;

  // is the project marked modified here
  bool modified;
};



K3bProjectTabWidget::K3bProjectTabWidget( QWidget *parent, const char *name, WFlags f )
  : QTabWidget( parent, name, f )
{
  tabBar()->setAcceptDrops(true);
  tabBar()->installEventFilter( this );

  m_projectActionMenu = new KActionMenu( i18n("Project"), this );
}


K3bProjectTabWidget::~K3bProjectTabWidget()
{
}


void K3bProjectTabWidget::addTab( QWidget* child, const QString& label )
{
  QTabWidget::addTab( child, label );
  tabBar()->setShown( count() != 1 );
}


void K3bProjectTabWidget::addTab( QWidget* child, const QIcon& iconset, const QString& label )
{
  QTabWidget::addTab( child, iconset, label );
  tabBar()->setShown( count() != 1 );
}


void K3bProjectTabWidget::addTab( QWidget* child, QTab* tab )
{
  QTabWidget::addTab( child, tab );
  tabBar()->setShown( count() != 1 );
}


void K3bProjectTabWidget::insertTab( QWidget* child, const QString& label, int index )
{
  QTabWidget::insertTab( child, label, index );
  tabBar()->setShown( count() != 1 );
}


void K3bProjectTabWidget::insertTab( QWidget* child, const QIcon& iconset, const QString& label, int index )
{
  QTabWidget::insertTab( child, iconset, label, index );
  tabBar()->setShown( count() != 1 );
}


void K3bProjectTabWidget::insertTab( QWidget* child, QTab* tab, int index )
{
  QTabWidget::insertTab( child, tab, index );
  tabBar()->setShown( count() != 1 );
}


void K3bProjectTabWidget::removePage( QWidget* w )
{
  QTabWidget::removePage( w );
  tabBar()->setShown( count() != 1 );
}


void K3bProjectTabWidget::insertTab( K3bDoc* doc )
{
  QTabWidget::insertTab( doc->view(), doc->URL().fileName() );
  connect( k3bappcore->projectManager(), SIGNAL(projectSaved(K3bDoc*)), this, SLOT(slotDocSaved(K3bDoc*)) );
  connect( doc, SIGNAL(changed(K3bDoc*)), this, SLOT(slotDocChanged(K3bDoc*)) );

  m_projectDataMap[doc] = ProjectData( doc );

  if( doc->isModified() )
    slotDocChanged( doc );
  else
    slotDocSaved( doc );
}


void K3bProjectTabWidget::insertAction( KAction* action )
{
  m_projectActionMenu->insert( action );
}


void K3bProjectTabWidget::slotDocChanged( K3bDoc* doc )
{
  // we need to cache the icon changes since the changed() signal will be emitted very often
  if( !m_projectDataMap[doc].modified ) {
    setTabIconSet( doc->view(), SmallIconSet( "filesave" ) );
    m_projectDataMap[doc].modified = true;

    // we need this one for the session management
    changeTab( doc->view(), doc->URL().fileName() );
  }
}


void K3bProjectTabWidget::slotDocSaved( K3bDoc* doc )
{
  setTabIconSet( doc->view(), QIcon() );
  changeTab( doc->view(), doc->URL().fileName() );
}


K3bDoc* K3bProjectTabWidget::projectAt( const QPoint& pos ) const
{
  QTab* tab = tabBar()->selectTab( pos );
  if( tab ) {
    QWidget* w = page( tabBar()->indexOf( tab->identifier() ) );
    if( K3bView* view = dynamic_cast<K3bView*>(w) )
      return view->doc();
  }

  return 0;
}


bool K3bProjectTabWidget::eventFilter( QObject* o, QEvent* e )
{
  if( o == tabBar() ) {
    if( e->type() == QEvent::MouseButtonPress ) {
      QMouseEvent* me = static_cast<QMouseEvent*>(e);
      if( me->button() == Qt::RightButton ) {
	if( projectAt( me->pos() ) ) {
	  // we need change the tab because the actions work on the current tab
	  QTab* clickedTab = tabBar()->selectTab( me->pos() );
	  if( clickedTab ) {
	    tabBar()->setCurrentTab( clickedTab );
	    
	    // show the popup menu
	    m_projectActionMenu->popup( me->globalPos() );
	  }
	}
	return true;
      }
    }

    else if( e->type() == QEvent::DragMove ) {
      QDragMoveEvent* de = static_cast<QDragMoveEvent*>(e);
      de->accept( KURLDrag::canDecode(de) && projectAt(de->pos()) );
      return true;
    }

    else if( e->type() == QEvent::Drop ) {
      QDropEvent* de = static_cast<QDropEvent*>(e);
      KURL::List l;
      if( KURLDrag::decode( de, l ) ) {
	if( K3bDoc* doc = projectAt( de->pos() ) )
	  dynamic_cast<K3bView*>(doc->view())->addUrls( l );
      }
      return true;
    }
  }

  return QTabWidget::eventFilter( o, e );
}

#include "k3bprojecttabwidget.moc"

