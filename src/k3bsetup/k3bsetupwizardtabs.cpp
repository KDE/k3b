
#include "k3bsetupwizardtabs.h"
#include "k3bsetup.h"

#include "../device/k3bdevicemanager.h"
#include "../device/k3bdevice.h"
#include "../device/k3bdevicewidget.h"
#include "../tools/k3bexternalbinmanager.h"

#include <qlabel.h>
#include <qcheckbox.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qheader.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qpixmap.h>
#include <qptrlist.h>
#include <qpixmap.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <klineeditdlg.h>
#include <kiconloader.h>
#include <klistview.h>
#include <kdialog.h>
#include <kfiledialog.h>

#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>



// == WELCOME-TAB ===========================================================================================================

WelcomeTab::WelcomeTab( int i, int o, K3bSetupWizard* wizard )
  : K3bSetupTab( i, o, i18n("Welcome to K3bSetup"), wizard )
{
  setPixmap( QPixmap(locate( "data", "k3b/pics/k3bsetup_welcome.png" )) );

  QLabel* label = new QLabel( this, "m_labelWelcome" );
  label->setText( i18n( "<h1>Welcome to K3b Setup.</h1>"
			"<p>This Wizard will help you to prepare your system for cd writing with K3b. "
			"First of all: DO NOT fear linux permissions anymore. K3b Setup takes care of nearly everything."
			" So if you are new to linux and know nothing about things like suid root don't panic!"
			" Just click next.</p>"
			"<p>If you know what is needed to write cds under linux and think you do not need a wizard like this"
			" please do not exit since there are some things special to K3b and K3b Setup will not change anything"
			" on your system before you finished the setup.</p>"
			"<p><b>Thanx for using K3b.</b></p>" ) );
  label->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter ) );

  setMainWidget( label );
}

// ========================================================================================================== WELCOME-TAB ==






// == DEVICES-TAB ===========================================================================================================

DeviceTab::DeviceTab( int i, int o, K3bSetupWizard* wizard )
  : K3bSetupTab( i, o, i18n("Setup CD Devices"), wizard )
{
  setPixmap( QPixmap(locate( "data", "k3b/pics/k3bsetup_devices.png" )) );

  QWidget* main = new QWidget( this );
  QVBoxLayout* mainLayout = new QVBoxLayout( main );
  mainLayout->setMargin( 0 );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setAutoAdd( true );

  QLabel* infoLabel = new QLabel( main );
  infoLabel->setText( i18n( "<p>K3b Setup has detected the following CD drives.</p>"
			    "<p>You can add additional devices (like /dev/cdrom) if your drive has not "
			    "been detected.</p>"
			    "<p>K3b will only detect the capabilities of generic-mmc drives correct. "
			    "For all other drives you need to set them manually." ) );
  infoLabel->setAlignment( int( QLabel::WordBreak | QLabel::AlignTop ) );
  m_deviceWidget = new K3bDeviceWidget( setup()->deviceManager(), main );
  setMainWidget( main );

  connect( m_deviceWidget, SIGNAL(refreshButtonClicked()), this, SLOT(slotRefreshButtonClicked()) );
}


void DeviceTab::readSettings()
{
  m_deviceWidget->init();
}


bool DeviceTab::saveSettings()
{
  m_deviceWidget->apply();
  if( setup()->deviceManager()->allDevices().isEmpty() )
    if( KMessageBox::warningYesNo( this, i18n("K3b Setup did not find any cd devices on your system. "
					      "K3b is not of much use without any. Do you really want to continue?"),
				   i18n("Missing CD Devices") ) == KMessageBox::No )
      return false;

  return true;
}


void DeviceTab::slotRefreshButtonClicked()
{
  // reread devices
  setup()->deviceManager()->clear();
  setup()->deviceManager()->scanbus();
  
  m_deviceWidget->init();
}

// ========================================================================================================== DEVICES-TAB ==




// == NOWRITER-TAB ===========================================================================================================

