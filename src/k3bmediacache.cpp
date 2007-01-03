/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bmediacache.h"
#include "k3bmedium.h"

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


// ////////////////////////////////////////////////////////////////////////////////
// MEDIA CACHE SUPPORT CLASSES
// ////////////////////////////////////////////////////////////////////////////////

class K3bMediaCache::MediaChangeEvent : public QCustomEvent
{
public:
  static const int EventCode;

  MediaChangeEvent( K3bDevice::Device* dev )
    : QCustomEvent( EventCode ),
      m_device( dev ) {}

  K3bDevice::Device* device() const { return m_device; }

private:
  K3bDevice::Device* m_device;
};

const int K3bMediaCache::MediaChangeEvent::EventCode = QEvent::User + 22;


class K3bMediaCache::DeviceEntry
{
public:
  DeviceEntry( K3bMediaCache* cache, K3bDevice::Device* dev );
  ~DeviceEntry();

  K3bMedium medium;

  int blockedId;
  
  QMutex mutex;

  K3bMediaCache::PollThread* thread;

  K3bMediaCache* cache;
  
  void clear() {
    medium.reset();
  }
};


class K3bMediaCache::PollThread : public QThread
{
public:
  PollThread( K3bMediaCache::DeviceEntry* de )
    : m_deviceEntry( de ) {}

protected:
  void run();

private:
  K3bMediaCache::DeviceEntry* m_deviceEntry;
};




// ////////////////////////////////////////////////////////////////////////////////
// MEDIA CACHE SUPPORT CLASSES IMPL
// ////////////////////////////////////////////////////////////////////////////////


K3bMediaCache::DeviceEntry::DeviceEntry( K3bMediaCache* c, K3bDevice::Device* dev )
  : medium(dev),
    blockedId(0),
    cache(c)
{
  thread = new K3bMediaCache::PollThread( this );
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
      
      //
      // The medium has changed. We need to update the information.
      //
      K3bMedium m( m_deviceEntry->medium.device() );
      m.update();
      
      // block the info since it is not valid anymore
      m_deviceEntry->mutex.lock();
      
      //      m_deviceEntry->medium.update();
      m_deviceEntry->medium = m;
      	
      //
      // inform the media cache about the media change
      //
      if( m_deviceEntry->blockedId == 0 )
	QApplication::postEvent( m_deviceEntry->cache,
				 new K3bMediaCache::MediaChangeEvent( m_deviceEntry->medium.device() ) );
      
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


K3bMediaCache::K3bMediaCache( QObject* parent )
  : QObject( parent )
{
}


K3bMediaCache::~K3bMediaCache()
{
  clearDeviceList();
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
      e->blockedId = KApplication::random();

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

    // for security reasons we emit no medium signal at this point
    // otherwise a job might resuse the old medium information
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


QValueList<int> K3bMediaCache::writingSpeeds( K3bDevice::Device* dev )
{
  if( DeviceEntry* e = findDeviceEntry( dev ) ) {
    e->mutex.lock();
    QValueList<int> ws = e->medium.writingSpeeds();
    e->mutex.unlock();
    return ws;
  }
  else
    return QValueList<int>();
}


QString K3bMediaCache::mediumString( K3bDevice::Device* device, bool useContent )
{
  if( DeviceEntry* e = findDeviceEntry( device ) ) {
    return e->medium.shortString( useContent );
  }
  else
    return QString::null;
}


void K3bMediaCache::clearDeviceList()
{
  kdDebug() << k_funcinfo << endl;

  // make all the threads stop
  for( QMap<K3bDevice::Device*, DeviceEntry*>::iterator it = m_deviceMap.begin(); 
       it != m_deviceMap.end(); ++it ) {
    it.data()->blockedId = 1;
  }

  // and remove them
  for( QMap<K3bDevice::Device*, DeviceEntry*>::iterator it = m_deviceMap.begin(); 
       it != m_deviceMap.end(); ++it ) {
    kdDebug() << k_funcinfo << " waiting for info thread " << it.key()->blockDeviceName() << " to finish" << endl;
    it.data()->thread->wait();
    delete it.data();
  }

  m_deviceMap.clear();
}


void K3bMediaCache::buildDeviceList( K3bDevice::DeviceManager* dm )
{
  // remember blocked ids
  QMap<K3bDevice::Device*, int> blockedIds;
  for( QMap<K3bDevice::Device*, DeviceEntry*>::iterator it = m_deviceMap.begin(); 
       it != m_deviceMap.end(); ++it )
    blockedIds.insert( it.key(), it.data()->blockedId );

  clearDeviceList();

  const QPtrList<K3bDevice::Device>& devices = dm->allDevices();
  for( QPtrListIterator<K3bDevice::Device> it( devices ); *it; ++it ) {
    m_deviceMap.insert( *it, new DeviceEntry( this, *it ) );
    QMap<K3bDevice::Device*, int>::const_iterator bi_it = blockedIds.find( *it );
    if( bi_it != blockedIds.end() )
      m_deviceMap[*it]->blockedId = bi_it.data();
  }

  // start all the polling threads
  for( QMap<K3bDevice::Device*, DeviceEntry*>::iterator it = m_deviceMap.begin(); 
       it != m_deviceMap.end(); ++it ) {
    if( !it.data()->blockedId )
      it.data()->thread->start();
  }
}


K3bMediaCache::DeviceEntry* K3bMediaCache::findDeviceEntry( K3bDevice::Device* dev )
{
  QMap<K3bDevice::Device*, DeviceEntry*>::iterator it = m_deviceMap.find( dev );
  if( it != m_deviceMap.end() )
    return it.data();
  else
    return 0;
}


void K3bMediaCache::customEvent( QCustomEvent* e )
{
  if( e->type() == MediaChangeEvent::EventCode )
    emit mediumChanged( static_cast<MediaChangeEvent*>( e )->device() );
}

#include "k3bmediacache.moc"
