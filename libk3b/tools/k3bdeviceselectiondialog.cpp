/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#include "k3bdeviceselectiondialog.h"
#include <k3bdevice.h>
#include <k3bdevicecombobox.h>
#include <k3bcore.h>
#include <k3bdevicemanager.h>

#include <qcombobox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qstring.h>
#include <qframe.h>

#include <klocale.h>


class K3bDeviceSelectionDialog::Private
{
public:
  K3bDeviceComboBox* comboDevices;
};


K3bDeviceSelectionDialog::K3bDeviceSelectionDialog( QWidget* parent, 
						    const char* name, 
						    const QString& text,
						    bool modal )
  : KDialogBase( KDialogBase::Plain, 
		 i18n("Device Selection"), 
		 Ok|Cancel, 
		 Ok,
		 parent,
		 name,
		 modal )
{
  d = new Private();

  QGridLayout* lay = new QGridLayout( plainPage() );

  QLabel* label = new QLabel( text.isEmpty() ? i18n("Please select a device:") : text, plainPage() );
  d->comboDevices = new K3bDeviceComboBox( plainPage() );

  //  lay->setMargin( marginHint() );
  lay->setSpacing( spacingHint() );
  lay->addWidget( label, 0, 0 );
  lay->addWidget( d->comboDevices, 1, 0 );
  lay->setRowStretch( 2, 1 );
}


K3bDeviceSelectionDialog::~K3bDeviceSelectionDialog()
{
  delete d;
}


void K3bDeviceSelectionDialog::addDevice( K3bDevice::Device* dev )
{
  d->comboDevices->addDevice( dev );
}


void K3bDeviceSelectionDialog::addDevices( const QPtrList<K3bDevice::Device>& list )
{
  d->comboDevices->addDevices( list );
}


K3bDevice::Device* K3bDeviceSelectionDialog::selectedDevice() const
{
  return d->comboDevices->selectedDevice();
}


void K3bDeviceSelectionDialog::setSelectedDevice( K3bDevice::Device* dev )
{
  d->comboDevices->setSelectedDevice( dev );
}


K3bDevice::Device* K3bDeviceSelectionDialog::selectDevice( QWidget* parent, 
							       const QPtrList<K3bDevice::Device>& devices,
							       const QString& text )
{
  if( devices.isEmpty() )
    return 0;
  if( devices.count() == 1 )
    return devices.getFirst();

  K3bDeviceSelectionDialog dlg( parent, 0, text );
  dlg.addDevices( devices );

  if( dlg.exec() == Accepted )
    return dlg.selectedDevice();
  else
    return 0;
}

K3bDevice::Device* K3bDeviceSelectionDialog::selectDevice( QWidget* parent, 
							       const QString& text )
{
  return selectDevice( parent, k3bcore->deviceManager()->allDevices(), text );


}


K3bDevice::Device* K3bDeviceSelectionDialog::selectWriter( QWidget* parent, const QString& text )
{
  return selectDevice( parent, k3bcore->deviceManager()->burningDevices(), text );
}


#include "k3bdeviceselectiondialog.moc"
