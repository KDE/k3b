/*
 *
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bplacesmodel.h"
#include "k3bdevicemodel.h"

#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bcore.h>

#include <KDirModel>
#include <KDirLister>
#include <KIcon>


namespace {
    class Place {
    public:
        QString name;
        KIcon icon;
        KDirModel* model;
    };
}

class K3bPlacesModel::Private
{
public:
    K3bDeviceModel* deviceModel;
    QList<Place> places;

    int modelRow( const QAbstractItemModel* model ) {
        if ( model == deviceModel ) {
            return 0;
        }
        else {
            for ( int i = 0; i < places.count(); ++i ) {
                if ( places[i].model == model ) {
                    return i+1;
                }
            }
        }
        return -1;
    }
};



class K3bDirModel : public KDirModel {
public:
    K3bDirModel( QObject* parent )
        : KDirModel( parent ) {
    }

    QModelIndex parent( const QModelIndex& index ) const {
        QModelIndex i = KDirModel::parent( index );
        if ( i.isValid() ) {
            return i;
        }
        else {
            return static_cast<K3bPlacesModel*>( QObject::parent() )->parent( index );
        }
    }
};


K3bPlacesModel::K3bPlacesModel( QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private() )
{
    d->deviceModel = new K3bDeviceModel( this );
    connect( d->deviceModel, SIGNAL( modelAboutToBeReset() ),
             this, SIGNAL( modelAboutToBeReset() ) );
    connect( d->deviceModel, SIGNAL( modelReset() ),
             this, SIGNAL( modelReset() ) );
    slotDevicesChanged( k3bcore->deviceManager() );
}


K3bPlacesModel::~K3bPlacesModel()
{
    delete d;
}


int K3bPlacesModel::columnCount( const QModelIndex& parent ) const
{
    return 1;
}


QVariant K3bPlacesModel::data( const QModelIndex& index, int role ) const
{
    kDebug() << index << role;
    if ( index.model() == this ) {
        // provide the root elements of the places
        int row = index.row() - d->deviceModel->rowCount();
        switch( role ) {
        case Qt::DisplayRole:
            return d->places[row].name;

        case Qt::DecorationRole:
            return d->places[row].icon;

        default:
            return QVariant();
        }
    }
    else {
        return index.data( role );
    }
}


QModelIndex K3bPlacesModel::index( int row, int column, const QModelIndex& parent ) const
{
    kDebug() << row << column << parent;
    if ( row < 0 ) {
        return QModelIndex();
    }

    if ( parent.isValid() ) {
        if ( parent.model() == this ) {
            // listing the root of a place
            QModelIndex i = d->places[parent.row() - d->deviceModel->rowCount()].model->index( row, column );
            kDebug() << "Creating index for place first level:" << i;
            return i;//createIndex( i.row(), i.column(), i.internalPointer() );
        }
        else {
            // the default case of listing a place
            QModelIndex i = parent.model()->index( row, column, parent );
            kDebug() << "Creating place index:" << i;
            return i;
        }
    }
    else {
        if ( row < d->deviceModel->rowCount() ) {
            // one of our device items
            QModelIndex i = d->deviceModel->index( row, column );
            kDebug() << "Creating device index:" << i;
            return i;
        }
        else if ( row < d->deviceModel->rowCount() + d->places.count() ) {
            // one of our places root items
            QModelIndex i = createIndex( row, column );
            kDebug() << "Creating place root index:" << i;
            return i;
        }
    }

    return QModelIndex();
}


QModelIndex K3bPlacesModel::parent( const QModelIndex& index ) const
{
    kDebug() << index;
    if ( index.model() == this ) {
        return QModelIndex();
    }
    else if ( index.model() == d->deviceModel ) {
        // This never happens
        kDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX - parent for device!";
        return QModelIndex();
    }
    else {
        // gets only called for top level place items
        return createIndex( d->modelRow( index.model() ), 0 );
    }
}


int K3bPlacesModel::rowCount( const QModelIndex& parent ) const
{
    kDebug() << parent;
    if ( parent.isValid() ) {
        if ( parent.model() == this ) {
            if ( parent.row() < d->deviceModel->rowCount() ) {
                // I don't think this can happen
                Q_ASSERT( 0 );
            }
            else {
                // get the root count from the place
                kDebug() << "valid parent. rowcount place root:" << d->places[parent.row() - d->deviceModel->rowCount()].model->rowCount();
                return d->places[parent.row() - d->deviceModel->rowCount()].model->rowCount();
            }
        }
        else {
            kDebug() << "valid parent. rowcount within a place:" << parent.model()->rowCount( parent );
            return parent.model()->rowCount( parent );
        }
    }
    else {
        kDebug() << "invalid parent. Global rowcount:" << d->deviceModel->rowCount() + d->places.count();
        return d->deviceModel->rowCount() + d->places.count();
    }
}


void K3bPlacesModel::addPlace( const QString& name, const KIcon& icon, const KUrl& rootUrl )
{
    Place place;
    place.name = name;
    place.icon = icon;
    place.model = new K3bDirModel( this );
    place.model->dirLister()->openUrl( rootUrl, KDirLister::Keep );

    connect( place.model, SIGNAL( modelAboutToBeReset() ),
             this, SIGNAL( modelAboutToBeReset() ) );
    connect( place.model, SIGNAL( modelReset() ),
             this, SIGNAL( modelReset() ) );

    connect( place.model, SIGNAL( rowsAboutToBeInserted( const QModelIndex&, int, int ) ),
             this, SIGNAL( rowsAboutToBeInserted( const QModelIndex&, int, int ) ) );
    connect( place.model, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
             this, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ) );

    connect( place.model, SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int )  ),
             this, SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ) );
    connect( place.model, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ),
             this, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ) );

    connect( place.model, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ),
             this, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ) );

    d->places.append( place );
}


void K3bPlacesModel::slotDevicesChanged( K3bDevice::DeviceManager* dm )
{
    d->deviceModel->setDevices( dm->allDevices() );
}

#include "k3bplacesmodel.moc"
