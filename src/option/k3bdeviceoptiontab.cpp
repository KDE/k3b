
#include "k3bdeviceoptiontab.h"
#include "../device/k3bdevicemanager.h"
#include "../device/k3bdevice.h"
#include "../k3b.h"

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



class K3bDeviceOptionTab::PrivateTempDevice 
{
public:
  PrivateTempDevice( K3bDevice* d );
  
  K3bDevice* device;
  int maxReadSpeed;
  int maxWriteSpeed;
  QString cdrdaoDriver;
  bool cdTextCapable;
};


class K3bDeviceOptionTab::PrivateDeviceViewItem : public KListViewItem 
{
public:
  PrivateDeviceViewItem( PrivateTempDevice* dev, KListView* view )
    : KListViewItem( view ) { device = dev; }
  PrivateDeviceViewItem( PrivateTempDevice* dev, QListViewItem* item )
    : KListViewItem( item ) { device = dev; }
  
  PrivateTempDevice* device;
};


K3bDeviceOptionTab::PrivateTempDevice::PrivateTempDevice( K3bDevice* d )
{
  device = d;
  cdrdaoDriver = d->cdrdaoDriver();
  maxReadSpeed = d->maxReadSpeed();
  maxWriteSpeed = d->maxWriteSpeed();
  cdTextCapable = ( d->cdTextCapable() != 2 );
}


