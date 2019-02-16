/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bfileview.h"
#include "k3b.h"
#include "k3bdiroperator.h"
#include "k3bapplication.h"

#include <KFileFilterCombo>
#include <KFileItem>
#include <KLocalizedString>
#include <KDirLister>
#include <KActionMenu>
#include <KToolBarSpacerAction>
#include <KActionCollection>
#include <KToolBar>

#include <QDebug>
#include <QDir>
#include <QUrl>
#include <QIcon>
#include <QAction>
#include <QHBoxLayout>
#include <QLayout>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>


class K3b::FileView::Private
{
public:
    KToolBar* toolBox;
    DirOperator* dirOp;
    KFileFilterCombo* filterWidget;
    QAction* actionShowBookmarks;
};


K3b::FileView::FileView(QWidget *parent )
    : K3b::ContentsView( false, parent),
      d( new Private )
{
    d->dirOp = new K3b::DirOperator( QUrl::fromLocalFile(QDir::home().absolutePath()), this );
    d->toolBox = new KToolBar( this );
    d->toolBox->setToolButtonStyle( Qt::ToolButtonIconOnly );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );
    layout->addWidget( d->toolBox );
    layout->addWidget( d->dirOp, 1 );

    // setup actions
    QAction* actionBack = d->dirOp->actionCollection()->action("back");
    QAction* actionForward = d->dirOp->actionCollection()->action("forward");
    QAction* actionUp = d->dirOp->actionCollection()->action("up");
    QAction* actionReload = d->dirOp->actionCollection()->action("reload");

    // create filter selection combobox
    QWidget* filterBox = new QWidget( d->toolBox );
    QHBoxLayout* filterLayout = new QHBoxLayout( filterBox );
    filterLayout->addWidget( new QLabel( i18n("Filter:"), filterBox ) );
    d->filterWidget = new KFileFilterCombo( filterBox );
    filterLayout->addWidget( d->filterWidget );
    filterLayout->setContentsMargins( 0, 0, 0, 0 );

    d->filterWidget->setEditable( true );
    QString filter = i18n("*|All Files");
    filter += '\n' + i18n("audio/x-mp3 audio/x-wav application/x-ogg |Sound Files");
    filter += '\n' + i18n("audio/x-wav |Wave Sound Files");
    filter += '\n' + i18n("audio/x-mp3 |MP3 Sound Files");
    filter += '\n' + i18n("application/x-ogg |Ogg Vorbis Sound Files");
    filter += '\n' + i18n("video/mpeg |MPEG Video Files");
    d->filterWidget->setFilter(filter);

    d->actionShowBookmarks = new QAction( i18n("Show Bookmarks"), d->toolBox );
    d->actionShowBookmarks->setCheckable( true );

    KActionMenu* actionOptions = new KActionMenu( QIcon::fromTheme("configure"), i18n("Options"), d->toolBox );
    actionOptions->setDelayed( false );
    actionOptions->addAction( d->dirOp->actionCollection()->action("sorting menu") );
    actionOptions->addAction( d->dirOp->actionCollection()->action("view menu") );
    actionOptions->addSeparator();
    actionOptions->addAction( d->dirOp->actionCollection()->action("decoration menu") );
    actionOptions->addSeparator();
    actionOptions->addAction( d->dirOp->actionCollection()->action("show hidden") );
    actionOptions->addAction( d->actionShowBookmarks );
    actionOptions->addAction( d->dirOp->actionCollection()->action("preview") );

    d->toolBox->addAction( actionBack );
    d->toolBox->addAction( actionForward );
    d->toolBox->addAction( actionUp );
    d->toolBox->addAction( actionReload );
    d->toolBox->addSeparator();
    d->toolBox->addAction( d->dirOp->actionCollection()->action("short view") );
    d->toolBox->addAction( d->dirOp->actionCollection()->action("detailed view") );
    d->toolBox->addSeparator();
    d->toolBox->addSeparator();
    d->toolBox->addWidget( filterBox );
    d->toolBox->addAction( new KToolBarSpacerAction( d->toolBox ) );
    d->toolBox->addAction( actionOptions );
    d->toolBox->addAction( d->dirOp->bookmarkMenu() );

    if( QAction* action = d->dirOp->actionCollection()->action("show hidden") ) {
        action->setShortcut( Qt::ALT + Qt::Key_Period );
        action->setShortcutContext( Qt::ApplicationShortcut );
    }

    connect( d->dirOp, SIGNAL(urlEntered(QUrl)), this, SIGNAL(urlEntered(QUrl)) );
    connect( d->filterWidget, SIGNAL(filterChanged()), SLOT(slotFilterChanged()) );
    connect( d->actionShowBookmarks, SIGNAL(toggled(bool)), d->dirOp->bookmarkMenu(), SLOT(setVisible(bool)) );
}


K3b::FileView::~FileView()
{
    delete d;
}


KActionCollection* K3b::FileView::actionCollection() const
{
    return d->dirOp->actionCollection();
}


void K3b::FileView::setUrl(const QUrl& url, bool forward)
{
    qDebug() << url;
    d->dirOp->setUrl( url, forward );
}


QUrl K3b::FileView::url()
{
    return d->dirOp->url();
}

void K3b::FileView::slotFilterChanged()
{
    QString filter = d->filterWidget->currentFilter();
    d->dirOp->clearFilter();

    if( filter.indexOf( '/' ) > -1 ) {
        QStringList types = filter.split( ' ' );
        types.prepend( "inode/directory" );
        d->dirOp->setMimeFilter( types );
    }
    else
        d->dirOp->setNameFilter( filter );

    d->dirOp->rereadDir();
}


void K3b::FileView::reload()
{
    d->dirOp->rereadDir();
}


void K3b::FileView::saveConfig( KConfigGroup grp )
{
    d->dirOp->writeConfig(grp);
}


void K3b::FileView::readConfig( const KConfigGroup& grp )
{
    d->dirOp->readConfig(grp);
    d->actionShowBookmarks->setChecked( d->dirOp->bookmarkMenu()->isVisible() );
}


