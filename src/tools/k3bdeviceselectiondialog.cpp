/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#include "k3bdeviceselectiondialog.h"
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>
#include <k3bcore.h>

#include <qcombobox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qstring.h>
#include <qframe.h>

#include <klocale.h>


class K3bDeviceSelectionDialog::Private
{
public:
  QComboBox* comboDevices;
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
  d->comboDevices = new QComboBox( plainPage() );

  lay->addWidget( label, 0, 0 );
  lay->addWidget( d->comboDevices, 1, 0 );
  lay->setRowStretch( 2, 1 );
}


K3bDeviceSelectionDialog::~K3bDeviceSelectionDialog()
{
}


void K3bDeviceSelectionDialog::addDevice( K3bCdDevice::CdDevice* dev )
{
  d->comboDevices->insertItem( dev->vendor() + " " + dev->description() + " (" + dev->blockDeviceName() + ")" );
}


K3bDevice* K3bDeviceSelectionDialog::selectedDevice() const
{
  const QString& s = d->comboDevices->currentText();

  QString strDev = s.mid( s.find('(') + 1, s.find(')') - s.find('(') - 1 );
 
  K3bDevice* dev =  k3bcore->deviceManager()->deviceByName( strDev );
  if( !dev )
    kdDebug() << "(K3bDeviceSelectionDialog) could not find device " << s << endl;
		
  return dev;
}


K3bCdDevice::CdDevice* K3bDeviceSelectionDialog::selectDevice( QWidget* parent, 
							       const QPtrList<K3bCdDevice::CdDevice>& devices,
							       const QString& text )
{
  if( devices.isEmpty() )
    return 0;
  if( devices.count() == 1 )
    return devices.getFirst();

  K3bDeviceSelectionDialog dlg( parent, 0, text );
  for( QPtrListIterator<K3bCdDevice::CdDevice> it( devices ); it.current(); ++it )
    dlg.addDevice( it.current() );

  if( dlg.exec() == Accepted )
    return dlg.selectedDevice();
  else
    return 0;
}


K3bDevice* K3bDeviceSelectionDialog::selectWriter( QWidget* parent, const QString& text )
{
  return selectDevice( parent, k3bcore->deviceManager()->burningDevices(), text );
}


#include "k3bdeviceselectiondialog.moc"