K3bDeviceOptionTab::K3bDeviceOptionTab( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  m_currentTempDevice = 0;
  devicesChanged = false;

  QGridLayout* frameLayout = new QGridLayout( this );
  frameLayout->setSpacing( KDialog::spacingHint() );
  frameLayout->setMargin( KDialog::marginHint() );


  // Info Label
  // ------------------------------------------------
  m_labelDevicesInfo = new QLabel( this, "m_labelDevicesInfo" );
  m_labelDevicesInfo->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter | QLabel::AlignLeft ) );
  m_labelDevicesInfo->setText( i18n( "K3b tries to detect all your devices properly. Sometimes this does not work for the read or the write speed. In this case you can change them manually. You can add not detected devices and change the cdrdao driver for the generic scsi drives." ) );

  frameLayout->addMultiCellWidget( m_labelDevicesInfo, 0, 0, 0, 1 );
  // ------------------------------------------------


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
  frameLayout->addMultiCellLayout( refreshButtonGrid, 3, 3, 0, 1 );
  // ------------------------------------------------


  // Reading Devices
  // ------------------------------------------------
  m_groupReader = new QGroupBox( this, "m_groupReader" );
  m_groupReader->setTitle( i18n( "Reading Devices" ) );
  m_groupReader->setColumnLayout(0, Qt::Vertical );
  m_groupReader->layout()->setSpacing( 0 );
  m_groupReader->layout()->setMargin( 0 );
  QHBoxLayout* m_groupReaderLayout = new QHBoxLayout( m_groupReader->layout() );
  m_groupReaderLayout->setAlignment( Qt::AlignTop );
  m_groupReaderLayout->setSpacing( KDialog::spacingHint() );
  m_groupReaderLayout->setMargin( KDialog::marginHint() );

  m_viewDevicesReader = new KListView( m_groupReader, "m_viewDevicesReader" );
  m_viewDevicesReader->addColumn( "" );
  m_viewDevicesReader->addColumn( i18n( "Vendor" ) );
  m_viewDevicesReader->addColumn( i18n( "Description" ) );
  m_viewDevicesReader->setAllColumnsShowFocus( TRUE );
  m_viewDevicesReader->header()->hide();
  m_groupReaderLayout->addWidget( m_viewDevicesReader );

  frameLayout->addWidget( m_groupReader, 1, 0 );
  // ------------------------------------------------


  // Writing Devices
  // ------------------------------------------------
  m_groupWriter = new QGroupBox( this, "m_groupWriter" );
  m_groupWriter->setTitle( i18n( "Writing Devices" ) );
  m_groupWriter->setColumnLayout(0, Qt::Vertical );
  m_groupWriter->layout()->setSpacing( 0 );
  m_groupWriter->layout()->setMargin( 0 );
  QHBoxLayout* m_groupWriterLayout = new QHBoxLayout( m_groupWriter->layout() );
  m_groupWriterLayout->setAlignment( Qt::AlignTop );
  m_groupWriterLayout->setSpacing( KDialog::spacingHint() );
  m_groupWriterLayout->setMargin( KDialog::marginHint() );

  m_viewDevicesWriter = new KListView( m_groupWriter, "m_viewDevicesWriter" );
  m_viewDevicesWriter->addColumn( "" );
  m_viewDevicesWriter->addColumn( i18n( "Vendor" ) );
  m_viewDevicesWriter->addColumn( i18n( "Description" ) );
  m_viewDevicesWriter->setAllColumnsShowFocus( TRUE );
  m_viewDevicesWriter->header()->hide();
  m_groupWriterLayout->addWidget( m_viewDevicesWriter );

  frameLayout->addWidget( m_groupWriter, 2, 0 );
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
  m_checkBurnProof = new QLabel( m_groupDeviceInfo, "m_checkBurnProof" );
  m_labelCdText = new QLabel( i18n( "Write CD-Text:" ), m_groupDeviceInfo, "labelCdText" );
  m_line3 = new QFrame( m_groupDeviceInfo, "line3" );
  m_line3->setFrameStyle( QFrame::HLine | QFrame::Sunken );


  // set the backgroud colors of the labels that display the actual values
  m_labelDevicename->setBackgroundColor( Qt::white );
  m_labelDeviceInterface->setBackgroundColor( Qt::white );
  m_labelVersion->setBackgroundColor( Qt::white );
  m_labelVendor->setBackgroundColor( Qt::white );
  m_labelDescription->setBackgroundColor( Qt::white );
  m_checkBurnProof->setBackgroundColor( Qt::white );


  m_labelDevicename->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  m_labelDeviceInterface->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  m_labelVersion->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  m_labelVendor->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  m_labelDescription->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  m_checkBurnProof->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );



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


  frameLayout->addMultiCellWidget( m_groupDeviceInfo, 1, 2, 1, 1 );
  // ------------------------------------------------

  showWriterSpecificProps( false );
  // -------------------------------------


  // temporary device lists settings
  // ------------------------------------------------
  m_tempReader.setAutoDelete( true );
  m_tempWriter.setAutoDelete( true );
  // ------------------------------------------------


  // connections
  // ------------------------------------------------		
  connect( m_buttonRefreshDevices, SIGNAL(clicked()), this, SLOT(slotRefreshDevices()) );
  connect( m_buttonAddDevice, SIGNAL(clicked()), this, SLOT(slotNewDevice()) );
	
  connect( m_viewDevicesReader, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotDeviceSelected(QListViewItem*)) );
  connect( m_viewDevicesWriter, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotDeviceSelected(QListViewItem*)) );

  connect( m_comboDriver, SIGNAL(activated(const QString&)), this, SLOT(slotCdrdaoDriverChanged(const QString&)) );
  connect( m_comboCdText, SIGNAL(activated(const QString&)), this, SLOT(slotCdTextCapabilityChanged(const QString&)) );
  connect( m_spinWriteSpeed, SIGNAL(valueChanged(int)), this, SLOT(slotWriteSpeedChanged(int)) );
  connect( m_spinReadSpeed, SIGNAL(valueChanged(int)), this, SLOT(slotReadSpeedChanged(int)) );
  // ------------------------------------------------

  // fill the driver-combo-box
  // -----------------------------------------
  for( int i = 0; i < 13; i++ ) {
    m_comboDriver->insertItem( K3bDevice::cdrdao_drivers[i] );
  }
  // -----------------------------------------
}


K3bDeviceOptionTab::~K3bDeviceOptionTab()
{
  m_tempReader.clear();
  m_tempWriter.clear();
}


