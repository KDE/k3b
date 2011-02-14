/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010-2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bapplication.h"
#include "k3b.h"
#include "k3bsplash.h"
#include "k3bprojectmanager.h"
#include "k3bappdevicemanager.h"
#include "k3blsofwrapperdialog.h"
#include "config-k3b.h"

#include "k3bcore.h"
#include "k3bdevicemanager.h"
#include "k3bthread.h"
#ifdef ENABLE_HAL_SUPPORT
#include "k3bhalconnection.h"
#endif
#include "k3bexternalbinmanager.h"
#include "k3bdefaultexternalprograms.h"
#include "k3bglobals.h"
#include "k3bversion.h"
#include "k3bdoc.h"
#include "k3bsystemproblemdialog.h"
#include "k3bpluginmanager.h"
#include "k3bthememanager.h"
#include "k3bmsf.h"
#include "k3bmovixprogram.h"
#include "k3bview.h"
#include "k3bjob.h"
#include "k3bmediacache.h"

#include <KAboutData>
#include <KCmdLineArgs>
#include <KConfig>
#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KNotification>
#include <QTimer>


K3b::Application::Core* K3b::Application::Core::s_k3bAppCore = 0;


K3b::Application::Application()
    : KUniqueApplication(),
      m_mainWindow( 0 ),
      m_initialized( false )
{
    // insert library i18n data
    KGlobal::locale()->insertCatalog( "libk3bdevice" );
    KGlobal::locale()->insertCatalog( "libk3b" );

    m_core = new Core( this );
    
    KConfigGroup generalOptions( KGlobal::config(), "General Options" );

    Splash* splash = 0;
    if( !qApp->isSessionRestored() ) {
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

        if( generalOptions.readEntry("Show splash", true) && args->isSet( "splash" ) ) {
            // we need the correct splash pic
            m_core->m_themeManager->readConfig( generalOptions );

            splash = new Splash( 0 );
            splash->show();
        }
    }

    m_mainWindow = new MainWindow();
    m_core->m_mainWindow = m_mainWindow;

    if( isSessionRestored() ) {
        // we only have one single mainwindow to restore
        m_mainWindow->restore(1);
    }
    else {
        m_mainWindow->show();
    }
    
    processEvents();
    
    if( splash )
        QMetaObject::invokeMethod( splash, "close", Qt::QueuedConnection );

    qRegisterMetaType<KSharedConfig::Ptr>( "KSharedConfig::Ptr" );
    //
    // Load device, external programs, and stuff.
    //
    QMetaObject::invokeMethod( m_core, "init", Qt::QueuedConnection );
    QMetaObject::invokeMethod( m_core, "readSettings", Qt::QueuedConnection, Q_ARG( KSharedConfig::Ptr, KGlobal::config() ) );
    QMetaObject::invokeMethod( m_core->deviceManager(), "printDevices", Qt::QueuedConnection );
    QMetaObject::invokeMethod( this, "init", Qt::QueuedConnection );

    connect( this, SIGNAL(aboutToQuit()), SLOT(slotShutDown()) );
}


K3b::Application::~Application()
{
    // we must not delete m_mainWindow here, QApplication takes care of it
}


void K3b::Application::init()
{
    if( !isSessionRestored() ) {
        
        if( SystemProblemDialog::readCheckSystemConfig() ) {
            SystemProblemDialog::checkSystem( m_mainWindow );
        }
        
        QMetaObject::invokeMethod( this, "processCmdLineArgs", Qt::QueuedConnection );
    }

    // write the current version to make sure checks such as SystemProblemDialog::readCheckSystemConfig
    // use a proper value
    KConfigGroup generalOptions( KGlobal::config(), "General Options" );
    generalOptions.writeEntry( "config version", QString(m_core->version()) );
    
    m_initialized = true;
}


int K3b::Application::newInstance()
{
    processCmdLineArgs();
    
    return KUniqueApplication::newInstance();
}


