/***************************************************************************
                          k3bdevicewidget.cpp  -  description
                             -------------------
    begin                : Wed Apr 17 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bdevicewidget.h"
#include "k3bdevicemanager.h"
#include "k3bdevice.h"

#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kdialog.h>
#include <klocale.h>
#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <tools/k3blistview.h>

#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qheader.h>
#include <qstring.h>
#include <qcolor.h>



class K3bDeviceWidget::PrivateTempDevice 
{
public:
  PrivateTempDevice( K3bDevice* d ) {
    device = d;
    cdrdaoDriver = d->cdrdaoDriver();
    maxReadSpeed = d->maxReadSpeed();
    maxWriteSpeed = d->maxWriteSpeed();
    cdTextCapable = ( d->cdTextCapable() != 2 );
    writer = d->burner();
    cdrw = d->writesCdrw();
    burnproof = d->burnproof();
    bufferSize = d->bufferSize();
    dao = d->dao();
  }
  
  K3bDevice* device;
  int maxReadSpeed;
  int maxWriteSpeed;
  QString cdrdaoDriver;
  bool cdTextCapable;
  bool writer;
  bool cdrw;
  bool burnproof;
  bool dao;
  int bufferSize;
};


class K3bDeviceWidget::PrivateDeviceViewItem2 : public QCheckListItem 
{
public:
  PrivateDeviceViewItem2( int type, PrivateTempDevice* dev, QListView* view, QListViewItem* after )
    : QCheckListItem( view, QString::null, CheckBox ),
      m_type(type) { 
    this->dev = dev;
    init();

    // QT 3.0.x does not have a QCheckListItem constructor that takes the after item
    moveItem(after);
  }

  PrivateDeviceViewItem2( int type, PrivateTempDevice* dev, QListViewItem* item, QListViewItem* after )
    : QCheckListItem( item, QString::null, CheckBox ),
      m_type(type) { 
    this->dev = dev;
    init();

    // QT 3.0.x does not have a QCheckListItem constructor that takes the after item
    moveItem(after);
  }

  QString text( int col ) const {
    if( col == 0 ) {
      switch(m_type) {
      case t_cdrw:
	return i18n("CD/RW drive");
      case t_burnproof:
	return i18n("Supports Burnfree");
      case t_dao:
	return i18n("Supports DAO writing");
      }
    }
    return "";
  }

  enum itemType { t_cdrw, t_burnproof, t_dao };

  PrivateTempDevice* dev;

protected:
  void stateChanged( bool on ) {
    switch(m_type) {
    case t_cdrw:
      dev->cdrw = on;
      break;
    case t_burnproof:
      dev->burnproof = on;
      break;
    case t_dao:
      dev->dao = on;
      break;
    }
  }

private:
  void init() {
    switch(m_type) {
    case t_cdrw:
      setOn(dev->cdrw);
      break;
    case t_burnproof:
      setOn(dev->burnproof);
      break;
    case t_dao:
      setOn(dev->dao);
      break;
    }
  }

  int m_type;
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
      case t_maxReadSpeed:
	dev->maxReadSpeed = text.toInt();
	break;
      case t_maxWriteSpeed:
	dev->maxWriteSpeed = text.toInt();
	break;
      case t_cdrdaoDriver:
	dev->cdrdaoDriver = text;
	break;
      case t_bufferSize:
	dev->bufferSize = text.toInt();
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
    case t_maxReadSpeed:
      return (col == 0 ? i18n("Max read speed:") : QString::number(dev->maxReadSpeed) );
      break;
    case t_maxWriteSpeed:
      return (col == 0 ? i18n("Max write speed:") : QString::number(dev->maxWriteSpeed) );
      break;
    case t_cdrdaoDriver:
      return (col == 0 ? i18n("Cdrdao driver:") : dev->cdrdaoDriver );
      break;
    case t_bufferSize:
      return (col == 0 ? i18n("Buffer size:") : QString::number(dev->bufferSize) );
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

  enum itemType { t_maxReadSpeed, t_maxWriteSpeed, t_cdrdaoDriver, t_bufferSize, t_cdTextCapable };
  
  PrivateTempDevice* dev;

private:
  void init() {
    switch(m_type) {
    case t_maxReadSpeed:
      setEditor( 1, SPIN );
      break;
    case t_maxWriteSpeed:
      setEditor( 1, SPIN );
      break;
    case t_cdrdaoDriver:
      static QStringList l;
      if( l.isEmpty() )
	for( int i = 0; i < 13; i++ )
	  l.append(K3bDevice::cdrdao_drivers[i]);
      
      setEditor( 1, COMBO, l );
      break;
    case t_bufferSize:
      setEditor( 1, SPIN );
      break;
    case t_cdTextCapable:
      static QStringList l2;
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






K3bDeviceWidget::K3bDeviceWidget( K3bDeviceManager* manager, QWidget *parent, const char *name )
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
  QToolTip::add( m_buttonRefreshDevices, i18n( "Rescan the Devices" ) );
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
  m_viewDevices->setAllColumnsShowFocus( TRUE );
  m_viewDevices->header()->hide();
  m_viewDevices->setSorting( -1 );
  m_viewDevices->setDoubleClickForEdit(false);
  m_viewDevices->setAlternateBackground( QColor() );
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
  K3bDevice* dev = m_deviceManager->readingDevices().first();
  while( dev ) {
    m_tempDevices.append( new PrivateTempDevice( dev ) );
    dev = m_deviceManager->readingDevices().next();
  }
	
  // add the writing devices
  dev = m_deviceManager->burningDevices().first();
  while( dev ) {
    m_tempDevices.append( new PrivateTempDevice( dev ) );
    dev = m_deviceManager->burningDevices().next();
  }

  updateDeviceListViews();
}


void K3bDeviceWidget::updateDeviceListViews()
{
  m_viewDevices->clear();

  // create the parent view items
  // -----------------------------------------
  m_writerParentViewItem = new QListViewItem( m_viewDevices, i18n("Writer") );
  m_writerParentViewItem->setPixmap( 0, SmallIcon( "cdwriter_unmount" ) );
  m_readerParentViewItem = new QListViewItem( m_viewDevices, i18n("Reader") );
  m_readerParentViewItem->setPixmap( 0, SmallIcon( "cdrom_unmount" ) );
  // -----------------------------------------


  PrivateTempDevice* dev = m_tempDevices.first();
  while( dev ) {
    // create the root device item
    QListViewItem* devRoot = new QListViewItem( (dev->writer ? m_writerParentViewItem : m_readerParentViewItem),
						dev->device->vendor() + " " + dev->device->description() );
    // create the read-only info items
    QListViewItem* systemDeviceItem = new QListViewItem( devRoot, i18n("System device name:") );
    if( dev->device->interfaceType() == K3bDevice::SCSI )
      systemDeviceItem->setText( 1, QString("%1 (%2)").arg(dev->device->devicename()).arg(dev->device->busTargetLun()) );
    else
      systemDeviceItem->setText( 1, dev->device->devicename() );

    QListViewItem* interfaceItem = new QListViewItem( devRoot, systemDeviceItem, 
						      i18n("Interface type:"),
						      ( dev->device->interfaceType() == K3bDevice::SCSI ? 
							i18n("Generic SCSI") : 
							i18n("ATAPI") ) );

    QListViewItem* vendorItem = new QListViewItem( devRoot, interfaceItem, 
						   i18n("Vendor:"),
						   dev->device->vendor() );
    QListViewItem* modelItem = new QListViewItem( devRoot, vendorItem, 
						   i18n("Description:"),
						   dev->device->description() );
    QListViewItem* versionItem = new QListViewItem( devRoot, modelItem, 
						   i18n("Version:"),
						   dev->device->version() );


    // now add the reader (both interfaces) items
    PrivateDeviceViewItem1* maxReadSpeedItem = new PrivateDeviceViewItem1( PrivateDeviceViewItem1::t_maxReadSpeed,
									   dev,
									   devRoot,
									   versionItem );

    // now add the SCSI specific items
    if( dev->device->interfaceType() == K3bDevice::SCSI ) {
      PrivateDeviceViewItem1* cdrdaoDriverItem = new PrivateDeviceViewItem1( PrivateDeviceViewItem1::t_cdrdaoDriver,
									     dev,
									     devRoot,
									     maxReadSpeedItem );

      // the writer specific items
      if( dev->writer ) {
	// add max write speed item after the maxreadspeed item
	(void)new PrivateDeviceViewItem1( PrivateDeviceViewItem1::t_maxWriteSpeed,
					  dev,
					  devRoot,
					  maxReadSpeedItem );

	PrivateDeviceViewItem1* cdTextItem = new PrivateDeviceViewItem1( PrivateDeviceViewItem1::t_cdTextCapable,
									 dev,
									 devRoot,
									 cdrdaoDriverItem );

	PrivateDeviceViewItem2* burnfreeItem = new PrivateDeviceViewItem2( PrivateDeviceViewItem2::t_burnproof,
									   dev,
									   devRoot,
									   cdTextItem );

	PrivateDeviceViewItem2* cdrwItem = new PrivateDeviceViewItem2( PrivateDeviceViewItem2::t_cdrw,
								       dev,
								       devRoot,
								       burnfreeItem );

	PrivateDeviceViewItem2* daoItem = new PrivateDeviceViewItem2( PrivateDeviceViewItem2::t_dao,
								      dev,
								      devRoot,
								      cdrwItem );

	// and at last the write modes
	QString wm;
	if( dev->device->supportsWriteMode( K3bDevice::SAO ) )
	  wm += "SAO ";
	if( dev->device->supportsWriteMode( K3bDevice::SAO_R96R ) )
	  wm += "SAO/R96R ";
	if( dev->device->supportsWriteMode( K3bDevice::SAO_R96P ) )
	  wm += "SAO/R96P ";
	if( dev->device->supportsWriteMode( K3bDevice::PACKET ) )
	  wm += "PACKET ";
	if( dev->device->supportsWriteMode( K3bDevice::TAO ) )
	  wm += "TAO ";
	if( dev->device->supportsWriteMode( K3bDevice::RAW_R16 ) )
	  wm += "RAW/R16 ";
	if( dev->device->supportsWriteMode( K3bDevice::RAW_R96R ) )
	  wm += "RAW/R96R ";
	if( dev->device->supportsWriteMode( K3bDevice::RAW_R96P ) )
	  wm += "RAW/R96P ";

	(void)new QListViewItem( devRoot, daoItem, i18n("Write modes:"), wm );
      }

      devRoot->setOpen(true);
    }

    dev = m_tempDevices.next();
  }

  m_writerParentViewItem->setOpen( true );
  m_readerParentViewItem->setOpen( true );
}


void K3bDeviceWidget::slotNewDevice()
{
  bool ok;
  QString newDevicename = KLineEditDlg::getText( i18n("Please enter the device name where K3b should search\n for a new drive (example: /dev/mebecdrom):"), "/dev/", &ok, this );

  if( ok ) {
    if( K3bDevice* dev = m_deviceManager->addDevice( newDevicename ) ) {
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
    tempDev->device->setMaxReadSpeed( tempDev->maxReadSpeed );
    tempDev->device->setMaxWriteSpeed( tempDev->maxWriteSpeed );
    tempDev->device->setCdrdaoDriver( tempDev->cdrdaoDriver );
    tempDev->device->setCdTextCapability( tempDev->cdTextCapable );
    tempDev->device->setIsWriter( tempDev->writer );
    tempDev->device->setBurnproof( tempDev->burnproof );
    tempDev->device->setWritesCdrw( tempDev->cdrw );
    tempDev->device->setDao( tempDev->dao );
    tempDev->device->setBufferSize( tempDev->bufferSize );
    
    tempDev = m_tempDevices.next();
  }
}


#include "k3bdevicewidget.moc"
