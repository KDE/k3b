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
#include <kstddirs.h>
#include <klistview.h>

#include <qcombobox.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qframe.h>
#include <qheader.h>
#include <qstring.h>



class K3bDeviceWidget::PrivateTempDevice 
{
public:
  PrivateTempDevice( K3bDevice* d );
  
  K3bDevice* device;
  int maxReadSpeed;
  int maxWriteSpeed;
  QString cdrdaoDriver;
  bool cdTextCapable;
  bool writer;
  bool cdrw;
  bool burnproof;
  int bufferSize;
};


class K3bDeviceWidget::PrivateDeviceViewItem : public KListViewItem 
{
public:
  PrivateDeviceViewItem( PrivateTempDevice* dev, QListView* view )
    : KListViewItem( view, dev->device->vendor(), dev->device->description() ) { 
    device = dev;
    //    setPixmap( 0, dev->writer ? SmallIcon( "cdwriter_unmount" ) : SmallIcon( "cdrom_unmount" ) );
  }

  PrivateDeviceViewItem( PrivateTempDevice* dev, QListViewItem* item )
    : KListViewItem( item, dev->device->vendor(), dev->device->description() ) { 
    device = dev;
    //    setPixmap( 0, dev->writer ? SmallIcon( "cdwriter_unmount" ) : SmallIcon( "cdrom_unmount" ) );
  }
  
  PrivateTempDevice* device;
};


K3bDeviceWidget::PrivateTempDevice::PrivateTempDevice( K3bDevice* d )
{
  device = d;
  cdrdaoDriver = d->cdrdaoDriver();
  maxReadSpeed = d->maxReadSpeed();
  maxWriteSpeed = d->maxWriteSpeed();
  cdTextCapable = ( d->cdTextCapable() != 2 );
  writer = d->burner();
  cdrw = d->writesCdrw();
  burnproof = d->burnproof();
  bufferSize = d->bufferSize();
}


