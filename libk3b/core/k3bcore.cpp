/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config-k3b.h>

#include "k3bcore.h"
#include "k3bjob.h"
#include "k3bmediacache.h"

#include "k3bdevicemanager.h"
#include "k3bexternalbinmanager.h"
#include "k3bdefaultexternalprograms.h"
#include "k3bglobals.h"
#include "k3bversion.h"
#include "k3bjob.h"
#include "k3bthreadwidget.h"
#include "k3bglobalsettings.h"
#include "k3bpluginmanager.h"

#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kaboutdata.h>
#include <kstandarddirs.h>
#include <kapplication.h>

#include <qthread.h>
#include <qmutex.h>
#include <QEvent>


static QThread* s_guiThreadHandle = 0;

// We cannot use QWaitCondition here since the event might be handled faster
// than the thread starts the waiting
class DeviceBlockingEventDoneCondition {
public:
    DeviceBlockingEventDoneCondition()
        : m_done(false) {
    }

    void done() {
        m_doneMutex.lock();
        m_done = true;
        m_doneMutex.unlock();
    }

    void wait() {
        while( true ) {
            m_doneMutex.lock();
            bool done = m_done;
            m_doneMutex.unlock();
            if( done )
                return;
        }
    }

private:
    QMutex m_doneMutex;
    bool m_done;
};


class DeviceBlockingEvent : public QEvent
{
public:
    DeviceBlockingEvent( bool block_, K3b::Device::Device* dev, DeviceBlockingEventDoneCondition* cond_, bool* success_ )
        : QEvent( QEvent::User ),
          block(block_),
          device(dev),
          cond(cond_),
          success(success_) {
    }

    bool block;
    K3b::Device::Device* device;
    DeviceBlockingEventDoneCondition* cond;
    bool* success;
};


class K3b::Core::Private {
public:
    Private()
        : version( LIBK3B_VERSION ),
          mediaCache(0),
          deviceManager(0),
          externalBinManager(0),
          pluginManager(0),
          globalSettings(0) {
    }

    K3b::Version version;
    K3b::MediaCache* mediaCache;
    K3b::Device::DeviceManager* deviceManager;
    K3b::ExternalBinManager* externalBinManager;
    K3b::PluginManager* pluginManager;
    K3b::GlobalSettings* globalSettings;

    QList<K3b::Job*> runningJobs;
    QList<K3b::Device::Device*> blockedDevices;
};



K3b::Core* K3b::Core::s_k3bCore = 0;



K3b::Core::Core( QObject* parent )
    : QObject( parent )
{
    d = new Private();

    if( s_k3bCore )
        qFatal("ONLY ONE INSTANCE OF K3BCORE ALLOWED!");
    s_k3bCore = this;

    // ew are probably constructed in the GUi thread :(
    s_guiThreadHandle = QThread::currentThread();

    // create the thread widget instance in the GUI thread
    K3b::ThreadWidget::instance();
}


K3b::Core::~Core()
{
    s_k3bCore = 0;

    delete d->globalSettings;
    delete d;
}


K3b::MediaCache* K3b::Core::mediaCache() const
{
    const_cast<K3b::Core*>(this)->initMediaCache();
    return d->mediaCache;
}


K3b::Device::DeviceManager* K3b::Core::deviceManager() const
{
    const_cast<K3b::Core*>(this)->initDeviceManager();
    return d->deviceManager;
}


K3b::ExternalBinManager* K3b::Core::externalBinManager() const
{
    const_cast<K3b::Core*>(this)->initExternalBinManager();
    return d->externalBinManager;
}


K3b::PluginManager* K3b::Core::pluginManager() const
{
    const_cast<K3b::Core*>(this)->initPluginManager();
    return d->pluginManager;
}


K3b::GlobalSettings* K3b::Core::globalSettings() const
{
    const_cast<K3b::Core*>(this)->initGlobalSettings();
    return d->globalSettings;
}


K3b::Version K3b::Core::version() const
{
    return d->version;
}


void K3b::Core::init()
{
    initGlobalSettings();
    initExternalBinManager();
    initDeviceManager();
    initPluginManager();

    // load the plugins before doing anything else
    // they might add external bins
    pluginManager()->loadAll();

    externalBinManager()->search();

    deviceManager()->scanBus();

    mediaCache()->buildDeviceList( deviceManager() );
}


