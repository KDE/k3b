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

// KDE-includes
#include <kmimetype.h>
#include <kcursor.h>
#include <kfiledetailview.h>
#include <kfileviewitem.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <kurl.h>

#include "kiotree/kiotree.h"
#include "kiotree/kiotreemodule.h"
#include "kiotree/kiotreeitem.h"
#include "kiotree/kiotreetoplevelitem.h"
#include "audio/k3bcdview.h"
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
}

K3bDirView::~K3bDirView(){
        delete m_fileView;
        delete m_cdView;
}

void K3bDirView::slotViewChanged( KFileView* newView )
{
  newView->setSelectionMode( KFile::Extended );
  if( KListView* _x = dynamic_cast<KListView*>( newView->widget() ) )
    _x->setDragEnabled( true );
}

void K3bDirView::slotDirActivated( const KURL& url )
{
     qDebug("show url:" + url.path() );
	  // hack due to I dont know how to setup the drives proper
	  //if (!m_initialized)
	  //		setupAudioDrives();
	  		
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

void K3bDirView::setupAudioDrives(K3bDeviceManager *dm){
    //m_initialized = true;
    //m_kiotree->takeItem(m_kiotree->selectedItem() );
    //K3bDeviceManager *dm = app->deviceManager();
    K3bDevice *dev;
    KURL result;
    QList<K3bDevice> devices = dm->readingDevices();
    for ( dev=devices.first(); dev != 0; dev=devices.next() ){
			KURL url = KURL( dev->devicename() );
			url.setProtocol("k3b_cdview");
			result = url;
         m_kiotree->addTopLevelDir( url, "Audio CD " + dev->vendor() + "/" + dev->description() );
    }
    devices = dm->burningDevices();
    for ( dev=devices.first(); dev != 0; dev=devices.next() ){
			KURL url = KURL(dev->devicename() );
			url.setProtocol("k3b_cdview");
         m_kiotree->addTopLevelDir( url, "Audio CD" + dev->vendor() + "/" + dev->description() );
    }
    //return result;
}