NoWriterTab::NoWriterTab( int i, int o, K3bSetupWizard* wizard )
  : K3bSetupTab( i, o, i18n("No CD Writer Found"), wizard )
{
  setPixmap( QPixmap(locate( "data", "k3b/pics/k3bsetup_devices.png" )) );

  QLabel* label = new QLabel( this, "m_labelNoWriter" );
  label->setText( i18n( "<p><b>K3b Setup did not find a cd writer on your system.</b></p>\n"
			"<p>If you have no cd writer and want to use K3b only for cd ripping everything is fine.</p>\n"
			"<p>If you are sure you have either a SCSI cd writer or enabled SCSI emulation please go back "
			"and add the device manually. If that does not work... well... please report!</p>\n"
			"<p>Otherwise you need to enable SCSI emulation for (at least) your ATAPI cd writer (although "
			"it is recommended to enable SCSI emulation for all cd drives it is not nessesary) since this "
			"is the only thing K3b Setup is not able to do for you (yet).</p>\n"
			"<p><b>How to enable SCSI emulation</b></p>\n"
			"\n"
			"<ol>\n"
			"<li>Make sure your kernel supports SCSI emulation at least as a module. If you use a standard "
			"kernel from your distribution this likely is the case. Try loading the module with <pre>modprobe "
			"ide-scsi</pre> as root. If your kernel does not support SCSI emulation you need to build your own "
			"kernel or at least a module. Sorry but that goes beyond the scope of this documentation.</li>\n"
			"<li>If your kernel supports SCSI emulation as a module it is recommended to add an entry in "
			"/etc/modules.conf so that the module is loaded automagically. Otherwise you have to load it "
			"with <pre>modprobe ide-scsi</pre> everytime you need it.\n"
			"</li>\n"
			"<li>The last step is to inform the kernel that you are about to use SCSI emulation at boottime. "
			"If you are using lilo (what is highly recommended) you need to add "
			"<pre>append = \"hdc=ide-scsi hdd=ide-scsi\"</pre> to your /etc/lilo.conf and rerun lilo as "
			"root to install the new configuration.\n"
			"After rebooting your system your cd writer is ready for action.</li>\n"
			"</ol>\n"
			"<p>If you experience problems feel free to contact me.</p>" ) );
  label->setAlignment( QLabel::WordBreak );

  setMainWidget( label );
}

bool NoWriterTab::appropriate()
{
  return setup()->deviceManager()->burningDevices().isEmpty();
}


// ========================================================================================================== NOWRITER-TAB ==




// == FSTAB-TAB ===========================================================================================================

FstabEntriesTab::FstabEntriesTab( int i, int o, K3bSetupWizard* wizard )
  : K3bSetupTab( i, o, i18n("Setup Mount Points for the CD Drives"), wizard )
{
  setPixmap( QPixmap(locate( "data", "k3b/pics/k3bsetup_fstab.png" )) );

  QWidget* main = new QWidget( this, "main" );
  QGridLayout* mainGrid = new QGridLayout( main ); 
  mainGrid->setSpacing( KDialog::spacingHint() );
  mainGrid->setMargin( 0 );

  m_labelFstab = new QLabel( main );
  m_labelFstab->setText( i18n( "<p>On Linux cd devices are mounted into the file tree. Normally only root has "
			       "permission to mount drives. K3b Setup can create entries in /etc/fstab for each of "
			       "the detected cd drives. Every user and especially K3b will then be able to mount "
			       "the devices on the given path.</p>" ) );
  m_labelFstab->setAlignment( int( QLabel::WordBreak | QLabel::AlignTop ) );

  m_viewFstab = new KListView( main, "m_viewFstab" );
  m_viewFstab->addColumn( i18n( "CD drive" ) );
  m_viewFstab->addColumn( i18n( "System device" ) );
  m_viewFstab->addColumn( i18n( "Mount point" ) );
  m_viewFstab->setAllColumnsShowFocus( true );
  m_viewFstab->setItemsRenameable( true );
  m_viewFstab->setRenameable( 0, false );
  m_viewFstab->setRenameable( 1, false );
  m_viewFstab->setRenameable( 2, true );

  m_checkFstab = new QCheckBox( i18n("Let K3b Setup create fstab entries."), main );
  m_checkFstab->setChecked( true );

  m_buttonSelectMountPoint = new QPushButton( i18n("Select Mount Point"), main );


  mainGrid->addMultiCellWidget( m_labelFstab, 0, 0, 0, 1 );
  mainGrid->addMultiCellWidget( m_viewFstab, 1, 1, 0, 1 );
  mainGrid->addWidget( m_checkFstab, 2, 0 );
  mainGrid->addWidget( m_buttonSelectMountPoint, 2, 1 );

  mainGrid->setColStretch( 0, 1 );

  QToolTip::add( m_viewFstab, i18n("Click twice to change a value") );

  setMainWidget( main );

  connect( m_buttonSelectMountPoint, SIGNAL(clicked()), this, SLOT(slotSelectMountPoint()) );
  connect( m_viewFstab, SIGNAL(itemRenamed(QListViewItem*, const QString&, int)), 
	   this, SLOT(slotMountPointChanged(QListViewItem*, const QString&, int)) );
}


