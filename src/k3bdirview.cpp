/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config-k3b.h>

#include "k3bdirview.h"
#include "k3bapplication.h"
#include "k3b.h"

#include "rip/k3baudiocdview.h"
#include "rip/k3bvideocdview.h"
#ifdef HAVE_LIBDVDREAD
#include "rip/videodvd/k3bvideodvdrippingview.h"
#endif
#include "k3bfileview.h"
#include "k3bfiletreeview.h"
#include "k3bappdevicemanager.h"
#include "k3bdiskinfoview.h"
#include <k3bdevicehandler.h>
#include <k3bdevice.h>
#include <k3bthememanager.h>
#include <k3bmediacache.h>
#include <k3bexternalbinmanager.h>
#include <k3bpassivepopup.h>
#include <KActionMenu>
#include <kactioncollection.h>
#include <kmenu.h>
#include <unistd.h>
// QT-includes
#include <qdir.h>
#include <q3listview.h>
#include <qstring.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <qstringlist.h>
#include <q3strlist.h>
#include <q3header.h>
#include <qsplitter.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qicon.h>
#include <q3valuelist.h>
#include <qlabel.h>
#include <q3widgetstack.h>
#include <q3scrollview.h>
#include <qpainter.h>
#include <q3simplerichtext.h>

// KDE-includes
#include <kmimetype.h>
#include <kcursor.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <kurl.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/global.h>
#include <krun.h>
#include <k3process.h>
#include <kio/job.h>
#include <kcombobox.h>
#include <k3filetreeview.h>
#include <kdialog.h>
#include <kmessagebox.h>
#include <kstandardaction.h>
#include <kconfig.h>
#include <kaction.h>
#include <kinputdialog.h>



class K3bDirView::Private
{
public:
  bool contextMediaInfoRequested;
};



K3bDirView::K3bDirView(K3bFileTreeView* treeView, QWidget *parent, const char *name )
  : Q3VBox(parent),
    m_fileTreeView(treeView),
    m_bViewDiskInfo(false)
{
  d = new Private;
  d->contextMediaInfoRequested = false;

  if( !m_fileTreeView ) {
    m_mainSplitter = new QSplitter( this );
    m_fileTreeView = new K3bFileTreeView( m_mainSplitter );
    m_viewStack    = new Q3WidgetStack( m_mainSplitter );
  }
  else {
    m_viewStack    = new Q3WidgetStack( this );
    m_mainSplitter = 0;
  }

  m_fileTreeView->header()->hide();

  m_fileView     = new K3bFileView(m_viewStack, "fileView");
  m_cdView       = new K3bAudioCdView(m_viewStack, "cdview");
  m_videoView    = new K3bVideoCdView(m_viewStack, "videoview");
  m_infoView     = new K3bDiskInfoView(m_viewStack);
#ifdef HAVE_LIBDVDREAD
  m_movieView    = new K3bVideoDVDRippingView(m_viewStack, "movieview");
#endif

  m_viewStack->raiseWidget( m_fileView );

  m_fileTreeView->addDefaultBranches();
  m_fileTreeView->addCdDeviceBranches( k3bcore->deviceManager() );
  m_fileTreeView->setCurrentDevice( k3bappcore->appDeviceManager()->currentDevice() );

  m_fileView->setAutoUpdate( true ); // in case we look at the mounted path

  if( m_mainSplitter ) {
    // split
    Q3ValueList<int> sizes = m_mainSplitter->sizes();
    int all = sizes[0] + sizes[1];
    sizes[1] = all*2/3;
    sizes[0] = all - sizes[1];
    m_mainSplitter->setSizes( sizes );
  }

  connect( m_fileTreeView, SIGNAL(urlExecuted(const KUrl&)),
	   this, SLOT(slotDirActivated(const KUrl&)) );
  connect( m_fileTreeView, SIGNAL(deviceExecuted(K3bDevice::Device*)),
	   this, SLOT(showDevice(K3bDevice::Device*)) );
  connect( m_fileTreeView, SIGNAL(deviceExecuted(K3bDevice::Device*)),
	   this, SIGNAL(deviceSelected(K3bDevice::Device*)) );
  connect( m_fileTreeView, SIGNAL(contextMenu(K3bDevice::Device*, const QPoint&)),
	   this, SLOT(slotFileTreeContextMenu(K3bDevice::Device*, const QPoint&)) );

  connect( m_fileView, SIGNAL(urlEntered(const KUrl&)), m_fileTreeView, SLOT(followUrl(const KUrl&)) );
  connect( m_fileView, SIGNAL(urlEntered(const KUrl&)), this, SIGNAL(urlEntered(const KUrl&)) );

  connect( k3bappcore->appDeviceManager(), SIGNAL(mountFinished(const QString&)),
	   this, SLOT(slotMountFinished(const QString&)) );
  connect( k3bappcore->appDeviceManager(), SIGNAL(unmountFinished(bool)),
	   this, SLOT(slotUnmountFinished(bool)) );
  connect( k3bappcore->appDeviceManager(), SIGNAL(detectingDiskInfo(K3bDevice::Device*)),
	   this, SLOT(slotDetectingDiskInfo(K3bDevice::Device*)) );
}

