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

#include "../device/k3bdevice.h"
#include "../device/k3btoc.h"
#include "../rip/k3btcwrapper.h"
#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../tools/k3bglobals.h"

#include <kdebug.h>

#include <qtimer.h>
#include <qfile.h>

#include <sys/ioctl.h>		// ioctls
#include <unistd.h>		// lseek, read. etc
#include <fcntl.h>		// O_RDONLY etc.
#include <linux/cdrom.h>	// ioctls for cdrom
#include <stdlib.h>


K3bDiskInfoDetector::K3bDiskInfoDetector( QObject* parent )
  : QObject( parent ),
  m_tcWrapper(0)
{
}


K3bDiskInfoDetector::~K3bDiskInfoDetector()
{
}


void K3bDiskInfoDetector::detect( K3bDevice* device )
{
  if( !device ) {
    kdDebug() << "(K3bDiskInfoDetector) detect should really not be called with NULL!" << endl;
    return;
  }

  m_device = device;

  // reset
  m_info = K3bDiskInfo();
  m_info.device = m_device;

  QTimer::singleShot(0,this,SLOT(fetchTocInfo()));
}


void K3bDiskInfoDetector::finish(bool success)
{
  m_info.valid=success;
  ::close(m_cdfd);
  emit diskInfoReady(m_info);

}


void K3bDiskInfoDetector::fetchDiskInfo()
{
  int empty = m_device->isEmpty();        
  m_info.appendable = (empty < 2);
  m_info.empty = (empty == 0);
  m_info.cdrw = (m_device->rewritable() == 1);
  int size = m_device->discSize();
  if ( size != -1 ) {
    m_info.size = ((size >> 16) & 0xFF)*4500 + ((size >> 8) & 0xFF)*75 + (size & 0xFF) - 150;
    m_info.sizeString = K3b::framesToString(m_info.size);
  }
  if ( m_info.empty ) {
    m_info.remaining = m_info.size;
    m_info.remainingString = m_info.sizeString;
  } else {
    size = m_device->remainingSize();
    if ( size != -1 ) {
      m_info.remaining = m_info.size - ((size >> 16) & 0xFF)*4500 - ((size >> 8) & 0xFF)*75 - (size & 0xFF) - 4650;
      m_info.remainingString = K3b::framesToString(m_info.remaining);
    }
  }

}

