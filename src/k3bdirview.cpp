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


#include "k3bdirview.h"
#include "k3bapplication.h"

#include "rip/k3baudiocdview.h"
#include "rip/k3bvideocdview.h"
#include "k3bfileview.h"
#include "rip/k3bmovieview.h"
#include "k3bfiletreeview.h"
#include "k3bappdevicemanager.h"
#include "cdinfo/k3bdiskinfodetector.h"
#include "cdinfo/k3bdiskinfoview.h"
#include <k3bdevicehandler.h>
#include <k3bdevice.h>
#include <k3bthememanager.h>

#include <unistd.h>
// QT-includes
#include <qdir.h>
#include <qlistview.h>
#include <qstring.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <qstringlist.h>
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
#include <qsimplerichtext.h>

// KDE-includes
#include <kmimetype.h>
#include <kcursor.h>
#include <kfiledetailview.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <kurl.h>
#include <klocale.h>
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
#include <kaction.h>
#include <kinputdialog.h>


class K3bDirView::NoViewView : public QWidget
{
public:
  NoViewView( QWidget* parent )
    : QWidget( parent, 0, WNoAutoErase ),
      m_text(0),
      m_device(0) {
  }

  ~NoViewView() {
    delete m_text;
  }

  QSize sizeHint() const {
    if( m_text ) {
      // margin of width 9 + line of size 1 + margin of width 10
      // + width of the pixmap + max(widthused of the text, width of the pixmap)
      // + max( height of the text, height of the pixmap )
      return QSize( 40 + m_messageWidth, 40 + m_messageHeight );
    }
    else
      return QWidget::sizeHint();
  }

  void setDevice( K3bDevice::Device* dev ) {
    m_device = dev;
    init();
    update();
  }

protected:
  void paintEvent( QPaintEvent* ) {
    QPainter p( this );

    if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
      p.fillRect( rect(), theme->backgroundColor() );
      p.setPen( theme->foregroundColor() );
      //      p.drawRect( 10, 10, width() - 20, height() - 20 );

      // position of the message
      int mX = QMAX( 20, (width() - m_messageWidth)/2 );
      int mY = QMAX( 20, (height() - m_messageHeight)/2 );

      p.drawRect( mX-1, mY-1, m_messageWidth+2, m_messageHeight+2 );
      p.drawPixmap( mX, mY, theme->pixmap( K3bTheme::PROBING ) );
      mX += (theme->pixmap( K3bTheme::PROBING ).width() + 10);
      mY += QMAX( 10, (m_messageHeight - m_text->height())/2 );
      QColorGroup grp( colorGroup() );
      grp.setColor( QColorGroup::Text, theme->foregroundColor() );
      if( m_text )
	m_text->draw( &p, mX, mY, QRect(), grp );
    }
  }

  void resizeEvent( QResizeEvent* ) {
    init();
    update();
  }

private:
  void init() {
    delete m_text;
    if( m_device ) {
      m_text = new QSimpleRichText( i18n("<p>Please wait while K3b is retrieving information about "
					 "the media in <b>%1</b>.")
				    .arg( m_device->vendor() + " - " + m_device->description() ), font() );
      m_messageHeight = 50;
      int optWidth = 100;
      if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
	m_messageWidth = theme->pixmap( K3bTheme::PROBING ).width();
	optWidth = QMAX( 0, m_messageWidth - 20 );
	m_messageHeight = QMAX( m_messageHeight, theme->pixmap( K3bTheme::PROBING ).height() );
      }
      
      m_text->setWidth( optWidth );
      while( m_text->height() > m_messageHeight-20 && optWidth < m_text->widthUsed() )
	m_text->setWidth( ++optWidth );

      m_messageHeight = QMAX( m_messageHeight, m_text->height()+20 );
      m_messageWidth += QMAX( m_messageWidth, m_text->widthUsed()+20 );
    }
  }

  QSimpleRichText* m_text;
  K3bDevice::Device* m_device;
  int m_messageHeight;
  int m_messageWidth;
};



class K3bDirView::Private
{
public:
  bool contextMediaInfoRequested;
};



