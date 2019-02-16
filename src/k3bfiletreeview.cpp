/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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


#include "k3bfiletreeview.h"
#include "k3bappdevicemanager.h"
#include "k3bapplication.h"
#include "k3bdevice.h"
#include "k3bglobals.h"
#include "k3bplacesmodel.h"
#include "k3bdevicedelegate.h"
#include "k3bdevicemenu.h"
#include "k3baction.h"
#include "k3b.h"

#include <KLocalizedString>
#include <KFileItem>
#include <KActionMenu>

#include <QUrl>
#include <QAction>



class K3b::FileTreeView::Private
{
public:
    Private()
        : deviceManager(0) {
    }

    K3b::Device::DeviceManager* deviceManager;

    K3b::PlacesModel* model;

    KActionCollection* actionCollection;
    K3b::DeviceMenu* devicePopupMenu;
    KActionMenu* urlPopupMenu;
    bool menuEnabled;
};


K3b::FileTreeView::FileTreeView( QWidget *parent )
    : QTreeView( parent ),
      d( new Private() )
{
    setHeaderHidden( true );

    setContextMenuPolicy( Qt::CustomContextMenu );
    setEditTriggers( QAbstractItemView::NoEditTriggers );
    setSelectionMode(QAbstractItemView::SingleSelection);
//    setSortingEnabled( true );
//    setRootIsDecorated( false );
    setDragEnabled( true );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );

    K3b::DeviceDelegate* delegate = new K3b::DeviceDelegate(this);
    setItemDelegate(delegate);

    d->model = new K3b::PlacesModel( this );
    setModel( d->model );

    d->actionCollection = new KActionCollection( this );
    d->devicePopupMenu = new K3b::DeviceMenu( this );
    d->urlPopupMenu = new KActionMenu(this);
    initActions();

    // react on K3b::PlacesModel::expandToUrl calls
    connect( d->model, SIGNAL(expand(QModelIndex)),
             this, SLOT(slotExpandUrl(QModelIndex)) );

    connect( this, SIGNAL(clicked(QModelIndex)), SLOT(slotClicked(QModelIndex)) );
    connect( this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(slotContextMenu(QPoint)) );
}


K3b::FileTreeView::~FileTreeView()
{
    delete d;
}

void K3b::FileTreeView::initActions()
{
    // those actions are supposed to be used with url items
    d->urlPopupMenu->addAction( K3b::createAction(this,i18n("&Add to Project"), 0, Qt::SHIFT+Qt::Key_Return,
                                                  this, SLOT(slotAddFilesToProject()),
                                                  d->actionCollection, "add_files_to_project") );
}

K3b::Device::Device* K3b::FileTreeView::selectedDevice() const
{
    return d->model->deviceForIndex( currentIndex() );
}


QUrl K3b::FileTreeView::selectedUrl() const
{
    KFileItem fileItem = d->model->itemForIndex( currentIndex() );
    if( fileItem.isNull() )
        return QUrl();
    else
        return fileItem.url();
}


void K3b::FileTreeView::slotExpandUrl( const QModelIndex& index )
{
    qDebug();
    setCurrentIndex( index );
    scrollTo( index );
}

void K3b::FileTreeView::slotAddFilesToProject()
{
    QModelIndexList indexes = selectedIndexes();
    QList<QUrl> files;
    foreach(const QModelIndex &index, indexes)
    {
        KFileItem item = d->model->itemForIndex(index);
        if (item.isNull())
            continue;

        files.append(item.url());
    }

    if (!files.isEmpty())
        k3bappcore->k3bMainWindow()->addUrls(files);
}


void K3b::FileTreeView::setSelectedUrl( const QUrl& url )
{
    qDebug();
    KFileItem fileItem = d->model->itemForIndex( currentIndex() );
    if( fileItem.isNull() || !fileItem.url().matches( url, QUrl::StripTrailingSlash ) ) {
        d->model->expandToUrl( url );
    }
}


void K3b::FileTreeView::setSelectedDevice( K3b::Device::Device* dev )
{
    Device::Device* currentDev = d->model->deviceForIndex( currentIndex() );
    if( currentDev != dev ) {
        setCurrentIndex( d->model->indexForDevice( dev ) );
    }
}


void K3b::FileTreeView::slotClicked( const QModelIndex& index )
{
    if ( K3b::Device::Device* dev = d->model->deviceForIndex( index ) ) {
        k3bappcore->appDeviceManager()->setCurrentDevice( dev );
        emit activated( dev );
    }
    else if ( index.isValid() ) {
        KFileItem fileItem = d->model->itemForIndex( index );
        if ( !fileItem.isNull() ) {
            emit activated( fileItem.url() );
        }
    }
}


void K3b::FileTreeView::slotContextMenu( const QPoint& pos )
{
    // check if the context menu is for a device item
    QModelIndex index = indexAt( pos );
    if ( K3b::Device::Device* dev = d->model->deviceForIndex( index ) ) {
        k3bappcore->appDeviceManager()->setCurrentDevice( dev );
        d->devicePopupMenu->exec( mapToGlobal( pos ) );
    }

    // ... or if it is for an url item
    KFileItem item = d->model->itemForIndex( index );
    if ( !item.isNull() )
    {
        // enable/disable the "add to project" action
        d->actionCollection->action("add_files_to_project")->setEnabled(k3bappcore->k3bMainWindow()->activeView() != 0);

        // and shows the menu
        d->urlPopupMenu->menu()->exec( mapToGlobal( pos ) );
    }
}


