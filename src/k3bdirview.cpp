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

// KDE-includes
#include <kmimetype.h>
#include <kcursor.h>
#include <kfiledetailview.h>
#include <kfileviewitem.h>
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

#include "kiotree/kiotree.h"
#include "kiotree/kiotreemodule.h"
#include "kiotree/kiotreeitem.h"
#include "kiotree/kiotreetoplevelitem.h"
#include "rip/k3bcdview.h"
#include "k3bfileview.h"
#include "device/k3bdevicemanager.h"
#include "device/k3bdevice.h"
#include "k3b.h"

// K3bDirView
////////////////////////////////////////////////////////////////////

K3bDirView::K3bDirView(QWidget *parent, const char *name )
  : QVBox(parent, name)
{
  m_mainSplitter = new QSplitter( this );
  QVBox* box = new QVBox( m_mainSplitter );
  QVBox *box2 = new QVBox( m_mainSplitter );

  m_kiotree = new KioTree( box );
  m_kiotree->addTopLevelDir( KURL( QDir::homeDirPath() ), "Home" );
  m_kiotree->addTopLevelDir( KURL( "/" ), "Root" );
  //KURL url = KURL();
  //url.setProtocol("k3b_cdview");
  //m_kiotree->addTopLevelDir( url, "Audio CD Init");
	
  m_fileView = new K3bFileView(box2, "fileview");

  // cd view
  m_cdView = new K3bCdView(box2, "cdview");
  m_cdView->hide();
  m_initialized = false;
  connect( m_kiotree, SIGNAL(urlActivated(const KURL&)), this, SLOT(slotDirActivated(const KURL&)) );
  connect( m_cdView, SIGNAL(showDirView( QString )), this, SLOT(slotCDDirActivated( QString )) );

  // split in the middle
  QValueList<int> sizes = m_mainSplitter->sizes();
  int all = sizes[0] + sizes[1];
  sizes[0] = all/2 + (all%2);
  sizes[1] = all/2;
  m_mainSplitter->setSizes( sizes );
}

K3bDirView::~K3bDirView()
{
}

void K3bDirView::setupFinalize( K3bDeviceManager *dm )
{
  m_fileView->show();
  K3bDevice *dev;
  KURL result;
  QList<K3bDevice> devices = dm->readingDevices();
  for ( dev = devices.first(); dev != 0; dev=devices.next() ) {
    KURL url = KURL( dev->devicename() );
    url.setProtocol("k3b_cdview");
    result = url;
    m_kiotree->addTopLevelDir( url, i18n("Drive: ") + dev->vendor() );
    for( int i=0; i<m_kiotree->childCount(); i++){
      KioTreeItem *item = dynamic_cast<KioTreeItem*> (m_kiotree->itemAtIndex(i) );
      KURL url = item->externalURL();
      if( url.path().find( dev->devicename() ) == 0){
	item->setPixmap(0, KGlobal::iconLoader()->loadIcon( "cdrom_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );
      }
    }
  }
  devices = dm->burningDevices();
  for ( dev=devices.first(); dev != 0; dev=devices.next() ){
    KURL url = KURL(dev->devicename() );
    url.setProtocol("k3b_cdview");
    m_kiotree->addTopLevelDir( url, i18n("Drive: ") + dev->vendor() );
    for( int i=0; i<m_kiotree->childCount(); i++){
      KioTreeItem *item = dynamic_cast<KioTreeItem*> (m_kiotree->itemAtIndex(i) );
      KURL url = item->externalURL();
      if( url.path().find(dev->devicename() ) == 0){
	item->setPixmap(0, KGlobal::iconLoader()->loadIcon( "cdwriter_unmount", KIcon::NoGroup, KIcon::SizeSmall ) );
      }
    }
  }
  //return result;
}

void K3bDirView::slotViewChanged( KFileView* newView )
{
  newView->setSelectionMode( KFile::Extended );
  if( KListView* _x = dynamic_cast<KListView*>( newView->widget() ) )
    _x->setDragEnabled( true );
}

void K3bDirView::slotCDDirActivated(QString device){
    /* without root right i dont know how to mount the device to a specific directory.
       i can only mount devices listed in fstab. so i have to check if fstab devices are the
       same as our sgX device and can mount then on the fstab directory but not temporary
       for k3b. :-(  Any idea ??????
    */
    KConfig* c = kapp->config();
    c->setGroup( "General Options" );
    QString tempdir = c->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) );
    qDebug("(K3bDirView) Mount dir: " + tempdir);
    QDir mt = QDir(tempdir);
    if( !mt.exists() )
        mt.mkdir("tempcd");
    qDebug("(K3bDirView) new dir for cd.");
    //KAutoMount *tempCdView = new KAutoMount ( true, "iso9660", "/dev/cdrecorder", "/abc", "/home/ft0001/bla", false );
    //KRun::runCommand( "ls -l" );//mount -t iso9660 /dev/cdrecorder /abc");
    //FileProtocol *fp = new FileProtocol("dummy", "k3b");
    //fp->mount( true, "iso9660", device, "/abc" );
    //KIO::mount( true, "iso9660", "/dev/cdrecorder", "/home/ft0001/cdr", true);
    //const KURL url = KURL( tempdir + "tempcd");
    //slotDirActivated( url );
}

void K3bDirView::slotDirActivated( const KURL& url )
{
	  		
     if( url.protocol().compare("k3b_cdview") !=0 ){
          m_fileView->setUrl(url, true);
          m_fileView->show();
          m_cdView->hide();
     } else {
          m_fileView->hide();
          m_cdView->show();
          m_cdView->showCdView( url.path() );
    }
}

