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

#include "k3bdevice.h"
#include "k3btoc.h"
#include "../rip/k3btcwrapper.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../tools/k3bglobals.h"
#include "k3bdevicehandler.h"

#include <kdebug.h>

#include <qtimer.h>
#include <qfile.h>


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
  connect( K3bCdDevice::diskInfo(m_device),
           SIGNAL(finished(K3bCdDevice::DeviceHandler *)),
           this,
	   SLOT(slotDeviceHandlerFinished(K3bCdDevice::DeviceHandler *)) );
}


void K3bCdDevice::DiskInfoDetector::finish(bool success)
{
  m_info.valid=success;
  emit diskInfoReady(m_info);
}


void K3bCdDevice::DiskInfoDetector::fetchExtraInfo()
{
  kdDebug() << "(K3bCdDevice::DiskInfoDetector) fetchExtraInfo()" << endl;

  if (m_info.tocType != DiskInfo::AUDIO)
    fetchIsoInfo();

  m_info.toc.calculateDiscId();

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
    m_info.isoSize = *(int *) &buf[16*2048+80];
  }

  m_device->close();
}


void K3bCdDevice::DiskInfoDetector::testForVideoDvd()
{
  kdDebug() << "(K3bCdDevice::DiskInfoDetector) testForVideoDvd()" << endl;

  if( K3bTcWrapper::supportDvd() ) {
    // check if it is a dvd we can display

    if( !m_tcWrapper ) {
      kdDebug() << "(K3bDiskInfoDetector) testForVideoDvd" << endl;
      m_tcWrapper = new K3bTcWrapper( this );
      connect( m_tcWrapper, SIGNAL(successfulDvdCheck(bool)), this, SLOT(slotIsVideoDvd(bool)) );
    }

    m_tcWrapper->isDvdInsert( m_device );

  } 
  else {
    kdDebug() << "(K3bDiskInfoDetector) no tcprobe" << endl;
    finish(true);
  }
}

void K3bCdDevice::DiskInfoDetector::slotIsVideoDvd( bool dvd )
{
  if( dvd )
    m_info.isVideoDvd = true;

  finish(true);
}

void K3bCdDevice::DiskInfoDetector::testForVCD()
{
  kdDebug() << "(K3bCdDevice::DiskInfoDetector) testForVCD()" << endl;

  if (m_info.tocType == DiskInfo::DATA && m_info.toc.count() > 1 && m_info.sessions == 1 ) {
    connect(K3bCdDevice::mount(m_device),
            SIGNAL(finished(K3bCdDevice::DeviceHandler *)),
            this,
            SLOT(slotIsVCD(K3bCdDevice::DeviceHandler *)));
  }
  else if (m_info.tocType == DiskInfo::DVD) {
    testForVideoDvd();
  }
  else
    finish(true);
}

void K3bCdDevice::DiskInfoDetector::slotIsVCD(K3bCdDevice::DeviceHandler *handler)
{
  kdDebug() << "(K3bCdDevice::DiskInfoDetector) isVCD" << endl;

  m_info.isVCD = false;

  if ( handler->success() ) {
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
    connect(K3bCdDevice::unmount(m_device),
            SIGNAL(finished(K3bCdDevice::DeviceHandler *)),
	    this,
	    SLOT(slotFinished(K3bCdDevice::DeviceHandler *)) );
  } else
    finish(true);
}

void K3bCdDevice::DiskInfoDetector::slotFinished(K3bCdDevice::DeviceHandler*)
{
//  finish(handler->success());
//  umount may fail, if the CD was already mounted and is busy
    finish(true);
}

void K3bCdDevice::DiskInfoDetector::slotDeviceHandlerFinished( K3bCdDevice::DeviceHandler *handler)
{
  kdDebug() << "(K3bCdDevice::DiskInfoDetector) slotDeviceHandlerFinished()" << endl;

  bool success = handler->success();
  if( success ) {
    m_info = handler->diskInfo();
    fetchExtraInfo();
  }
  else
    finish( false );
}

#include "k3bdiskinfodetector.moc"