void K3bDeviceOptionTab::readDevices()
{
  K3bDeviceManager* dm = k3bMain()->deviceManager();
	
  // fill the temporary lists
  m_tempReader.clear();
  m_tempWriter.clear();

  // add the reading devices
  K3bDevice* dev = dm->readingDevices().first();
  while( dev ) {
    m_tempReader.append( new PrivateTempDevice( dev ) );
    dev = dm->readingDevices().next();
  }
	
  // add the writing devices
  dev = dm->burningDevices().first();
  while( dev ) {
    m_tempWriter.append( new PrivateTempDevice( dev ) );
    dev = dm->burningDevices().next();
  }

  devicesChanged = false;

  updateDeviceListViews();
}


void K3bDeviceOptionTab::updateDeviceListViews()
{
  m_viewDevicesReader->clear();
  m_viewDevicesWriter->clear();

  PrivateDeviceViewItem* item;

  PrivateTempDevice* dev = m_tempReader.first();
  while( dev ) {
    // add item to m_viewDevices
    item = new PrivateDeviceViewItem( dev, m_viewDevicesReader );
    item->setPixmap( 0, SmallIcon( "cdrom_unmount" ) );
    item->setText( 1, dev->device->vendor() );
    item->setText( 2, dev->device->description() );

    dev = m_tempReader.next();
  }

  dev = m_tempWriter.first();
  while( dev ) {
    // add item to m_viewDevices
    item = new PrivateDeviceViewItem( dev, m_viewDevicesWriter );
    item->setPixmap( 0, SmallIcon( "cdwriter_unmount" ) );
    item->setText( 1, dev->device->vendor() );
    item->setText( 2, dev->device->description() );

    dev = m_tempWriter.next();
  }

  if( QListViewItem* i = m_viewDevicesReader->firstChild() ) {
    m_viewDevicesReader->setSelected( i, true );
  }
  else if( QListViewItem* i = m_viewDevicesWriter->firstChild() ) {
    m_viewDevicesWriter->setSelected( i, true );
  }
}


