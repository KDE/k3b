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


#include "k3bapplication.h"
#include "k3b.h"
#include "k3bwriterspeedverificationdialog.h"
#include "k3bsplash.h"
#include "k3baudioserver.h"
#include "k3binterface.h"

#include <k3bcore.h>
#include <k3bdevicemanager.h>
#include <k3bexternalbinmanager.h>
#include <k3bdefaultexternalprograms.h>
#include <k3bglobals.h>
#include <k3bversion.h>
#include <k3bdoc.h>
#include "k3bsystemproblemdialog.h"
#include <k3bthread.h>
#include <k3bpluginmanager.h>
#include <k3bthememanager.h>
#include <k3bmsf.h>
#include <k3bmovixprogram.h>

#include <ktip.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kstandarddirs.h>
#include <kstartupinfo.h>
#include <kmessagebox.h>

#include <qguardedptr.h>
#include <qtimer.h>


K3bApplication::Core* K3bApplication::Core::s_k3bAppCore = 0;


K3bApplication::K3bApplication()
  : KUniqueApplication(),
    m_interface(0),
    m_mainWindow(0),
    m_needToInit(true)
{
  // insert library i18n data
  KGlobal::locale()->insertCatalogue( "libk3bdevice" );
  KGlobal::locale()->insertCatalogue( "libk3b" );

  m_core = new Core( this );

  // TODO: move to K3bCore?
  // from this point on available through K3bAudioServer::instance()
  m_audioServer = new K3bAudioServer( this, "K3bAudioServer" );

  connect( m_core, SIGNAL(initializationInfo(const QString&)),
	   SIGNAL(initializationInfo(const QString&)) );

  connect( this, SIGNAL(shutDown()), SLOT(slotShutDown()) );
}


K3bApplication::~K3bApplication()
{
  // we must not delete m_mainWindow here, QApplication takes care of it
}


void K3bApplication::init()
{
  QGuardedPtr<K3bSplash> splash;
  if( !isRestored() ) {
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    config()->setGroup( "General Options" );
    if( config()->readBoolEntry("Show splash", true) && args->isSet( "splash" ) ) {
      splash = new K3bSplash( 0 );
      splash->connect( this, SIGNAL(initializationInfo(const QString&)), SLOT(addInfo(const QString&)) );
      
      // kill the splash after 5 seconds
      QTimer::singleShot( 5000, splash, SLOT(close()) );
      
      splash->show();
    }
  }

  //
  // Load device, external programs, and stuff.
  //
  m_core->init();
  m_core->readSettings( config() );

  m_core->deviceManager()->printDevices();

  config()->setGroup( "General Options" );
  m_audioServer->setOutputMethod( config()->readEntry( "Audio Output System", "arts" ).local8Bit() );

  emit initializationInfo( i18n("Creating GUI...") );

  m_mainWindow = new K3bMainWindow();
  m_core->m_mainWindow = m_mainWindow;

  m_interface = new K3bInterface( m_mainWindow );
  dcopClient()->setDefaultObject( m_interface->objId() );

  if( isRestored() ) {
    // we only have one single mainwindow to restore  
    m_mainWindow->restore(1);
  }
  else {
    setMainWidget( m_mainWindow );

    m_mainWindow->show();

    emit initializationInfo( i18n("Ready.") );

    config()->setGroup( "General Options" );

    //
    // K3b is able to autodetect most every device feature. One exception is the writing speed
    // So the first time K3b is started with some device configuration we ask the user to verify
    // the writing speeds if any writers are installed
    //
    // To check if there is a writer whose config has not been verified yet we search for a config
    // entry for every writer containing a valid writing speed entry (we changed from cd writing 
    // speed multiplicator to KB/s)
    //
    K3bVersion configVersion( config()->readEntry( "config version", "0.1" ) );
    QPtrList<K3bDevice::Device> wlist( k3bcore->deviceManager()->cdWriter() );
    bool needToVerify = ( configVersion < K3bVersion( 0, 10, 99 ) );
    if( !needToVerify ) {
      // search the config
      config()->setGroup( "Devices" );

      for( QPtrListIterator<K3bDevice::Device> it( k3bcore->deviceManager()->cdWriter() ); *it; ++it ) {
	K3bDevice::Device* dev = *it;
	QString configEntryName = dev->vendor() + " " + dev->description();
	QStringList list = config()->readListEntry( configEntryName );
	if( list.count() > 1 && list[1].toInt() > 175 )
	  wlist.removeRef( dev );
      }
    
      // the devices left in wlist are the once not verified yet
      needToVerify = !wlist.isEmpty();
    }

    if( needToVerify && !wlist.isEmpty() ) {
      if( splash )
	splash->close();
      K3bWriterSpeedVerificationDialog::verify( wlist, m_mainWindow );
    }


    config()->setGroup( "General Options" );
    if( config()->readBoolEntry( "check system config", true ) ) {
      emit initializationInfo( i18n("Checking System") );
      K3bSystemProblemDialog::checkSystem();
    }

    if( processCmdLineArgs() )
      KTipDialog::showTip( m_mainWindow );

    emit initializationDone();
  }
}


