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

#include "k3bdevicecombobox.h"
#include <k3bdevice.h>

#include <klocale.h>

#include <qmap.h>
#include <qptrvector.h>


class K3bDeviceComboBox::Private
{
public:
  QMap<QString, int> deviceIndexMap;
  QPtrVector<K3bDevice::Device> devices;
};


K3bDeviceComboBox::K3bDeviceComboBox( QWidget* parent, const char* name )
  : KComboBox( parent, name )
{
  d = new Private();
  connect( this, SIGNAL(activated(int)),
	   this, SLOT(slotActivated(int)) );
}


K3bDeviceComboBox::~K3bDeviceComboBox()
{
  delete d;
}

K3bDevice::Device* K3bDeviceComboBox::selectedDevice() const
{
  if ( count() > 0 )
    return d->devices[currentItem()];
  else
    return 0;
}


void K3bDeviceComboBox::addDevice( K3bDevice::Device* dev )
{
  insertItem( dev->vendor() + " " + dev->description() /*+ " (" + dev->blockDeviceName() + ")"*/ );
  d->deviceIndexMap[dev->devicename()] = count()-1;
  d->devices.resize( count() );
  d->devices.insert(count()-1, dev);
}


void K3bDeviceComboBox::addDevices( const QPtrList<K3bDevice::Device>& list )
{
  for( QPtrListIterator<K3bDevice::Device> it( list );
       it.current(); ++it )
    addDevice( it.current() );
}


void K3bDeviceComboBox::setSelectedDevice( K3bDevice::Device* dev )
{
  if( dev ) {
    if( d->deviceIndexMap.contains(dev->devicename()) ) {
      setCurrentItem( d->deviceIndexMap[dev->devicename()] );
      emit selectionChanged( dev );
    }
  }
}


void K3bDeviceComboBox::clear()
{
  d->deviceIndexMap.clear();
  d->devices.clear();
  KComboBox::clear();
}


void K3bDeviceComboBox::slotActivated( int i )
{
  emit selectionChanged( d->devices[i] );
}

#include "k3bdevicecombobox.moc"