K3bDeviceWidget::K3bDeviceWidget( K3bDeviceManager* manager, QWidget *parent, const char *name )
  : QWidget( parent, name ), m_deviceManager( manager )
{
  m_currentTempDevice = 0;

  QGridLayout* frameLayout = new QGridLayout( this );
  frameLayout->setSpacing( KDialog::spacingHint() );
  frameLayout->setMargin( 0 );


  // buttons
  // ------------------------------------------------
  QGridLayout* refreshButtonGrid = new QGridLayout;
  refreshButtonGrid->setSpacing( KDialog::spacingHint() );
  refreshButtonGrid->setMargin(0);
  m_buttonRefreshDevices = new QPushButton( i18n( "Refresh" ), this, "m_buttonRefreshDevices" );
  m_buttonAddDevice = new QPushButton( i18n( "Add Device" ), this, "m_buttonAddDevice" );
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

  m_viewDevices = new KListView( groupDevices, "m_viewDevicesReader" );
  m_viewDevices->addColumn( "V" );
  m_viewDevices->addColumn( "D" );
  m_viewDevices->setAllColumnsShowFocus( TRUE );
  m_viewDevices->header()->hide();
  m_viewDevices->setSorting( -1 );
  // ------------------------------------------------


  // Device Info Box
  // ------------------------------------------------
  m_groupDeviceInfo = new QGroupBox( this, "groupDeviceInfo" );
  m_groupDeviceInfo->setTitle( i18n( "Device Info" ) );
  m_groupDeviceInfo->setColumnLayout(0, Qt::Vertical );
  m_groupDeviceInfo->layout()->setSpacing( 0 );
  m_groupDeviceInfo->layout()->setMargin( 10 );
  QGridLayout* groupDeviceInfoLayout = new QGridLayout( m_groupDeviceInfo->layout() );
  groupDeviceInfoLayout->setAlignment( Qt::AlignTop );
  groupDeviceInfoLayout->setSpacing( KDialog::spacingHint() );
  groupDeviceInfoLayout->setMargin( KDialog::marginHint() );

  QLabel* TextLabel1 = new QLabel( i18n( "System devicename:" ), m_groupDeviceInfo, "TextLabel1" );
  m_labelDevicename = new QLabel( m_groupDeviceInfo, "m_labelDevicename" );
  QLabel* labelInterfaceText = new QLabel( i18n("Interface type:"), m_groupDeviceInfo, "interfaceText" );
  m_labelDeviceInterface = new QLabel( m_groupDeviceInfo );
  QLabel* TextLabel7 = new QLabel( i18n( "Firmware version:" ), m_groupDeviceInfo, "TextLabel7" );
  m_labelVendor = new QLabel( m_groupDeviceInfo, "m_labelVendor" );
  QLabel* TextLabel5 = new QLabel( i18n( "Vendor:" ), m_groupDeviceInfo, "TextLabel5" );
  m_labelDescription = new QLabel( m_groupDeviceInfo, "m_labelDescription" );
  m_labelVersion = new QLabel( m_groupDeviceInfo, "m_labelVersion" );
  QLabel* TextLabel6 = new QLabel( i18n( "Model:" ), m_groupDeviceInfo, "TextLabel6" );
  QFrame* line1 = new QFrame( m_groupDeviceInfo, "line1" );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  m_spinReadSpeed = new KIntNumInput( m_groupDeviceInfo, "m_spinReadSpeed" );
  QLabel* labelReadSpeed = new QLabel( i18n( "Max read speed:" ), m_groupDeviceInfo, "labelReadSpeed" );
  QFrame* line2 = new QFrame( m_groupDeviceInfo, "line2" );
  line2->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  m_labelWriteSpeed = new QLabel( i18n( "Max write speed:" ), m_groupDeviceInfo, "labelWriteSpeed" );
  m_spinWriteSpeed = new KIntNumInput( m_groupDeviceInfo, "m_spinWriteSpeed" );
  m_comboDriver = new QComboBox( FALSE, m_groupDeviceInfo, "m_comboDriver" );
  m_labelDriver = new QLabel( i18n( "Cdrdao driver:" ), m_groupDeviceInfo, "labelDriver" );
  m_comboCdText = new QComboBox( false, m_groupDeviceInfo, "m_comboCdText" );
  m_labelBurnProof = new QLabel( i18n( "BURN-Proof:" ), m_groupDeviceInfo, "labelBurnProof" );
  m_checkBurnProof = new QCheckBox( m_groupDeviceInfo, "m_checkBurnProof" );
  m_labelCdrw = new QLabel( i18n( "Write CDRW:" ), m_groupDeviceInfo, "labelCdrw" );
  m_checkCdrw = new QCheckBox( m_groupDeviceInfo, "m_checkCdrw" );
  m_labelCdText = new QLabel( i18n( "Write CD-Text:" ), m_groupDeviceInfo, "labelCdText" );
  m_line3 = new QFrame( m_groupDeviceInfo, "line3" );
  m_line3->setFrameStyle( QFrame::HLine | QFrame::Sunken );


  // set the backgroud colors of the labels that display the actual values
  m_labelDevicename->setBackgroundColor( Qt::white );
  m_labelDeviceInterface->setBackgroundColor( Qt::white );
  m_labelVersion->setBackgroundColor( Qt::white );
  m_labelVendor->setBackgroundColor( Qt::white );
  m_labelDescription->setBackgroundColor( Qt::white );


  m_labelDevicename->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  m_labelDeviceInterface->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  m_labelVersion->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  m_labelVendor->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  m_labelDescription->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );


  groupDeviceInfoLayout->addWidget( TextLabel1, 0, 0 );
  groupDeviceInfoLayout->addWidget( m_labelDevicename, 0, 1 );
  groupDeviceInfoLayout->addWidget( labelInterfaceText, 1, 0 );
  groupDeviceInfoLayout->addWidget( m_labelDeviceInterface, 1, 1 );
  groupDeviceInfoLayout->addMultiCellWidget( line1, 2, 2, 0, 1 );
  groupDeviceInfoLayout->addWidget( TextLabel5, 3, 0 );
  groupDeviceInfoLayout->addWidget( m_labelVendor, 3, 1 );
  groupDeviceInfoLayout->addWidget( TextLabel6, 4, 0 );
  groupDeviceInfoLayout->addWidget( m_labelDescription, 4, 1 );
  groupDeviceInfoLayout->addWidget( TextLabel7, 5, 0 );
  groupDeviceInfoLayout->addWidget( m_labelVersion, 5, 1 );
  groupDeviceInfoLayout->addMultiCellWidget( line2, 6, 6, 0, 1 );
  groupDeviceInfoLayout->addWidget( labelReadSpeed, 7, 0 );
  groupDeviceInfoLayout->addWidget( m_spinReadSpeed, 7, 1 );
  groupDeviceInfoLayout->addWidget( m_labelWriteSpeed, 8, 0 );
  groupDeviceInfoLayout->addWidget( m_spinWriteSpeed, 8, 1 );
  groupDeviceInfoLayout->addMultiCellWidget( m_line3, 9, 9, 0, 1 );
  groupDeviceInfoLayout->addWidget( m_labelDriver, 10, 0 );
  groupDeviceInfoLayout->addWidget( m_comboDriver, 10, 1 );
  groupDeviceInfoLayout->addWidget( m_labelCdText, 11, 0 );
  groupDeviceInfoLayout->addWidget( m_comboCdText, 11, 1 );
  groupDeviceInfoLayout->addWidget( m_labelBurnProof, 12, 0 );
  groupDeviceInfoLayout->addWidget( m_checkBurnProof, 12, 1 );
  groupDeviceInfoLayout->addWidget( m_labelCdrw, 13, 0 );
  groupDeviceInfoLayout->addWidget( m_checkCdrw, 13, 1 );


  frameLayout->addWidget( groupDevices, 0, 0 );
  frameLayout->addWidget( m_groupDeviceInfo, 0, 1 );
  frameLayout->addMultiCellLayout( refreshButtonGrid, 1, 1, 0, 1 );
  // ------------------------------------------------

  showWriterSpecificProps( false );
  // -------------------------------------


  // temporary device lists settings
  // ------------------------------------------------
  m_tempDevices.setAutoDelete( true );
  // ------------------------------------------------


  // connections
  // ------------------------------------------------		
  //  connect( m_buttonRefreshDevices, SIGNAL(clicked()), this, SLOT(slotRefreshDevices()) );
  connect( m_buttonRefreshDevices, SIGNAL(clicked()), this, SIGNAL(refreshButtonClicked()) );
  connect( m_buttonAddDevice, SIGNAL(clicked()), this, SLOT(slotNewDevice()) );
	
  connect( m_viewDevices, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotDeviceSelected(QListViewItem*)) );

  connect( m_comboDriver, SIGNAL(activated(const QString&)), this, SLOT(slotCdrdaoDriverChanged(const QString&)) );
  connect( m_comboCdText, SIGNAL(activated(const QString&)), this, SLOT(slotCdTextCapabilityChanged(const QString&)) );

  connect( m_spinWriteSpeed, SIGNAL(valueChanged(int)), this, SLOT(slotWriteSpeedChanged(int)) );
  connect( m_spinReadSpeed, SIGNAL(valueChanged(int)), this, SLOT(slotReadSpeedChanged(int)) );
  connect( m_checkCdrw, SIGNAL(toggled(bool)), this, SLOT(slotCdrwChanged(bool)) );
  connect( m_checkBurnProof, SIGNAL(toggled(bool)), this, SLOT(slotBurnproofChanged(bool)) );
  // ------------------------------------------------

  // fill the driver-combo-box
  // -----------------------------------------
  for( int i = 0; i < 13; i++ ) {
    m_comboDriver->insertItem( K3bDevice::cdrdao_drivers[i] );
  }
  // -----------------------------------------
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


  PrivateDeviceViewItem* item;

  PrivateTempDevice* dev = m_tempDevices.first();
  while( dev ) {
    // add item to m_viewDevices
    item = new PrivateDeviceViewItem( dev, dev->writer ? m_writerParentViewItem : m_readerParentViewItem );

    dev = m_tempDevices.next();
  }

  m_writerParentViewItem->setOpen( true );
  m_readerParentViewItem->setOpen( true );

  if( QListViewItem* i = m_readerParentViewItem->firstChild() ) {
    m_viewDevices->setSelected( i, true );
  }
  else if( QListViewItem* i = m_writerParentViewItem->firstChild() ) {
    m_viewDevices->setSelected( i, true );
  }
}


