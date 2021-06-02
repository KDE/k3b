/*

    SPDX-FileCopyrightText: 2005-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include "k3bmediacache.h"
#include "k3bmediacache_p.h"
#include "k3bmedium.h"
#include "k3bmedium_p.h"
#include "k3bcddb.h"
#include "k3bdevicemanager.h"
#include "k3bdeviceglobals.h"
#include "k3bcore.h"
#include "k3b_i18n.h"

#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QEvent>
#include <QRandomGenerator>

#include <KCddb/Client>



K3b::MediaCache::DeviceEntry::DeviceEntry( K3b::MediaCache* c, K3b::Device::Device* dev )
    : medium(dev),
      blockedId(0),
      cache(c)
{
    thread = new K3b::MediaCache::PollThread( this );
    connect( thread, SIGNAL(mediumChanged(K3b::Device::Device*)),
             c, SLOT(_k_mediumChanged(K3b::Device::Device*)),
             Qt::QueuedConnection );
    connect( thread, SIGNAL(checkingMedium(K3b::Device::Device*,QString)),
             c, SIGNAL(checkingMedium(K3b::Device::Device*,QString)),
             Qt::QueuedConnection );
}


K3b::MediaCache::DeviceEntry::~DeviceEntry()
{
    delete thread;
}


void K3b::MediaCache::PollThread::run()
{
    while( m_deviceEntry->blockedId == 0 ) {
        bool unitReady = m_deviceEntry->medium.device()->testUnitReady();
        bool mediumCached = ( m_deviceEntry->medium.diskInfo().diskState() != K3b::Device::STATE_NO_MEDIA );

        //
        // we only get the other information in case the disk state changed or if we have
        // no info at all (FIXME: there are drives around that are not able to provide a proper
        // disk state)
        //
        if( m_deviceEntry->medium.diskInfo().diskState() == K3b::Device::STATE_UNKNOWN ||
            unitReady != mediumCached ) {

            if( m_deviceEntry->blockedId == 0 )
                emit checkingMedium( m_deviceEntry->medium.device(), QString() );

            //
            // we block for writing before the update
            // This is important to make sure we do not overwrite a reset operation
            //
            m_deviceEntry->writeMutex.lock();

            //
            // The medium has changed. We need to update the information.
            //
            K3b::Medium m( m_deviceEntry->medium.device() );
            m.update();

            // block the info since it is not valid anymore
            m_deviceEntry->readMutex.lock();

            m_deviceEntry->medium = m;

            // the information is valid. let the info go.
            m_deviceEntry->readMutex.unlock();
            m_deviceEntry->writeMutex.unlock();

            //
            // inform the media cache about the media change
            //
            if( m_deviceEntry->blockedId == 0 )
                emit mediumChanged( m_deviceEntry->medium.device() );
        }

        if( m_deviceEntry->blockedId == 0 )
            QThread::sleep( 2 );
    }
}





// ////////////////////////////////////////////////////////////////////////////////
// MEDIA CACHE IMPL
// ////////////////////////////////////////////////////////////////////////////////


class K3b::MediaCache::Private
{
public:
    QMap<K3b::Device::Device*, DeviceEntry*> deviceMap;
    KCDDB::Client cddbClient;

    K3b::MediaCache* q;

    void _k_mediumChanged( K3b::Device::Device* );
    void _k_cddbJobFinished( KJob* job );
};


// called from the device thread which updated the medium
void K3b::MediaCache::Private::_k_mediumChanged( K3b::Device::Device* dev )
{
    if ( q->medium( dev ).content() & K3b::Medium::ContentAudio ) {
        K3b::CDDB::CDDBJob* job = K3b::CDDB::CDDBJob::queryCddb( q->medium( dev ) );
        connect( job, SIGNAL(result(KJob*)),
                 q, SLOT(_k_cddbJobFinished(KJob*)) );
        emit q->checkingMedium( dev, i18n( "CDDB Lookup" ) );
    }
    else {
        emit q->mediumChanged( dev );
    }
}


// once the cddb job is finished the medium is really updated
void K3b::MediaCache::Private::_k_cddbJobFinished( KJob* job )
{
    K3b::CDDB::CDDBJob* cddbJob = dynamic_cast<K3b::CDDB::CDDBJob*>( job );
    K3b::Medium oldMedium = cddbJob->medium();

    // make sure the medium did not change during the job
    if ( oldMedium.sameMedium( q->medium( oldMedium.device() ) ) ) {
        if ( !job->error() ) {
            // update it
            deviceMap[oldMedium.device()]->medium.d->cddbInfo = cddbJob->cddbResult();
            emit q->mediumCddbChanged( oldMedium.device() );
        }

        emit q->mediumChanged( oldMedium.device() );
    }
}



K3b::MediaCache::MediaCache( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    d->q = this;
}


K3b::MediaCache::~MediaCache()
{
    clearDeviceList();
    delete d;
}


int K3b::MediaCache::blockDevice( K3b::Device::Device* dev )
{
    qDebug() << dev->blockDeviceName();
    DeviceEntry* e = findDeviceEntry( dev );
    if( e ) {
        if( e->blockedId )
            return -1;
        else {
            // block the information
            e->readMutex.lock();

            // create (hopefully) unique id
            e->blockedId = QRandomGenerator::global()->bounded(RAND_MAX);

            // let the info go
            e->readMutex.unlock();

            // wait for the thread to stop
            e->thread->wait();

            return e->blockedId;
        }
    }
    else
        return -1;
}


bool K3b::MediaCache::unblockDevice( K3b::Device::Device* dev, int id )
{
    qDebug() << dev->blockDeviceName();
    DeviceEntry* e = findDeviceEntry( dev );
    if( e && e->blockedId && e->blockedId == id ) {
        e->blockedId = 0;

        e->medium = K3b::Medium( dev );

        // restart the poll thread
        e->thread->start();

        return true;
    }
    else
        return false;
}


bool K3b::MediaCache::isBlocked( K3b::Device::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) )
        return ( e->blockedId != 0 );
    else
        return false;
}


K3b::Medium K3b::MediaCache::medium( K3b::Device::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        e->readMutex.lock();
        K3b::Medium m = e->medium;
        e->readMutex.unlock();
        return m;
    }
    else
        return K3b::Medium();
}


K3b::Device::DiskInfo K3b::MediaCache::diskInfo( K3b::Device::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        e->readMutex.lock();
        K3b::Device::DiskInfo di = e->medium.diskInfo();
        e->readMutex.unlock();
        return di;
    }
    else
        return K3b::Device::DiskInfo();
}


K3b::Device::Toc K3b::MediaCache::toc( K3b::Device::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        e->readMutex.lock();
        K3b::Device::Toc toc = e->medium.toc();
        e->readMutex.unlock();
        return toc;
    }
    else
        return K3b::Device::Toc();
}


K3b::Device::CdText K3b::MediaCache::cdText( K3b::Device::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        e->readMutex.lock();
        K3b::Device::CdText cdt = e->medium.cdText();
        e->readMutex.unlock();
        return cdt;
    }
    else
        return K3b::Device::CdText();
}


QList<int> K3b::MediaCache::writingSpeeds( K3b::Device::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        e->readMutex.lock();
        QList<int> ws = e->medium.writingSpeeds();
        e->readMutex.unlock();
        return ws;
    }
    else
        return QList<int>();
}


QString K3b::MediaCache::mediumString( K3b::Device::Device* device, bool useContent )
{
    if( DeviceEntry* e = findDeviceEntry( device ) ) {
        return e->medium.shortString( useContent ? Medium::WithContents : Medium::NoStringFlags );
    }
    else
        return QString();
}


void K3b::MediaCache::clearDeviceList()
{
    qDebug();

    // make all the threads stop
    for( QMap<K3b::Device::Device*, DeviceEntry*>::iterator it = d->deviceMap.begin();
         it != d->deviceMap.end(); ++it ) {
        it.value()->blockedId = 1;
    }

    // and remove them
    for( QMap<K3b::Device::Device*, DeviceEntry*>::iterator it = d->deviceMap.begin();
         it != d->deviceMap.end(); ++it ) {
        qDebug() << " waiting for info thread " << it.key()->blockDeviceName() << " to finish";
        it.value()->thread->wait();
        delete it.value();
    }

    d->deviceMap.clear();
}


void K3b::MediaCache::buildDeviceList( K3b::Device::DeviceManager* dm )
{
    // remember blocked ids
    QMap<K3b::Device::Device*, int> blockedIds;
    for( QMap<K3b::Device::Device*, DeviceEntry*>::iterator it = d->deviceMap.begin();
         it != d->deviceMap.end(); ++it )
        blockedIds.insert( it.key(), it.value()->blockedId );

    clearDeviceList();

    QList<K3b::Device::Device *> items(dm->allDevices());
    for( QList<K3b::Device::Device *>::const_iterator it = items.constBegin();
         it != items.constEnd(); ++it ) {
        d->deviceMap.insert( *it, new DeviceEntry( this, *it ) );
        QMap<K3b::Device::Device*, int>::const_iterator bi_it = blockedIds.constFind( *it );
        if( bi_it != blockedIds.constEnd() )
            d->deviceMap[*it]->blockedId = bi_it.value();
    }

    // start all the polling threads
    for( QMap<K3b::Device::Device*, DeviceEntry*>::iterator it = d->deviceMap.begin();
         it != d->deviceMap.end(); ++it ) {
        if( !it.value()->blockedId )
            it.value()->thread->start();
    }
}


K3b::MediaCache::DeviceEntry* K3b::MediaCache::findDeviceEntry( K3b::Device::Device* dev )
{
    QMap<K3b::Device::Device*, DeviceEntry*>::iterator it = d->deviceMap.find( dev );
    if( it != d->deviceMap.end() )
        return it.value();
    else
        return 0;
}


void K3b::MediaCache::lookupCddb( K3b::Device::Device* dev )
{
    K3b::Medium m = medium( dev );
    if ( m.content() & K3b::Medium::ContentAudio ) {
        K3b::CDDB::CDDBJob* job = K3b::CDDB::CDDBJob::queryCddb( m );
        connect( job, SIGNAL(result(KJob*)),
                 this, SLOT(_k_cddbJobFinished(KJob*)) );
        emit checkingMedium( dev, i18n( "CDDB Lookup" ) );
    }
}


void K3b::MediaCache::resetDevice( K3b::Device::Device* dev )
{
    if( DeviceEntry* e = findDeviceEntry( dev ) ) {
        qDebug() << "Resetting medium in" << dev->blockDeviceName();
        e->writeMutex.lock();
        e->readMutex.lock();
        e->medium.reset();
        e->readMutex.unlock();
        e->writeMutex.unlock();
        // no need to emit mediumChanged here. The poll thread will act on it soon
    }
}

#include "moc_k3bmediacache.cpp"
