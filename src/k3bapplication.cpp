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


#include "k3bapplication.h"
#include "k3b.h"
#include "k3binterface.h"
#include "k3bwriterspeedverificationdialog.h"

#include <k3bcore.h>
#include <k3bdevicemanager.h>
#include <k3bexternalbinmanager.h>
#include <k3bdefaultexternalprograms.h>
#include <k3bglobals.h>
#include <k3bversion.h>
#include <songdb/k3bsongmanager.h>
#include <k3bdoc.h>
#include <k3bsystemproblemdialog.h>
#include <k3bthread.h>
#include <k3bpluginmanager.h>
#include <k3bthememanager.h>
#include <k3bmsf.h>

#include <ktip.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kstandarddirs.h>


K3bApplication* K3bApplication::s_k3bApp = 0;



K3bApplication::K3bApplication()
  : KApplication(),
    m_interface(0),
    m_mainWindow(0)
{
  m_core = new K3bCore( aboutData()->version(), config(), this );

  m_songManager = K3bSongManager::instance();  // this is bad stuff!!!

  connect( m_core, SIGNAL(initializationInfo(const QString&)),
	   SIGNAL(initializationInfo(const QString&)) );
  s_k3bApp = this;

  K3bThemeManager* themeManager = new K3bThemeManager( this );
  themeManager->loadThemes();
  themeManager->readConfig( config() );

  connect( this, SIGNAL(shutDown()), SLOT(slotShutDown()) );
}


K3bApplication::~K3bApplication()
{
  // we must not delete m_mainWindow here, QApplication takes care of it
  delete m_interface;
  delete m_songManager;  // this is bad stuff!!!
}


K3bMainWindow* K3bApplication::k3bMainWindow() const
{
  return m_mainWindow;
}


void K3bApplication::init()
{
  // load the plugins before doing anything else
  // they might add external bins
  K3bPluginManager* pluginManager = new K3bPluginManager( this );
  pluginManager->loadAll();

  m_core->init();

  emit initializationInfo( i18n("Reading local Song database...") );
  config()->setGroup( "General Options" );

  QString filename = config()->readPathEntry( "songlistPath", locateLocal("data", "k3b") + "/songlist.xml" );
  K3bSongManager::instance()->load( filename );

  emit initializationInfo( i18n("Creating GUI...") );

  m_mainWindow = new K3bMainWindow();
  setMainWidget( m_mainWindow );

  if( dcopClient()->attach() ) {
    dcopClient()->registerAs( "k3b" );
    m_interface = new K3bInterface( m_mainWindow );
    dcopClient()->setDefaultObject( m_interface->objId() );
  }
  else {
    kdDebug() << "(K3bApplication) unable to attach to dcopserver!" << endl;
  }

  m_mainWindow->show();

  emit initializationInfo( i18n("Ready.") );

  config()->setGroup( "General Options" );

  //
  // K3b is able to autodetect most every device feature. One exception is the writing speed
  // So the first time K3b is started with some device configuration we ask the user to verify
  // the writing speeds if any writers are installed
  //
  // To check if there is a writer whose config has not been verified yet we search for a config
  // entry for every writer. If there is a config entry from a K3b version >= 0.10.99 it should
  // already be verified.
  //
  K3bVersion configVersion( config()->readEntry( "config version", "0.1" ) );
  QPtrList<K3bCdDevice::CdDevice> wlist( k3bcore->deviceManager()->cdWriter() );
  bool needToVerify = ( configVersion < K3bVersion( 0, 10, 99 ) );
  if( !needToVerify ) {
    // search the config
    config()->setGroup( "Devices" );

    int devNum = 1;
    QStringList list = config()->readListEntry( "Device1" );
    devNum = 1;
    while( !list.isEmpty() ) {
      if( K3bDevice* dev = k3bcore->deviceManager()->deviceByName( list[0] ) ) {
	if( dev->vendor() == list[1] || dev->description() == list[2] ) {
	  wlist.removeRef( dev );
	}
      }
    
      devNum++;
      list = config()->readListEntry( QString( "Device%1" ).arg( devNum ) );
    }

    // the devices left in wlist are the once not verified yet
    needToVerify = !wlist.isEmpty();
  }

  if( needToVerify ) {
    K3bWriterSpeedVerificationDialog::verify( wlist, m_mainWindow );
  }


  config()->setGroup( "General Options" );
  if( config()->readBoolEntry( "check system config", true ) ) {
    emit initializationInfo( i18n("Checking System") );
    K3bSystemProblemDialog::checkSystem();
  }

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if( args->isSet( "datacd" ) ) {
    // create new data project and add all arguments
    K3bDoc* doc = m_mainWindow->slotNewDataDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "audiocd" ) ) {
    // create new audio project and add all arguments
    K3bDoc* doc = m_mainWindow->slotNewAudioDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "mixedcd" ) ) {
    // create new audio project and add all arguments
    K3bDoc* doc = m_mainWindow->slotNewMixedDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "videocd" ) ) {
    // create new audio project and add all arguments
    K3bDoc* doc = m_mainWindow->slotNewVcdDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "emovixcd" ) ) {
    // create new audio project and add all arguments
    K3bDoc* doc = m_mainWindow->slotNewMovixDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "datadvd" ) ) {
    // create new audio project and add all arguments
    K3bDoc* doc = m_mainWindow->slotNewDvdDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "emovixdvd" ) ) {
    // create new audio project and add all arguments
    K3bDoc* doc = m_mainWindow->slotNewMovixDvdDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "cdimage" ) ) {
    if ( args->count() == 1 )
      m_mainWindow->slotWriteCdImage( args->url(0) );
    else
      m_mainWindow->slotWriteCdImage();
  }
  else if(args->count()) {
    for( int i = 0; i < args->count(); i++ ) {
      m_mainWindow->openDocument( args->url(i) );
    }
  }

  if( args->isSet("copycd") )
    m_mainWindow->slotCdCopy();
  else if( args->isSet("clonecd") )
    m_mainWindow->slotCdClone();
  else if( args->isSet("erasecd") )
    m_mainWindow->slotBlankCdrw();
  else if( args->isSet("formatdvd") )
    m_mainWindow->slotFormatDvd();

  args->clear();

  KTipDialog::showTip( m_mainWindow );
}


void K3bApplication::slotShutDown()
{
  songManager()->save();

  K3bThread::waitUntilFinished();

  k3bthememanager->saveConfig( config() );
}

#include "k3bapplication.moc"
