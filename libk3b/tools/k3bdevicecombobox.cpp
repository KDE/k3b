/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdevicecombobox.h"
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bcore.h>

#include <klocale.h>

#include <qmap.h>
#include <q3ptrvector.h>
//Added by qt3to4:
#include <Q3PtrList>


class K3bDeviceComboBox::Private
{
public:
  QMap<QString, int> deviceIndexMap;
  Q3PtrVector<K3bDevice::Device> devices;
};


K3bDeviceComboBox::K3bDeviceComboBox( QWidget* parent, const char* name )
  : KComboBox( parent, name )
{
  d = new Private();
  connect( this, SIGNAL(activated(int)),
	   this, SLOT(slotActivated(int)) );
  connect( k3bcore->deviceManager(), SIGNAL(changed(K3bDevice::DeviceManager*)),
	   this, SLOT(slotDeviceManagerChanged(K3bDevice::DeviceManager*)) );
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
  int devIndex = -2;
  bool addDevice = false;
  for( int i = 0; i < count(); ++i ) {
    if( dev->vendor() == d->devices[i]->vendor() &&
	dev->description() == d->devices[i]->description() ) {
      addDevice = true;
      if( devIndex < -1 ) // when devIndex == -1 we already found two devices.
	devIndex = i;
      else
	devIndex = -1; // when there are already two or more equal devices they have already been updated
    }
  }

  // update the existing device item
  if( devIndex >= 0 ) {
    changeItem( d->devices[devIndex]->vendor() + " " + 
		d->devices[devIndex]->description() + 
		" (" + d->devices[devIndex]->blockDeviceName() + ")",
		devIndex );
    d->deviceIndexMap[d->devices[devIndex]->devicename()] = devIndex;
  }

  // add the new device item
  if( addDevice )
    insertItem( dev->vendor() + " " + dev->description() + " (" + dev->blockDeviceName() + ")" );
  else
    insertItem( dev->vendor() + " " + dev->description() );

  d->deviceIndexMap[dev->devicename()] = count()-1;
  d->devices.resize( count() );
  d->devices.insert(count()-1, dev);
}


void K3bDeviceComboBox::removeDevice( K3bDevice::Device* dev )
{
  if( dev ) {
    if( d->deviceIndexMap.contains(dev->devicename()) ) {
      // let's make it easy and recreate the whole list
      K3bDevice::Device* selDev = selectedDevice();
      Q3PtrList<K3bDevice::Device> devices;
      for( unsigned int i = 0; i < d->devices.size(); ++i )
	devices.append( d->devices[i] );

      clear();

      devices.removeRef( dev );

      addDevices( devices );
      setSelectedDevice( selDev );
    }
  }
}


void K3bDeviceComboBox::addDevices( const Q3PtrList<K3bDevice::Device>& list )
{
  for( Q3PtrListIterator<K3bDevice::Device> it( list );
       it.current(); ++it )
    addDevice( it.current() );
}


void K3bDeviceComboBox::refreshDevices( const Q3PtrList<K3bDevice::Device>& list )
{
  K3bDevice::Device* selDev = selectedDevice();
  clear();
  if( !list.containsRef( selDev ) )
    selDev = 0;
  addDevices( list );
  setSelectedDevice( selDev );
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


void K3bDeviceComboBox::slotDeviceManagerChanged( K3bDevice::DeviceManager* dm )
{
  unsigned int i = 0;
  while( i < d->devices.size() ) {
    if( !dm->allDevices().containsRef( d->devices[i] ) ) {
      removeDevice( d->devices[i] );
      i = 0;
    }
    else
      ++i;
  }
}

#include "k3bdevicecombobox.moc"
