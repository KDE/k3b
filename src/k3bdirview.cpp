/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdirview.h"
#include "k3bapplication.h"
#include "k3b.h"
#include "k3bcore.h"

#include "rip/k3bcdview.h"
#include "k3bfileview.h"
#include "device/k3bdevicemanager.h"
#include "device/k3bdevice.h"
#include "rip/k3bmovieview.h"
#include "k3bfiletreeview.h"
#include "device/k3bdiskinfodetector.h"
#include "cdinfo/k3bdiskinfoview.h"
#include <device/k3bdevicehandler.h>

#include <unistd.h>
// QT-includes
#include <qdir.h>
#include <qlistview.h>
#include <qstring.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <qstringlist.h>
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
#include <qpainter.h>

// KDE-includes
#include <kmimetype.h>
#include <kcursor.h>
#include <kfiledetailview.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <kurl.h>
#include <klocale.h>
#include <kautomount.h>
#include <kstandarddirs.h>
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
#include <kconfig.h>


class K3bNoViewView : public QWidget
{
public:
  K3bNoViewView( QWidget* parent )
    : QWidget( parent ) {
    setPaletteBackgroundColor( QColor(201, 208, 255) );
  }

protected:
  void paintEvent( QPaintEvent* ) {
    QPainter p( this );

    QPixmap pix(locate( "data", "k3b/pics/k3b_probing_cd.png" ));
    p.drawPixmap( 0, 0, pix );
    p.setPen( Qt::white );
    p.drawText( pix.width() + 10, pix.height() /3, i18n("K3b is trying to retrieve information about the inserted disk.") );
  }
};



K3bDirView::K3bDirView(K3bFileTreeView* treeView, QWidget *parent, const char *name )
  : QVBox(parent, name), 
    m_fileTreeView(treeView),
    m_bViewDiskInfo(false), 
    m_lastDevice(0)
{
  m_diskInfoDetector = new K3bDiskInfoDetector( this );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bCdDevice::DiskInfo&)),
	   k3bMain(), SLOT(endBusy()) );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bCdDevice::DiskInfo&)),
	   this, SLOT(slotDiskInfoReady(const K3bCdDevice::DiskInfo&)) );

  //  KToolBar* toolBar = new KToolBar( this, "dirviewtoolbar" );


  if( !m_fileTreeView ) {
    m_mainSplitter = new QSplitter( this );
    m_fileTreeView = new K3bFileTreeView( m_mainSplitter );
    m_viewStack    = new QWidgetStack( m_mainSplitter );
  }
  else {  
    m_viewStack    = new QWidgetStack( this );
    m_mainSplitter = 0;
  }

  m_fileTreeView->header()->hide();
  m_fileTreeView->addCdDeviceBranches( k3bcore->deviceManager() );

  m_fileView     = new K3bFileView(m_viewStack, "fileView");
  m_cdView       = new K3bCdView(m_viewStack, "cdview");
  m_movieView    = new K3bMovieView(m_viewStack, "movieview");
  m_infoView     = new K3bDiskInfoView(m_viewStack, "infoView");

  m_noViewView = new K3bNoViewView( m_viewStack );

  m_viewStack->raiseWidget( m_fileView );

  m_fileTreeView->addDefaultBranches();

  if( m_mainSplitter ) {
    // split
    QValueList<int> sizes = m_mainSplitter->sizes();
    int all = sizes[0] + sizes[1];
    sizes[1] = all*2/3;
    sizes[0] = all - sizes[1];
    m_mainSplitter->setSizes( sizes );
  }


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


  m_devicePopupMenu = new KActionMenu( this, "device_popup_menu" );
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


void K3bDirView::showUrl( const KURL& url )
{
  slotDirActivated( url );
}


void K3bDirView::showDevice( K3bDevice* dev )
{
  slotDetectDiskInfo( dev );
}


void K3bDirView::slotDetectDiskInfo( K3bDevice* dev )
{
  m_viewStack->raiseWidget( m_noViewView );
  k3bMain()->showBusyInfo( i18n("Trying to fetch information about the inserted disk.") );
//  if ( m_fileView->Url().path().startsWith( dev->mountPoint()) ) {
//    home();
//    dev->unmount();
//  }
  m_diskInfoDetector->detect( dev );
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
    if( info.isVideoDvd ) {
      m_movieView->setDevice( info.device );
      m_viewStack->raiseWidget( m_movieView );
      m_movieView->reload();
    }
    else
      slotMountDevice( info.device );
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
					    i18n("Mount CD"),
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
    m_lastDevice = device;
    connect( K3bCdDevice::mount(device),
             SIGNAL(finished(K3bCdDevice::DeviceHandler *)),
	     this, SLOT( slotMountFinished(K3bCdDevice::DeviceHandler *) ) );
  }
  else {
    KMessageBox::error( this, i18n("K3b could not mount %1. Please run K3bSetup.").arg(device->mountDevice()),
			i18n("I/O error") );
  }
}

void K3bDirView::slotMountFinished(K3bCdDevice::DeviceHandler * )
{
   KURL url = KURL(m_lastDevice->mountPoint());
   KFileTreeViewItem* item = m_fileTreeView->currentKFileTreeViewItem();
   item->branch()->openURL(url);
   item->branch()->setAutoUpdate(true);
   item->branch()->updateDirectory(url);
   slotDirActivated( url );
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
  k3bMain()->showBusyInfo( i18n("Unmounting disk.") );
  if( m_lastDevice ) {
    KFileTreeViewItem* item = m_fileTreeView->currentKFileTreeViewItem();
    item->branch()->setAutoUpdate(false);
    if ( m_fileView->Url().path().startsWith(m_lastDevice->mountPoint()) )
    	home();
    connect( K3bCdDevice::unmount(m_lastDevice),SIGNAL(finished(K3bCdDevice::DeviceHandler *)),
	           this, SLOT( slotUnmountFinished(K3bCdDevice::DeviceHandler *) ) );
    m_fileTreeView->setCurrentItem( item );
 }
}

void K3bDirView::slotUnmountFinished(K3bCdDevice::DeviceHandler *)
{
  KFileTreeViewItem* item = m_fileTreeView->currentKFileTreeViewItem();

  item->branch()->updateDirectory(KURL(m_lastDevice->mountPoint()));

  k3bMain()->endBusy();
}

void K3bDirView::slotEjectDisk()
{
    if ( m_lastDevice )
      if ( m_fileView->Url().path().startsWith(m_lastDevice->mountPoint()) )
        home();

     K3bCdDevice::eject( m_lastDevice );
}


void K3bDirView::slotUpdateURLCombo( const KURL& )
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
//   m_diskInfoDetector->cancel();
  k3bMain()->endBusy();

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


void K3bDirView::saveConfig( KConfig* c )
{
  m_fileView->saveConfig(c);
}


#include "k3bdirview.moc"
