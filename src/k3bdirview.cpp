/***************************************************************************
                          k3bdirview.cpp  -  description
                             -------------------
    begin                : Mon Mar 26 2001
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

#include "k3bdirview.h"

#include <unistd.h>
// QT-includes
#include <qdir.h>
#include <qlistview.h>
#include <qstring.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qsplitter.h>
#include <qdragobject.h>
#include <qstrlist.h>
#include <qheader.h>
#include <qsplitter.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qiconset.h>
#include <qvaluelist.h>
#include <qlabel.h>
#include <qwidgetstack.h>
#include <qscrollview.h>

// KDE-includes
#include <kmimetype.h>
#include <kcursor.h>
#include <kfiledetailview.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <kurl.h>
#include <klocale.h>
#include <kautomount.h>
#include <kstddirs.h>
#include <kio/file.h>
#include <kio/global.h>
#include <krun.h>
#include <kprocess.h>
#include <kio/job.h>
#include <kcombobox.h>
#include <kfiletreeview.h>
#include <kdialog.h>
#include <kmessagebox.h>
#include <kstdaction.h>

#include "rip/k3bcdview.h"
#include "k3bfileview.h"
#include "device/k3bdevicemanager.h"
#include "device/k3bdevice.h"
#include "k3b.h"
#include "rip/k3bfilmview.h"
#include "k3bfiletreeview.h"
#include "cdinfo/k3bdiskinfodetector.h"
#include "cdinfo/k3bdiskinfoview.h"



K3bDirView::K3bDirView(QWidget *parent, const char *name )
  : QVBox(parent, name), m_bViewDiskInfo(false), m_lastDevice(0)
{
  m_diskInfoDetector = new K3bDiskInfoDetector( this );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bDiskInfo&)),
	   k3bMain(), SLOT(endBusy()) );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bDiskInfo&)),
	   this, SLOT(slotDiskInfoReady(const K3bDiskInfo&)) );

  //  KToolBar* toolBar = new KToolBar( this, "dirviewtoolbar" );

  m_mainSplitter = new QSplitter( this );
  m_fileTreeView = new K3bFileTreeView( m_mainSplitter );
  m_fileTreeView->header()->hide();

  m_viewStack    = new QWidgetStack( m_mainSplitter );
  m_fileView     = new K3bFileView(m_viewStack, "fileView");
  m_cdView       = new K3bCdView(m_viewStack, "cdview");
  m_filmView     = new K3bFilmView(m_viewStack, "filmview");
  m_infoView     = new K3bDiskInfoView(m_viewStack, "infoView");

  m_noViewView = new QWidget( m_viewStack );
  QHBoxLayout* noViewLayout = new QHBoxLayout( m_noViewView );
  noViewLayout->setAutoAdd( true );
  m_noViewView->setPaletteBackgroundColor( QColor(201, 208, 255) );
  QLabel* penguinLabel = new QLabel( m_noViewView );
  penguinLabel->setPixmap( QPixmap(locate( "data", "k3b/pics/k3b_probing_cd.png" )) );
  penguinLabel->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored ) );
  m_noViewLabel = new QLabel( i18n("K3b is trying to retrieve information about the inserted disk."), m_noViewView );
  m_noViewLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter | Qt::WordBreak );
  m_noViewLabel->setPaletteForegroundColor( Qt::white );


  m_viewStack->raiseWidget( m_fileView );

  m_fileTreeView->addDefaultBranches();

  // split
  QValueList<int> sizes = m_mainSplitter->sizes();
  int all = sizes[0] + sizes[1];
  sizes[1] = all*2/3;
  sizes[0] = all - sizes[1];
  m_mainSplitter->setSizes( sizes );


  m_actionCollection = new KActionCollection( this );

  // add some actions to the toolbar
  //  m_fileView->actionCollection()->action("up")->plug( toolBar );
//   KStdAction::home( this, SLOT(home()), m_actionCollection )->plug( toolBar );
//   KStdAction::redisplay( this, SLOT(reload()), m_actionCollection )->plug( toolBar );
//   toolBar->insertSeparator();

  // add a url combobox to the toolbar
//   (void)new QLabel( i18n("Location"), toolBar );
//   m_urlCombo = new KComboBox( true, toolBar );
//   m_urlCombo->setEditText( QDir::homeDirPath() );
//   m_urlCombo->setDuplicatesEnabled( false );


  m_devicePopupMenu = new KActionMenu( this );
  KAction* actionDiskInfo = new KAction( i18n("&Disk info"), "info", 0, this, SLOT(slotShowDiskInfo()), 
					 this, "disk_info");
  KAction* actionUnmount = new KAction( i18n("&Unmount"), "cdrom_unmount", 0, this, SLOT(slotUnmountDisk()), 
					this, "disk_unmount");
  KAction* actionEject = new KAction( i18n("&Eject"), "", 0, this, SLOT(slotEjectDisk()), 
					this, "disk_eject");

  m_devicePopupMenu->insert( actionDiskInfo );
  m_devicePopupMenu->insert( new KActionSeparator( this ) );
  m_devicePopupMenu->insert( actionUnmount );
  m_devicePopupMenu->insert( actionEject );


//   connect( m_urlCombo, SIGNAL(returnPressed(const QString&)), this, SLOT(slotDirActivated(const QString&)) );
//   connect( m_urlCombo, SIGNAL(activated(const QString&)), this, SLOT(slotDirActivated(const QString&)) );

  connect( m_fileTreeView, SIGNAL(urlExecuted(const KURL&)), this, SLOT(slotDirActivated(const KURL&)) );
  connect( m_fileTreeView, SIGNAL(deviceExecuted(K3bDevice*)), this, SLOT(slotDetectDiskInfo(K3bDevice*)) );
  connect( m_fileTreeView, SIGNAL(contextMenu(K3bDevice*, const QPoint&)),
	   this, SLOT(slotFileTreeContextMenu(K3bDevice*, const QPoint&)) );
  connect( m_fileView, SIGNAL(urlEntered(const KURL&)), m_fileTreeView, SLOT(followUrl(const KURL&)) );
  connect( m_fileView, SIGNAL(urlEntered(const KURL&)), this, SLOT(slotUpdateURLCombo(const KURL&)) );
}

K3bDirView::~K3bDirView()
{
}

void K3bDirView::setupFinalize( K3bDeviceManager *dm )
{
  m_fileTreeView->addCdDeviceBranches( dm );
}


void K3bDirView::slotDetectDiskInfo( K3bDevice* dev )
{
//   KDialog* infoDialog = new KDialog( this, "waitForDiskInfoDialog", true, WDestructiveClose );
//   infoDialog->setCaption( i18n("Please wait...") );
//   QHBoxLayout* infoLayout = new QHBoxLayout( infoDialog );
//   infoLayout->setSpacing( KDialog::spacingHint() );
//   infoLayout->setMargin( KDialog::marginHint() );
//   infoLayout->setAutoAdd( true );
//   QLabel* picLabel = new QLabel( infoDialog );
//   picLabel->setPixmap( DesktopIcon( "cdwriter_unmount" ) );
//   QLabel* infoLabel = new QLabel( i18n("K3b is trying to fetch information about the inserted disk."), infoDialog );

  m_viewStack->raiseWidget( m_noViewView );
  k3bMain()->showBusyInfo( i18n("Trying to fetch information about the inserted disk.") );

  m_diskInfoDetector->detect( dev );
//   connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bDiskInfo&)),
// 	   infoDialog, SLOT(close()) );

//   infoDialog->show();
}


void K3bDirView::slotDiskInfoReady( const K3bDiskInfo& info )
{
  if( m_bViewDiskInfo ||info.empty || info.noDisk ) {
    // show cd info
    m_viewStack->raiseWidget( m_infoView );
    m_infoView->displayInfo( info );
    m_bViewDiskInfo = false;
  }
  else if( info.tocType == K3bDiskInfo::DVD  ) {
    m_filmView->setDevice( info.device );
    m_viewStack->raiseWidget( m_filmView );
    m_filmView->reload();
  }
  else if( info.tocType == K3bDiskInfo::DATA  ) {
    slotMountDevice( info.device );
  }
  else {
    // check for MIXED_MODE and ask
    bool mount = false;
    if( info.tocType == K3bDiskInfo::MIXED  ) {
      mount = ( KMessageBox::questionYesNo( this, 
					    i18n("Found Mixed-mode CD. Do you want K3b to mount the data part or show all the tracks?"),
					    i18n("Mixed-mode CD"), 
					    i18n("Mount cd"),
					    i18n("Show tracks") ) == KMessageBox::Yes );
    }

    if( mount )
      slotMountDevice( info.device );
    else {
      m_viewStack->raiseWidget( m_cdView );
      m_cdView->showCdView( info );
    }
  }
}

void K3bDirView::slotMountDevice( K3bDevice* device )
{
  const QString& mountPoint = device->mountPoint();

  if( !mountPoint.isEmpty() ){
    if( KIO::findDeviceMountPoint( device->ioctlDevice() ).isEmpty() )
      connect( KIO::mount( true, "autofs", device->ioctlDevice(), mountPoint, true ), SIGNAL(result(KIO::Job*)),
	       this, SLOT(reload()) );
    
    KURL url = KURL( mountPoint );
    slotDirActivated( url );
  }
  else {
    KMessageBox::error( this, i18n("K3b could not mount %1. Please run K3bSetup.").arg(device->ioctlDevice()),
			i18n("I/O error") );
  }
}


void K3bDirView::slotFileTreeContextMenu( K3bDevice* dev, const QPoint& p )
{
  m_lastDevice = dev;
  m_devicePopupMenu->popup( p );
}


void K3bDirView::slotShowDiskInfo()
{
  if( m_lastDevice ) {
    m_bViewDiskInfo = true;
    slotDetectDiskInfo( m_lastDevice );
  }
}


void K3bDirView::slotUnmountDisk()
{
  if( m_lastDevice ) {
    KIO::unmount( m_lastDevice->mountPoint() );    
  }
}


void K3bDirView::slotEjectDisk()
{
  // cancel any previous disk info retrieval
  m_diskInfoDetector->cancel();

  if( m_lastDevice ) {
    m_lastDevice->eject();
    // TODO: check if this was the currently displayed device and if so return to home dir
  }
}


void K3bDirView::slotUpdateURLCombo( const KURL& url )
{
//   m_urlCombo->setEditText( url.path() );
}


void K3bDirView::slotDirActivated( const QString& url )
{
//   m_urlCombo->insertItem( url, 0 );
  slotDirActivated( KURL(url) );
}


void K3bDirView::slotDirActivated( const KURL& url )
{
  // cancel any previous disk info retrieval
  m_diskInfoDetector->cancel();

  m_fileView->setUrl(url, true);
//   m_urlCombo->setEditText( url.path() );

  m_viewStack->raiseWidget( m_fileView );
}


void K3bDirView::reload()
{
  K3bCdContentsView* v = (K3bCdContentsView*)m_viewStack->visibleWidget();

  v->reload();
}


void K3bDirView::home()
{
  slotDirActivated( QDir::homeDirPath() );
}


#include "k3bdirview.moc"
