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
#include "k3bdevicemanager.h"
#include "k3b.h"

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
#include <qstringlist.h>
#include <qpoint.h>

#include <klistview.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kapp.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <kaction.h>


K3bOptionDialog::K3bOptionDialog(QWidget *parent, const char *name, bool modal )
	: KDialogBase( IconList, i18n("Options"), Help|Apply|Ok|Cancel, Ok, parent,name, modal, true)
{
	devicesChanged = false;

	setupDevicePage();	
	setupProgramsPage();
	
	readPrograms();
	readDevices();
	
	resize( 620, 500 );
}


K3bOptionDialog::~K3bOptionDialog()
{
}


void K3bOptionDialog::slotOk()
{
	// save and only exit if all was successful
	bool bAccept = true;
	
	bAccept = bAccept && savePrograms();

	saveDevices();
	
	if( bAccept )
		accept();
}

void K3bOptionDialog::slotApply()
{
	// save all the shit!
	savePrograms();
	saveDevices();
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

    // set the second column renameable
	m_viewPrograms->setItemsRenameable( true );
	m_viewPrograms->setRenameable( 0, false );
	m_viewPrograms->setRenameable( 1, true );

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

    item = new QListViewItem( m_viewPrograms );
    item->setText( 0, "cdrecord"  );
    item->setText( 1, config->readEntry( "cdrecord path", "/usr/bin/cdrecord" ) );

    item = new QListViewItem( m_viewPrograms );
    item->setText( 0, "readcd"  );
    item->setText( 1,  config->readEntry( "readcd path", "/usr/bin/readcd" ) );

    item = new QListViewItem( m_viewPrograms );
    item->setText( 0, "mpg123"  );
    item->setText( 1,  config->readEntry( "mpg123 path", "/usr/bin/mpg123" ) );

    item = new QListViewItem( m_viewPrograms );
    item->setText( 0, "cdda2wav"  );
    item->setText( 1,  config->readEntry( "cdda2wav path", "/usr/bin/cdda2wav" ) );

    item = new QListViewItem( m_viewPrograms );
    item->setText( 0, "cdrdao"  );
    item->setText( 1,  config->readEntry( "cdrdao path", "/usr/bin/cdrdao" ) );
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

    m_labelDevicesInfo = new QLabel( frame, "m_labelDevicesInfo" );
    m_labelDevicesInfo->setText( i18n( "Normally K3b should detect all your SCSI-devices properly. If it does not, here you can change or add devices manually." ) );
    m_labelDevicesInfo->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter | QLabel::AlignLeft ) );

    frameLayout->addMultiCellWidget( m_labelDevicesInfo, 0, 0, 0, 1 );

    m_buttonRefreshDevices = new QPushButton( frame, "m_buttonRefreshDevices" );
    m_buttonRefreshDevices->setText( i18n( "Refresh" ) );
    QToolTip::add(  m_buttonRefreshDevices, i18n( "Scan for SCSI-Devices" ) );

    frameLayout->addWidget( m_buttonRefreshDevices, 3, 1 );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    frameLayout->addItem( spacer, 3, 0 );

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
	m_viewDevicesReader->addColumn( i18n("Bus") );
    m_viewDevicesReader->addColumn( i18n( "Target" ) );
	m_viewDevicesReader->addColumn( i18n( "Lun" ) );
    m_viewDevicesReader->addColumn( i18n( "Vendor" ) );
    m_viewDevicesReader->addColumn( i18n( "Description" ) );
    m_viewDevicesReader->addColumn( i18n( "Version" ) );
    m_viewDevicesReader->addColumn( i18n( "Max Readspeed" ) );
    m_viewDevicesReader->setAllColumnsShowFocus( TRUE );
    m_groupReaderLayout->addWidget( m_viewDevicesReader );

    frameLayout->addMultiCellWidget( m_groupReader, 1, 1, 0, 1 );

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
	m_viewDevicesWriter->addColumn( i18n("Bus") );
	m_viewDevicesWriter->addColumn( i18n("Target") );
    m_viewDevicesWriter->addColumn( i18n( "Lun" ) );
    m_viewDevicesWriter->addColumn( i18n( "Vendor" ) );
    m_viewDevicesWriter->addColumn( i18n( "Description" ) );
    m_viewDevicesWriter->addColumn( i18n( "Version" ) );
    m_viewDevicesWriter->addColumn( i18n( "Max Readspeed" ) );
    m_viewDevicesWriter->addColumn( i18n( "Max Writespeed" ) );
    m_viewDevicesWriter->setAllColumnsShowFocus( TRUE );
    m_groupWriterLayout->addWidget( m_viewDevicesWriter );

    frameLayout->addMultiCellWidget( m_groupWriter, 2, 2, 0, 1 );

    m_viewDevicesReader->setItemsRenameable( true );
    m_viewDevicesReader->setRenameable( 0, false );
	m_viewDevicesReader->setRenameable( 1, true );
	m_viewDevicesReader->setRenameable( 2, true );
	m_viewDevicesReader->setRenameable( 3, true );
	m_viewDevicesReader->setRenameable( 4, true );
	m_viewDevicesReader->setRenameable( 5, true );
	m_viewDevicesReader->setRenameable( 6, true );
	m_viewDevicesReader->setRenameable( 7, true );

	m_viewDevicesWriter->setItemsRenameable( true );
	m_viewDevicesWriter->setRenameable( 0, false );
	m_viewDevicesWriter->setRenameable( 1, true );
	m_viewDevicesWriter->setRenameable( 2, true );
	m_viewDevicesWriter->setRenameable( 3, true );
	m_viewDevicesWriter->setRenameable( 4, true );
	m_viewDevicesWriter->setRenameable( 5, true );
	m_viewDevicesWriter->setRenameable( 6, true );
	m_viewDevicesWriter->setRenameable( 7, true );
	m_viewDevicesWriter->setRenameable( 8, true );
	
	// POPUP Menu
	m_menuDevices = new KActionMenu( "Devices", SmallIconSet("blockdevice"), this, "DevicesPopup" );
	m_actionNewDevice = new KAction( "New Device", 0, this, SLOT(slotNewDevice()), this );
	m_actionRemoveDevice = new KAction( "Remove Device", SmallIconSet("editdelete"), 0, this, SLOT(slotRemoveDevice()), this );
	m_menuDevices->insert( m_actionNewDevice );
	m_menuDevices->insert( new KActionSeparator(this) );
	m_menuDevices->insert( new KAction( "Refresh Devices", SmallIconSet("reload"), 0, this, SLOT(slotRefreshDevices()), this ) );
	
		
	connect( m_viewDevicesReader, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
		this, SLOT(slotDevicesPopup(QListViewItem*, const QPoint&)) );
	connect( m_viewDevicesWriter, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
		this, SLOT(slotDevicesPopup(QListViewItem*, const QPoint&)) );
	
	connect( m_buttonRefreshDevices, SIGNAL(clicked()), this, SLOT(slotRefreshDevices()) );
	
	connect( m_viewDevicesReader, SIGNAL(itemRenamed(QListViewItem*)), this, SLOT(slotDevicesChanged()) );
	connect( m_viewDevicesWriter, SIGNAL(itemRenamed(QListViewItem*)), this, SLOT(slotDevicesChanged()) );
}

