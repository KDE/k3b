/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bfiletreeview.h"
//#include "k3b.h"
//#include "k3bdirview.h"

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bdiskinfo.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>
#include <kstandarddirs.h>
#include <kaction.h>
#include <kio/global.h>
#include <kfileitem.h>
#include <kapplication.h>
#include <kglobalsettings.h>

#include <qdir.h>
#include <qevent.h>
#include <qdragobject.h>
#include <qcursor.h>
#include <qnamespace.h>
#include <qmap.h>
#include <qptrdict.h>



K3bDeviceBranch::K3bDeviceBranch( KFileTreeView* view, K3bDevice::Device* dev, KFileTreeViewItem* item )
  : KFileTreeBranch( view, 
		     KURL(dev->mountPoint()), 
		     QString("%1 - %2").arg(dev->vendor()).arg(dev->description()),
		     ( dev->burner()
		       ? SmallIcon("cdwriter_unmount")
		       : SmallIcon("cdrom_unmount") ),
		     false, 
		     item == 0 
		     ? new K3bDeviceBranchViewItem( view,
						    new KFileItem( KURL(dev->mountPoint()), "inode/directory",
								   S_IFDIR  ),
						    this )
		     : item ), 
    m_device( dev )
{
  setAutoUpdate(true);
  root()->setExpandable(false);
}


void K3bDeviceBranch::mount()
{
  if( !m_device->mountPoint().isEmpty() ) {
    QString mp = KIO::findDeviceMountPoint( m_device->mountDevice() );
    if( mp.isEmpty() )
      connect( KIO::mount( true, 0, m_device->mountDevice(), m_device->mountPoint(), false ), SIGNAL(result(KIO::Job*)),
	       this, SLOT( slotMountFinished(KIO::Job*) ) );
    else {
      kdDebug() << "(K3bDeviceBranch) device already mounted on " << mp << endl;
      emit clear();
      populate( KURL::fromPathOrURL(mp), root() );
      emit mountFinished( this, mp );
    }
  }
  else
    emit mountFinished( this, QString::null );
}


void K3bDeviceBranch::unmount()
{
  QString mp = KIO::findDeviceMountPoint( m_device->mountDevice() );
  if( mp.isEmpty() ) {
    emit unmountFinished( this, true );   
  }
  else {
    setAutoUpdate(false);
    connect( KIO::unmount( device()->mountPoint(), false ), SIGNAL(result(KIO::Job*)),
	     this, SLOT( slotUnmountFinished(KIO::Job*) ) );
  }
}


void K3bDeviceBranch::slotMountFinished( KIO::Job* job )
{
  if( job->error() && !m_device->automount() ) {
    job->showErrorDialog();
    emit mountFinished( this, QString::null );
  }
  else {
    emit clear();
    populate( KURL::fromPathOrURL(m_device->mountPoint()), root() );
    emit mountFinished( this, m_device->mountPoint() );
  }
}


void K3bDeviceBranch::slotUnmountFinished( KIO::Job* job )
{
  if( job->error() ) {
    job->showErrorDialog();
    emit unmountFinished( this, false );
  }
  else {
    emit clear();
    emit unmountFinished( this, true );
  }

  setAutoUpdate(true);
}


K3bFileTreeBranch::K3bFileTreeBranch( KFileTreeView* view,
				      const KURL& url,
				      const QString& name,
				      const QPixmap& pix,
				      bool showHidden,
				      KFileTreeViewItem* item )
  : KFileTreeBranch( view, url, name, pix, showHidden,
		     item == 0
		     ? new K3bFileTreeViewItem( view,
						new KFileItem( url, "inode/directory",
							       S_IFDIR  ),
						this )
		     : item )
{
}



K3bDeviceBranchViewItem::K3bDeviceBranchViewItem( KFileTreeViewItem* parent, KFileItem* item, KFileTreeBranch* branch )
  : KFileTreeViewItem( parent, item, branch )
{
}


K3bDeviceBranchViewItem::K3bDeviceBranchViewItem( KFileTreeView* parent, KFileItem* item, KFileTreeBranch* branch )
  : KFileTreeViewItem( parent, item, branch )
{
}


QString K3bDeviceBranchViewItem::key( int column, bool ascending ) const
{
  return "0" + KFileTreeViewItem::key( column, ascending );
}


K3bFileTreeViewItem::K3bFileTreeViewItem( KFileTreeViewItem* parent, KFileItem* item, KFileTreeBranch* branch )
  : KFileTreeViewItem( parent, item, branch )
{
}


K3bFileTreeViewItem::K3bFileTreeViewItem( KFileTreeView* parent, KFileItem* item, KFileTreeBranch* branch )
  : KFileTreeViewItem( parent, item, branch )
{
}


QString K3bFileTreeViewItem::key( int column, bool ascending ) const
{
  return "1" + KFileTreeViewItem::key( column, ascending );
}



class K3bFileTreeView::Private
{
public:
  Private()
    : deviceManager(0) {
  }

