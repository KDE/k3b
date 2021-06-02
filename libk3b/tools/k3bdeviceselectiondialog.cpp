/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/



#include "k3bdeviceselectiondialog.h"
#include "k3bdevice.h"
#include "k3bdevicecombobox.h"
#include "k3bcore.h"
#include "k3bdevicemanager.h"
#include "k3b_i18n.h"

#include <QString>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLayout>
#include <QLabel>
#include <QVBoxLayout>


class K3b::DeviceSelectionDialog::Private
{
public:
    K3b::DeviceComboBox* comboDevices;
};


K3b::DeviceSelectionDialog::DeviceSelectionDialog( QWidget* parent,
						    const QString& text )
    : QDialog( parent ),
      d( new Private() )
{
    setWindowTitle( i18n("Device Selection") );

    QVBoxLayout* lay = new QVBoxLayout( this );

    QLabel* label = new QLabel( text.isEmpty() ? i18n("Please select a device:") : text, this );
    d->comboDevices = new K3b::DeviceComboBox( this );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
    connect( buttonBox, SIGNAL(accepted()), this, SLOT(accept()) );
    connect( buttonBox, SIGNAL(rejected()), this, SLOT(reject()) );

    lay->addWidget( label );
    lay->addWidget( d->comboDevices );
    lay->addWidget( buttonBox );
}


K3b::DeviceSelectionDialog::~DeviceSelectionDialog()
{
    delete d;
}


void K3b::DeviceSelectionDialog::addDevice( K3b::Device::Device* dev )
{
    d->comboDevices->addDevice( dev );
}


void K3b::DeviceSelectionDialog::addDevices( const QList<K3b::Device::Device*>& list )
{
    d->comboDevices->addDevices( list );
}


K3b::Device::Device* K3b::DeviceSelectionDialog::selectedDevice() const
{
    return d->comboDevices->selectedDevice();
}


void K3b::DeviceSelectionDialog::setSelectedDevice( K3b::Device::Device* dev )
{
    d->comboDevices->setSelectedDevice( dev );
}


K3b::Device::Device* K3b::DeviceSelectionDialog::selectDevice( QWidget* parent,
                                                           const QList<K3b::Device::Device*>& devices,
                                                           const QString& text )
{
    if( devices.isEmpty() )
        return 0;
    if( devices.count() == 1 )
        return devices[0];

    K3b::DeviceSelectionDialog dlg( parent, text );
    dlg.addDevices( devices );

    if( dlg.exec() == Accepted )
        return dlg.selectedDevice();
    else
        return 0;
}

K3b::Device::Device* K3b::DeviceSelectionDialog::selectDevice( QWidget* parent,
                                                           const QString& text )
{
    return selectDevice( parent, k3bcore->deviceManager()->allDevices(), text );


}


K3b::Device::Device* K3b::DeviceSelectionDialog::selectWriter( QWidget* parent, const QString& text )
{
    return selectDevice( parent, k3bcore->deviceManager()->burningDevices(), text );
}



