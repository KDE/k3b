/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bmediacache.h"
#include "k3bmedium.h"
#include "k3bmedium_p.h"
#include "k3bcddb.h"

#include <k3bdevicemanager.h>
#include <k3bdeviceglobals.h>
#include <k3bscsicommand.h>
#include <k3bcore.h>

#include <kdebug.h>
#include <kapplication.h>
#include <klocale.h>

#include <qthread.h>
#include <qmutex.h>
#include <qevent.h>
#include <QCustomEvent>
#include <krandom.h>

#include <libkcddb/client.h>




K3bMediaCache::DeviceEntry::DeviceEntry( K3bMediaCache* c, K3bDevice::Device* dev )
    : medium(dev),
      blockedId(0),
      cache(c)
{
    thread = new K3bMediaCache::PollThread( this );
    connect( thread, SIGNAL( mediumChanged( K3bDevice::Device* ) ),
             c, SLOT( _k_mediumChanged( K3bDevice::Device* ) ),
             Qt::QueuedConnection );
    connect( thread, SIGNAL( checkingMedium( K3bDevice::Device*, QString ) ),
             c, SIGNAL( checkingMedium( K3bDevice::Device*, QString ) ),
             Qt::QueuedConnection );
}


K3bMediaCache::DeviceEntry::~DeviceEntry()
{
    delete thread;
}


void K3bMediaCache::PollThread::run()
{
    while( m_deviceEntry->blockedId == 0 ) {
        bool unitReady = m_deviceEntry->medium.device()->testUnitReady();
        bool mediumCached = ( m_deviceEntry->medium.diskInfo().diskState() != K3bDevice::STATE_NO_MEDIA );

        //
        // we only get the other information in case the disk state changed or if we have
        // no info at all (FIXME: there are drives around that are not able to provide a proper
        // disk state)
        //
        if( m_deviceEntry->medium.diskInfo().diskState() == K3bDevice::STATE_UNKNOWN ||
            unitReady != mediumCached ) {

            if( m_deviceEntry->blockedId == 0 )
                emit checkingMedium( m_deviceEntry->medium.device(), QString() );

            //
            // The medium has changed. We need to update the information.
            //
            K3bMedium m( m_deviceEntry->medium.device() );
            m.update();

            // block the info since it is not valid anymore
            m_deviceEntry->mutex.lock();

            m_deviceEntry->medium = m;

            //
            // inform the media cache about the media change
            //
            if( m_deviceEntry->blockedId == 0 )
                emit mediumChanged( m_deviceEntry->medium.device() );

            // the information is valid. let the info go.
            m_deviceEntry->mutex.unlock();
        }

        if( m_deviceEntry->blockedId == 0 )
            QThread::sleep( 2 );
    }
}





// ////////////////////////////////////////////////////////////////////////////////
// MEDIA CACHE IMPL
// ////////////////////////////////////////////////////////////////////////////////


class K3bMediaCache::Private
{
public:
    QMap<K3bDevice::Device*, DeviceEntry*> deviceMap;
    KCDDB::Client cddbClient;

    K3bMediaCache* q;

    void _k_mediumChanged( K3bDevice::Device* );
    void _k_cddbJobFinished( KJob* job );
};


// called from the device thread which updated the medium
void K3bMediaCache::Private::_k_mediumChanged( K3bDevice::Device* dev )
{
    if ( q->medium( dev ).content() & K3bMedium::CONTENT_AUDIO ) {
        K3bCDDB::CDDBJob* job = K3bCDDB::CDDBJob::queryCddb( q->medium( dev ) );
        connect( job, SIGNAL( result( KJob* ) ),
                 q, SLOT( _k_cddbJobFinished( KJob* ) ) );
        emit q->checkingMedium( dev, i18n( "CDDB Lookup" ) );
    }
    else {
        emit q->mediumChanged( dev );
    }
}


// once the cddb job is finished the medium is really updated
void K3bMediaCache::Private::_k_cddbJobFinished( KJob* job )
{
    K3bCDDB::CDDBJob* cddbJob = dynamic_cast<K3bCDDB::CDDBJob*>( job );
    K3bMedium oldMedium = cddbJob->medium();

    // make sure the medium did not change during the job
    if ( oldMedium.sameMedium( q->medium( oldMedium.device() ) ) ) {
        if ( !job->error() ) {
            // update it
            deviceMap[oldMedium.device()]->medium.d->cddbInfo = cddbJob->cddbResult();
        }

        emit q->mediumChanged( oldMedium.device() );
    }
}



K3bMediaCache::K3bMediaCache( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    d->q = this;
}


K3bMediaCache::~K3bMediaCache()
{
    clearDeviceList();
    delete d;
}


int K3bMediaCache::blockDevice( K3bDevice::Device* dev )
{
    DeviceEntry* e = findDeviceEntry( dev );
    if( e ) {
        if( e->blockedId )
            return -1;
        else {
            // block the information
            e->mutex.lock();

            // create (hopefully) unique id
            e->blockedId = KRandom::random();

            // let the info go
            e->mutex.unlock();

            // wait for the thread to stop
            e->thread->wait();

            return e->blockedId;
        }
    }
    else
        return -1;
}