K3bDirView::~K3bDirView()
{
  delete d;
}


void K3bDirView::showUrl( const KUrl& url )
{
  slotDirActivated( url );
}


void K3bDirView::showDevice( K3bDevice::Device* dev )
{
  d->contextMediaInfoRequested = true;
  m_fileTreeView->setSelectedDevice( dev );
  showMediumInfo( k3bappcore->mediaCache()->medium( dev ) );
}


void K3bDirView::slotDetectingDiskInfo( K3bDevice::Device* dev )
{
  d->contextMediaInfoRequested = false;
  m_fileTreeView->setSelectedDevice( dev );
  showMediumInfo( k3bappcore->mediaCache()->medium( dev ) );
}


void K3bDirView::showMediumInfo( const K3bMedium& medium )
{
  if( !d->contextMediaInfoRequested ||
      medium.diskInfo().diskState() == K3bDevice::STATE_EMPTY ||
      medium.diskInfo().diskState() == K3bDevice::STATE_NO_MEDIA ) {

    // show cd info
    m_viewStack->raiseWidget( m_infoView );
    m_infoView->reload( medium );
    return;
  }

#ifdef HAVE_LIBDVDREAD
  else if( medium.content() & K3bMedium::CONTENT_VIDEO_DVD ) {
    KMessageBox::ButtonCode r = KMessageBox::Yes;
    if( KMessageBox::shouldBeShownYesNo( "videodvdripping", r ) ) {
      r = (KMessageBox::ButtonCode)
	KMessageBox::questionYesNoCancel( this,
					  i18n("<p>You have selected the K3b Video DVD ripping tool."
					       "<p>It is intended to <em>rip single titles</em> from a video DVD "
					       "into a compressed format such as XviD. Menu structures are completely ignored."
					       "<p>If you intend to copy the plain Video DVD vob files from the DVD "
					       "(including decryption) for further processing with another application, "
					       "please use the following link to access the Video DVD file structure: "
					       "<a href=\"videodvd:/\">videodvd:/</a>"
					       "<p>If you intend to make a copy of the entire Video DVD including all menus "
					       "and extras it is recommended to use the K3b DVD Copy tool."),
					  i18n("Video DVD ripping"),
					  i18n("Continue"),
					  i18n("Open DVD Copy Dialog"),
					  "videodvdripping",
					  KMessageBox::AllowLink );
    }
    else { // if we do not show the dialog we always continue with the ripping. Everything else would be confusing
      r = KMessageBox::Yes;
    }

    if( r == KMessageBox::Cancel ) {
      //      m_viewStack->raiseWidget( m_fileView );
    }
    else if( r == KMessageBox::No ) {
      m_viewStack->raiseWidget( m_fileView );
      static_cast<K3bMainWindow*>( kapp->mainWidget() )->slotMediaCopy();
    }
    else {
      m_movieView->reload( medium );
      m_viewStack->raiseWidget( m_movieView );
    }

    return;
  }
#endif

  else if( medium.content() & K3bMedium::CONTENT_DATA ) {
    bool mount = true;
    if( medium.content() & K3bMedium::CONTENT_VIDEO_CD ) {
      if( !k3bcore ->externalBinManager() ->foundBin( "vcdxrip" ) ) {
	KMessageBox::sorry( this,
			    i18n("K3b uses vcdxrip from the vcdimager package to rip Video CDs. "
				 "Please make sure it is installed.") );
      }
      else {
	if( KMessageBox::questionYesNo( this,
					i18n("Found %1. Do you want K3b to mount the data part "
					     "or show all the tracks?", i18n("Video CD") ),
					i18n("Video CD"),
					KGuiItem(i18n("Mount CD")),
					KGuiItem(i18n("Show Video Tracks")) ) == KMessageBox::No ) {
	  mount = false;
	  m_viewStack->raiseWidget( m_videoView );
	  m_videoView->reload( medium );
	}
      }
    }
    else if( medium.content() & K3bMedium::CONTENT_AUDIO ) {
      if( KMessageBox::questionYesNo( this,
				      i18n("Found %1. Do you want K3b to mount the data part "
					   "or show all the tracks?", i18n("Audio CD") ),
				      i18n("Audio CD"),
				      KGuiItem(i18n("Mount CD")),
				      KGuiItem(i18n("Show Audio Tracks")) ) == KMessageBox::No ) {
	mount = false;
	m_viewStack->raiseWidget( m_cdView );
	m_cdView->reload( medium );
      }
    }

    if( mount )
      k3bappcore->appDeviceManager()->mountDisk( medium.device() );
  }

  else if( medium.content() & K3bMedium::CONTENT_AUDIO ) {
    m_viewStack->raiseWidget( m_cdView );
    m_cdView->reload( medium );
  }

  else {
    // show cd info
    m_viewStack->raiseWidget( m_infoView );
    m_infoView->reload( medium );
  }

  d->contextMediaInfoRequested = false;
}