  QPtrDict<K3bDeviceBranch> deviceBranchDict;
  QMap<KFileTreeBranch*, K3bDevice::Device*> branchDeviceMap;

  K3bDevice::DeviceManager* deviceManager;
};

K3bFileTreeView::K3bFileTreeView( QWidget *parent, const char *name )
  : KFileTreeView( parent,  name )
{
  d = new Private();

  addColumn( i18n("Directories") );
  setDragEnabled( true );
  setAlternateBackground( QColor() );
  setFullWidth();
  setRootIsDecorated(true);
  setSorting(0);

  m_dirOnlyMode = true;
  m_menuEnabled = false;

  connect( this, SIGNAL(executed(QListViewItem*)), this, SLOT(slotItemExecuted(QListViewItem*)) );
  connect( this, SIGNAL(contextMenu(KListView*, QListViewItem* , const QPoint& )),
	   this, SLOT(slotContextMenu(KListView*, QListViewItem* , const QPoint& )) );

  // we always simulate the single click
  slotSettingsChangedK3b(KApplication::SETTINGS_MOUSE);
  if (kapp)
    connect( kapp, SIGNAL(settingsChanged(int)), SLOT(slotSettingsChangedK3b(int)) );

  initActions();
}


K3bFileTreeView::~K3bFileTreeView()
{
  delete d;
}


void K3bFileTreeView::clear()
{
  KFileTreeView::clear();
  if( d->deviceManager )
    d->deviceManager->disconnect( this );
  d->deviceManager = 0;
}


void K3bFileTreeView::initActions()
{
//   m_actionCollection = new KActionCollection( this );

//   m_devicePopupMenu = new KActionMenu( m_actionCollection, "device_popup_menu" );
//   m_urlPopupMenu = new KActionMenu( m_actionCollection, "url_popup_menu" );

//   KAction* actionDiskInfo = new KAction( i18n("&Disk Info"), "info", 0, this, SLOT(slotShowDiskInfo()),
// 					 m_actionCollection, "disk_info");
//   KAction* actionUnmount = new KAction( i18n("&Unmount"), "cdrom_unmount", 0, this, SLOT(slotUnmountDisk()),
// 					m_actionCollection, "disk_unmount");
//   KAction* actionEject = new KAction( i18n("&Eject"), "", 0, this, SLOT(slotEjectDisk()),
// 					m_actionCollection, "disk_eject");

//   m_devicePopupMenu->insert( actionDiskInfo );
//   m_devicePopupMenu->insert( new KActionSeparator( this ) );
//   m_devicePopupMenu->insert( actionUnmount );
//   m_devicePopupMenu->insert( actionEject );

}


void K3bFileTreeView::addDefaultBranches()
{
  KURL home = KURL( QDir::homeDirPath() );
  KURL root = KURL( "/" );

  KFileTreeBranch* treeBranch = addBranch( new K3bFileTreeBranch( this, root, i18n("Root"), SmallIcon("folder_red") ) );
  treeBranch = addBranch( new K3bFileTreeBranch( this, home, i18n("Home"), SmallIcon("folder_home") ) );
  treeBranch->setOpen( true );
}


void K3bFileTreeView::addCdDeviceBranches( K3bDevice::DeviceManager* dm )
{
  kdDebug() << "(K3bFileTreeView::addCdDeviceBranches)" << endl;

  // remove all previous added device branches
  for( QMap<KFileTreeBranch*, K3bDevice::Device*>::Iterator it = d->branchDeviceMap.begin();
       it != d->branchDeviceMap.end(); ++it ) {
    removeBranch( it.key() );
  }

  // clear the maps
  d->branchDeviceMap.clear();
  d->deviceBranchDict.clear();

  QPtrList<K3bDevice::Device>& devices = dm->allDevices();
  for ( K3bDevice::Device* dev = devices.first(); dev != 0; dev = devices.next() ) {

    K3bDeviceBranch* newBranch = new K3bDeviceBranch( this, dev );
    addBranch( newBranch );

    connect( newBranch, SIGNAL(mountFinished(K3bDeviceBranch*, const QString&)),
	     this, SIGNAL(mountFinished(K3bDeviceBranch*, const QString&)) );
    connect( newBranch, SIGNAL(unmountFinished(K3bDeviceBranch*, bool)),
	     this, SIGNAL(unmountFinished(K3bDeviceBranch*, bool)) );

    // add to maps
    d->branchDeviceMap.insert( newBranch, dev );
    d->deviceBranchDict.insert( (void*)dev, newBranch );
  }

  if( dm != d->deviceManager ) {
    if( d->deviceManager )
      d->deviceManager->disconnect( this );
    d->deviceManager = dm;

    // make sure we get changes to the config
    connect( dm, SIGNAL(changed(K3bDevice::DeviceManager*)),
	     this, SLOT(addCdDeviceBranches(K3bDevice::DeviceManager*)) );
  }

  kdDebug() << "(K3bFileTreeView::addCdDeviceBranches) done" << endl;
}