K3bDirView::K3bDirView(K3bFileTreeView* treeView, QWidget *parent, const char *name )
  : QVBox(parent, name),
    m_fileTreeView(treeView),
    m_bViewDiskInfo(false)
{
  d = new Private;
  d->contextMediaInfoRequested = false;

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

  m_fileView     = new K3bFileView(m_viewStack, "fileView");
  m_cdView       = new K3bAudioCdView(m_viewStack, "cdview");
  m_videoView    = new K3bVideoCdView(m_viewStack, "videoview");
  m_movieView    = new K3bMovieView(m_viewStack, "movieview");
  m_infoView     = new K3bDiskInfoView(m_viewStack, "infoView");

  m_noViewView = new NoViewView( m_viewStack );

  m_viewStack->raiseWidget( m_fileView );

  m_fileTreeView->addDefaultBranches();
  m_fileTreeView->addCdDeviceBranches( k3bcore->deviceManager() );

  m_fileView->setAutoUpdate( true ); // in case we look at the mounted path

  if( m_mainSplitter ) {
    // split
    QValueList<int> sizes = m_mainSplitter->sizes();
    int all = sizes[0] + sizes[1];
    sizes[1] = all*2/3;
    sizes[0] = all - sizes[1];
    m_mainSplitter->setSizes( sizes );
  }

  connect( m_fileTreeView, SIGNAL(urlExecuted(const KURL&)),
	   this, SLOT(slotDirActivated(const KURL&)) );
  connect( m_fileTreeView, SIGNAL(deviceExecuted(K3bDevice::Device*)),
	   this, SLOT(showDevice(K3bDevice::Device*)) );
  connect( m_fileTreeView, SIGNAL(contextMenu(K3bDevice::Device*, const QPoint&)),
	   this, SLOT(slotFileTreeContextMenu(K3bDevice::Device*, const QPoint&)) );

  connect( m_fileView, SIGNAL(urlEntered(const KURL&)), m_fileTreeView, SLOT(followUrl(const KURL&)) );
  connect( m_fileView, SIGNAL(urlEntered(const KURL&)), this, SIGNAL(urlEntered(const KURL&)) );

  connect( k3bappcore->appDeviceManager(), SIGNAL(diskInfoReady(K3bDevice::DiskInfoDetector*)),
	   this, SLOT(slotDiskInfoReady(K3bDevice::DiskInfoDetector*)) );
  connect( k3bappcore->appDeviceManager(), SIGNAL(mountFinished(const QString&)),
	   this, SLOT(slotMountFinished(const QString&)) );
  connect( k3bappcore->appDeviceManager(), SIGNAL(detectingDiskInfo(K3bDevice::Device*)),
	   this, SLOT(slotDetectingDiskInfo(K3bDevice::Device*)) );
}

K3bDirView::~K3bDirView()
{
  delete d;
}


void K3bDirView::showUrl( const KURL& url )
{
  slotDirActivated( url );
}


void K3bDirView::showDevice( K3bDevice::Device* dev )
{
  d->contextMediaInfoRequested = true;
  m_fileTreeView->setSelectedDevice( dev );
  k3bappcore->appDeviceManager()->diskInfo( dev );
}


void K3bDirView::slotDetectingDiskInfo( K3bDevice::Device* dev )
{
  m_noViewView->setDevice( dev );
  m_viewStack->raiseWidget( m_noViewView );
  k3bappcore->requestBusyInfo( i18n("Trying to fetch information about the inserted disk.") );
}


void K3bDirView::slotDiskInfoReady( K3bDevice::DiskInfoDetector* did )
{
  k3bappcore->requestBusyFinish();

  if( !d->contextMediaInfoRequested ||
      did->diskInfo().diskState() == K3bDevice::STATE_EMPTY ||
      did->diskInfo().diskState() == K3bDevice::STATE_NO_MEDIA ) {
    
    // show cd info
    m_viewStack->raiseWidget( m_infoView );
    m_infoView->displayInfo( did );
  }
  else if( did->isVideoDvd() ) {
    m_movieView->setDevice( did->device() );
    m_viewStack->raiseWidget( m_movieView );
    m_movieView->reload();
  }
  else if( did->toc().contentType() == K3bDevice::DATA ) {
    // check for VCD and ask
    bool mount = true;
    if( did->isVideoCd() ) {
      mount = ( KMessageBox::questionYesNo( this,
					    i18n("Found %1. Do you want K3b to mount the data part "
						 "or show all the tracks?").arg( i18n("Video CD") ),
					    i18n("Video CD"),
					    i18n("Mount CD"),
					    i18n("Show Video Tracks") ) == KMessageBox::Yes );
    }
    
    if( mount ) {
      k3bappcore->appDeviceManager()->mountDisk();
    }
    else {
      m_viewStack->raiseWidget( m_videoView );
      m_videoView->setDisk( did );
    }
  }
  else {
    m_viewStack->raiseWidget( m_cdView );
    m_cdView->setDisk( did );
  }

  d->contextMediaInfoRequested = false;
}


void K3bDirView::slotMountFinished( const QString& mp )
{
  if( !mp.isEmpty() ) {
    slotDirActivated( mp );
    reload(); // HACK to get the contents shown... FIXME
  }
  else {
    m_viewStack->raiseWidget( m_fileView );
    // mounting failed.
    // FIXME: use some silent message box like the amarok one here
    // fallback to diskinfo
  }
}

void K3bDirView::slotFileTreeContextMenu( K3bDevice::Device* /*dev*/, const QPoint& p )
{
  KAction* a = k3bappcore->appDeviceManager()->actionCollection()->action( "device_popup" );
  if( KActionMenu* m = dynamic_cast<KActionMenu*>(a) )
    m->popup( p );
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
  k3bappcore->requestBusyFinish();

  m_fileView->setUrl(url, true);
//   m_urlCombo->setEditText( url.path() );

  m_viewStack->raiseWidget( m_fileView );
}


void K3bDirView::reload()
{
  // TODO: the fileview should be a special case
  //       and then the boolean withHeader parameter should be removed from
  //       K3bCdContentsView
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


void K3bDirView::readConfig( KConfig* c )
{
  m_fileView->readConfig(c);
}

#include "k3bdirview.moc"
