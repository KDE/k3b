/*
 *
 * $Id$
 * Copyright (C) 2003-2004 Sebastian Trueg <trueg@k3b.org>
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

#include <qlayout.h>
#include <qmap.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qscrollview.h>
#include <qtimer.h>

#include <klocale.h>
#include <kglobal.h>
#include <kgenericfactory.h>
#include <klistview.h>
#include <keditlistbox.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kdeversion.h>

#include "k3bsetup2.h"
#include "base_k3bsetup2.h"

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bexternalbinmanager.h>
#include <k3bdefaultexternalprograms.h>
#include <k3bglobals.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>



class K3bSetup2::Private
{
public:
  K3bDevice::DeviceManager* deviceManager;
  K3bExternalBinManager* externalBinManager;

  bool changesNeeded;

  QMap<QCheckListItem*, K3bDevice::Device*> listDeviceMap;
  QMap<K3bDevice::Device*, QCheckListItem*> deviceListMap;

  QMap<QCheckListItem*, K3bExternalBin*> listBinMap;
  QMap<K3bExternalBin*, QCheckListItem*> binListMap;

  KConfig* config;
};



K3bSetup2::K3bSetup2( QWidget *parent, const char *, const QStringList& )
  : KCModule( parent, "k3bsetup" )
{
  d = new Private();
  d->config = new KConfig( "k3bsetup2rc" );

  m_aboutData = new KAboutData(I18N_NOOP("k3bsetup2"),
			       I18N_NOOP("K3bSetup 2"),
			       0, 0, KAboutData::License_GPL,
			       I18N_NOOP("(C) 2003-2004 Sebastian Trueg"));
  m_aboutData->addAuthor("Sebastian Trueg", 0, "trueg@k3b.org");

  setButtons( KCModule::Apply|KCModule::Cancel|KCModule::Ok|KCModule::Default );

  QHBoxLayout* box = new QHBoxLayout( this );
  box->setAutoAdd(true);
  box->setMargin(0);
  box->setSpacing( KDialog::spacingHint() );

  QScrollView* labelScroll = new QScrollView( this );
  QLabel* label = new QLabel( i18n("<p>This simple setup assistant is able to set the permissions needed by K3b in order to "
				   "burn CDs and DVDs. "
				   "<p>It does not take things like devfs or resmgr into account. In most cases this is not a "
				   "problem but on some systems the permissions may be altered the next time you login or restart "
				   "your computer. In those cases it is best to consult the distribution documentation."
				   "<p><b>Caution:</b> Although K3bSetup 2 should not be able "
				   "to mess up your system no guarantee can be given."), labelScroll->viewport() );
  label->setMargin( 5 );
  //  label->setBackgroundMode( PaletteBase );
  labelScroll->addChild( label );
  labelScroll->viewport()->setPaletteBackgroundPixmap( locate( "data", "k3b/pics/crystal/k3b_3d_logo.png" ) );
  label->setPaletteBackgroundPixmap( locate( "data", "k3b/pics/crystal/k3b_3d_logo.png" ) );
  labelScroll->setFixedWidth( 200 );
  label->setFixedWidth( labelScroll->contentsRect().width() );
  label->setFixedHeight( label->heightForWidth( labelScroll->contentsRect().width() ) );

  w = new base_K3bSetup2( this );
  //  w->m_pixLabel->setPixmap( locate( "data", "k3b/pics/k3bsetup_1.png" ) );

  // TODO: enable this and let root specify users
  w->m_editUsers->hide();
  w->textLabel2->hide();


  connect( w->m_checkUseBurningGroup, SIGNAL(toggled(bool)),
	   this, SLOT(updateViews()) );
  connect( w->m_editBurningGroup, SIGNAL(textChanged(const QString&)),
	   this, SLOT(updateViews()) );
  connect( w->m_editSearchPath, SIGNAL(changed()),
	   this, SLOT(slotSearchPrograms()) );
  connect( w->m_buttonAddDevice, SIGNAL(clicked()),
	   this, SLOT(slotAddDevice()) );


  d->externalBinManager = new K3bExternalBinManager( this );
  d->deviceManager = new K3bDevice::DeviceManager( this );

  // these are the only programs that need special permissions
  //  d->externalBinManager->addProgram( new K3bReadcdProgram() );
  d->externalBinManager->addProgram( new K3bCdrdaoProgram() );
  d->externalBinManager->addProgram( new K3bCdrecordProgram(false) );

  d->externalBinManager->search();
  d->deviceManager->scanBus();

  load();

  //
  // This is a hack to work around a kcm bug which makes the faulty assumption that
  // every module starts without anything to apply
  //
  QTimer::singleShot( 0, this, SLOT(updateViews()) );

  if( getuid() != 0 || !d->config->checkConfigFilesWritable( true ) )
    makeReadOnly();
}


K3bSetup2::~K3bSetup2()
{
  delete d->config;
  delete d;
  delete m_aboutData;
}


void K3bSetup2::updateViews()
{
  d->changesNeeded = false;

  updatePrograms();
  updateDevices();

  emit changed( ( getuid() != 0 ) ? false : d->changesNeeded );
}


void K3bSetup2::updatePrograms()
{
  // first save which were checked
  QMap<K3bExternalBin*, bool> checkMap;
  QListViewItemIterator listIt( w->m_viewPrograms );
  for( ; listIt.current(); ++listIt )
    checkMap.insert( d->listBinMap[(QCheckListItem*)*listIt], ((QCheckListItem*)*listIt)->isOn() );

  w->m_viewPrograms->clear();
  d->binListMap.clear();
  d->listBinMap.clear();

  // load programs
  const QMap<QString, K3bExternalProgram*>& map = d->externalBinManager->programs();
  for( QMap<QString, K3bExternalProgram*>::const_iterator it = map.begin(); it != map.end(); ++it ) {
    K3bExternalProgram* p = it.data();

    QPtrListIterator<K3bExternalBin> binIt( p->bins() );
    for( ; binIt.current(); ++binIt ) {
      K3bExternalBin* b = *binIt;

      QFileInfo fi( b->path );
      // we need the uid bit which is not supported by QFileInfo
      struct stat s;
      if( ::stat( QFile::encodeName(b->path), &s ) ) {
	kdDebug() << "(K3bSetup2) unable to stat " << b->path << endl;
      }
      else {
	// create a checkviewitem
	QCheckListItem* bi = new QCheckListItem( w->m_viewPrograms, b->name(), QCheckListItem::CheckBox );
	bi->setText( 1, b->version );
	bi->setText( 2, b->path );

	d->listBinMap.insert( bi, b );
	d->binListMap.insert( b, bi );

	// check the item on first insertion or if it was checked before
	bi->setOn( checkMap.contains(b) ? checkMap[b] : true );

	int perm = s.st_mode & 0007777;

	//
	// Since kernel 2.6.8 cdrecord/cdrdao is not able to use the SCSI subsystem when running suid root anymore
	// So for now (until cdrecord has been properly patched) we ignore the suid root issue with kernel >= 2.6.8
	//

	QString wantedGroup("root");
	if( w->m_checkUseBurningGroup->isChecked() )
	  wantedGroup = burningGroup();

	int wantedPerm = 0;
	if( K3b::simpleKernelVersion() >= K3bVersion( 2, 6, 8 ) ) {
	  if( w->m_checkUseBurningGroup->isChecked() )
	    wantedPerm = 0000750;
	  else
	    wantedPerm = 0000755;
	}
	else {
	  if( w->m_checkUseBurningGroup->isChecked() )
	    wantedPerm = 0004710;
	  else
	    wantedPerm = 0004711;
	}

	bi->setText( 3, QString::number( perm, 8 ).rightJustify( 4, '0' ) + " " + fi.owner() + "." + fi.group() );
	if( perm != wantedPerm ||
	    fi.owner() != "root" ||
	    fi.group() != wantedGroup ) {
	  bi->setText( 4, QString("%1 root.%2").arg(wantedPerm,0,8).arg(wantedGroup) );
	  if( bi->isOn() )
	    d->changesNeeded = true;
	}
	else
	  bi->setText( 4, i18n("no change") );
      }
    }
  }
}


void K3bSetup2::updateDevices()
{
  // first save which were checked
  QMap<K3bDevice::Device*, bool> checkMap;
  QListViewItemIterator listIt( w->m_viewDevices );
  for( ; listIt.current(); ++listIt )
    checkMap.insert( d->listDeviceMap[(QCheckListItem*)*listIt], ((QCheckListItem*)*listIt)->isOn() );

  w->m_viewDevices->clear();
  d->listDeviceMap.clear();
  d->deviceListMap.clear();

  QPtrListIterator<K3bDevice::Device> it( d->deviceManager->allDevices() );
  for( ; it.current(); ++it ) {
    K3bDevice::Device* device = *it;

    QFileInfo fi( device->blockDeviceName() );
    struct stat s;
    if( ::stat( QFile::encodeName(device->blockDeviceName()), &s ) ) {
      kdDebug() << "(K3bSetup2) unable to stat " << device->blockDeviceName() << endl;
    }
    else {
      // create a checkviewitem
      QCheckListItem* bi = new QCheckListItem( w->m_viewDevices,
					       device->vendor() + " " + device->description(),
					       QCheckListItem::CheckBox );

      d->listDeviceMap.insert( bi, device );
      d->deviceListMap.insert( device, bi );

      // check the item on first insertion or if it was checked before
      bi->setOn( checkMap.contains(device) ? checkMap[device] : true );

      bi->setText( 1, device->blockDeviceName() );

      int perm = s.st_mode & 0000777;

      bi->setText( 2, QString::number( perm, 8 ).rightJustify( 3, '0' ) + " " + fi.owner() + "." + fi.group() );
      if( w->m_checkUseBurningGroup->isChecked() ) {
	// we ignore the device's owner here
	if( perm != 0000660 ||
	    fi.group() != burningGroup() ) {
	  bi->setText( 3, "660 " + fi.owner() + "." + burningGroup() );
	  if( bi->isOn() )
	    d->changesNeeded = true;
	}
	else
	  bi->setText( 3, i18n("no change") );
      }
      else {
	// we ignore the device's owner and group here
	if( perm != 0000666 ) {
	  bi->setText( 3, "666 " + fi.owner() + "." + fi.group()  );
	  if( bi->isOn() )
	    d->changesNeeded = true;
	}
	else
	  bi->setText( 3, i18n("no change") );
      }
    }
  }
}


void K3bSetup2::load()
{
  if( d->config->hasGroup("External Programs") ) {
    d->config->setGroup( "External Programs" );
    d->externalBinManager->readConfig( d->config );
  }
  if( d->config->hasGroup("Devices") ) {
    d->config->setGroup( "Devices" );
    d->deviceManager->readConfig( d->config );
  }

  d->config->setGroup( "General Settings" );
  w->m_checkUseBurningGroup->setChecked( d->config->readBoolEntry( "use burning group", false ) );
  w->m_editBurningGroup->setText( d->config->readEntry( "burning group", "burning" ) );


  // load search path
  w->m_editSearchPath->clear();
  w->m_editSearchPath->insertStringList( d->externalBinManager->searchPath() );

  updateViews();
}


void K3bSetup2::defaults()
{
  w->m_checkUseBurningGroup->setChecked(false);
  w->m_editBurningGroup->setText( "burning" );

  //
  // This is a hack to work around a kcm bug which makes the faulty assumption that
  // every module defaults to a state where nothing is to be applied
  //
  QTimer::singleShot( 0, this, SLOT(updateViews()) );
}


void K3bSetup2::save()
{
  d->config->setGroup( "General Settings" );
  d->config->writeEntry( "use burning group", w->m_checkUseBurningGroup->isChecked() );
  d->config->writeEntry( "burning group", burningGroup() );
  d->config->setGroup( "External Programs");
  d->externalBinManager->saveConfig( d->config );
  d->config->setGroup( "Devices");
  d->deviceManager->saveConfig( d->config );


  bool success = true;

  struct group* g = 0;
  if( w->m_checkUseBurningGroup->isChecked() ) {
    // TODO: create the group if it's not there
    g = getgrnam( burningGroup().local8Bit() );
    if( !g ) {
      KMessageBox::error( this, i18n("There is no group %1.").arg(burningGroup()) );
      return;
    }
  }


  // save the device permissions
  QListViewItemIterator listIt( w->m_viewDevices );
  for( ; listIt.current(); ++listIt ) {

    QCheckListItem* checkItem = (QCheckListItem*)listIt.current();

    if( checkItem->isOn() ) {
      K3bDevice::Device* dev = d->listDeviceMap[checkItem];

      if( w->m_checkUseBurningGroup->isChecked() ) {
	if( ::chmod( QFile::encodeName(dev->blockDeviceName()), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP ) )
	  success = false;

	if( ::chown( QFile::encodeName(dev->blockDeviceName()), (gid_t)-1, g->gr_gid ) )
	  success = false;

	if( dev->interfaceType() == K3bDevice::SCSI && QFile::exists( dev->genericDevice() ) )
	  if( ::chmod( QFile::encodeName(dev->genericDevice()), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP ) )
	    success = false;
      }
      else {
	if( ::chmod( QFile::encodeName(dev->blockDeviceName()), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH ) )
	  success = false;

	if( dev->interfaceType() == K3bDevice::SCSI && QFile::exists( dev->genericDevice() ) )
	  if( ::chmod( QFile::encodeName(dev->genericDevice()), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH ) )
	    success = false;
      }
    }
  }


  // save the program permissions
  listIt = QListViewItemIterator( w->m_viewPrograms );
  for( ; listIt.current(); ++listIt ) {

    QCheckListItem* checkItem = (QCheckListItem*)listIt.current();

    if( checkItem->isOn() ) {

      K3bExternalBin* bin = d->listBinMap[checkItem];

      //
      // Since kernel 2.6.8 cdrecord/cdrdao is not able to use the SCSI subsystem when running suid root anymore
      // So for now (until cdrecord has been properly patched) we ignore the suid root issue with kernel >= 2.6.8
      //

      if( w->m_checkUseBurningGroup->isChecked() ) {
	if( ::chown( QFile::encodeName(bin->path), (gid_t)0, g->gr_gid ) )
	  success = false;

	int perm = 0;
	if( K3b::simpleKernelVersion() >= K3bVersion( 2, 6, 8 ) )
	  perm = S_IRWXU|S_IXGRP|S_IRGRP;
	else
	  perm = S_ISUID|S_IRWXU|S_IXGRP;

	if( ::chmod( QFile::encodeName(bin->path), perm ) )
	  success = false;
      }
      else {
	if( ::chown( QFile::encodeName(bin->path), 0, 0 ) )
	  success = false;

	int perm = 0;
	if( K3b::simpleKernelVersion() >= K3bVersion( 2, 6, 8 ) )
	  perm = S_IRWXU|S_IXGRP|S_IRGRP|S_IXOTH|S_IROTH;
	else
	  perm = S_ISUID|S_IRWXU|S_IXGRP|S_IXOTH;

	if( ::chmod( QFile::encodeName(bin->path), perm ) )
	  success = false;
      }
    }
  }


  if( success )
    KMessageBox::information( this, i18n("Successfully updated all permissions.") );
  else {
    if( getuid() )
      KMessageBox::error( this, i18n("Could not update all permissions. You should run K3bSetup 2 as root.") );
    else
      KMessageBox::error( this, i18n("Could not update all permissions.") );
  }

  // WE MAY USE "newgrp -" to reinitialize the environment if we add users to a group

  updateViews();
}


QString K3bSetup2::quickHelp() const
{
  return i18n("<h2>K3bSetup 2</h2>"
	      "<p>This simple setup assistant is able to set the permissions needed by K3b in order to "
	      "burn CDs and DVDs."
	      "<p>It does not take into account devfs or resmgr, or similar. In most cases this is not a "
	      "problem, but on some systems the permissions may be altered the next time you login or restart "
	      "your computer. In these cases it is best to consult the distribution's documentation."
	      "<p>The important task that K3bSetup 2 performs is grant write access to the CD and DVD devices."
	      "<p><b>Caution:</b> Although K3bSetup 2 should not be able "
	      "to damage your system, no guarantee can be given.");
}


QString K3bSetup2::burningGroup() const
{
  QString g = w->m_editBurningGroup->text();
  return g.isEmpty() ? QString("burning") : g;
}


void K3bSetup2::slotSearchPrograms()
{
  d->externalBinManager->setSearchPath( w->m_editSearchPath->items() );
  d->externalBinManager->search();
  updatePrograms();

  emit changed( d->changesNeeded );
}


void K3bSetup2::slotAddDevice()
{
  bool ok;
  QString newDevicename = KInputDialog::getText( i18n("Location of New Drive"),
                                                 i18n("Please enter the device name where K3b should search\n"
						 "for a new drive (example: /dev/mebecdrom):"),
						 "/dev/", &ok, this );

  if( ok ) {
    if( d->deviceManager->addDevice( newDevicename ) ) {
      updateDevices();

      emit changed( d->changesNeeded );
    }
    else
      KMessageBox::error( this, i18n("Could not find an additional device at\n%1").arg(newDevicename),
			  i18n("Error"), false );
  }
}

void K3bSetup2::makeReadOnly()
{
    w->m_checkUseBurningGroup->setEnabled( false );
    w->m_editBurningGroup->setEnabled( false );
    w->m_editUsers->setEnabled( false );
    w->m_viewDevices->setEnabled( false );
    w->m_buttonAddDevice->setEnabled( false );
    w->m_viewPrograms->setEnabled( false );
    w->m_editSearchPath->setEnabled( false );
}


typedef KGenericFactory<K3bSetup2, QWidget> K3bSetup2Factory;
K_EXPORT_COMPONENT_FACTORY( kcm_k3bsetup2, K3bSetup2Factory("k3bsetup") )


#include "k3bsetup2.moc"