void FstabEntriesTab::readSettings()
{
  m_viewFstab->clear();

  K3bDevice* dev = setup()->deviceManager()->readingDevices().first();
  int i = 0;
  while( dev != 0 ) {
    K3bDeviceViewItem* item = new K3bDeviceViewItem( dev, m_viewFstab );
    item->setPixmap( 0, SmallIcon( "cdrom_unmount" ) );
    item->setText( 0, dev->vendor() + " " + dev->description() );
    item->setText( 1, dev->ioctlDevice() );
    if( !dev->mountPoint().isEmpty() )
      item->setText( 2, dev->mountPoint() );
    else {
      if( i == 0 )
	item->setText( 2, "/cdrom" );
      else
	item->setText( 2, QString("/cdrom%1").arg(i) );

      dev->setMountPoint( item->text(2) );
    }
      
    i++;
    dev = setup()->deviceManager()->readingDevices().next();
  }

  dev = setup()->deviceManager()->burningDevices().first();
  i = 0;
  while( dev != 0 ) {
    K3bDeviceViewItem* item = new K3bDeviceViewItem( dev, m_viewFstab );
    item->setPixmap( 0, SmallIcon( "cdwriter_unmount" ) );
    item->setText( 0, dev->vendor() + " " + dev->description() );
    item->setText( 1, dev->ioctlDevice() );
    if( !dev->mountPoint().isEmpty() )
      item->setText( 2, dev->mountPoint() );
    else {
      if( i == 0 )
	item->setText( 2, "/cdrecorder" );
      else
	item->setText( 2, QString("/cdrecorder%1").arg(i) );

      dev->setMountPoint( item->text(2) );
    }
      
    i++;
    dev = setup()->deviceManager()->burningDevices().next();
  }
}


bool FstabEntriesTab::saveSettings()
{
  setup()->setCreateFstabEntries( m_checkFstab->isChecked() );
  return true;
}


void FstabEntriesTab::slotMountPointChanged( QListViewItem* item, const QString& str, int )
{
  QString newMp(str);
  K3bDeviceViewItem* deviceItem = dynamic_cast<K3bDeviceViewItem*>( item );
  if( deviceItem != 0 ) {
    // check if newMp is an absolute path
    if( !newMp.startsWith("/") )
      newMp.prepend("/");
    
    item->setText( 2, newMp );
    deviceItem->device->setMountPoint( newMp );
  }
}


void FstabEntriesTab::slotSelectMountPoint()
{
  if( m_viewFstab->selectedItem() == 0 )
    return;

  K3bDeviceViewItem* deviceItem = dynamic_cast<K3bDeviceViewItem*>( m_viewFstab->selectedItem() );
  if( deviceItem != 0 ) {
    QString newMp = KFileDialog::getExistingDirectory( deviceItem->device->mountPoint(), this,
						       i18n("Select new mount point for %1").arg(deviceItem->device->ioctlDevice()) );
    if( !newMp.isEmpty() ) {
      deviceItem->setText( 2, newMp );
      deviceItem->device->setMountPoint( newMp );
    }
  }
}

// ========================================================================================================== FSTAB-TAB ==





// == EXTBIN-TAB ===========================================================================================================

