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


#include "k3bdevicewidget.h"
#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include <k3blistview.h>

#include <kinputdialog.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kdialog.h>
#include <klocale.h>
#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kio/global.h>

#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qheader.h>
#include <qstring.h>
#include <qcolor.h>
#include <qptrlist.h>


class K3bDeviceWidget::PrivateTempDevice
{
public:
  PrivateTempDevice( K3bDevice::Device* d ) {
    device = d;
    cdrdaoDriver = d->cdrdaoDriver();
    cdTextCapable = ( d->cdTextCapable() != 2 );
    writer = d->burner();
  }

  K3bDevice::Device* device;
  QString cdrdaoDriver;
  bool cdTextCapable;
  bool writer;
};


class K3bDeviceWidget::PrivateDeviceViewItem1 : public K3bListViewItem
{
public:
  PrivateDeviceViewItem1( int type, PrivateTempDevice* dev, QListView* view, QListViewItem* after )
    : K3bListViewItem( view, after ),
      m_type(type) {
    this->dev = dev;
    init();
  }

  PrivateDeviceViewItem1( int type, PrivateTempDevice* dev, QListViewItem* item, QListViewItem* after )
    : K3bListViewItem( item, after ),
      m_type(type) {
    this->dev = dev;
    init();
  }

  void setText(int col, const QString& text) {
    if( col == 1 ) {
      switch(m_type) {
      case t_cdrdaoDriver:
	dev->cdrdaoDriver = text;
	break;
      case t_cdTextCapable:
	if( dev->cdrdaoDriver != "auto" )
	  dev->cdTextCapable = ( text == i18n("yes") );
	break;
      }
    }
  }

  QString text( int col ) const {
    switch(m_type) {
    case t_cdrdaoDriver:
      return (col == 0 ? i18n("Cdrdao driver:") : dev->cdrdaoDriver );
      break;
    case t_cdTextCapable:
      if( col == 0 )
	return i18n("CD-Text capable:");
      else {
	if( dev->cdrdaoDriver == "auto" )
	  return "auto";
	else return ( dev->cdTextCapable ? i18n("yes") : i18n("no") );
      }
    }
    return "???";
  }

  enum itemType { t_cdrdaoDriver, t_cdTextCapable };

  PrivateTempDevice* dev;

private:
  void init() {
    static QStringList l;
    static QStringList l2;

    switch(m_type) {
    case t_cdrdaoDriver:
      if( l.isEmpty() )
	for( int i = 0; i < 13; i++ )
	  l.append(K3bDevice::Device::cdrdao_drivers[i]);

      setEditor( 1, COMBO, l );
      break;
    case t_cdTextCapable:
      if( l2.isEmpty() ) {
	l2.append(i18n("auto"));
	l2.append(i18n("yes"));
	l2.append(i18n("no"));
      }

      setEditor( 1, COMBO, l2 );
    }
  }

  int m_type;
};






K3bDeviceWidget::K3bDeviceWidget( K3bDevice::DeviceManager* manager, QWidget *parent, const char *name )
  : QWidget( parent, name ), m_deviceManager( manager )
{
  QGridLayout* frameLayout = new QGridLayout( this );
  frameLayout->setSpacing( KDialog::spacingHint() );
  frameLayout->setMargin( 0 );


  // buttons
  // ------------------------------------------------
  QGridLayout* refreshButtonGrid = new QGridLayout;
  refreshButtonGrid->setSpacing( KDialog::spacingHint() );
  refreshButtonGrid->setMargin(0);
  m_buttonRefreshDevices = new QPushButton( i18n( "Refresh" ), this, "m_buttonRefreshDevices" );
  m_buttonAddDevice = new QPushButton( i18n( "Add Device..." ), this, "m_buttonAddDevice" );
  QToolTip::add( m_buttonRefreshDevices, i18n( "Rescan the devices" ) );
  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  refreshButtonGrid->addItem( spacer, 0, 0 );
  refreshButtonGrid->addWidget( m_buttonRefreshDevices, 0, 2 );
  refreshButtonGrid->addWidget( m_buttonAddDevice, 0, 1 );
  // ------------------------------------------------


  // Devices Box
  // ------------------------------------------------
  QGroupBox* groupDevices = new QGroupBox( 1, Qt::Vertical, i18n( "CD/DVD Drives" ), this );
  groupDevices->layout()->setSpacing( KDialog::spacingHint() );
  groupDevices->layout()->setMargin( KDialog::marginHint() );

  m_viewDevices = new K3bListView( groupDevices, "m_viewDevicesReader" );
  m_viewDevices->addColumn( "V" );
  m_viewDevices->addColumn( "D" );
  m_viewDevices->setAllColumnsShowFocus( true );
  m_viewDevices->header()->hide();
  m_viewDevices->setSorting( -1 );
  m_viewDevices->setDoubleClickForEdit(false);
  m_viewDevices->setAlternateBackground( QColor() );
  m_viewDevices->setSelectionMode( QListView::NoSelection );
  m_viewDevices->setFullWidth(true);
  // ------------------------------------------------


  frameLayout->addWidget( groupDevices, 0, 0 );
  frameLayout->addLayout( refreshButtonGrid, 1, 0 );
  // ------------------------------------------------

  // temporary device lists settings
  // ------------------------------------------------
  m_tempDevices.setAutoDelete( true );
  // ------------------------------------------------


  // connections
  // ------------------------------------------------
  //  connect( m_buttonRefreshDevices, SIGNAL(clicked()), this, SLOT(slotRefreshDevices()) );
  connect( m_buttonRefreshDevices, SIGNAL(clicked()), this, SIGNAL(refreshButtonClicked()) );
  connect( m_buttonAddDevice, SIGNAL(clicked()), this, SLOT(slotNewDevice()) );
  connect( m_deviceManager, SIGNAL(changed()), this, SLOT(init()) );
  // ------------------------------------------------
}


