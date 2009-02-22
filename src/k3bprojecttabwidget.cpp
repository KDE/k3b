/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
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
#include <k3urldrag.h>
#include <klocale.h>
#include <kactionmenu.h>
#include <kmenu.h>

#include <qevent.h>
#include <qtabbar.h>
//Added by qt3to4:
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragMoveEvent>


class K3b::ProjectTabWidget::ProjectData
{
public:
    ProjectData()
        : doc(0),
          modified(false) {
    }

    ProjectData( K3b::Doc* doc_ )
        : doc(doc_),
          modified(false) {
    }

    // the project
    K3b::Doc* doc;

    // is the project marked modified here
    bool modified;
};



K3b::ProjectTabWidget::ProjectTabWidget( QWidget *parent )
    : QTabWidget( parent )
{
    tabBar()->setAcceptDrops(true);
    tabBar()->installEventFilter( this );

    m_projectActionMenu = new KActionMenu( i18n("Project"), this );
}


K3b::ProjectTabWidget::~ProjectTabWidget()
{
}


void K3b::ProjectTabWidget::addTab( QWidget* child, const QString& label )
{
    QTabWidget::addTab( child, label );
    tabBar()->setVisible( count() != 1 );
}


void K3b::ProjectTabWidget::addTab( QWidget* child, const QIcon& iconset, const QString& label )
{
    QTabWidget::addTab( child, iconset, label );
    tabBar()->setVisible( count() != 1 );
}

void K3b::ProjectTabWidget::tabInserted ( int index )
{
    QTabWidget::tabInserted ( index );
    tabBar()->setVisible( count() != 1 );
}

void K3b::ProjectTabWidget::insertTab( QWidget* child, const QString& label, int index )
{
    QTabWidget::insertTab( index, child, label );
    tabBar()->setVisible( count() != 1 );
}


void K3b::ProjectTabWidget::insertTab( QWidget* child, const QIcon& iconset, const QString& label, int index )
{
    QTabWidget::insertTab( index, child, iconset, label );
    tabBar()->setVisible( count() != 1 );
}

void K3b::ProjectTabWidget::removePage( QWidget* w )
{
    QTabWidget::removeTab( indexOf( w ) );
    tabBar()->setVisible( count() != 1 );
}


void K3b::ProjectTabWidget::insertTab( K3b::Doc* doc )
{
    QTabWidget::addTab( doc->view(), doc->URL().fileName() );
    connect( k3bappcore->projectManager(), SIGNAL(projectSaved(K3b::Doc*)), this, SLOT(slotDocSaved(K3b::Doc*)) );
    connect( doc, SIGNAL(changed(K3b::Doc*)), this, SLOT(slotDocChanged(K3b::Doc*)) );

    m_projectDataMap[doc] = ProjectData( doc );

    if( doc->isModified() )
        slotDocChanged( doc );
    else
        slotDocSaved( doc );
}


void K3b::ProjectTabWidget::insertAction( KAction* action )
{
    m_projectActionMenu->addAction( action );
}


void K3b::ProjectTabWidget::slotDocChanged( K3b::Doc* doc )
{
    // we need to cache the icon changes since the changed() signal will be emitted very often
    if( !m_projectDataMap[doc].modified ) {
        setTabIcon( indexOf( doc->view() ), KIcon( "document-save" ) );
        m_projectDataMap[doc].modified = true;

        // we need this one for the session management
        setTabText( indexOf( doc->view() ), doc->URL().fileName() );
    }
}


void K3b::ProjectTabWidget::slotDocSaved( K3b::Doc* doc )
{
    setTabIcon( indexOf( doc->view() ), QIcon() );
    setTabText( indexOf( doc->view() ), doc->URL().fileName() );
}


K3b::Doc* K3b::ProjectTabWidget::projectAt( const QPoint& pos ) const
{
    int tabPos = tabBar()->tabAt(pos);
    if( tabPos != -1 )
    {
        QWidget *w = widget(tabPos);
        if(K3b::View* view = dynamic_cast<K3b::View*>(w) )
            return view->doc();
    }
    return 0;
}


bool K3b::ProjectTabWidget::eventFilter( QObject* o, QEvent* e )
{
    if( o == tabBar() ) {
        if( e->type() == QEvent::MouseButtonPress ) {
            QMouseEvent* me = static_cast<QMouseEvent*>(e);
            if( me->button() == Qt::RightButton ) {
                if( projectAt( me->pos() ) ) {
                    int tabPos = tabBar()->tabAt(me->pos());
                    if(tabPos!=-1){
                        setCurrentIndex(tabPos);
                        // show the popup menu
                        m_projectActionMenu->menu()->exec( me->globalPos() );
                    }
                }
                return true;
            }
        }

        else if( e->type() == QEvent::DragMove ) {
            QDragMoveEvent* de = static_cast<QDragMoveEvent*>(e);
            de->setAccepted( K3URLDrag::canDecode(de) && projectAt(de->pos()) );
            return true;
        }

        else if( e->type() == QEvent::Drop ) {
            QDropEvent* de = static_cast<QDropEvent*>(e);
            KUrl::List l;
            if( K3URLDrag::decode( de, l ) ) {
                if( K3b::Doc* doc = projectAt( de->pos() ) )
                    dynamic_cast<K3b::View*>(doc->view())->addUrls( l );
            }
            return true;
        }
    }

    return QTabWidget::eventFilter( o, e );
}

#include "k3bprojecttabwidget.moc"