void K3b::Application::processCmdLineArgs()
{
    // There were cases when newInstance() has been called before init()
    // (when user run k3b two times at once). It resulted in crash. So we
    // check here if m_mainWindow is initalized and if not, we go back.
    if( !m_initialized || !m_mainWindow )
        return;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    bool dialogOpen = false;

    if( k3bcore->jobsRunning() > 0 ) {
        KNotification::event( "Busy",
                              i18n("K3b is busy"),
                              i18n("K3b is currently busy and cannot start any other operations.") );
        return;
    }

    Doc* doc = 0;
    if( args->isSet( "data" ) ) {
        doc = m_mainWindow->slotNewDataDoc();
    }
    else if( args->isSet( "audiocd" ) ) {
        doc = m_mainWindow->slotNewAudioDoc();
    }
    else if( args->isSet( "mixedcd" ) ) {
        doc = m_mainWindow->slotNewMixedDoc();
    }
    else if( args->isSet( "videocd" ) ) {
        doc = m_mainWindow->slotNewVcdDoc();
    }
    else if( args->isSet( "emovix" ) ) {
        doc = m_mainWindow->slotNewMovixDoc();
    }
    else if( args->isSet( "videodvd" ) ) {
        doc = m_mainWindow->slotNewVideoDvdDoc();
    }

    // if we created a doc the urls are used to populate it
    if( doc ) {
        KUrl::List urls;
        for( int i = 0; i < args->count(); i++ )
            urls.append( args->url(i) );
        dynamic_cast<View*>( doc->view() )->addUrls( urls );
    }
    // otherwise we open them as documents
    else {
        for( int i = 0; i < args->count(); i++ ) {
            m_mainWindow->openDocument( args->url(i) );
        }
    }

    // we only allow one dialog to be opened
    if( args->isSet( "image" ) ) {
        dialogOpen = true;
        if( k3bcore->jobsRunning() == 0 ) {
            m_mainWindow->slotWriteImage( KUrl(args->getOption( "image" ) ) );
        }
    }
    else if( args->isSet("copy") ) {
        dialogOpen = true;
        m_mainWindow->mediaCopy( m_core->deviceManager()->findDeviceByUdi( args->getOption( "copy"  ) ) );
    }
    else if( args->isSet("format") ) {
        dialogOpen = true;
        m_mainWindow->formatMedium( m_core->deviceManager()->findDeviceByUdi( args->getOption( "format" ) ) );
    }

    // no dialog used here
    if( args->isSet( "cddarip" ) ) {
        m_mainWindow->cddaRip( m_core->deviceManager()->findDeviceByUdi( args->getOption( "cddarip" ) ) );
    }
    else if( args->isSet( "videodvdrip" ) ) {
        m_mainWindow->videoDvdRip( m_core->deviceManager()->findDeviceByUdi( args->getOption( "videodvdrip" ) ) );
    }
    else if( args->isSet( "videocdrip" ) ) {
        m_mainWindow->videoCdRip( m_core->deviceManager()->findDeviceByUdi(  args->getOption( "videocdrip" ) ) );
    }

    if( !dialogOpen && args->isSet( "burn" ) ) {
        if( m_core->projectManager()->activeDoc() ) {
            dialogOpen = true;
            static_cast<View*>( m_core->projectManager()->activeDoc()->view() )->slotBurn();
        }
    }

    args->clear();
}


void K3b::Application::slotShutDown()
{
    k3bcore->mediaCache()->clearDeviceList();
    Thread::waitUntilFinished();
}



K3b::Application::Core::Core( QObject* parent )
    : K3b::Core( parent )
{
    s_k3bAppCore = this;
    m_themeManager = new ThemeManager( this );
    m_projectManager = new ProjectManager( this );
    // we need the themes on startup (loading them is fast anyway :)
    m_themeManager->loadThemes();
}


K3b::Application::Core::~Core()
{
}


void K3b::Application::Core::init()
{
    //
    // The eMovix program is a special case which is not part of
    // the default programs handled by K3b::Core
    //
    externalBinManager()->addProgram( new MovixProgram() );
    externalBinManager()->addProgram( new NormalizeProgram() );
    addTranscodePrograms( externalBinManager() );
    addVcdimagerPrograms( externalBinManager() );

    K3b::Core::init();

    connect( deviceManager(), SIGNAL(changed(K3b::Device::DeviceManager*)),
             mediaCache(), SLOT(buildDeviceList(K3b::Device::DeviceManager*)) );
    // FIXME: move this to libk3b
    appDeviceManager()->setMediaCache( mediaCache() );
}


void K3b::Application::Core::readSettings( KSharedConfig::Ptr cnf )
{
    K3b::Core::readSettings( cnf );
    m_themeManager->readConfig( cnf->group( "General Options" ) );
}


void K3b::Application::Core::saveSettings( KSharedConfig::Ptr cnf )
{
    K3b::Core::saveSettings( cnf );
    m_themeManager->saveConfig( cnf->group( "General Options" ) );
}


K3b::AppDeviceManager* K3b::Application::Core::appDeviceManager() const
{
    return static_cast<AppDeviceManager*>( deviceManager() );
}


K3b::Device::DeviceManager* K3b::Application::Core::createDeviceManager() const
{
    // our very own special device manager
    return new AppDeviceManager( const_cast<Application::Core*>( this ) );
}


bool K3b::Application::Core::internalBlockDevice( K3b::Device::Device* dev )
{
    if( K3b::Core::internalBlockDevice( dev ) ) {
        if( mediaCache() ) {
            m_deviceBlockMap[dev] = mediaCache()->blockDevice( dev );
        }

#ifdef ENABLE_HAL_SUPPORT
        if( Device::HalConnection::instance()->lock( dev ) != Device::HalConnection::org_freedesktop_Hal_Success )
            kDebug() << "(K3b::InterferingSystemsHandler) HAL lock failed.";
#endif

        //
        // Check if the device is in use
        //
        // FIXME: Use the top level widget as parent
        LsofWrapperDialog::checkDevice( dev );

        return true;
    }
    else
        return false;
}


void K3b::Application::Core::internalUnblockDevice( K3b::Device::Device* dev )
{
    if( mediaCache() ) {
        mediaCache()->unblockDevice( dev, m_deviceBlockMap[dev] );
        m_deviceBlockMap.remove( dev );
    }

#ifdef ENABLE_HAL_SUPPORT
    Device::HalConnection::instance()->unlock( dev );
#endif

    K3b::Core::internalUnblockDevice( dev );
}

#include "k3bapplication.moc"