void K3bDeviceWidget::updateDeviceInfoBox( PrivateTempDevice* tempDev )
{
  m_currentTempDevice = tempDev;

  if( tempDev != 0 ) {
    m_groupDeviceInfo->setEnabled( true );

    K3bDevice* dev = tempDev->device;

    if( dev->interfaceType() == K3bDevice::SCSI )
      m_labelDevicename->setText( QString("%1 (%2)").arg(dev->devicename()).arg(dev->busTargetLun()) );
    else
      m_labelDevicename->setText( dev->devicename() );
    m_labelDeviceInterface->setText( dev->interfaceType() == K3bDevice::SCSI ? i18n("Generic SCSI") : i18n("IDE") );
    m_labelVendor->setText( dev->vendor() );
    m_labelDescription->setText( dev->description() );
    m_labelVersion->setText( dev->version() );
    m_spinReadSpeed->setValue( tempDev->maxReadSpeed );
    m_comboDriver->setEnabled( true );

    int i = 0;
    while( i < m_comboDriver->count() ) {
      if( m_comboDriver->text(i) == tempDev->cdrdaoDriver ) {
	m_comboDriver->setCurrentItem(i);
	break;
      }
      i++;
    }

    // since cdrdao cannot use ide-devices there
    // is no need to offer an option for those
    if( dev->interfaceType() == K3bDevice::IDE ) {
      m_comboDriver->hide();
      m_labelDriver->hide();
      m_line3->hide();
    }
    else {
      m_comboDriver->show();
      m_labelDriver->show();
      m_line3->show();
    }
    
    if( tempDev->writer ) {
      m_spinWriteSpeed->setValue( tempDev->maxWriteSpeed );
      m_checkBurnProof->setChecked( tempDev->burnproof );
      m_checkCdrw->setChecked( tempDev->cdrw );
       
      slotCdrdaoDriverChanged( tempDev->cdrdaoDriver );

      showWriterSpecificProps( true );
    }
    else {
      showWriterSpecificProps( false );
    }
  }
  else {
    // disable all
    showWriterSpecificProps( false );
    m_labelDevicename->setText( "" );
    m_labelDeviceInterface->setText( "" );
    m_labelVendor->setText( "" );
    m_labelDescription->setText( "" );
    m_labelVersion->setText( "" );
    m_spinReadSpeed->setValue( 0 );
    m_comboDriver->setDisabled( true );
  }    
}