K3bDeviceWidget::~K3bDeviceWidget()
{
  m_tempDevices.clear();
}


void K3bDeviceWidget::init()
{
  // fill the temporary lists
  m_tempDevices.clear();

  // add the reading devices
  for( QPtrListIterator<K3bDevice::Device> it( m_deviceManager->allDevices() ); *it; ++it )
    m_tempDevices.append( new PrivateTempDevice( *it ) );

  updateDeviceListViews();
}


void K3bDeviceWidget::updateDeviceListViews()
{
  m_viewDevices->clear();

  // create the parent view items
  // -----------------------------------------
  m_writerParentViewItem = new QListViewItem( m_viewDevices, i18n("Writer Drives") );
  m_writerParentViewItem->setPixmap( 0, SmallIcon( "cdwriter_unmount" ) );
  // spacer item
  (void)new QListViewItem( m_viewDevices );
  m_readerParentViewItem = new QListViewItem( m_viewDevices, i18n("Readonly Drives") );
  m_readerParentViewItem->setPixmap( 0, SmallIcon( "cdrom_unmount" ) );
  // -----------------------------------------

  QFont fBold( m_viewDevices->font() );
  fBold.setBold(true);
  QFont fItalic( m_viewDevices->font() );
  fItalic.setItalic(true);

  PrivateTempDevice* dev = m_tempDevices.first();
  while( dev ) {
    // create the root device item
    K3bListViewItem* devRoot = new K3bListViewItem( (dev->writer ? m_writerParentViewItem : m_readerParentViewItem),
						    dev->device->vendor() + " " + dev->device->description() );
    devRoot->setFont( 0, fBold );

    // create the read-only info items
    K3bListViewItem* systemDeviceItem = new K3bListViewItem( devRoot, i18n("System device name:") );
    if( dev->device->interfaceType() == K3bDevice::SCSI )
      systemDeviceItem->setText( 1, QString("%1 (%2)").arg(dev->device->devicename()).arg(dev->device->busTargetLun()) );
    else
      systemDeviceItem->setText( 1, dev->device->devicename() );
    systemDeviceItem->setForegroundColor( 1, palette().disabled().foreground() );

    K3bListViewItem* interfaceItem = new K3bListViewItem( devRoot, systemDeviceItem,
							  i18n("Interface type:"),
							  ( dev->device->interfaceType() == K3bDevice::SCSI ?
							    i18n("Generic SCSI") :
							    i18n("ATAPI") ) );
    interfaceItem->setForegroundColor( 1, palette().disabled().foreground() );

    K3bListViewItem* vendorItem = new K3bListViewItem( devRoot, interfaceItem,
						   i18n("Vendor:"),
						   dev->device->vendor() );
    vendorItem->setForegroundColor( 1, palette().disabled().foreground() );
    K3bListViewItem* modelItem = new K3bListViewItem( devRoot, vendorItem,
						   i18n("Description:"),
						   dev->device->description() );
    modelItem->setForegroundColor( 1, palette().disabled().foreground() );
    K3bListViewItem* versionItem = new K3bListViewItem( devRoot, modelItem,
						   i18n("Firmware:"),
						   dev->device->version() );
    versionItem->setForegroundColor( 1, palette().disabled().foreground() );


    // drive type
    // --------------------------------
    K3bListViewItem* typeItem = new K3bListViewItem( devRoot, versionItem,
						     i18n("Writes CD-R:"),
						     dev->device->writesCd() ? i18n("yes") : i18n("no") );
    typeItem->setForegroundColor( 1, palette().disabled().foreground() );
    typeItem = new K3bListViewItem( devRoot, typeItem,
				    i18n("Writes CD-RW:"),
				    dev->device->writesCdrw() ? i18n("yes") : i18n("no") );
    typeItem->setForegroundColor( 1, palette().disabled().foreground() );
    typeItem = new K3bListViewItem( devRoot, typeItem, 
				    i18n("Reads DVD:"),
				    dev->device->readsDvd() ? i18n("yes") : i18n("no") );
    typeItem->setForegroundColor( 1, palette().disabled().foreground() );
    typeItem = new K3bListViewItem( devRoot, typeItem,
				    i18n("Writes DVD-R(W):"),
				    dev->device->writesDvdMinus() ? i18n("yes") : i18n("no") );
    typeItem->setForegroundColor( 1, palette().disabled().foreground() );
    typeItem = new K3bListViewItem( devRoot, typeItem,
				    i18n("Writes DVD-R Dual Layer:"),
				    (dev->device->type() & K3bDevice::DEVICE_DVD_R_DL)
				    ? i18n("yes") : i18n("no") );
    typeItem->setForegroundColor( 1, palette().disabled().foreground() );
    typeItem = new K3bListViewItem( devRoot, typeItem,
				    i18n("Writes DVD+R(W):"),
				    dev->device->writesDvdPlus() ? i18n("yes") : i18n("no") );
    typeItem->setForegroundColor( 1, palette().disabled().foreground() );
    typeItem = new K3bListViewItem( devRoot, typeItem,
				    i18n("Writes DVD+R Double Layer:"),
				    (dev->device->type() & K3bDevice::DEVICE_DVD_PLUS_R_DL)
				    ? i18n("yes") : i18n("no") );
    typeItem->setForegroundColor( 1, palette().disabled().foreground() );
    // --------------------------------


    // now add the reader (both interfaces) items
    if( dev->device->bufferSize() > 0 ) {
      typeItem = new K3bListViewItem( devRoot, typeItem,
				      i18n("Buffer Size:"),
				      KIO::convertSizeFromKB(dev->device->bufferSize()) );
      typeItem->setForegroundColor( 1, palette().disabled().foreground() );
    }

    PrivateDeviceViewItem1* cdrdaoDriverItem = new PrivateDeviceViewItem1( PrivateDeviceViewItem1::t_cdrdaoDriver,
									   dev,
									   devRoot,
									   typeItem );


    // now add the writer specific items
    if( dev->writer ) {
      PrivateDeviceViewItem1* cdTextItem = new PrivateDeviceViewItem1( PrivateDeviceViewItem1::t_cdTextCapable,
								       dev,
								       devRoot,
								       cdrdaoDriverItem );

      typeItem = new K3bListViewItem( devRoot, cdTextItem,
				      i18n("Supports Burnfree:"),
				      dev->device->burnfree() ? i18n("yes") : i18n("no") );
      typeItem->setForegroundColor( 1, palette().disabled().foreground() );

      
      // and at last the write modes
      (new K3bListViewItem( devRoot, 
			    typeItem, 
			    i18n("Write modes:"), 
			    K3bDevice::writingModeString(dev->device->writingModes()) ))->setForegroundColor( 1, palette().disabled().foreground() );
    }

    devRoot->setOpen(true);

    dev = m_tempDevices.next();
  }

  // create empty items
  if( m_writerParentViewItem->childCount() == 0 ) {
    K3bListViewItem* item = new K3bListViewItem( m_writerParentViewItem, i18n("none") );
    item->setFont( 0, fItalic );
  }
  if( m_readerParentViewItem->childCount() == 0 ) {
    K3bListViewItem* item = new K3bListViewItem( m_readerParentViewItem, i18n("none") );
    item->setFont( 0, fItalic );
  }

  m_writerParentViewItem->setOpen( true );
  m_readerParentViewItem->setOpen( true );
}


void K3bDeviceWidget::slotNewDevice()
{
  bool ok;
  QString newDevicename = KInputDialog::getText( i18n("Location of New Drive"),
						 i18n("Please enter the device name where K3b should search\nfor a new drive (example: /dev/cdrom):"),
						 "/dev/", &ok, this );

  if( ok ) {
    if( K3bDevice::Device* dev = m_deviceManager->addDevice( newDevicename ) ) {
      m_tempDevices.append( new PrivateTempDevice( dev ) );

      updateDeviceListViews();
    }
    else
      KMessageBox::error( this, i18n("Could not find an additional device at\n%1").arg(newDevicename), i18n("Error"), false );
  }
}


void K3bDeviceWidget::apply()
{
  // update the devices
  PrivateTempDevice* tempDev = m_tempDevices.first();
  while( tempDev != 0 ) {
    tempDev->device->setCdrdaoDriver( tempDev->cdrdaoDriver );
    tempDev->device->setCdTextCapability( tempDev->cdTextCapable );

    tempDev = m_tempDevices.next();
  }
}


#include "k3bdevicewidget.moc"
