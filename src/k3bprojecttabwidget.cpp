/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bview.h"
#include "k3bdoc.h"

#include <KLocalizedString>
#include <KIconLoader>
#include <KActionMenu>

#include <QDebug>
#include <QMimeData>
#include <QUrl>
#include <QDragMoveEvent>
#include <QIcon>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMouseEvent>
#include <QAction>
#include <QMenu>
#include <QTabBar>

namespace {
    
    class ProjectData
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
    
} // namespace

class K3b::ProjectTabWidget::Private
{
public:
    KActionMenu* projectActionMenu;
    QMap<Doc*, ProjectData> projectDataMap;
};


K3b::ProjectTabWidget::ProjectTabWidget( QWidget *parent )
    : QTabWidget( parent ),
      d( new Private )
{
    setDocumentMode( true );
    setTabsClosable( true );
    setMovable( true );
    tabBar()->setAcceptDrops(true);
    tabBar()->installEventFilter( this );

    d->projectActionMenu = new KActionMenu( i18n("Project"), this );
    
    connect( this, SIGNAL(tabCloseRequested(int)), SLOT(slotTabCloseRequested(int)));
}


K3b::ProjectTabWidget::~ProjectTabWidget()
{
    delete d;
}


void K3b::ProjectTabWidget::addTab( Doc* doc )
{
    QTabWidget::addTab( doc->view(), doc->URL().fileName() );
    connect( k3bappcore->projectManager(), SIGNAL(projectSaved(K3b::Doc*)), this, SLOT(slotDocSaved(K3b::Doc*)) );
    connect( doc, SIGNAL(changed(K3b::Doc*)), this, SLOT(slotDocChanged(K3b::Doc*)) );

    d->projectDataMap[doc] = ProjectData( doc );

    if( doc->isModified() )
        slotDocChanged( doc );
    else
        slotDocSaved( doc );
}


void K3b::ProjectTabWidget::removeTab( Doc* doc )
{
    if( doc != 0 ) {
        QTabWidget::removeTab( indexOf( doc->view() ) );
    }
}


void K3b::ProjectTabWidget::setCurrentTab( Doc* doc )
{
    if( doc != 0 ) {
        setCurrentWidget( doc->view() );
        doc->view()->setFocus();
    }
}


K3b::Doc* K3b::ProjectTabWidget::currentTab() const
{
    QWidget* widget = currentWidget();
    if( K3b::View* view = qobject_cast<K3b::View*>(widget) )
        return view->doc();
    else
        return 0;
}


void K3b::ProjectTabWidget::addAction( QAction* action )
{
    d->projectActionMenu->addAction( action );
}


K3b::Doc* K3b::ProjectTabWidget::projectAt( const QPoint& pos ) const
{
    int tabPos = tabBar()->tabAt(pos);
    if( tabPos != -1 )
    {
        QWidget *w = widget(tabPos);
        if(View* view = qobject_cast<View*>(w) )
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
                        d->projectActionMenu->menu()->exec( me->globalPos() );
                    }
                }
                return true;
            }
        }

        else if( e->type() == QEvent::DragEnter ) {
            QDragEnterEvent* de = static_cast<QDragEnterEvent*>(e);
            de->setAccepted( de->mimeData()->hasUrls() );
            return true;
        }

        else if( e->type() == QEvent::DragMove ) {
            QDragMoveEvent* de = static_cast<QDragMoveEvent*>(e);
            de->setAccepted( projectAt(de->pos()) != 0 );
            return true;
        }

        else if( e->type() == QEvent::Drop ) {
            QDropEvent* de = static_cast<QDropEvent*>(e);
            if( de->mimeData()->hasUrls() ) {
                if( Doc* doc = projectAt( de->pos() ) ) {
                    QList<QUrl> urls;
                    Q_FOREACH( const QUrl& url, de->mimeData()->urls() ) {
                        urls.append( url );
                    }
                    qobject_cast<View*>(doc->view())->addUrls( urls );
                }
            }
            return true;
        }
    }

    return QTabWidget::eventFilter( o, e );
}


void K3b::ProjectTabWidget::slotDocChanged( K3b::Doc* doc )
{
    // we need to cache the icon changes since the changed() signal will be emitted very often
    if( !d->projectDataMap[doc].modified ) {
        setTabIcon( indexOf( doc->view() ), QIcon::fromTheme( "document-save" ) );
        d->projectDataMap[doc].modified = true;

        // we need this one for the session management
        setTabText( indexOf( doc->view() ), doc->URL().fileName() );
    }
}


void K3b::ProjectTabWidget::slotDocSaved( K3b::Doc* doc )
{
    setTabIcon( indexOf( doc->view() ), QIcon() );
    setTabText( indexOf( doc->view() ), doc->URL().fileName() );
}


void K3b::ProjectTabWidget::slotTabCloseRequested( int index )
{
    QWidget* w = widget( index );
    if( View* view = qobject_cast<View*>(w) ) {  
        emit tabCloseRequested( view->doc() );
    }
}



