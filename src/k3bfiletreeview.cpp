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


#include "k3bfiletreeview.h"
//#include "k3b.h"
//#include "k3bdirview.h"

#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>
#include <device/k3bdiskinfo.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>
#include <kstandarddirs.h>
#include <kaction.h>
#include <kio/global.h>
#include <kfileitem.h>

#include <qdir.h>
#include <qevent.h>
#include <qdragobject.h>
#include <qcursor.h>
#include <qnamespace.h>
#include <qmap.h>
#include <qptrdict.h>



K3bDeviceBranch::K3bDeviceBranch( KFileTreeView* view, K3bCdDevice::CdDevice* dev, KFileTreeViewItem* item )
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
      populate( mp, root() );
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
    setAutoUpdate(false);
    connect( KIO::unmount( device()->mountPoint(), false ), SIGNAL(result(KIO::Job*)),
	     this, SLOT( slotUnmountFinished(KIO::Job*) ) );
  }
  else
    emit unmountFinished( this, true );   
}


void K3bDeviceBranch::slotMountFinished( KIO::Job* job )
{
  if( job->error() ) {
    job->showErrorDialog();
    emit mountFinished( this, QString::null );
  }
  else {
    emit clear();
    populate( m_device->mountPoint(), root() );
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
  Private() {
  }

  QPtrDict<K3bDeviceBranch> deviceBranchDict;
  QMap<KFileTreeBranch*, K3bCdDevice::CdDevice*> branchDeviceMap;
};

K3bFileTreeView::K3bFileTreeView( QWidget *parent, const char *name )
  : KFileTreeView( parent,  name )
{
  d = new Private();

  addColumn( i18n("Directories") );
  setDragEnabled( true );
  setAcceptDrops( true );
  setDropVisualizer( true );
  setAlternateBackground( QColor() );
  setFullWidth();
  setRootIsDecorated(true);
  setSorting(0);

  m_dirOnlyMode = true;
  m_menuEnabled = false;

  //connect( this, SIGNAL(currentChanged(QListViewItem*)), this, SLOT(slotItemExecuted(QListViewItem*)) );
  connect( this, SIGNAL(executed(QListViewItem*)), this, SLOT(slotItemExecuted(QListViewItem*)) );
  connect( this, SIGNAL(contextMenu(KListView*, QListViewItem* , const QPoint& )),
	   this, SLOT(slotContextMenu(KListView*, QListViewItem* , const QPoint& )) );

  initActions();
}


K3bFileTreeView::~K3bFileTreeView()
{
  delete d;
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


/*
void K3bFileTreeView::slotDropped() {
    kdDebug() << "(K3bFileTreeView:slotDropped)" << endl;
}
*/
void K3bFileTreeView::contentsDropEvent(QDropEvent* ) {
//     kdDebug() << "(K3bFileTreeView:contentsDropEvent)" << endl;
//     QString text;
//     if ( QTextDrag::decode(event, text) ) {
//         if ( text == CD_DRAG) {
//             event->accept();
//             kdDebug() << "(K3bFileTreeView) Drop text index: " << text << endl;
//             // correct entry  if scrollbar are visible and scrolled
//             KFileTreeViewItem *item = dynamic_cast<KFileTreeViewItem*>( itemAt( contentsToViewport( event->pos() ) ));
//             if( item ){
//                 kdDebug() << "(K3bFileTreeView) Path: " << item->path() << endl;
//                 QApplication::setOverrideCursor( Qt::ArrowCursor );
//                 kdDebug() << "(K3bFileTreeView) Start rip dialog" << endl;
//                 k3bMain()->mainWindow()->getCdView()->slotPrepareRipping( item->path() );
//                 QApplication::restoreOverrideCursor();
//             }
//         }
//     }
}

void K3bFileTreeView::contentsDragMoveEvent ( QDragMoveEvent* ){
//    QString text;
//    QTextDrag::decode( e, text );
//    if ( text == CD_DRAG ) {
//        KFileTreeView::contentsDragMoveEvent( e );
//    } else {
//        e->ignore();
//    }
}
/*
* overwrite original to support K3b DND Objects. That will enable autoOpenFolder in the tree.
*/
bool K3bFileTreeView::acceptDrag(QDropEvent*  ) const {
//    bool result = false;
//    QString text;
//    QTextDrag::decode( e, text );
//    if ( text == CD_DRAG) {
//        result = true;
//    }
//    if( !result ){
//        // the original from KFileTreeView to support copy/link/move
//        result = QUriDrag::canDecode( e ) &&
//            ( e->action() == QDropEvent::Copy
//            || e->action() == QDropEvent::Move
//            || e->action() == QDropEvent::Link );
//    }
//    return result;
  return false;
}

void K3bFileTreeView::addDefaultBranches()
{
  KURL home = KURL( QDir::homeDirPath() );
  KURL root = KURL( "/" );

  KFileTreeBranch* treeBranch = addBranch( new K3bFileTreeBranch( this, root, i18n("Root"), SmallIcon("folder_red") ) );
  treeBranch = addBranch( new K3bFileTreeBranch( this, home, i18n("Home"), SmallIcon("folder_home") ) );
  treeBranch->setOpen( true );
}


void K3bFileTreeView::addCdDeviceBranches( K3bDeviceManager* dm )
{
  // remove all previous added device branches
  for( QMap<KFileTreeBranch*, K3bCdDevice::CdDevice*>::Iterator it = d->branchDeviceMap.begin();
       it != d->branchDeviceMap.end(); ++it ) {
    removeBranch( it.key() );
  }

  // clear the maps
  d->branchDeviceMap.clear();
  d->deviceBranchDict.clear();

  QPtrList<K3bCdDevice::CdDevice>& devices = dm->allDevices();
  for ( K3bCdDevice::CdDevice* dev = devices.first(); dev != 0; dev = devices.next() ) {

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
  KFileTreeBranchIterator it( branches() );
  for( ; *it; ++it ) {
    if( !d->branchDeviceMap.contains( *it ) )
      if( KFileTreeViewItem* item = (*it)->findTVIByURL( url ) ) {
	setCurrentItem( item );
	setSelected(item, true);
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


K3bCdDevice::CdDevice* K3bFileTreeView::selectedDevice() const
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

void K3bFileTreeView::setSelectedDevice(K3bCdDevice::CdDevice* dev)
{
  for(QMap<KFileTreeBranch*, K3bCdDevice::CdDevice*>::iterator it = d->branchDeviceMap.begin();
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


K3bDeviceBranch* K3bFileTreeView::branch( K3bCdDevice::CdDevice* dev )
{
  return d->deviceBranchDict.find( (void*)dev );
}

#include "k3bfiletreeview.moc"