ExternalBinTab::ExternalBinTab( int i, int o, K3bSetupWizard* wizard )
  : K3bSetupTab( i, o, i18n("Setup External Applications Used by K3b"), wizard )
{
  setPixmap( QPixmap(locate( "data", "k3b/pics/k3bsetup_programs.png" )) );

  QWidget* main = new QWidget( this, "m_page5" );
  QGridLayout* mainGrid = new QGridLayout( main ); 
  mainGrid->setSpacing( KDialog::spacingHint() );
  mainGrid->setMargin( 0 );

  m_labelExternalPrograms = new QLabel( main, "m_labelExternalPrograms" );
  m_labelExternalPrograms->setText( i18n( "<p>K3b uses cdrdao, cdrecord and mkisofs to actually write the cds. "
					  "It is recommended to install these programs. K3b will run without them but major"
					  " functions (for example cd writing ;-) will be disabled.</p>"
					  "<p>K3b Setup tries to find the executables. You can change the paths manually if you"
					  " want other versions to be used or K3b Setup did not find your installation.</p>" ) );
  m_labelExternalPrograms->setAlignment( int( QLabel::WordBreak | QLabel::AlignTop ) );


  m_labelWarning = new QLabel( main );
  m_labelWarning->setPaletteForegroundColor( red );
  m_labelWarning->setAlignment( int( QLabel::WordBreak | QLabel::AlignTop ) );

  m_viewExternalPrograms = new KListView( main, "m_viewExternalPrograms" );
  m_viewExternalPrograms->addColumn( i18n( "Found" ) );
  m_viewExternalPrograms->addColumn( i18n( "Program" ) );
  m_viewExternalPrograms->addColumn( i18n( "Version" ) );
  m_viewExternalPrograms->addColumn( i18n( "Path" ) );
  m_viewExternalPrograms->setAllColumnsShowFocus( true );
  m_viewExternalPrograms->setItemsRenameable( true );
  m_viewExternalPrograms->setRenameable( 0, false );
  m_viewExternalPrograms->setRenameable( 3, true );

  m_buttonSelectExternalBin = new QPushButton( i18n("Find Program"), main );

  mainGrid->addMultiCellWidget( m_labelExternalPrograms, 0, 0, 0, 1 );
  mainGrid->addMultiCellWidget( m_labelWarning, 1, 1, 0, 1 );
  mainGrid->addMultiCellWidget( m_viewExternalPrograms, 2, 2, 0, 1 );
  mainGrid->addWidget( m_buttonSelectExternalBin, 3, 1 );

  mainGrid->setColStretch( 0, 1 );

  QToolTip::add( m_viewExternalPrograms, i18n("Click twice to change a value") );

  setMainWidget( main );

  connect( m_buttonSelectExternalBin, SIGNAL(clicked()), this, SLOT(slotSelectExternalBin()) );
  connect( m_viewExternalPrograms, SIGNAL(itemRenamed(QListViewItem*, const QString&, int)), 
	   this, SLOT(slotExternalProgramItemRenamed(QListViewItem*, const QString&, int)) );
}


void ExternalBinTab::aboutToShow()
{
  // search for programs
  setup()->externalBinManager()->checkVersions();

  readSettings();
}


void ExternalBinTab::readSettings()
{
  m_viewExternalPrograms->clear();

  KListViewItem* item = new KListViewItem( m_viewExternalPrograms );
  item->setPixmap( 0, setup()->externalBinManager()->foundBin( "cdrecord" ) ? SmallIcon( "ok" ) : SmallIcon( "stop" ) );
  item->setText( 1, "cdrecord" );
  item->setText( 2, setup()->externalBinManager()->binObject( "cdrecord" )->version );
  item->setText( 3, setup()->externalBinManager()->binPath( "cdrecord" ) );


  item = new KListViewItem( m_viewExternalPrograms, item );
  item->setPixmap( 0, setup()->externalBinManager()->foundBin( "mkisofs" ) ? SmallIcon( "ok" ) : SmallIcon( "stop" ) );
  item->setText( 1, "mkisofs" );
  item->setText( 2, setup()->externalBinManager()->binObject( "mkisofs" )->version );
  item->setText( 3, setup()->externalBinManager()->binPath( "mkisofs" ) );


  item = new KListViewItem( m_viewExternalPrograms, item );
  item->setPixmap( 0, setup()->externalBinManager()->foundBin( "cdrdao" ) ? SmallIcon( "ok" ) : SmallIcon( "stop" ) );
  item->setText( 1, "cdrdao" );
  item->setText( 2, setup()->externalBinManager()->binObject( "cdrdao" )->version );
  item->setText( 3, setup()->externalBinManager()->binPath( "cdrdao" ) );


  // check if cdrecord was found
  if( !setup()->externalBinManager()->foundBin( "cdrecord" ) ) {
    m_labelWarning->setText( i18n("<p><b><font color=\"red\">K3bSetup was not able to find cdrecord. You will not be able to write cds or get "
				  "information about your CD drives without it. It is highly recommended to install "
				  "cdrecord.</font></b></p>") );
    m_labelWarning->show();
  }
  else {
    //  m_labelWarning->setText("");
    m_labelWarning->hide();
  }
}


