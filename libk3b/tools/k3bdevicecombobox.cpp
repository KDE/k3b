/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bdevicecombobox.h"
#include "k3bdevicemodel.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bcore.h"
#include "k3b_i18n.h"

#include <QMap>


class K3b::DeviceComboBox::Private
{
public:
    K3b::DeviceModel* model;
};


K3b::DeviceComboBox::DeviceComboBox( QWidget* parent )
    : KComboBox( parent )
{
    d = new Private();
    d->model = new K3b::DeviceModel( this );
    setModel( d->model );

    connect( this, SIGNAL(activated(int)),
             this, SLOT(slotActivated(int)) );
}


K3b::DeviceComboBox::~DeviceComboBox()
{
    delete d;
}


K3b::Device::Device* K3b::DeviceComboBox::selectedDevice() const
{
    int index = currentIndex();
    if ( index >= 0 ) {
        return d->model->deviceForIndex( d->model->index( index, 0 ) );
    }
    else {
        return 0;
    }
}


void K3b::DeviceComboBox::addDevice( K3b::Device::Device* dev )
{
    d->model->addDevice( dev );
}


void K3b::DeviceComboBox::removeDevice( K3b::Device::Device* dev )
{
    d->model->removeDevice( dev );
}


void K3b::DeviceComboBox::addDevices( const QList<K3b::Device::Device*>& list )
{
    d->model->addDevices( list );
}


void K3b::DeviceComboBox::refreshDevices( const QList<K3b::Device::Device*>& list )
{
    d->model->setDevices( list );}


void K3b::DeviceComboBox::setSelectedDevice( K3b::Device::Device* dev )
{
    setCurrentIndex( d->model->indexForDevice( dev ).row() );
}


void K3b::DeviceComboBox::slotActivated( int i )
{
    emit selectionChanged( d->model->deviceForIndex( d->model->index( i, 0 ) ) );
}


