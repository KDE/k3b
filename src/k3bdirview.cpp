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

#include "rip/k3bcdview.h"
#include "k3bfileview.h"
#include "device/k3bdevicemanager.h"
#include "device/k3bdevice.h"
#include "k3b.h"
#include "rip/k3bfilmview.h"
#include "k3bfiletreeview.h"




K3bDirView::K3bDirView(QWidget *parent, const char *name )
  : QVBox(parent, name)
{
  KToolBar* toolBar = new KToolBar( k3bMain(), this, "dirviewtoolbar" );

  m_mainSplitter    = new QSplitter( this );
  QVBox* box  = new QVBox( m_mainSplitter );
  QVBox* box2 = new QVBox( m_mainSplitter );


  m_fileTreeView = new K3bFileTreeView( box );
  m_fileTreeView->addDefaultBranches();

  m_fileView = new K3bFileView(box2, "fileview");

  // cd view
  m_cdView = new K3bCdView(box2, "cdview");
  m_cdView->hide();
  m_filmView = new K3bFilmView(box2, "filmview");
  m_filmView->hide();
  m_initialized = false;

  // split
  QValueList<int> sizes = m_mainSplitter->sizes();
  int all = sizes[0] + sizes[1];
  sizes[1] = all*2/3;
  sizes[0] = all - sizes[1];
  m_mainSplitter->setSizes( sizes );


  // add some actions to the toolbar
  m_fileView->actionCollection()->action("up")->plug( toolBar );
  m_fileView->actionCollection()->action("home")->plug( toolBar );
  m_fileView->actionCollection()->action("reload")->plug( toolBar );
  toolBar->insertSeparator();

  // add a url combobox to the toolbar
  (void)new QLabel( i18n("Location"), toolBar );
  m_urlCombo = new KComboBox( true, toolBar );
  m_urlCombo->setEditText( QDir::homeDirPath() );
  m_urlCombo->setDuplicatesEnabled( false );

  connect( m_urlCombo, SIGNAL(returnPressed(const QString&)), this, SLOT(slotDirActivated(const QString&)) );
  connect( m_urlCombo, SIGNAL(activated(const QString&)), this, SLOT(slotDirActivated(const QString&)) );

  //connect( m_cdView, SIGNAL(showDirView(const QString&)), this, SLOT(slotCDDirActivated(const QString&)) );

  connect( m_fileTreeView, SIGNAL(urlExecuted(const KURL&)), this, SLOT(slotDirActivated(const KURL&)) );
  connect( m_fileTreeView, SIGNAL(deviceExecuted(K3bDevice*)), this, SLOT(slotDeviceActivated(K3bDevice*)) );

  connect( m_fileView, SIGNAL(urlEntered(const KURL&)), m_fileTreeView, SLOT(followUrl(const KURL&)) );
  connect( m_fileView, SIGNAL(urlEntered(const KURL&)), this, SLOT(slotUpdateURLCombo(const KURL&)) );

  connect( m_cdView, SIGNAL(notSupportedDisc( const QString& ) ), this, SLOT( slotCheckDvd( const QString& )) );
  connect( m_filmView, SIGNAL(notSupportedDisc( const QString& ) ), this, SLOT(slotMountDevice( const QString& )) );

}

K3bDirView::~K3bDirView()
{
}

void K3bDirView::setupFinalize( K3bDeviceManager *dm )
{
  m_fileTreeView->addCdDeviceBranches( dm );
}


void K3bDirView::slotDeviceActivated( K3bDevice* dev )
{
   m_fileView->hide();
   m_filmView->hide();
   m_cdView->show();
   m_cdView->showCdView( dev->devicename() );
}


void K3bDirView::slotMountDevice( const QString& device )
{
  K3bDevice* dev = k3bMain()->deviceManager()->deviceByName( device );
  QString mountPoint = dev->mountPoint();
  if( !mountPoint.isEmpty() ){
    KIO::mount( true, "autofs", dev->ioctlDevice(), mountPoint, true );
  }
  const KURL url = KURL( mountPoint);
  slotDirActivated( url );
}

void K3bDirView::slotCheckDvd( const QString& device ) {
    // cdview calls this so hide cd
    m_cdView->hide();
    K3bDevice *dev = k3bMain()->deviceManager()->deviceByName( device );
    // if insert disc can't be read a notSupportDisc(string device) signal is emitted -> mount cd and show data
    m_filmView->setDevice( device );
    m_filmView->showAndCheck();
}

void K3bDirView::slotUpdateURLCombo( const KURL& url )
{
  m_urlCombo->setEditText( url.path() );
}


void K3bDirView::slotDirActivated( const QString& url )
{
  m_urlCombo->insertItem( url, 0 );
  slotDirActivated( KURL(url) );
}


void K3bDirView::slotDirActivated( const KURL& url )
{
  m_fileView->setUrl(url, true);
  m_urlCombo->setEditText( url.path() );

  m_fileView->show();

  m_cdView->hide();
  m_filmView->hide();
}


#include "k3bdirview.moc"