bool ExternalBinTab::saveSettings()
{
  return true;
}


void ExternalBinTab::slotExternalProgramItemRenamed( QListViewItem* item, const QString& newText, int )
{
  QString bin = item->text(1);
  K3bExternalBin* binO = setup()->externalBinManager()->binObject( bin );
  if( binO ) {
    binO->path = newText;
    setup()->externalBinManager()->checkVersions();
    readSettings();
  }
  else {
    kdDebug() <<  "(K3bSetupWizard) Could not find bin " << bin << endl;
  }
}


void ExternalBinTab::slotSelectExternalBin()
{
  QListViewItem* item = m_viewExternalPrograms->selectedItem();

  if( item == 0 )
    return;

  QString newPath;
  newPath = KFileDialog::getOpenFileName( QString::null, QString::null, this, 
					  i18n("Please Select %1 Executable").arg(item->text(1)) );

  if( !newPath.isEmpty() )
    slotExternalProgramItemRenamed( item, newPath, 2 );
}

// ========================================================================================================== EXTBIN-TAB ==




// == PERMISSION-TAB ===========================================================================================================

PermissionTab::PermissionTab( int i, int o, K3bSetupWizard* wizard )
  : K3bSetupTab( i, o, i18n("Setup Some Necessary Permissions for K3b"), wizard )
{
  setPixmap( QPixmap(locate( "data", "k3b/pics/k3bsetup_permissions.png" )) );


  QWidget* main = new QWidget( this, "main" );
  QGridLayout* mainGrid = new QGridLayout( main ); 
  mainGrid->setSpacing( KDialog::spacingHint() );
  mainGrid->setMargin( 0 );

  m_labelPermissions1 = new QLabel( main, "m_labelPermissions1" );
  m_labelPermissions1->setText( i18n( "<p>The external programs need to be run as root since they need write access to "
				      "the cd drives and run with higher priority. K3b also needs write access to the "
				      "cd drives (for extended functionality like detecting the capacity of a cd).</p>"
				      "<p>If you know what you are doing you can skip this and setup the permissions "
				      "for yourself. But it is recommended to let K3bSetup make the changes "
				      "(To learn more about what K3b Setup will do press <i>Details</i>).</p>"
				      "<p>Please specify the users that will use K3b. You can also specify an alternative "
				      "group name. If you do not know what that means just leave the default.</p>" ) );
  m_labelPermissions1->setAlignment( int( QLabel::WordBreak | QLabel::AlignTop ) );

  m_groupUsers = new QGroupBox( main, "m_groupUsers" );
  m_groupUsers->setTitle( i18n( "K3b Users" ) );
  m_groupUsers->setColumnLayout(0, Qt::Vertical );
  m_groupUsers->layout()->setSpacing( 0 );
  m_groupUsers->layout()->setMargin( 0 );
  QGridLayout* groupUsersLayout = new QGridLayout( m_groupUsers->layout() );
  groupUsersLayout->setSpacing( KDialog::spacingHint() );
  groupUsersLayout->layout()->setMargin( KDialog::marginHint() );
  groupUsersLayout->setAlignment( Qt::AlignTop );

  m_boxUsers = new QListBox( m_groupUsers, "m_boxUsers" );
  m_buttonRemoveUser = new QPushButton( i18n( "Remove User" ), m_groupUsers, "m_buttonRemoveUser" );
  m_buttonAddUser = new QPushButton( i18n( "Add User" ), m_groupUsers, "m_buttonAddUser" );
  QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );

  groupUsersLayout->addMultiCellWidget( m_boxUsers, 0, 2, 0, 0 );
  groupUsersLayout->addWidget( m_buttonRemoveUser, 0, 1 );
  groupUsersLayout->addWidget( m_buttonAddUser, 1, 1 );
  groupUsersLayout->addItem( spacer_2, 2, 1 );

  m_checkPermissionsDevices = new QCheckBox( main, "m_checkPermissionsDevices" );
  m_checkPermissionsDevices->setText( i18n( "Let K3b Setup do the needed changes for the devices" ) );
  m_checkPermissionsDevices->setChecked( TRUE );

  m_checkPermissionsExternalPrograms = new QCheckBox( main, "m_checkPermissionsExternalPrograms" );
  m_checkPermissionsExternalPrograms->setText( i18n( "Let K3b Setup do the needed changes for the external programs" ) );
  m_checkPermissionsExternalPrograms->setChecked( TRUE );

