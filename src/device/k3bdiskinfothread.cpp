/*
 *
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

#include "k3bdiskinfothread.h"

#include "k3bdevice.h"
#include "k3btoc.h"
#include "../k3b.h"
#include "k3bprogressinfoevent.h"

#include <kdebug.h>

#include <sys/ioctl.h>		// ioctls
#include <unistd.h>		// lseek, read. etc
#include <fcntl.h>		// O_RDONLY etc.
#include <linux/cdrom.h>	// ioctls for cdrom
#include <stdlib.h>


K3bCdDevice::DiskInfoThread::DiskInfoThread( QObject *p, CdDevice* d ,DiskInfo *i)
  : QThread(),
  m_parent(p),m_device(d),m_info(i),m_cdfd(0)
{
}


K3bCdDevice::DiskInfoThread::~DiskInfoThread()
{
}


void K3bCdDevice::DiskInfoThread::run()
{

    fetchInfo();
}

void K3bCdDevice::DiskInfoThread::finish(bool success){
  m_info->valid=success;
  QCustomEvent* ready = new QCustomEvent(K3bProgressInfoEvent::Finished);
  QApplication::postEvent(m_parent, ready);
}

void K3bCdDevice::DiskInfoThread::fetchInfo()
{
   struct cdrom_tochdr tochdr;
   struct cdrom_tocentry tocentry;

   if ( (m_cdfd = ::open(m_device->ioctlDevice().latin1(),O_RDONLY | O_NONBLOCK)) == -1 ) {
    kdDebug() << "(K3bDiskInfoThread) could not open device !" << endl;
    finish(false);
    return;
  }

  int ready = m_device->isReady();
  if (ready == 3) {  // no disk or tray open
    m_info->valid=true;
    finish(true);
    return;
  }
  m_info->tocType = m_device->diskType();
  if ( m_info->tocType == DiskInfo::NODISC ) {
     m_info->valid=true;
     finish(true);
     return;
  } else if (m_info->tocType == DiskInfo::UNKNOWN ) {
     m_info->noDisk = false;
     if (m_device->burner())
        fetchSizeInfo();
     finish(true);
     return;
  }
  m_info->noDisk = false;

  m_info->sessions = m_device->numSessions();

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
  Track lastTrack;
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
		   m_info->toc.append( Track( lastTrack.firstSector(), startSec-1, lastTrack.type(), lastTrack.mode() ) );
	  }
    int trackType = 0;
    int trackMode = Track::UNKNOWN;
	  if( control & 0x04 ) {
	  	trackType = Track::DATA;
		  if( mode == 1 )
		    trackMode = Track::MODE1;
		  else if( mode == 2 )
		    trackMode = Track::MODE2;
	  } else
		  trackType = Track::AUDIO;

	  lastTrack = Track( startSec, startSec, trackType, trackMode );

  }
  if (m_device->burner())
    fetchSizeInfo();

  ::close(m_cdfd);
  finish (true);
}


void K3bCdDevice::DiskInfoThread::fetchSizeInfo()
{
  int empty = m_device->isEmpty();
  m_info->appendable = (empty < 2);
  m_info->empty = (empty == 0);
  m_info->cdrw = (m_device->rewritable() == 1);
  K3b::Msf size = m_device->discSize();
  if ( size != K3b::Msf(0) ) {
    m_info->size = size - 150;
  }
  if ( m_info->empty ) {
    m_info->remaining = m_info->size;
  } else {
    size = m_device->remainingSize();
    if ( size != K3b::Msf(0) ) {
      m_info->remaining = size - 4650;
    }
  }
}

