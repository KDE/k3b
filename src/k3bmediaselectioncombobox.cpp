/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bmediaselectioncombobox.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"

#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bdeviceglobals.h>
#include <k3bdiskinfo.h>
#include <k3btoc.h>
#include <k3bcdtext.h>

#include <klocale.h>

#include <qfont.h>
#include <qmap.h>
#include <qptrvector.h>
#include <qtooltip.h>
#include <qlistbox.h>


class K3bMediaSelectionComboBox::ToolTip : public QToolTip
{
public:
  ToolTip( K3bMediaSelectionComboBox* box );
  
  void maybeTip( const QPoint &pos );
  
private:
  K3bMediaSelectionComboBox* m_box;
};


K3bMediaSelectionComboBox::ToolTip::ToolTip( K3bMediaSelectionComboBox* box )
  : QToolTip( box->listBox()->viewport() ),
    m_box( box )
{
}

void K3bMediaSelectionComboBox::ToolTip::maybeTip( const QPoint& pos )
{
  if( !parentWidget() || !m_box )
    return;

  QListBoxItem* item = m_box->listBox()->itemAt( pos );
  if( !item )
    return;

  int index = m_box->listBox()->index( item );

  if( K3bDevice::Device* dev = m_box->deviceAt( index ) ) {
    tip( m_box->listBox()->itemRect( item ),
	 i18n("%1 in %2 %3 (%4)")
	 .arg( k3bappcore->mediaCache()->mediumString( dev ) )
	 .arg( dev->vendor() )
	 .arg( dev->description() )
	 .arg( dev->blockDeviceName() ) );
  }
}





class K3bMediaSelectionComboBox::Private
{
public:
  QMap<K3bDevice::Device*, int> deviceIndexMap;
  QPtrVector<K3bDevice::Device> devices;

  int wantedMediumType;
  int wantedMediumState;

  QFont font;
};


K3bMediaSelectionComboBox::K3bMediaSelectionComboBox( QWidget* parent )
  : KComboBox( false, parent )
{
  d = new Private();

  // set defaults
  d->wantedMediumType = K3bDevice::MEDIA_WRITABLE_CD;
  d->wantedMediumState = K3bDevice::STATE_EMPTY;

  d->font = font();

  connect( this, SIGNAL(activated(int)),
	   this, SLOT(slotActivated(int)) );
  connect( k3bcore->deviceManager(), SIGNAL(changed(K3bDevice::DeviceManager*)),
	   this, SLOT(slotDeviceManagerChanged(K3bDevice::DeviceManager*)) );
  connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3bDevice::Device*)),
	   this, SLOT(slotMediumChanged(K3bDevice::Device*)) );

  updateMedia();

  // initialize the tooltip
  (void)new ToolTip( this );
}


K3bMediaSelectionComboBox::~K3bMediaSelectionComboBox()
{
  delete d;
}


K3bDevice::Device* K3bMediaSelectionComboBox::selectedDevice() const
{
  if( d->devices.count() > 0 )
    return d->devices[currentItem()];
  else
    return 0;
}


void K3bMediaSelectionComboBox::setSelectedDevice( K3bDevice::Device* dev )
{
  if( dev ) {
    if( d->deviceIndexMap.contains( dev ) ) {
      setCurrentItem( d->deviceIndexMap[dev] );
      emit selectionChanged( dev );
    }
  }
}


void K3bMediaSelectionComboBox::setWantedMediumType( int type )
{
  if( type != 0 ) {
    d->wantedMediumType = type;
    updateMedia();
  }
}


void K3bMediaSelectionComboBox::setWantedMediumState( int state )
{
  if( state != 0 ) {
    d->wantedMediumState = state;
    updateMedia();
  }
}


void K3bMediaSelectionComboBox::slotActivated( int i )
{
  if( d->devices.count() > 0 )
    emit selectionChanged( d->devices[i] );
}


void K3bMediaSelectionComboBox::slotMediumChanged( K3bDevice::Device* dev )
{
  updateMedium( dev );
}


void K3bMediaSelectionComboBox::clear()
{
  d->deviceIndexMap.clear();
  d->devices.clear();
  KComboBox::clear();
}


