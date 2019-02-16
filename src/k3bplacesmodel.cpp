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

#include "k3bcore.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bmediacache.h"
#include "k3bmedium.h"

#include <KDirSortFilterProxyModel>
#include <KFilePlacesModel>
#include <KDirModel>
#include <KDirLister>
#include <Solid/StorageAccess>

#include <QIcon>

typedef QMap<KDirModel*, KDirSortFilterProxyModel*> DirModels;


class K3b::PlacesModel::Private
{
public:
    K3b::DeviceModel* deviceModel;
    KFilePlacesModel* filePlacesModel;
    DirModels dirModels;
};



K3b::PlacesModel::PlacesModel( QObject* parent )
    : K3b::MetaItemModel( parent ),
      d( new Private() )
{
    d->deviceModel = new K3b::DeviceModel( this );
    d->filePlacesModel = new KFilePlacesModel( this );
    addSubModel( "Devices", QIcon::fromTheme( "media-optical" ), d->deviceModel, true );
    
    // TODO: Currently our place list doesn't follow changes KFilePlacesModel.
    //       This needs to be changed. Adding, removing and editing places would be also nice.
    for( int i = 0; i < d->filePlacesModel->rowCount(); ++i ) {
        QModelIndex place = d->filePlacesModel->index( i, 0 );
        QUrl url = d->filePlacesModel->url( place );
        
        // Let's filter out device-related places
        // and custom protocols (we doesn't support burning from them)
        if( !d->filePlacesModel->isDevice( place ) && url.isLocalFile() ) {
            addPlace(
                d->filePlacesModel->text( place ),
                d->filePlacesModel->icon( place ),
                url );
        }
    }

    connect( d->deviceModel, SIGNAL(modelAboutToBeReset()),
             this, SIGNAL(modelAboutToBeReset()) );
    connect( d->deviceModel, SIGNAL(modelReset()),
             this, SIGNAL(modelReset()) );
    connect( k3bcore->deviceManager(), SIGNAL(changed(K3b::Device::DeviceManager*)),
             this, SLOT(slotDevicesChanged(K3b::Device::DeviceManager*)) );
    slotDevicesChanged( k3bcore->deviceManager() );
}


K3b::PlacesModel::~PlacesModel()
{
    delete d;
}


KFileItem K3b::PlacesModel::itemForIndex( const QModelIndex& index ) const
{
    // KDirSortFilterProxyModel does not have the Q_OBJECT macro. Thus, we need to use dynamic_cast
    KDirSortFilterProxyModel* proxy = dynamic_cast<KDirSortFilterProxyModel*>( subModelForIndex( index ) );
    if ( proxy ) {
        KDirModel* model = qobject_cast<KDirModel*>( proxy->sourceModel() );
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


void K3b::PlacesModel::expandToUrl( const QUrl& url )
{
    qDebug() << url;
    
    // Check if url is not device's
    Q_FOREACH( Device::Device* device, d->deviceModel->devices() )
    {
        if( Solid::StorageAccess* solidStorage = device->solidStorage() ) {
            QUrl parent = QUrl::fromLocalFile( solidStorage->filePath() );
            if( parent.isParentOf( url ) ) {
                qDebug() << url << "will be expanded to device" << device->description();
                emit expand( mapFromSubModel( d->deviceModel->indexForDevice( device ) ) );
                return;
            }
        }
        else if( url.scheme() == "audiocd" )
        {
            const Medium& medium = k3bcore->mediaCache()->medium( device );
            if( medium.content() & Medium::ContentAudio )
            {
                qDebug() << url << "will be expanded to device" << device->description();
                emit expand( mapFromSubModel( d->deviceModel->indexForDevice( device ) ) );
                return;
            }
        }
    }

    // search for the best suited place that contains this URL
    int maxDepth = 0;
    KDirModel* modelToExpand = 0;

    for( DirModels::iterator it = d->dirModels.begin(); it != d->dirModels.end(); ++it ) {
        KDirModel* model = it.key();
        QUrl parent = model->dirLister()->url();
        if ( parent.isParentOf( url ) ) {
            if ( parent.path().length() > maxDepth ) {
                maxDepth = parent.path().length();
                modelToExpand = model;
            }
        }
    }

    if ( modelToExpand ) {
        qDebug() << modelToExpand->dirLister()->url() << " will be expanded.";
        if( modelToExpand->dirLister()->url().matches( url, QUrl::StripTrailingSlash ) ) {
            emit expand( indexForSubModel( d->dirModels[ modelToExpand ] ) );
        }
        else {
            modelToExpand->expandToUrl( url );
        }
    }
}


void K3b::PlacesModel::addPlace( const QString& name, const QIcon& icon, const QUrl& rootUrl )
{
    KDirModel* model = new KDirModel( this );
    connect( model, SIGNAL(expand(QModelIndex)), this, SLOT(slotExpand(QModelIndex)) );
    model->dirLister()->setAutoErrorHandlingEnabled( false, 0 );
    model->dirLister()->setDirOnlyMode( true );
    model->dirLister()->openUrl( rootUrl, KDirLister::Keep );

    KDirSortFilterProxyModel* proxy = new KDirSortFilterProxyModel( model );
    proxy->setSourceModel( model );
    d->dirModels.insert( model, proxy );
    addSubModel( name, icon, proxy );
}


void K3b::PlacesModel::slotExpand( const QModelIndex& index )
{
    qDebug() << index;
    KDirModel* model = ( KDirModel* )index.model();
    emit expand( mapFromSubModel( d->dirModels[model]->mapFromSource( index ) ) );
}


void K3b::PlacesModel::slotDevicesChanged( K3b::Device::DeviceManager* dm )
{
    qDebug();
    d->deviceModel->setDevices( dm->allDevices() );
}