void K3bOptionDialog::slotDevicesChanged()
{
	devicesChanged = true;
}


void K3bOptionDialog::readDevices()
{
	K3bDeviceManager* dm = k3bMain()->deviceManager();
	
	// add the reading devices
	m_viewDevicesReader->clear();
	K3bDevice* dev = dm->readingDevices().first();
	QListViewItem* item;
	while( dev ) {
		// add item to m_viewDevices
		item = new QListViewItem( m_viewDevicesReader );
		item->setPixmap( 0, KGlobal::instance()->iconLoader()->loadIcon( "cdrom_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );
		item->setText( 1, QString::number(dev->bus) );
		item->setText( 2, QString::number(dev->target) );
		item->setText( 3, QString::number(dev->lun) );
		item->setText( 4, dev->vendor );
		item->setText( 5, dev->description );
		item->setText( 6, dev->version );
		item->setText( 7, QString::number(dev->maxReadSpeed) );
		dev = dm->readingDevices().next();
	}
	
	// add the writing devices
	m_viewDevicesWriter->clear();
	dev = dm->burningDevices().first();
	while( dev ) {
		// add item to m_viewDevices
		item = new QListViewItem( m_viewDevicesWriter );
		item->setPixmap( 0, KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );
		item->setText( 1, QString::number(dev->bus) );
		item->setText( 2, QString::number(dev->target) );
		item->setText( 3, QString::number(dev->lun) );
		item->setText( 4, dev->vendor );
		item->setText( 5, dev->description );
		item->setText( 6, dev->version );
		item->setText( 7, QString::number(dev->maxReadSpeed) );
		item->setText( 8, QString::number(dev->maxWriteSpeed) );
		dev = dm->burningDevices().next();
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

	// clear the lists
	m_viewDevicesReader->clear();
	m_viewDevicesWriter->clear();
	
	// reread devices
	k3bMain()->deviceManager()->clear();
	k3bMain()->deviceManager()->scanbus();
	readDevices();
	
	devicesChanged = false;
}


void K3bOptionDialog::slotNewDevice()
{
	// check what kind of device
	if( m_viewDevicesReader->hasFocus() ) {
		QListViewItem* _item = new QListViewItem( m_viewDevicesReader );
		_item->setPixmap( 0, KGlobal::instance()->iconLoader()->loadIcon( "cdrom_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );
		m_viewDevicesReader->setSelected( _item, true );
	}
	else if( m_viewDevicesWriter->hasFocus() ) {
		QListViewItem* _item = new QListViewItem( m_viewDevicesWriter );
		_item->setPixmap( 0, KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );
		m_viewDevicesWriter->setSelected( _item, true );
	}
}


void K3bOptionDialog::slotRemoveDevice()
{
	// check if there is a device selected
	if( m_viewDevicesReader->hasFocus() ) {
		if( QListViewItem* _item = m_viewDevicesReader->selectedItem() ) {
			delete _item;
			devicesChanged = true;
		}
	}
	else if( m_viewDevicesWriter->hasFocus() ) {
		if( QListViewItem* _item = m_viewDevicesWriter->selectedItem() ) {
			delete _item;
			devicesChanged = true;
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
		
		QListViewItem* item = m_viewDevicesReader->firstChild();
		while( item ) {
			QStringList list;
			list.append( item->text(1) ); // device
			list.append( item->text(2) );
			list.append( item->text(3) );
			list.append( item->text(4) ); // vendor
			list.append( item->text(5) ); // description
			list.append( item->text(6) ); // version
			list.append( item->text(7) ); // max readspeed
			
			entryName = QString( "Reader%1").arg(entryNum);
			c->writeEntry( entryName, list );

			item = item->nextSibling();			
			entryNum++;
		}
		
		entryNum = 1;
		item = m_viewDevicesWriter->firstChild();
		while( item ) {
			QStringList list;
			list.append( item->text(1) ); // device
			list.append( item->text(2) );
			list.append( item->text(3) );
			list.append( item->text(4) ); // vendor
			list.append( item->text(5) ); // description
			list.append( item->text(6) ); // version
			list.append( item->text(7) ); // max readspeed
			list.append( item->text(8) ); // max writespeed
			
			entryName = QString( "Writer%1").arg(entryNum);
			c->writeEntry( entryName, list );
			
			item = item->nextSibling();
			entryNum++;
		}
		
		c->sync();
		
		// update Device Manager
		k3bMain()->deviceManager()->clear();
		k3bMain()->deviceManager()->readConfig();
	}
}


void K3bOptionDialog::slotDevicesPopup( QListViewItem* item, const QPoint& p )
{
	m_menuDevices->remove( m_actionRemoveDevice );
	if( item )
		m_menuDevices->insert( m_actionRemoveDevice, 1 );
	
	m_menuDevices->popup(p);	
}