KFileTreeBranch* K3bFileTreeView::addBranch( KFileTreeBranch* branch )
{
  KFileTreeBranch* newBranch = KFileTreeView::addBranch( branch );
  setDirOnlyMode( newBranch, m_dirOnlyMode );

  return newBranch;
}


KFileTreeBranch* K3bFileTreeView::addBranch( const KURL& url, const QString& name, const QPixmap& pix, bool showHidden )
{
  KFileTreeBranch* newBranch = KFileTreeView::addBranch( url, name, pix, showHidden );
  setDirOnlyMode( newBranch, m_dirOnlyMode );

  return newBranch;
}


void K3bFileTreeView::slotItemExecuted( QListViewItem* item )
{
  KFileTreeViewItem* treeItem = static_cast<KFileTreeViewItem*>(item);
  if( d->branchDeviceMap.contains( treeItem->branch() ) &&
      treeItem == treeItem->branch()->root() )
    emit deviceExecuted( d->branchDeviceMap[treeItem->branch()] );
  else
    emit urlExecuted( treeItem->url() );
}


void K3bFileTreeView::setTreeDirOnlyMode( bool b )
{
  m_dirOnlyMode = b;
}


void K3bFileTreeView::followUrl( const KURL& url )
{
  // TODO: first try the current branch
  KFileTreeBranchIterator it( branches() );
  for( ; *it; ++it ) {
    if( !d->branchDeviceMap.contains( *it ) )
      if( KFileTreeViewItem* item = (*it)->findTVIByURL( url ) ) {
	setCurrentItem( item );
	setSelected(item, true);
	ensureItemVisible( item );
	return;
      }
  }
}


void K3bFileTreeView::slotContextMenu( KListView*, QListViewItem* item, const QPoint& p )
{
  KFileTreeViewItem* treeItem = dynamic_cast<KFileTreeViewItem*>(item);
  if( treeItem ) {
    bool device = d->branchDeviceMap.contains( treeItem->branch() );

//     if( m_menuEnabled ) {
//       if( device )
// 	m_devicePopupMenu->popup(p);
//       else
// 	m_urlPopupMenu->popup(p);
//     }
//     else {
      setCurrentItem( treeItem );
      setSelected( treeItem, true);
      if( device )
	emit contextMenu( d->branchDeviceMap[treeItem->branch()], p );
      else
	emit contextMenu( treeItem->url(), p );
      //    }
  }
  else
    kdWarning() << "(K3bFileTreeView) found viewItem that is no KFileTreeViewItem!" << endl;
}


K3bDevice::Device* K3bFileTreeView::selectedDevice() const
{
  KFileTreeViewItem* treeItem = dynamic_cast<KFileTreeViewItem*>(selectedItem());
  if( treeItem ) {
    if( d->branchDeviceMap.contains( treeItem->branch() ) )
      return d->branchDeviceMap[treeItem->branch()];
  }
  return 0;
}


KURL K3bFileTreeView::selectedUrl() const
{
  KFileTreeViewItem* treeItem = dynamic_cast<KFileTreeViewItem*>(selectedItem());
  if( treeItem ) {
    if( !d->branchDeviceMap.contains( treeItem->branch() ) )
      return treeItem->url();
  }
  return KURL();
}

void K3bFileTreeView::setSelectedDevice(K3bDevice::Device* dev)
{
  for(QMap<KFileTreeBranch*, K3bDevice::Device*>::iterator it = d->branchDeviceMap.begin();
      it != d->branchDeviceMap.end(); ++it)
  {
    kdDebug() << "Select " << dev->devicename() << endl;
    if ( *it == dev ) {
      setCurrentItem( it.key()->root() );
      setSelected( it.key()->root(), true);
      return;
    }
  }
}


K3bDeviceBranch* K3bFileTreeView::branch( K3bDevice::Device* dev )
{
  return d->deviceBranchDict.find( (void*)dev );
}


void K3bFileTreeView::slotSettingsChangedK3b(int category)
{
  // we force single click like konqueror does. This really should be done in KFileTreeView

  if( category == KApplication::SETTINGS_MOUSE ) {
    disconnect(this, SIGNAL(mouseButtonClicked(int, QListViewItem*, const QPoint &, int)),
	       this, SLOT(slotMouseButtonClickedK3b(int, QListViewItem*, const QPoint &, int)));

    if( !KGlobalSettings::singleClick() )
      connect(this, SIGNAL(mouseButtonClicked(int, QListViewItem*, const QPoint &, int)),
	      this, SLOT(slotMouseButtonClickedK3b(int, QListViewItem*, const QPoint &, int)));
  }
}


void K3bFileTreeView::slotMouseButtonClickedK3b( int btn, QListViewItem *item, const QPoint &pos, int c )
{
  if( (btn == LeftButton) && item )
    emitExecute(item, pos, c);
}

#include "k3bfiletreeview.moc"
