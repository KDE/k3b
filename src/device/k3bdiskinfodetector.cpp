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

#include "k3bdiskinfodetector.h"

#include "k3bdevicehandler.h"

#include "k3bdevice.h"
#include "k3btoc.h"
#include "../rip/k3btcwrapper.h"
#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../tools/k3bglobals.h"

#include <kdebug.h>

#include <qtimer.h>
#include <qfile.h>

#include "k3bprogressinfoevent.h"

#include <sys/ioctl.h>		// ioctls
#include <unistd.h>		// lseek, read. etc
#include <fcntl.h>		// O_RDONLY etc.
/* Fix 2.5 kernel definitions */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,50)
typedef unsigned long long __u64;
#endif
#include <linux/cdrom.h>	// ioctls for cdrom
#include <stdlib.h>


K3bCdDevice::DiskInfoDetector::DiskInfoDetector( QObject* parent )
    : QObject( parent ),
    m_tcWrapper(0)
{
  m_deviceHandler = new DeviceHandler( this );
  connect( m_deviceHandler, SIGNAL(finished(bool)),
           this, SLOT(slotDeviceHandlerFinished(bool)) );
}


K3bCdDevice::DiskInfoDetector::~DiskInfoDetector()
{}


void K3bCdDevice::DiskInfoDetector::detect( CdDevice* device )
{
  if( !device ) {
    kdDebug() << "(K3bDiskInfoDetector) detect should really not be called with NULL!" << endl;
    return;
  }

  m_device = device;

  // reset
  m_info = DiskInfo();
  m_info.device = m_device;

  m_deviceHandler->setDevice( device );
  m_deviceHandler->getDiskInfo();
}


void K3bCdDevice::DiskInfoDetector::finish(bool success)
{
  m_info.valid=success;
  emit diskInfoReady(m_info);
}


void K3bCdDevice::DiskInfoDetector::fetchExtraInfo()
{
  if (m_info.tocType != DiskInfo::AUDIO)
    fetchIsoInfo();
  else
    calculateDiscId();

  testForVCD();
}

void K3bCdDevice::DiskInfoDetector::fetchIsoInfo()
{
  char buf[17*2048];
  int m_cdfd = m_device->open();
  if ( m_cdfd == -1 ) {
    kdDebug() << "(K3bDiskInfoDetector) could not open device !" << endl;
    emit diskInfoReady(m_info);
    return;
  }

  ::lseek( m_cdfd, 0, SEEK_SET );

  if( ::read( m_cdfd, buf, 17*2048 ) == 17*2048 ) {
    m_info.isoId = QString::fromLocal8Bit( &buf[16*2048+1], 5 ).stripWhiteSpace();
    m_info.isoSystemId = QString::fromLocal8Bit( &buf[16*2048+8], 32 ).stripWhiteSpace();
    m_info.isoVolumeId = QString::fromLocal8Bit( &buf[16*2048+40], 32 ).stripWhiteSpace();
    m_info.isoVolumeSetId = QString::fromLocal8Bit( &buf[16*2048+190], 128 ).stripWhiteSpace();
    m_info.isoPublisherId = QString::fromLocal8Bit( &buf[16*2048+318], 128 ).stripWhiteSpace();
    m_info.isoPreparerId = QString::fromLocal8Bit( &buf[16*2048+446], 128 ).stripWhiteSpace();
    m_info.isoApplicationId = QString::fromLocal8Bit( &buf[16*2048+574], 128 ).stripWhiteSpace();
  }

  m_device->close();
}



void K3bCdDevice::DiskInfoDetector::calculateDiscId()
{
  // calculate cddb-id
  unsigned int id = 0;
  for( K3bToc::iterator it = m_info.toc.begin(); it != m_info.toc.end(); ++it ) {
    unsigned int n = (*it).firstSector() + 150;
    n /= 75;
    while( n > 0 ) {
      id += n % 10;
      n /= 10;
    }
  }
  unsigned int l = m_info.toc.lastSector() - m_info.toc.firstSector();
  l /= 75;
  id = ( ( id % 0xff ) << 24 ) | ( l << 8 ) | m_info.toc.count();
  m_info.toc.setDiscId( id );

  kdDebug() << "(K3bDiskInfoDetector) calculated disk id: " << id << endl;
}

void K3bCdDevice::DiskInfoDetector::testForVideoDvd()
{

  if( K3bTcWrapper::supportDvd() ) {
    // check if it is a dvd we can display

    if( !m_tcWrapper ) {
      kdDebug() << "(K3bDiskInfoDetector) testForVideoDvd" << endl;
      m_tcWrapper = new K3bTcWrapper( this );
      connect( m_tcWrapper, SIGNAL(successfulDvdCheck(bool)), this, SLOT(slotIsVideoDvd(bool)) );
    }

    m_tcWrapper->isDvdInsert( m_device );

  } else
    finish(true);
}

void K3bCdDevice::DiskInfoDetector::slotIsVideoDvd( bool dvd )
{
  if( dvd )
    m_info.isVideoDvd = true;

  finish(true);
}

void K3bCdDevice::DiskInfoDetector::testForVCD()
{
  if (m_info.tocType == DiskInfo::DATA && m_info.toc.count() > 1 && m_info.sessions == 1 ) {
    if ( KIO::findDeviceMountPoint( m_device->mountDevice() ).isEmpty() )
      connect( KIO::mount( true, "auto", m_device->mountDevice(), m_device->mountPoint(), true ),
               SIGNAL(result(KIO::Job*)), this, SLOT(slotIsVCD(KIO::Job*)) );
    else
      slotIsVCD(0);
  } else if (m_info.tocType == DiskInfo::DVD)
      testForVideoDvd();
  else
    finish(true);
}

void K3bCdDevice::DiskInfoDetector::slotIsVCD(KIO::Job* job)
{
  m_info.isVCD = false;
  bool doit;
  if  (job == 0)
    doit = true;
  else
    doit = (job->error() == 0);

  if ( doit ) {
    QStringList files;
    files << QString("/vcd/info.vcd") << QString("/svcd/info.svd");
    for ( QStringList::Iterator it = files.begin(); it != files.end(); ++it ) {
      QFile f(m_device->mountPoint() + *it);
      QDataStream s(&f);
      char info[9];
      ::memset(info,0,9);
      if ( s.device()->open(IO_ReadOnly) ) {
        s.readRawBytes(info,8);
        kdDebug() << "(K3bDiskInfoDetector) VCD: " << QString(info) << endl;
        s.device()->close();
        if ( QString(info) == QString("VIDEO_CD") ||
             QString(info) == QString("SUPERVCD") ||
             QString(info) == QString("HQ-VCD  ") )
          m_info.isVCD = true;
      }
    }
    connect(KIO::unmount(m_device->mountPoint()), SIGNAL(result(KIO::Job*)), this, SLOT(slotFinished(KIO::Job*)) );
  } else
    finish(true);
}

void K3bCdDevice::DiskInfoDetector::slotFinished(KIO::Job*)
{
   finish(true);
}

void K3bCdDevice::DiskInfoDetector::slotDeviceHandlerFinished( bool success )
{
  m_info = m_deviceHandler->diskInfo();
  if( success )
    fetchExtraInfo();
  else
    finish( false );
}

#include "k3bdiskinfodetector.moc"