void K3bDeviceWidget::slotNewDevice()
{
  bool ok;
  QString newDevicename = KLineEditDlg::getText( i18n("Please enter the devicename where\n K3b shall search for a new drive\n(example: /dev/mebecdrom)"), "/dev/", &ok, this );

  if( ok ) {
    if( K3bDevice* dev = m_deviceManager->addDevice( newDevicename ) ) {
      m_tempDevices.append( new PrivateTempDevice( dev ) );

      updateDeviceListViews();
    }
    else
      KMessageBox::error( this, i18n("Sorry, could not find an additional device at\n%1").arg(newDevicename), i18n("Error"), false );
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
    tempDev->device->setBufferSize( tempDev->bufferSize );
    
    tempDev = m_tempDevices.next();
  }
}


void K3bDeviceWidget::slotDeviceSelected( QListViewItem* item )
{
  if( item == m_readerParentViewItem ) {
    m_viewDevices->clearSelection();
    if( QListViewItem* i = m_readerParentViewItem->firstChild() ) {
      m_viewDevices->setSelected( i, true );
    }
  }
  else if( item == m_writerParentViewItem ) {
    m_viewDevices->clearSelection();
    if( QListViewItem* i = m_writerParentViewItem->firstChild() ) {
      m_viewDevices->setSelected( i, true );
    }
  }

  else if( PrivateDeviceViewItem* deviceItem = dynamic_cast<PrivateDeviceViewItem*>(item) ) {
    updateDeviceInfoBox( deviceItem->device );
  }
}


void K3bDeviceWidget::slotCdrdaoDriverChanged( const QString& driver )
{
  if( m_currentTempDevice != 0 ) {
    m_currentTempDevice->cdrdaoDriver = driver;
    
    m_comboCdText->clear();
    if( driver == "auto" ) {
      m_comboCdText->insertItem( i18n("auto") );
    }
    else {
      m_comboCdText->insertItem( i18n("yes"), 0 );
      m_comboCdText->insertItem( i18n("no"), 1 );
      m_comboCdText->setCurrentItem( m_currentTempDevice->cdTextCapable ? 0 : 1 );
    }
  }
}


void K3bDeviceWidget::slotCdTextCapabilityChanged( const QString& s )
{
  if( m_currentTempDevice != 0 ) {
    m_currentTempDevice->cdTextCapable = ( s == i18n("yes") );
  }
}


void K3bDeviceWidget::slotWriteSpeedChanged( int i )
{
  if( m_currentTempDevice != 0 ) {
    m_currentTempDevice->maxWriteSpeed = i;
  }
}


void K3bDeviceWidget::slotReadSpeedChanged( int i )
{
  if( m_currentTempDevice != 0 ) {
    m_currentTempDevice->maxReadSpeed = i;
  }
}


void K3bDeviceWidget::slotCdrwChanged( bool b )
{
  if( m_currentTempDevice != 0 ) {
    m_currentTempDevice->cdrw = b;
  }
}


void K3bDeviceWidget::slotBurnproofChanged( bool b )
{
  if( m_currentTempDevice != 0 ) {
    m_currentTempDevice->burnproof = b;
  }
}


void K3bDeviceWidget::showWriterSpecificProps( bool b )
{
  if( b ) {
    m_labelWriteSpeed->show();
    m_spinWriteSpeed->show();
    m_labelCdText->show();
    m_comboCdText->show();
    m_labelBurnProof->show();
    m_checkBurnProof->show();
    m_labelCdrw->show();
    m_checkCdrw->show();
  }
  else {
    m_labelWriteSpeed->hide();
    m_spinWriteSpeed->hide();
    m_labelCdText->hide();
    m_comboCdText->hide();
    m_labelBurnProof->hide();
    m_checkBurnProof->hide();
    m_labelCdrw->hide();
    m_checkCdrw->hide();
  }
}


#include "k3bdevicewidget.moc"