void K3bDiskInfoDetector::fetchTocInfo()
{
  struct cdrom_tochdr tochdr;
  struct cdrom_tocentry tocentry;

  if ( (m_cdfd = ::open(m_device->ioctlDevice().latin1(),O_RDONLY | O_NONBLOCK)) == -1 ) {
    kdDebug() << "(K3bDiskInfoDetector) could not open device !" << endl;
    emit diskInfoReady(m_info);
    return;
  }

  int ready = m_device->isReady();
  if (ready == 3) {  // no disk or tray open
    m_info.valid=true;
    emit diskInfoReady(m_info);
    return;
  }    
  m_info.tocType = m_device->diskType();
  if ( m_info.tocType == K3bDiskInfo::NODISC ) {
     m_info.valid=true;
     finish(true);
     return;
  } else if (m_info.tocType == K3bDiskInfo::UNKNOWN ) {
     m_info.noDisk = false;
     if (m_info.device->burner())
        fetchDiskInfo();
     finish(true);
     return;  
  }
  m_info.noDisk = false;
  
  m_info.sessions = m_device->numSessions();

// 
// CDROMREADTOCHDR ioctl returns: 
// cdth_trk0: First Track Number
// cdth_trk1: Last Track Number
//
  if ( ::ioctl(m_cdfd,CDROMREADTOCHDR,&tochdr) != 0 )
  {
     kdDebug() << "(K3bDiskInfoDetector) could not get toc header !" << endl;
     finish(false);
     return;
  }
  K3bTrack lastTrack;
  for (int i = tochdr.cdth_trk0; i <= tochdr.cdth_trk1 + 1; i++) {
    ::memset(&tocentry,0,sizeof (struct cdrom_tocentry));
// get Lead-Out Information too
    tocentry.cdte_track = (i<=tochdr.cdth_trk1) ? i : CDROM_LEADOUT;
    tocentry.cdte_format = CDROM_LBA;
//
// CDROMREADTOCENTRY ioctl returns:
// cdte_addr.lba: Start Sector Number (LBA Format requested)
// cdte_ctrl:     4 ctrl bits
//                   00x0b: 2 audio Channels(no pre-emphasis)
//                   00x1b: 2 audio Channels(pre-emphasis)
//                   10x0b: audio Channels(no pre-emphasis),reserved in cd-rw
//                   10x1b: audio Channels(pre-emphasis),reserved in cd-rw
//                   01x0b: data track, recorded uninterrupted
//                   01x1b: data track, recorded incremental
//                   11xxb: reserved
//                   xx0xb: digital copy prohibited
//                   xx1xb: digital copy permitted
// cdte_addr:     4 addr bits (type of Q-Subchannel data)
//                   0000b: no Information
//                   0001b: current position data
//                   0010b: MCN
//                   0011b: ISRC
//                   0100b-1111b:  reserved
// cdte_datamode:  0: Data Mode1
//                 1: CD-I
//                 2: CD-XA Mode2
//

    if ( ::ioctl(m_cdfd,CDROMREADTOCENTRY,&tocentry) != 0)
      kdDebug() << "(K3bDiskInfoDetector) error reading tocentry " << i << endl; 
    int startSec = tocentry.cdte_addr.lba;
    int control  = tocentry.cdte_ctrl & 0x0f;
    int mode     = tocentry.cdte_datamode;
    if( !lastTrack.isEmpty() ) {
		   m_info.toc.append( K3bTrack( lastTrack.firstSector(), startSec-1, lastTrack.type(), lastTrack.mode() ) );
	  }
    int trackType = 0;
    int trackMode = K3bTrack::UNKNOWN;
	  if( control & 0x04 ) {
	  	trackType = K3bTrack::DATA;
		  if( mode == 1 )
		    trackMode = K3bTrack::MODE1;
		  else if( mode == 2 )
		    trackMode = K3bTrack::MODE2;
	  } else
		  trackType = K3bTrack::AUDIO;

	  lastTrack = K3bTrack( startSec, startSec, trackType, trackMode );

  }
  if (m_info.device->burner())
    fetchDiskInfo();

  if (m_info.tocType != K3bDiskInfo::AUDIO)
    fetchIsoInfo();
  else
    calculateDiscId();

  if (m_info.tocType == K3bDiskInfo::DATA && m_info.toc.count() > 1 && m_info.sessions == 1) {
    m_info.isVCD = true;
    finish(true);
    kdDebug() << "(k3bdiskinfodetector) found VCD" << endl;
  } else if ( m_device->isDVD() ) {
    m_info.empty = false;
    m_info.noDisk = false;
    m_info.tocType = K3bDiskInfo::DVD;
    testForVideoDvd();
  } else
     finish(true);
}

void K3bDiskInfoDetector::fetchIsoInfo()
{
  char buf[17*2048];
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
}


void K3bDiskInfoDetector::calculateDiscId()
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

void K3bDiskInfoDetector::testForVideoDvd()
{

  if( K3bTcWrapper::supportDvd() ) {
    // check if it is a dvd we can display

    if( !m_tcWrapper ) {
      kdDebug() << "(K3bDiskInfoDetector) testForVideoDvd" << endl;
      m_tcWrapper = new K3bTcWrapper( this );
      connect( m_tcWrapper, SIGNAL(successfulDvdCheck(bool)), this, SLOT(slotIsVideoDvd(bool)) );
    }

    m_tcWrapper->isDvdInsert( m_device );

  }
  else
    finish(true);
}

void K3bDiskInfoDetector::slotIsVideoDvd( bool dvd )
{
  if( dvd ) {
    m_info.empty = false;
    m_info.noDisk = false;
    m_info.isVideoDvd = true;
  }

  finish(true);
}


#include "k3bdiskinfodetector.moc"