//   QFrame* Line1 = new QFrame( main, "Line1" );
//   Line1->setProperty( "frameShape", (int)QFrame::HLine );
//   Line1->setFrameShadow( QFrame::Sunken );
//   Line1->setFrameShape( QFrame::HLine );

  QVBoxLayout* Layout1 = new QVBoxLayout( 0, 0, 6, "Layout1"); 
  QSpacerItem* spacer_3 = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  Layout1->addItem( spacer_3 );

  m_buttonPermissionsDetails = new QPushButton( main, "m_buttonPermissionsDetails" );
  m_buttonPermissionsDetails->setText( i18n( "Details" ) );
  Layout1->addWidget( m_buttonPermissionsDetails );

  m_groupWriterGroup = new QGroupBox( main, "m_groupWriterGroup" );
  m_groupWriterGroup->setTitle( i18n( "CD Writing Group" ) );
  m_groupWriterGroup->setColumnLayout(1, Qt::Vertical );
  m_groupWriterGroup->layout()->setSpacing( KDialog::spacingHint() );
  m_groupWriterGroup->layout()->setMargin( KDialog::marginHint() );

  m_editPermissionsGroup = new QLineEdit( m_groupWriterGroup, "LineEdit1" );
  m_editPermissionsGroup->setText( "cdrecording" );


  mainGrid->addMultiCellWidget( m_labelPermissions1, 0, 0, 0, 1 );
  mainGrid->addMultiCellWidget( m_groupWriterGroup, 2, 2, 0, 1 );
  mainGrid->addMultiCellWidget( m_groupUsers, 1, 1, 0, 1 );
  mainGrid->addWidget( m_checkPermissionsDevices, 5, 0 );
  mainGrid->addWidget( m_checkPermissionsExternalPrograms, 4, 0 );
  //  mainGrid->addMultiCellWidget( Line1, 3, 3, 0, 1 );
  mainGrid->addMultiCellLayout( Layout1, 4, 5, 1, 1 );

  mainGrid->setColStretch( 0, 1 );

  setMainWidget( main );

  connect( m_buttonPermissionsDetails, SIGNAL(clicked()), this, SLOT(slotPermissionsDetails()) );
  connect( m_buttonAddUser, SIGNAL(clicked()), this, SLOT(slotAddUser()) );
  connect( m_buttonRemoveUser, SIGNAL(clicked()), this, SLOT(slotRemoveUser()) );
}

void PermissionTab::readSettings()
{
  if( !setup()->cdWritingGroup().isEmpty())
    m_editPermissionsGroup->setText( setup()->cdWritingGroup() );
  else
    m_editPermissionsGroup->setText( "cdrecording" );

  m_boxUsers->clear();
  for ( QStringList::ConstIterator it = setup()->users().begin(); it != setup()->users().end(); ++it ) {
    m_boxUsers->insertItem( *it );
  }
}