void K3bDeviceOptionTab::updateDeviceInfoBox( PrivateTempDevice* tempDev )
{
  m_currentTempDevice = tempDev;

  if( tempDev != 0 ) {
    m_groupDeviceInfo->setEnabled( true );

    K3bDevice* dev = tempDev->device;

    m_labelDevicename->setText( dev->devicename() );
    m_labelDeviceInterface->setText( dev->interfaceType() == K3bDevice::SCSI ? "Generic Scsi" : "Ide" );
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
    
    if( dev->burner() ) {
      m_spinWriteSpeed->setValue( tempDev->maxWriteSpeed );
      m_checkBurnProof->setText( dev->burnproof() ? "yes" : "no" );

      m_comboCdText->clear();
      if( tempDev->cdrdaoDriver == "auto" ) {
	m_comboCdText->insertItem( "auto" );
      }
      else {
	m_comboCdText->insertItem( "yes", 0 );
	m_comboCdText->insertItem( "no", 1 );
	m_comboCdText->setCurrentItem( tempDev->cdTextCapable ? 0 : 1 );
      }

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


void K3bDeviceOptionTab::slotRefreshDevices()
{
  KSimpleConfig cfg( kapp->dirs()->findResource( "config", "k3brc" ) );
  if( cfg.hasGroup( "Devices" ) ) {
    // remove all old device entrys
    cfg.deleteGroup("Devices");
    cfg.sync();
		
    // let the main config know about it!
    kapp->config()->reparseConfiguration();
  }

  // reread devices
  k3bMain()->deviceManager()->clear();
  k3bMain()->deviceManager()->scanbus();
  readDevices();

  updateDeviceListViews();
  updateDeviceInfoBox();
	
  devicesChanged = false;
}


void K3bDeviceOptionTab::slotNewDevice()
{
  bool ok;
  QString newDevicename = KLineEditDlg::getText( "Please enter the devicename where\n K3b shall search for a new drive\n(example: /dev/mebecdrom)", "/dev/", &ok, this );

  if( ok ) {
    if( K3bDevice* dev = k3bMain()->deviceManager()->addDevice( newDevicename ) ) {
      if( dev->burner() )
	m_tempWriter.append( new PrivateTempDevice( dev ) );
      else
	m_tempReader.append( new PrivateTempDevice( dev ) );	

      updateDeviceListViews();
    }
    else
      KMessageBox::error( this, "Sorry, could not find an additional device at\n" + newDevicename, i18n("Error"), false );
  }
}


void K3bDeviceOptionTab::saveDevices()
{
  // only save devices to KConfig if the devices have been changed
  if( devicesChanged ) {

    // update the devices
    PrivateTempDevice* tempDev = m_tempWriter.first();
    while( tempDev != 0 ) {
      tempDev->device->setMaxReadSpeed( tempDev->maxReadSpeed );
      tempDev->device->setMaxWriteSpeed( tempDev->maxWriteSpeed );
      tempDev->device->setCdrdaoDriver( tempDev->cdrdaoDriver );
      tempDev->device->setCdTextCapability( tempDev->cdTextCapable );

      tempDev = m_tempWriter.next();
    }

    tempDev = m_tempReader.first();
    while( tempDev != 0 ) {
      tempDev->device->setMaxReadSpeed( tempDev->maxReadSpeed );
      tempDev->device->setCdrdaoDriver( tempDev->cdrdaoDriver );

      tempDev = m_tempReader.next();
    }

    // save the config
    k3bMain()->config()->setGroup( "Devices" );
    k3bMain()->deviceManager()->saveConfig( k3bMain()->config() );

    devicesChanged = false;
  }
}

void K3bDeviceOptionTab::slotDeviceSelected( QListViewItem* item )
{
  KListView* listView = (KListView*)item->listView();
  PrivateDeviceViewItem* deviceItem = (PrivateDeviceViewItem*)item;

  if( listView == m_viewDevicesWriter ) {
    m_viewDevicesReader->clearSelection();
  }
  else {
    m_viewDevicesWriter->clearSelection();
  }

  updateDeviceInfoBox( deviceItem->device );
}


void K3bDeviceOptionTab::slotCdrdaoDriverChanged( const QString& driver )
{
  if( m_currentTempDevice != 0 ) {
    m_currentTempDevice->cdrdaoDriver = driver;
    
    m_comboCdText->clear();
    if( driver == "auto" ) {
      m_comboCdText->insertItem( "auto" );
    }
    else {
      m_comboCdText->insertItem( "yes", 0 );
      m_comboCdText->insertItem( "no", 1 );
      m_comboCdText->setCurrentItem( m_currentTempDevice->cdTextCapable ? 0 : 1 );
    }

    devicesChanged = true;
  }
}


void K3bDeviceOptionTab::slotCdTextCapabilityChanged( const QString& s )
{
  if( m_currentTempDevice != 0 ) {
    m_currentTempDevice->cdTextCapable = ( s == "yes" );
    devicesChanged = true;
  }
}


void K3bDeviceOptionTab::slotWriteSpeedChanged( int i )
{
  if( m_currentTempDevice != 0 ) {
    m_currentTempDevice->maxWriteSpeed = i;
    devicesChanged = true;
  }
}


void K3bDeviceOptionTab::slotReadSpeedChanged( int i )
{
  if( m_currentTempDevice != 0 ) {
    m_currentTempDevice->maxReadSpeed = i;
    devicesChanged = true;
  }
}


void K3bDeviceOptionTab::showWriterSpecificProps( bool b )
{
  if( b ) {
    m_labelWriteSpeed->show();
    m_spinWriteSpeed->show();
    m_labelCdText->show();
    m_comboCdText->show();
    m_labelBurnProof->show();
    m_checkBurnProof->show();
  }
  else {
    m_labelWriteSpeed->hide();
    m_spinWriteSpeed->hide();
    m_labelCdText->hide();
    m_comboCdText->hide();
    m_labelBurnProof->hide();
    m_checkBurnProof->hide();
  }
}


#include "k3bdeviceoptiontab.moc"
