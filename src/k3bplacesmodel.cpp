/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bplacesmodel.h"
#include "k3bdevicemodel.h"

#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bcore.h"

#include <KDirModel>
#include <KDirSortFilterProxyModel>
#include <KDirLister>
#include <KIcon>


class K3b::PlacesModel::Private
{
public:
    K3b::DeviceModel* deviceModel;
    QMap<KDirModel*, KDirSortFilterProxyModel*> dirModels;
};



K3b::PlacesModel::PlacesModel( QObject* parent )
    : K3b::MetaItemModel( parent ),
      d( new Private() )
{
    d->deviceModel = new K3b::DeviceModel( this );
    addSubModel( "Devices", KIcon( "media-optical" ), d->deviceModel, true );

    connect( d->deviceModel, SIGNAL( modelAboutToBeReset() ),
             this, SIGNAL( modelAboutToBeReset() ) );
    connect( d->deviceModel, SIGNAL( modelReset() ),
             this, SIGNAL( modelReset() ) );
    connect( k3bcore->deviceManager(), SIGNAL( changed( K3b::Device::DeviceManager* ) ),
             this, SLOT( slotDevicesChanged( K3b::Device::DeviceManager* ) ) );
    slotDevicesChanged( k3bcore->deviceManager() );
}


K3b::PlacesModel::~PlacesModel()
{
    delete d;
}


KFileItem K3b::PlacesModel::itemForIndex( const QModelIndex& index ) const
{
    KDirSortFilterProxyModel* proxy = static_cast<KDirSortFilterProxyModel*>( subModelForIndex( index ) );
    KDirModel* model = qobject_cast<KDirModel*>( proxy->sourceModel() );
    if ( model ) {
        return model->itemForIndex( proxy->mapToSource( mapToSubModel( index ) ) );
    }
    return KFileItem();
}


K3b::Device::Device* K3b::PlacesModel::deviceForIndex( const QModelIndex& index ) const
{
    if ( qobject_cast<K3b::DeviceModel*>( subModelForIndex( index ) ) == d->deviceModel ) {
        return d->deviceModel->deviceForIndex( mapToSubModel( index ) );
    }
    return 0;
}


QModelIndex K3b::PlacesModel::indexForDevice( K3b::Device::Device* dev ) const
{
    return mapFromSubModel( d->deviceModel->indexForDevice( dev ) );
}


void K3b::PlacesModel::expandToUrl( const KUrl& url )
{
    kDebug() << url;

    // search for the best suited place that contains this URL
    int maxDepth = 0;
    KDirModel* modelToExpand = 0;

    for( QMap<KDirModel*, KDirSortFilterProxyModel*>::iterator it = d->dirModels.begin();
         it != d->dirModels.end(); ++it ) {
        KDirModel* model = it.key();
        KUrl url = model->dirLister()->url();
        if ( url.isParentOf( url ) ) {
            if ( url.path().length() > maxDepth ) {
                maxDepth = url.path().length();
                modelToExpand = model;
            }
        }
    }

    if ( modelToExpand ) {
        kDebug() << modelToExpand->dirLister()->url() << "will be expanded.";
        modelToExpand->expandToUrl( url );
    }
}


void K3b::PlacesModel::addPlace( const QString& name, const KIcon& icon, const KUrl& rootUrl )
{
    KDirModel* model = new KDirModel( this );
    connect( model, SIGNAL( expand( const QModelIndex& ) ), this, SLOT( slotExpand( const QModelIndex& ) ) );
    model->dirLister()->setDirOnlyMode( true );
    model->dirLister()->openUrl( rootUrl, KDirLister::Keep );

    KDirSortFilterProxyModel* proxy = new KDirSortFilterProxyModel( model );
    proxy->setSourceModel( model );
    d->dirModels.insert( model, proxy );
    addSubModel( name, icon, proxy );
}


void K3b::PlacesModel::slotExpand( const QModelIndex& index )
{
    kDebug() << index;
    KDirModel* model = ( KDirModel* )index.model();
    emit expand( mapFromSubModel( d->dirModels[model]->mapFromSource( index ) ) );
}


void K3b::PlacesModel::slotDevicesChanged( K3b::Device::DeviceManager* dm )
{
    kDebug();
    d->deviceModel->setDevices( dm->allDevices() );
}

#include "k3bplacesmodel.moc"