bool K3bMediaCache::unblockDevice( K3bDevice::Device* dev, int id )
{
    DeviceEntry* e = findDeviceEntry( dev );
    if( e && e->blockedId && e->blockedId == id ) {
        e->blockedId = 0;

        e->medium = K3bMedium( dev );
        emit mediumChanged( dev );

        // restart the poll thread
        e->thread->start();

        return true;
    }
    else
        return false;
}


bool K3bMediaCache::isBlocked( K3bDevice::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) )
        return ( e->blockedId != 0 );
    else
        return false;
}


K3bMedium K3bMediaCache::medium( K3bDevice::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        e->mutex.lock();
        K3bMedium m = e->medium;
        e->mutex.unlock();
        return m;
    }
    else
        return K3bMedium();
}


K3bDevice::DiskInfo K3bMediaCache::diskInfo( K3bDevice::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        e->mutex.lock();
        K3bDevice::DiskInfo di = e->medium.diskInfo();
        e->mutex.unlock();
        return di;
    }
    else
        return K3bDevice::DiskInfo();
}


K3bDevice::Toc K3bMediaCache::toc( K3bDevice::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        e->mutex.lock();
        K3bDevice::Toc toc = e->medium.toc();
        e->mutex.unlock();
        return toc;
    }
    else
        return K3bDevice::Toc();
}


K3bDevice::CdText K3bMediaCache::cdText( K3bDevice::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        e->mutex.lock();
        K3bDevice::CdText cdt = e->medium.cdText();
        e->mutex.unlock();
        return cdt;
    }
    else
        return K3bDevice::CdText();
}


QList<int> K3bMediaCache::writingSpeeds( K3bDevice::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        e->mutex.lock();
        QList<int> ws = e->medium.writingSpeeds();
        e->mutex.unlock();
        return ws;
    }
    else
        return QList<int>();
}


QString K3bMediaCache::mediumString( K3bDevice::Device* device, bool useContent )
{
    if( DeviceEntry* e = findDeviceEntry( device ) ) {
        return e->medium.shortString( useContent );
    }
    else
        return QString();
}


void K3bMediaCache::clearDeviceList()
{
    kDebug() << k_funcinfo;

    // make all the threads stop
    for( QMap<K3bDevice::Device*, DeviceEntry*>::iterator it = d->deviceMap.begin();
         it != d->deviceMap.end(); ++it ) {
        it.value()->blockedId = 1;
    }

    // and remove them
    for( QMap<K3bDevice::Device*, DeviceEntry*>::iterator it = d->deviceMap.begin();
         it != d->deviceMap.end(); ++it ) {
        kDebug() << k_funcinfo << " waiting for info thread " << it.key()->blockDeviceName() << " to finish";
        it.value()->thread->wait();
        delete it.value();
    }

    d->deviceMap.clear();
}


void K3bMediaCache::buildDeviceList( K3bDevice::DeviceManager* dm )
{
    // remember blocked ids
    QMap<K3bDevice::Device*, int> blockedIds;
    for( QMap<K3bDevice::Device*, DeviceEntry*>::iterator it = d->deviceMap.begin();
         it != d->deviceMap.end(); ++it )
        blockedIds.insert( it.key(), it.value()->blockedId );

    clearDeviceList();

    QList<K3bDevice::Device *> items(dm->allDevices());
    for( QList<K3bDevice::Device *>::const_iterator it = items.constBegin();
         it != items.constEnd(); ++it ) {
        d->deviceMap.insert( *it, new DeviceEntry( this, *it ) );
        QMap<K3bDevice::Device*, int>::const_iterator bi_it = blockedIds.constFind( *it );
        if( bi_it != blockedIds.constEnd() )
            d->deviceMap[*it]->blockedId = bi_it.value();
    }

    // start all the polling threads
    for( QMap<K3bDevice::Device*, DeviceEntry*>::iterator it = d->deviceMap.begin();
         it != d->deviceMap.end(); ++it ) {
        if( !it.value()->blockedId )
            it.value()->thread->start();
    }
}


K3bMediaCache::DeviceEntry* K3bMediaCache::findDeviceEntry( K3bDevice::Device* dev )
{
    QMap<K3bDevice::Device*, DeviceEntry*>::iterator it = d->deviceMap.find( dev );
    if( it != d->deviceMap.end() )
        return it.value();
    else
        return 0;
}


void K3bMediaCache::lookupCddb( K3bDevice::Device* dev )
{
    K3bMedium m = medium( dev );
    if ( m.content() & K3bMedium::CONTENT_AUDIO ) {
        K3bCDDB::CDDBJob* job = K3bCDDB::CDDBJob::queryCddb( m );
        connect( job, SIGNAL( result( KJob* ) ),
                 this, SLOT( _k_cddbJobFinished( KJob* ) ) );
        emit checkingMedium( dev, i18n( "CDDB Lookup" ) );
    }
}

#include "k3bmediacache.moc"