bool PermissionTab::saveSettings()
{
  setup()->setCdWritingGroup( m_editPermissionsGroup->text() );

  setup()->clearUsers();

  for( uint i = 0; i < m_boxUsers->count(); ++i ) {
    setup()->addUser( m_boxUsers->item(i)->text() );
  }

  setup()->setApplyDevicePermissions( m_checkPermissionsDevices->isChecked() );
  setup()->setApplyExternalBinPermissions( m_checkPermissionsExternalPrograms->isChecked() );

  if( ( m_checkPermissionsExternalPrograms->isChecked() || m_checkPermissionsDevices->isChecked() )
      && m_editPermissionsGroup->text().isEmpty() ) {
    KMessageBox::error( this, i18n("Please specify a CD writing group."), i18n("Missing Group Name") );
    return false;
  }

  if( m_boxUsers->count() == 0 && m_checkPermissionsExternalPrograms->isChecked() )
    if( KMessageBox::warningYesNo( this, i18n("You specified no users. Only root will be able to write CDs. Continue?") )
	== KMessageBox::No )
      return false;

  if( !m_checkPermissionsDevices->isChecked() && !setup()->deviceManager()->allDevices().isEmpty() )
    if( KMessageBox::warningYesNo( this, i18n("You chose not to change device permissions. K3b will not be able "
					       "to provide extended features like detection of writer capabilities. "
					      "Continue?") ) 
	== KMessageBox::No )
      return false;

  return true;
}


void PermissionTab::slotAddUser()
{
  QString user;
  QString text = i18n("Please enter a user name");
  bool ok = true;
  bool validUser = false;
  while( ok && !validUser ) {
    user = KLineEditDlg::getText( text, QString::null, &ok, this );
    if( ok && !user.isEmpty() )
      validUser = ( getpwnam( user.local8Bit() ) != 0 );
    else
      validUser = false;
    text = i18n("No valid user name. Please enter a user name");
  }

  if( ok )
    m_boxUsers->insertItem( user );
}


void PermissionTab::slotRemoveUser()
{
  int item = m_boxUsers->currentItem();
  if( item != -1 )
    m_boxUsers->removeItem( item );
}


void PermissionTab::slotPermissionsDetails()
{
  QString info = i18n("<p>These are the permission changes K3b Setup will perform and why:</p>\n"
		      "<table>\n"
		      "<tr>\n"
		      " <td>change permission for cdrecord, mksiofs, and cdrdao to 4710</td>\n"
		      " <td>cdrecord and cdrdao need write access to all the cd drives and all three programs run with higher "
		      "priority. That is why they need to be run as root.</td>\n"
		      "</tr>\n"
		      "<tr>\n"
		      " <td>add cdrecord, mkisofs, and cdrdao to group 'cdrecording'</td>\n"
		      " <td>not everybody shall be allowed to execute these programs since running a program as suid root "
		      "always is a security risk.</td>\n"
		      "</tr>\n"
		      "<tr>\n"
		      " <td>add the selected users to group 'cdrecording'</td>\n"
		      " <td>these users will be able to run cdrecord, mkisofs, and cdrdao</td>\n"
		      "</tr>\n"
		      "<tr>\n"
		      " <td>change permission for all detected SCSI drives to 660</td>\n"
		      " <td>K3b needs write access to the SCSI cd drives in order to detect writing speed and things like that</td>\n"
		      "</tr>\n"
		      "<tr>\n"
		      " <td>change permission for all detected ATAPI drives to 620</td>\n"
		      " <td>K3b needs only read access to the ATAPI drives since it is not (yet) able to detect speed and stuff "
		      "for ATAPI devices.</td>\n"
		      "</tr>\n"
		      "<tr>\n"
		      " <td>add all detected devices to group 'cdrecording'</td>\n"
		      " <td>since write access to devices is always a security risk (although one can not do much bad with writing "
		      "to a cdrom device) only the selcted users will be able to access the drives</td>\n"
		      "</tr>\n"
		      "</table>");

  KMessageBox::information( this, info, i18n("K3b Setup Permission Details") );
}


// ========================================================================================================== PERMISSION-TAB ==




// == FINISH-TAB ===========================================================================================================