int K3bApplication::newInstance()
{
  if( m_needToInit ) {
    //    init();
    m_needToInit = false;
  }
  else
    processCmdLineArgs();

  return KUniqueApplication::newInstance();
}


bool K3bApplication::processCmdLineArgs()
{
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  bool showTips = true;

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
  else if( args->isSet( "videodvd" ) ) {
    // create new audio project and add all arguments
    K3bDoc* doc = m_mainWindow->slotNewVideoDvdDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "cdimage" ) ) {
    showTips = false;
    if( k3bcore->jobsRunning() == 0 ) {
      if ( args->count() == 1 )
	m_mainWindow->slotWriteCdImage( args->url(0) );
      else
	m_mainWindow->slotWriteCdImage();
    }
  }
  else if( args->isSet( "dvdimage" ) ) {
    showTips = false;
    if( k3bcore->jobsRunning() == 0 ) {
      if ( args->count() == 1 )
	m_mainWindow->slotWriteDvdIsoImage( args->url(0) );
      else
	m_mainWindow->slotWriteDvdIsoImage();
    }
  }
  else if( args->isSet( "image" ) && args->count() == 1 ) {
    showTips = false;
    if( k3bcore->jobsRunning() == 0 ) {
      if( K3b::filesize( args->url(0) ) > 1000*1024*1024 )
	m_mainWindow->slotWriteDvdIsoImage( args->url(0) );
      else
	m_mainWindow->slotWriteCdImage( args->url(0) );
    }
  }
  else if(args->count()) {
    for( int i = 0; i < args->count(); i++ ) {
      m_mainWindow->openDocument( args->url(i) );
    }
  }

  if( k3bcore->jobsRunning() == 0 ) {
    if( args->isSet("copycd") ) {
      showTips = false;
      m_mainWindow->slotCdCopy();
    }
    else if( args->isSet("copydvd") ) {
      showTips = false;
      m_mainWindow->slotDvdCopy();
    }
    else if( args->isSet("erasecd") ) {
      showTips = false;
      m_mainWindow->slotBlankCdrw();
    }
    else if( args->isSet("formatdvd") ) {
      showTips = false;
      m_mainWindow->slotFormatDvd();
    }
  }

  // FIXME: seems not like the right place...
  if( args->isSet( "ao" ) )
    if( !m_audioServer->setOutputMethod( args->getOption( "ao" ) ) )
      KMessageBox::error( m_mainWindow, i18n("Could not find Audio Output plugin '%1'").arg( args->getOption("ao") ) );

  args->clear();

  return showTips;
}


void K3bApplication::slotShutDown()
{
  K3bThread::waitUntilFinished();
}



K3bApplication::Core::Core( QObject* parent )
  : K3bCore( parent )
{
  s_k3bAppCore = this;
  m_themeManager = new K3bThemeManager( this );
}


K3bApplication::Core::~Core()
{
}


KConfig* K3bApplication::Core::config() const
{
  return kapp->config();
}


void K3bApplication::Core::init()
{
  emit initializationInfo( i18n("Loading all plugins...") );
  pluginManager()->loadAll();

  emit initializationInfo( i18n("Loading all themes...") );
  m_themeManager->loadThemes();

  emit initializationInfo( i18n("Searching for external programs...") );

  //
  // The eMovix program is a special case which is not part of
  // the default programs handled by K3bCore
  //
  externalBinManager()->addProgram( new K3bMovixProgram() );
  externalBinManager()->addProgram( new K3bNormalizeProgram() );
  K3b::addTranscodePrograms( externalBinManager() );
  K3b::addVcdimagerPrograms( externalBinManager() );

  externalBinManager()->search();

  emit initializationInfo( i18n("Scanning for CD devices...") );

  if( !deviceManager()->scanBus() )
    kdDebug() << "No Devices found!" << endl;
}


void K3bApplication::Core::readSettings( KConfig* cnf )
{
  K3bCore::readSettings( cnf );

  KConfig* c = cnf;
  if( !c )
    c = config();

  QString oldGrp = c->group();

  m_themeManager->readConfig( config() );

  c->setGroup( oldGrp );
}


void K3bApplication::Core::saveSettings( KConfig* cnf )
{
  K3bCore::saveSettings( cnf );

  KConfig* c = cnf;
  if( !c )
    c = config();

  QString oldGrp = c->group();

  m_themeManager->saveConfig( config() );

  c->setGroup( oldGrp );
}

#include "k3bapplication.moc"