void K3bMediaSelectionComboBox::showNoMediumMessage()
{
  // make it italic
  QFont f( d->font );
  f.setItalic( true );
  setFont( f );

  QString stateString;
  if( d->wantedMediumState == K3bDevice::STATE_EMPTY )
    stateString = i18n("an empty %1 medium");
  else if( d->wantedMediumState == K3bDevice::STATE_INCOMPLETE )
    stateString = i18n("an appendable %1 medium");
  else if( d->wantedMediumState == K3bDevice::STATE_COMPLETE )
    stateString = i18n("a complete %1 medium");
  else if( d->wantedMediumState & (K3bDevice::STATE_EMPTY|K3bDevice::STATE_INCOMPLETE) &&
	   !(d->wantedMediumState & K3bDevice::STATE_COMPLETE) )
    stateString = i18n("an empty or appendable %1 medium");
  else if( d->wantedMediumState & (K3bDevice::STATE_COMPLETE|K3bDevice::STATE_INCOMPLETE) )
    stateString = i18n("a complete or appendable %1 medium");
  else
    stateString = i18n("a %1 medium");

  // this is basicly the same as in K3bEmptyDiskWaiter
  // FIXME: include things like only rewritable dvd or cd since we will probably need that
  QString mediumString;
  if( (d->wantedMediumType & K3bDevice::MEDIA_WRITABLE_DVD) &&
      (d->wantedMediumType & K3bDevice::MEDIA_WRITABLE_CD) )
    mediumString = i18n("CD-R(W) or DVD±R(W)");
  else if( d->wantedMediumType & K3bDevice::MEDIA_WRITABLE_DVD_SL )
    mediumString = i18n("DVD±R(W)");
  else if( d->wantedMediumType & K3bDevice::MEDIA_WRITABLE_DVD_DL )
    mediumString = i18n("Double Layer DVD±R");
  else if( d->wantedMediumType & K3bDevice::MEDIA_WRITABLE_CD )
    mediumString = i18n("CD-R(W)");
  else if( d->wantedMediumType & K3bDevice::MEDIA_DVD_ROM )
    mediumString = i18n("DVD-ROM");
  else
    mediumString = i18n("CD-ROM");

  insertItem( i18n("Please insert %1...").arg( stateString.arg( mediumString ) ) );
}


void K3bMediaSelectionComboBox::updateMedia()
{
  // reset font
  setFont( d->font );

  // remember last selected medium
  K3bDevice::Device* selected = selectedDevice();
  
  clear();

  const QPtrList<K3bDevice::Device>& devices = k3bcore->deviceManager()->allDevices();
  for( QPtrListIterator<K3bDevice::Device> it( devices ); *it; ++it ) {
    K3bDevice::DiskInfo diskInfo = k3bappcore->mediaCache()->diskInfo( *it );
    //
    // also use if wantedMediumState empty and medium rewritable
    // because we can always format/erase/overwrite it
    //
    // DVD+RW is never reported as appendable
    //
    if( diskInfo.mediaType() & d->wantedMediumType &&
	( diskInfo.diskState() & d->wantedMediumState 
	  ||
	  ( d->wantedMediumState & K3bDevice::STATE_EMPTY &&
	    diskInfo.rewritable() ) 
	  ||
	  ( d->wantedMediumState & K3bDevice::STATE_INCOMPLETE &&
	    !diskInfo.empty() &&
	    diskInfo.mediaType() & (K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR) ) ) 
	)
      addMedium( *it );
  }

  //
  // Now in case no usable medium was found show the user a little message
  //
  if( d->devices.isEmpty() ) {
    showNoMediumMessage();
    // inform that we have no medium at all
    emit selectionChanged( 0 );
  }
  else if( selected )
    setSelectedDevice( selected );
  else
    emit selectionChanged( selectedDevice() );
}


void K3bMediaSelectionComboBox::updateMedium( K3bDevice::Device* )
{
  // for now we simply rebuild the whole list
  updateMedia();
}


void K3bMediaSelectionComboBox::addMedium( K3bDevice::Device* dev )
{
  // FIXME: add tooltips with full information including the device

  //
  // In case we only want an empty medium (this might happen in case the
  // the medium is rewritable) we do not care about the contents but tell 
  // the user that the medium is rewritable.
  // Otherwise we show the contents type since this might also be used
  // for source selection.
  //
  insertItem( k3bappcore->mediaCache()->mediumString( dev, d->wantedMediumState != K3bDevice::STATE_EMPTY ) );

  d->deviceIndexMap[dev] = count()-1;
  d->devices.resize( count() );
  d->devices.insert(count()-1, dev);
}


void K3bMediaSelectionComboBox::slotDeviceManagerChanged( K3bDevice::DeviceManager* )
{
  updateMedia();
}


K3bDevice::Device* K3bMediaSelectionComboBox::deviceAt( unsigned int index )
{
  if( index < d->devices.count() )
    return d->devices[index];
  else
    return 0;
}

#include "k3bmediaselectioncombobox.moc"