FinishTab::FinishTab( int i, int o, K3bSetupWizard* wizard )
  : K3bSetupTab( i, o, i18n("Save Your Settings"), wizard )
{
  setPixmap( QPixmap(locate( "data", "k3b/pics/k3bsetup_finish.png" )) );

  QWidget* main = new QWidget( this );
  QGridLayout* mainGrid = new QGridLayout( main );
  mainGrid->setSpacing( KDialog::spacingHint() );
  mainGrid->setMargin( 0 );

  QLabel* finishedLabel = new QLabel( main, "finishedLabel" );
  finishedLabel->setText( i18n("<h1>Congratulations.</h1>"
			       "<p>You completed the K3b Setup. Just press the Finish button to save your changes "
			       "and then enjoy the new ease of cd writing with Linux/KDE.</p>") );
  finishedLabel->setAlignment( int( QLabel::WordBreak | QLabel::AlignTop ) );

  QGroupBox* groupChanges = new QGroupBox( i18n("Progress"), main );
  groupChanges->setColumnLayout( 1, Qt::Vertical );
  groupChanges->layout()->setSpacing( KDialog::spacingHint() );
  groupChanges->layout()->setMargin( KDialog::marginHint() );

  m_viewChanges = new KListView( groupChanges );
  m_viewChanges->addColumn( i18n("Setting") );
  m_viewChanges->addColumn( i18n("Value") );
  m_viewChanges->setRootIsDecorated( true );
  m_viewChanges->setSorting( -1 );
  m_viewChanges->header()->hide();

  mainGrid->addWidget( finishedLabel, 0, 0 );
  mainGrid->addWidget( groupChanges, 1, 0 );
  mainGrid->setRowStretch( 1, 1 );

  setMainWidget( main );

  connect( setup(), SIGNAL(writingSettings()), m_viewChanges, SLOT(clear()) );
  connect( setup(), SIGNAL(writingSetting(const QString&)), this, SLOT(slotWritingSetting(const QString&)) );
  connect( setup(), SIGNAL(error(const QString&)), this, SLOT(slotError(const QString&)) );
  connect( setup(), SIGNAL(settingWritten(bool, const QString&)), this, SLOT(slotSettingWritten(bool, const QString&)) );

  m_currentInfoViewItem = 0;
}


void FinishTab::slotWritingSetting( const QString& s )
{
  // KListView adds on the top per default and that's not what we want
  if( m_currentInfoViewItem )
    m_currentInfoViewItem = new KListViewItem( m_viewChanges, m_currentInfoViewItem, s );
  else
    m_currentInfoViewItem = new KListViewItem( m_viewChanges, s );
}


void FinishTab::slotSettingWritten( bool success, const QString& comment )
{
  if( m_currentInfoViewItem ) {
    m_currentInfoViewItem->setPixmap( 1, (success ? SmallIcon("apply") : SmallIcon("cancel")) );
    m_currentInfoViewItem->setText( 2, comment );
  }
}


void FinishTab::slotError( const QString& comment )
{
  KListViewItem* item = 0;

  if( m_currentInfoViewItem ) {
    item = new KListViewItem( m_currentInfoViewItem, i18n("Error") + ": ", comment );
    m_currentInfoViewItem->setOpen( true );
  }
  else {
    item = new KListViewItem( m_viewChanges, i18n("Error") + ": ", comment );
  }

  item->setPixmap( 0, SmallIcon("cancel") );
}

// ========================================================================================================== FINISH-TAB ==





K3bDeviceViewItem::K3bDeviceViewItem( K3bDevice* dev, KListView* parent, const QString& str )
  : KListViewItem( parent, str )
{
  device = dev;
}


K3bDeviceViewItem::K3bDeviceViewItem( K3bDevice* dev, KListViewItem* parent, const QString& str )
  : KListViewItem( parent, str )
{
  device = dev;
}


K3bDeviceViewItem::K3bDeviceViewItem( K3bDevice* dev, KListViewItem* parent, KListViewItem* prev, const QString& str )
  : KListViewItem( parent, prev, str )
{
  device = dev;
}


#include "k3bsetupwizardtabs.moc"