void K3bDirView::slotMountFinished( const QString& mp )
{
  if( !mp.isEmpty() ) {
    slotDirActivated( mp );
    m_fileView->reload(); // HACK to get the contents shown... FIXME
  }
  else {
    m_viewStack->raiseWidget( m_fileView );
    K3bPassivePopup::showPopup( i18n("<p>K3b was unable to mount medium <b>%1</b> in device <em>%2 - %3</em>")
				.arg( k3bappcore->mediaCache()->medium( k3bappcore->appDeviceManager()->currentDevice() ).shortString() )
				.arg( k3bappcore->appDeviceManager()->currentDevice()->vendor() )
				.arg( k3bappcore->appDeviceManager()->currentDevice()->description() ),
				i18n("Mount Failed"),
				K3bPassivePopup::Warning );
  }
}


void K3bDirView::slotUnmountFinished( bool success )
{
  if( success ) {
    // TODO: check if the fileview is still displaying a folder from the medium
  }
  else {
    K3bPassivePopup::showPopup( i18n("<p>K3b was unable to unmount medium <b>%1</b> in device <em>%2 - %3</em>")
				.arg( k3bappcore->mediaCache()->medium( k3bappcore->appDeviceManager()->currentDevice() ).shortString() )
				.arg( k3bappcore->appDeviceManager()->currentDevice()->vendor() )
				.arg( k3bappcore->appDeviceManager()->currentDevice()->description() ),
				i18n("Unmount Failed"),
				K3bPassivePopup::Warning );
  }
}


void K3bDirView::slotFileTreeContextMenu( K3bDevice::Device* /*dev*/, const QPoint& p )
{
  QAction* a = k3bappcore->appDeviceManager()->actionCollection()->action( "device_popup" );
  if( KActionMenu* m = dynamic_cast<KActionMenu*>(a) )
    m->menu()->exec( p );
}


void K3bDirView::slotDirActivated( const QString& url )
{
//   m_urlCombo->insertItem( url, 0 );
  slotDirActivated( KUrl(url) );
}


void K3bDirView::slotDirActivated( const KUrl& url )
{
  m_fileView->setUrl(url, true);
//   m_urlCombo->setEditText( url.path() );

  m_viewStack->raiseWidget( m_fileView );
}


void K3bDirView::home()
{
  slotDirActivated( QDir::homePath() );
}


void K3bDirView::saveConfig( KConfig* c )
{
  m_fileView->saveConfig(c);
}


void K3bDirView::readConfig( KConfig* c )
{
  m_fileView->readConfig(c);
}

#include "k3bdirview.moc"