void K3b::Core::initGlobalSettings()
{
    if( !d->globalSettings )
        d->globalSettings = new K3b::GlobalSettings();
}


void K3b::Core::initExternalBinManager()
{
    if( !d->externalBinManager ) {
        d->externalBinManager = new K3b::ExternalBinManager( this );
        K3b::addDefaultPrograms( d->externalBinManager );
    }
}


void K3b::Core::initDeviceManager()
{
    if( !d->deviceManager )
        d->deviceManager = new K3b::Device::DeviceManager( this );
}


void K3b::Core::initMediaCache()
{
    if ( !d->mediaCache ) {
        // create the media cache but do not connect it to the device manager
        // yet to speed up application start. We connect it in init()
        // once the devicemanager has scanned for devices.
        d->mediaCache = new K3b::MediaCache( this );
    }
}


void K3b::Core::initPluginManager()
{
    if( !d->pluginManager )
        d->pluginManager = new K3b::PluginManager( this );
}


void K3b::Core::readSettings( KSharedConfig::Ptr c )
{
    globalSettings()->readSettings( c->group( "General Options" ) );
    deviceManager()->readConfig( c->group( "Devices" ) );
    externalBinManager()->readConfig( c->group( "External Programs" ) );
}


void K3b::Core::saveSettings( KSharedConfig::Ptr c )
{
    KConfigGroup grp( c, "General Options" );
    grp.writeEntry( "config version", version().toString() );

    deviceManager()->saveConfig( c->group( "Devices" ) );
    externalBinManager()->saveConfig( c->group( "External Programs" ) );
    d->globalSettings->saveSettings( c->group( "General Options" ) );
}


void K3b::Core::registerJob( K3b::Job* job )
{
    d->runningJobs.append( job );
    emit jobStarted( job );
    if( K3b::BurnJob* bj = dynamic_cast<K3b::BurnJob*>( job ) )
        emit burnJobStarted( bj );
}


void K3b::Core::unregisterJob( K3b::Job* job )
{
    d->runningJobs.removeAll( job );
    emit jobFinished( job );
    if( K3b::BurnJob* bj = dynamic_cast<K3b::BurnJob*>( job ) )
        emit burnJobFinished( bj );
}


bool K3b::Core::jobsRunning() const
{
    return !d->runningJobs.isEmpty();
}


QList<K3b::Job*> K3b::Core::runningJobs() const
{
    return d->runningJobs;
}


bool K3b::Core::blockDevice( K3b::Device::Device* dev )
{
    if( QThread::currentThread() == s_guiThreadHandle ) {
        return internalBlockDevice( dev );
    }
    else {
        bool success = false;
        DeviceBlockingEventDoneCondition w;
        QApplication::postEvent( this, new DeviceBlockingEvent( true, dev, &w, &success ) );
        w.wait();
        return success;
    }
}


void K3b::Core::unblockDevice( K3b::Device::Device* dev )
{
    if( QThread::currentThread() == s_guiThreadHandle ) {
        internalUnblockDevice( dev );
    }
    else {
        DeviceBlockingEventDoneCondition w;
        QApplication::postEvent( this, new DeviceBlockingEvent( false, dev, &w, 0 ) );
        w.wait();
    }
}


bool K3b::Core::internalBlockDevice( K3b::Device::Device* dev )
{
    if( !d->blockedDevices.contains( dev ) ) {
        d->blockedDevices.append( dev );
        return true;
    }
    else
        return false;
}


void K3b::Core::internalUnblockDevice( K3b::Device::Device* dev )
{
    d->blockedDevices.removeAll( dev );
}


bool K3b::Core::deviceBlocked( Device::Device* dev ) const
{
    return d->blockedDevices.contains( dev );
}


void K3b::Core::customEvent( QEvent* e )
{
    if( DeviceBlockingEvent* de = dynamic_cast<DeviceBlockingEvent*>(e) ) {
        if( de->block )
            *de->success = internalBlockDevice( de->device );
        else
            internalUnblockDevice( de->device );
        de->cond->done();
    }
}

#include "k3bcore.moc"
