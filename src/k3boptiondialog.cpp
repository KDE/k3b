/***************************************************************************
                          k3boptiondialog.cpp  -  description
                             -------------------
    begin                : Tue Apr 17 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#include "k3boptiondialog.h"
#include "device/k3bdevicemanager.h"
#include "device/k3bdevice.h"
#include "k3b.h"
#include "option/k3boptioncddb.h"

#include <qheader.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qpixmap.h>
#include <qfile.h>
#include <qlist.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qstringlist.h>
#include <qpoint.h>
#include <qxembed.h>
#include <qwidgetstack.h>
#include <qhbox.h>

#include <klistview.h>
#include <klistbox.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kapp.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <kaction.h>
#include <dcopclient.h>
#include <kprocess.h>
#include <klineedit.h>


K3bOptionDialog::K3bOptionDialog(QWidget *parent, const char *name, bool modal )
  : KDialogBase( IconList, i18n("Options"), Help|Apply|Ok|Cancel|Default, Ok, parent,name, modal, true)
{
  devicesChanged = false;

  setupBurningPage();
  setupDevicePage();	
  setupProgramsPage();
  setupCddbPage();
  //	setupPermissionPage();
	
  readBurningSettings();
  readPrograms();
  readDevices();
  m_cddbPage->readSettings();
  	
  resize( 620, 500 );
}


K3bOptionDialog::~K3bOptionDialog()
{
	delete m_cddbPage;
}


void K3bOptionDialog::slotOk()
{
  // save and only exit if all was successful
  bool bAccept = true;
	
  bAccept = bAccept && savePrograms();

  slotApply();
	
  if( bAccept )
    accept();
}

void K3bOptionDialog::slotApply()
{
  // save all the shit!
  saveBurningSettings();
  savePrograms();
  saveDevices();
  m_cddbPage->apply();
}


void K3bOptionDialog::setupProgramsPage()
{
  QFrame* frame = addPage( i18n("Programs"), i18n("Setup external programs"),
			   KGlobal::instance()->iconLoader()->loadIcon( "gear", KIcon::NoGroup, KIcon::SizeMedium ) );
	
  // add all to frame
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  m_viewPrograms = new KListView( frame, "m_viewPrograms" );
  m_viewPrograms->addColumn( i18n( "Program" ) );
  m_viewPrograms->addColumn( i18n( "path" ) );
  m_viewPrograms->addColumn( i18n( "additional parameters" ) );
	
  // set the second column renameable
  m_viewPrograms->setItemsRenameable( true );
  m_viewPrograms->setRenameable( 0, false );
  m_viewPrograms->setRenameable( 1, true );
  m_viewPrograms->setRenameable( 2, true );

  frameLayout->addMultiCellWidget( m_viewPrograms, 1, 1, 0, 1 );

  m_buttonSearch = new QPushButton( frame, "m_buttonSearch" );
  m_buttonSearch->setText( i18n( "Search" ) );

  frameLayout->addWidget( m_buttonSearch, 2, 1 );

  m_labelInfo = new QLabel( frame, "m_labelInfo" );
  m_labelInfo->setText( i18n( "Please specify the paths to the external programs that K3b needs to work properly or press \"Search\" to let K3b search for the programs." ) );
  m_labelInfo->setScaledContents( FALSE );
  m_labelInfo->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter | QLabel::AlignLeft ) );

  frameLayout->addMultiCellWidget( m_labelInfo, 0, 0, 0, 1 );
  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  frameLayout->addItem( spacer, 2, 0 );

  // TODO: connect the search button to a suitable slot
  m_buttonSearch->setDisabled( true );
}


void K3bOptionDialog::readPrograms()
{
  KConfig* config = kapp->config();
  config->setGroup("External Programs");
	
  // clear the view before adding anything!
  m_viewPrograms->clear();
	
  QListViewItem * item = new QListViewItem( m_viewPrograms );
  item->setText( 0, "mkisofs" );
  item->setText( 1, config->readEntry( "mkisofs path", "/usr/bin/mkisofs" ) );
  item->setText( 2, config->readListEntry( "mkisofs parameters" ).join(" ") );

  item = new QListViewItem( m_viewPrograms );
  item->setText( 0, "cdrecord"  );
  item->setText( 1, config->readEntry( "cdrecord path", "/usr/bin/cdrecord" ) );
  item->setText( 2, config->readListEntry( "cdrecord parameters" ).join(" ") );

  //    item = new QListViewItem( m_viewPrograms );
  //    item->setText( 0, "readcd"  );
  //    item->setText( 1,  config->readEntry( "readcd path", "/usr/bin/readcd" ) );
  //    item->setText( 2,  config->readListEntry( "readcd parameters" ).join(" ") );

  item = new QListViewItem( m_viewPrograms );
  item->setText( 0, "mpg123"  );
  item->setText( 1,  config->readEntry( "mpg123 path", "/usr/bin/mpg123" ) );
  item->setText( 2,  config->readListEntry( "mpg123 parameters" ).join(" ") );

  //    item = new QListViewItem( m_viewPrograms );
  //    item->setText( 0, "cdparanoia"  );
  //    item->setText( 1,  config->readEntry( "cdparanoia path", "/usr/bin/cdparanoia" ) );
  //    item->setText( 2,  config->readListEntry( "cdparanoia parameters" ).join(" ") );

  item = new QListViewItem( m_viewPrograms );
  item->setText( 0, "cdrdao"  );
  item->setText( 1,  config->readEntry( "cdrdao path", "/usr/bin/cdrdao" ) );
  item->setText( 2,  config->readListEntry( "cdrdao parameters" ).join(" ") );

  item = new QListViewItem( m_viewPrograms );
  item->setText( 0, "sox"  );
  item->setText( 1,  config->readEntry( "sox path", "/usr/bin/sox" ) );
  item->setText( 2,  config->readListEntry( "sox parameters" ).join(" ") );
}

bool K3bOptionDialog::savePrograms()
{
  KConfig* config = kapp->config();
  config->setGroup( "External Programs" );
	
  // go through all the items in m_viewPrograms and save the values
  // test if the given path exists and show an error dialog if not!
  QString notFound;
	
  QListViewItem* _item = m_viewPrograms->firstChild();
  while( _item ) {
    if( !QFile::exists( _item->text(1) ) )
      notFound.append( _item->text(1) + "\n" );
    else {
      QString entryName = _item->text(0) + " path";
      config->writeEntry( entryName, _item->text(1) );
      entryName = _item->text(0) + " parameters";
      config->writeEntry( entryName, QStringList::split(' ', _item->text(2)) );
    }
    _item = _item->nextSibling();
  }
  config->sync();
	
  if( !notFound.isEmpty() ) {
    KMessageBox::error(	this, i18n("Could not find these programs: \n%s", notFound), i18n("Error") );
    return false;
  }
	
  return true;
}


void K3bOptionDialog::setupDevicePage()
{
  QFrame* frame = addPage( i18n("Devices"), i18n("Setup SCSI CD Devices"),
			   KGlobal::instance()->iconLoader()->loadIcon( "blockdevice", KIcon::NoGroup, KIcon::SizeMedium ) );

  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );


  // Info Label
  // ------------------------------------------------
  m_labelDevicesInfo = new QLabel( frame, "m_labelDevicesInfo" );
  m_labelDevicesInfo->setText( i18n( "Normally K3b should detect all your SCSI-devices properly. If it does not, here you can change or add devices manually." ) );
  m_labelDevicesInfo->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter | QLabel::AlignLeft ) );

  frameLayout->addMultiCellWidget( m_labelDevicesInfo, 0, 0, 0, 1 );
  // ------------------------------------------------


  // Refresh button
  // ------------------------------------------------
  QGridLayout* refreshButtonGrid = new QGridLayout;
  m_buttonRefreshDevices = new QPushButton( i18n( "Refresh" ), frame, "m_buttonRefreshDevices" );
  QToolTip::add(  m_buttonRefreshDevices, i18n( "Scan for SCSI-Devices" ) );
  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  refreshButtonGrid->addItem( spacer, 0, 0 );
  refreshButtonGrid->addWidget( m_buttonRefreshDevices, 0, 1 );

  frameLayout->addMultiCellLayout( refreshButtonGrid, 3, 3, 0, 1 );
  // ------------------------------------------------


  // Reading Devices
  // ------------------------------------------------
  m_groupReader = new QGroupBox( frame, "m_groupReader" );
  m_groupReader->setTitle( i18n( "Reading Devices" ) );
  m_groupReader->setColumnLayout(0, Qt::Vertical );
  m_groupReader->layout()->setSpacing( 0 );
  m_groupReader->layout()->setMargin( 0 );
  QHBoxLayout* m_groupReaderLayout = new QHBoxLayout( m_groupReader->layout() );
  m_groupReaderLayout->setAlignment( Qt::AlignTop );
  m_groupReaderLayout->setSpacing( spacingHint() );
  m_groupReaderLayout->setMargin( marginHint() );

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
  m_groupWriter = new QGroupBox( frame, "m_groupWriter" );
  m_groupWriter->setTitle( i18n( "Writing Devices" ) );
  m_groupWriter->setColumnLayout(0, Qt::Vertical );
  m_groupWriter->layout()->setSpacing( 0 );
  m_groupWriter->layout()->setMargin( 0 );
  QHBoxLayout* m_groupWriterLayout = new QHBoxLayout( m_groupWriter->layout() );
  m_groupWriterLayout->setAlignment( Qt::AlignTop );
  m_groupWriterLayout->setSpacing( spacingHint() );
  m_groupWriterLayout->setMargin( marginHint() );

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
  m_groupDeviceInfo = new QGroupBox( frame, "m_groupDeviceInfo" );
  m_groupDeviceInfo->setTitle( i18n( "Device Info" ) );
  m_groupDeviceInfo->setColumnLayout(0, Qt::Vertical );
  m_groupDeviceInfo->layout()->setSpacing( 0 );
  m_groupDeviceInfo->layout()->setMargin( 0 );

  QHBoxLayout* groupDeviceInfoLayout = new QHBoxLayout( m_groupDeviceInfo->layout() );
  groupDeviceInfoLayout->setAlignment( Qt::AlignTop );
  groupDeviceInfoLayout->setSpacing( spacingHint() );
  groupDeviceInfoLayout->setMargin( marginHint() );

  m_viewDeviceInfo = new KListView( m_groupDeviceInfo, "m_viewDeviceInfo" );
  m_viewDeviceInfo->addColumn( i18n("name") );
  m_viewDeviceInfo->addColumn( i18n("value") );
  m_viewDeviceInfo->header()->hide();
  m_viewDeviceInfo->setSorting( -1 ); // disable sorting
  //m_viewDeviceInfo->setRootIsDecorated( true );
  m_viewDeviceInfo->setItemsRenameable( true );
  m_viewDeviceInfo->setRenameable( 0, false );
  m_viewDeviceInfo->setRenameable( 1, true );
  groupDeviceInfoLayout->addWidget( m_viewDeviceInfo );

  frameLayout->addMultiCellWidget( m_groupDeviceInfo, 1, 2, 1, 1 );
  // ------------------------------------------------


  // POPUP Menu
  // ------------------------------------------------
  m_menuDevices = new KActionMenu( "Devices", SmallIconSet("blockdevice"), this, "DevicesPopup" );
  m_actionNewDevice = new KAction( "New Device", 0, this, SLOT(slotNewDevice()), this );
  m_actionRemoveDevice = new KAction( "Remove Device", SmallIconSet("editdelete"), 0, this, SLOT(slotRemoveDevice()), this );
  m_menuDevices->insert( m_actionNewDevice );
  m_menuDevices->insert( new KActionSeparator(this) );
  m_menuDevices->insert( new KAction( "Refresh Devices", SmallIconSet("reload"), 0, this, SLOT(slotRefreshDevices()), this ) );
  // ------------------------------------------------


  // temporary device lists settings
  // ------------------------------------------------
  m_tempReader.setAutoDelete( true );
  m_tempWriter.setAutoDelete( true );
  // ------------------------------------------------


  // connections
  // ------------------------------------------------		
  connect( m_viewDevicesReader, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(slotDevicesPopup(QListViewItem*, const QPoint&)) );
  connect( m_viewDevicesWriter, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(slotDevicesPopup(QListViewItem*, const QPoint&)) );
	
  connect( m_buttonRefreshDevices, SIGNAL(clicked()), this, SLOT(slotRefreshDevices()) );
	
  connect( m_viewDeviceInfo, SIGNAL(itemRenamed(QListViewItem*)), this, SLOT(slotDeviceInfoRenamed(QListViewItem*)) );

  connect( m_viewDevicesReader, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotDeviceSelected(QListViewItem*)) );
  connect( m_viewDevicesWriter, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotDeviceSelected(QListViewItem*)) );
  // ------------------------------------------------
}


void K3bOptionDialog::readDevices()
{
  K3bDeviceManager* dm = k3bMain()->deviceManager();
	
  // fill the temporary lists
  m_tempReader.clear();
  m_tempWriter.clear();

  // add the reading devices
  K3bDevice* dev = dm->readingDevices().first();
  while( dev ) {
    m_tempReader.append( new PrivateTempDevice( dev->vendor(), 
						dev->description(), 
						dev->version(),
						dev->burner(),
						dev->burnproof(),
						dev->maxReadSpeed(),
						dev->devicename(),
						dev->maxWriteSpeed() ) );
    dev = dm->readingDevices().next();
  }
	
  // add the writing devices
  dev = dm->burningDevices().first();
  while( dev ) {
    m_tempWriter.append( new PrivateTempDevice( dev->vendor(), 
						dev->description(), 
						dev->version(),
						dev->burner(),
						dev->burnproof(),
						dev->maxReadSpeed(),
						dev->devicename(),
						dev->maxWriteSpeed() ) );
    dev = dm->burningDevices().next();
  }

  updateDeviceListViews();
}


void K3bOptionDialog::updateDeviceListViews()
{
  m_viewDevicesReader->clear();
  m_viewDevicesWriter->clear();

  PrivateDeviceViewItem* item;

  PrivateTempDevice* dev = m_tempReader.first();
  while( dev ) {
    // add item to m_viewDevices
    item = new PrivateDeviceViewItem( dev, m_viewDevicesReader );
    item->setPixmap( 0, KGlobal::instance()->iconLoader()->loadIcon( "cdrom_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );
    item->setText( 1, dev->vendor );
    item->setText( 2, dev->description );

    dev = m_tempReader.next();
  }

  dev = m_tempWriter.first();
  while( dev ) {
    // add item to m_viewDevices
    item = new PrivateDeviceViewItem( dev, m_viewDevicesWriter );
    item->setPixmap( 0, KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );
    item->setText( 1, dev->vendor );
    item->setText( 2, dev->description );

    dev = m_tempWriter.next();
  }
}


void K3bOptionDialog::updateDeviceInfoBox( PrivateTempDevice* dev )
{
  m_viewDeviceInfo->clear();
  if( dev ) {
    PrivateDeviceViewItem* item;

    item = new PrivateDeviceViewItem( dev, m_viewDeviceInfo );
    item->setText( 0, "Linux device" );
    item->setText( 1, dev->devicename );

    if( dev->burner ) {
      item = new PrivateDeviceViewItem( dev, m_viewDeviceInfo );
      item->setText( 0, "BURN-PROOF" );
      item->setText( 1, (dev->burnproof ? "yes" : "no" ) );

      item = new PrivateDeviceViewItem( dev, m_viewDeviceInfo );
      item->setText( 0, "Max Writespeed" );
      item->setText( 1, QString::number( dev->maxWriteSpeed ) );
    }

    item = new PrivateDeviceViewItem( dev, m_viewDeviceInfo );
    item->setText( 0, "Max Readspeed" );
    item->setText( 1, QString::number( dev->maxReadSpeed ) );

    item = new PrivateDeviceViewItem( dev, m_viewDeviceInfo );
    item->setText( 0, "Firmware version" );
    item->setText( 1, dev->version );

    item = new PrivateDeviceViewItem( dev, m_viewDeviceInfo );
    item->setText( 0, "Model name" );
    item->setText( 1, dev->description );
    
    item = new PrivateDeviceViewItem( dev, m_viewDeviceInfo );
    item->setText( 0, "Vendor" );
    item->setText( 1, dev->vendor );
  }
}


void K3bOptionDialog::slotDeviceInfoRenamed( QListViewItem* item )
{
  if( item->listView() == m_viewDeviceInfo ) {
    PrivateTempDevice* dev = ((PrivateDeviceViewItem*)item)->device;
    if( item->text(0) == "Vendor" )
      dev->vendor = item->text(1);
    else if( item->text(0) == "Model name" )
      dev->description = item->text(1);
    else if( item->text(0) == "Firmware version" )
      dev->version = item->text(1);
    else if( item->text(0) == "Max Readspeed" )
      dev->maxReadSpeed = item->text(1).toInt();
    else if( item->text(0) == "Max Writespeed" )
      dev->maxWriteSpeed = item->text(1).toInt();
    else if( item->text(0) == "BURN-PROOF" )
      dev->burnproof = ( item->text(1) == "yes" ? true : false );
    else if( item->text(0) == "Linux device" )
      dev->devicename = item->text(1);

    updateDeviceInfoBox( dev );
    updateDeviceListViews();
    devicesChanged = true;
  }
}


void K3bOptionDialog::slotRefreshDevices()
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


void K3bOptionDialog::slotNewDevice()
{
  // check what kind of device
  if( m_viewDevicesReader->hasFocus() ) {
    PrivateTempDevice* dev = new PrivateTempDevice();
    m_tempReader.append( dev );
    PrivateDeviceViewItem* _item = new PrivateDeviceViewItem( dev, m_viewDevicesReader );
    _item->setPixmap( 0, KGlobal::instance()->iconLoader()->loadIcon( "cdrom_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );
    m_viewDevicesReader->setSelected( _item, true );
  }
  else if( m_viewDevicesWriter->hasFocus() ) {
    PrivateTempDevice* dev = new PrivateTempDevice();
    m_tempWriter.append( dev );
    PrivateDeviceViewItem* _item = new PrivateDeviceViewItem( dev, m_viewDevicesWriter );
    _item->setPixmap( 0, KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );
    m_viewDevicesWriter->setSelected( _item, true );
  }
}


void K3bOptionDialog::slotRemoveDevice()
{
  // check if there is a device selected
  if( m_viewDevicesReader->hasFocus() ) {
    if( QListViewItem* _item = m_viewDevicesReader->selectedItem() ) {
      PrivateDeviceViewItem* deviceItem = (PrivateDeviceViewItem*)_item;
      m_tempReader.remove( deviceItem->device );
      delete _item;
      devicesChanged = true;
      updateDeviceInfoBox();  // clear box
    }
  }
  else if( m_viewDevicesWriter->hasFocus() ) {
    if( QListViewItem* _item = m_viewDevicesWriter->selectedItem() ) {
      PrivateDeviceViewItem* deviceItem = (PrivateDeviceViewItem*)_item;
      m_tempWriter.remove( deviceItem->device );
      delete _item;
      devicesChanged = true;
      updateDeviceInfoBox();  // clear box
    }
  }
}


void K3bOptionDialog::saveDevices()
{
  // only save devices to KConfig if the devices have been changed
  if( devicesChanged ) {
    KConfig* c = kapp->config();
    c->setGroup("Devices");
    QString entryName;
    int entryNum = 1;
		
    PrivateTempDevice* dev = m_tempReader.first();
    while( dev ) {
      QStringList list;
      list.append( dev->vendor ); // vendor
      list.append( dev->description ); // description
      list.append( dev->version ); // version
      list.append( QString::number( dev->maxReadSpeed ) ); // max readspeed
      list.append( dev->devicename );
			
      entryName = QString( "Reader%1").arg(entryNum);
      c->writeEntry( entryName, list );

      dev = m_tempReader.next();
      entryNum++;
    }
		
    entryNum = 1;
    dev = m_tempWriter.first();
    while( dev ) {
      QStringList list;
      list.append( dev->vendor ); // vendor
      list.append( dev->description ); // description
      list.append( dev->version ); // version
      list.append( QString::number( dev->maxReadSpeed ) ); // max readspeed
      list.append( QString::number( dev->maxWriteSpeed ) ); // max writespeed
      list.append( dev->burnproof ? "yes" : "no" );
      list.append( dev->devicename );
			
      entryName = QString( "Writer%1").arg(entryNum);
      c->writeEntry( entryName, list );
			
      dev = m_tempWriter.next();
      entryNum++;
    }
		
    c->sync();
		
    // update Device Manager
    k3bMain()->deviceManager()->clear();
    k3bMain()->deviceManager()->readConfig();
  }
}

void K3bOptionDialog::slotDeviceSelected( QListViewItem* item )
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


void K3bOptionDialog::slotDevicesPopup( QListViewItem* item, const QPoint& p )
{
  m_menuDevices->remove( m_actionRemoveDevice );
  if( item )
    m_menuDevices->insert( m_actionRemoveDevice, 1 );
	
  m_menuDevices->popup(p);	
}


void K3bOptionDialog::slotDefault()
{
  switch( activePageIndex() )
    {
    case 0: // device page
      slotRefreshDevices();
      break;
    case 1: // programs page
      QListViewItem* _item = m_viewPrograms->firstChild();
      while( _item ) {
	QString entryName = "/usr/bin/" + _item->text(0);
	_item->setText( 1, entryName );
	if( _item->text(0) == "cdrdao" )
	  _item->setText( 2, "--driver generic-mmc" );
	else
	  _item->setText( 2, "" );
	_item = _item->nextSibling();
      }
      break;
    }
}


void K3bOptionDialog::setupBurningPage()
{
  QFrame* frame = addPage( i18n("Burning"), i18n("Some Burning Settings"),
			   KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", KIcon::NoGroup, KIcon::SizeMedium ) );
		
  QGridLayout* _frameLayout = new QGridLayout( frame );
  _frameLayout->setSpacing( spacingHint() );
  _frameLayout->setMargin( marginHint() );

  m_checkUseID3Tag = new QCheckBox( "Use ID3 Tags for filenames", frame );
  _frameLayout->addWidget( m_checkUseID3Tag, 0, 0 );
}


void K3bOptionDialog::readBurningSettings()
{
  kapp->config()->setGroup("ISO Options");
  m_checkUseID3Tag->setChecked( kapp->config()->readBoolEntry("Use ID3 Tag for mp3 renaming", false) );
	
}


void K3bOptionDialog::saveBurningSettings()
{
  kapp->config()->setGroup("ISO Options");
  kapp->config()->writeEntry( "Use ID3 Tag for mp3 renaming", m_checkUseID3Tag->isChecked() );
	
  k3bMain()->setUseID3TagForMp3Renaming( m_checkUseID3Tag->isChecked() );
}

void K3bOptionDialog::setupCddbPage()
{
  QFrame* frame = addPage( i18n("CDDB"), i18n("Setup the cddb server"),
			   KGlobal::instance()->iconLoader()->loadIcon( "gear", KIcon::NoGroup, KIcon::SizeMedium ) );
  m_cddbPage = new K3bOptionCddb(frame, "cddbpage");
}

