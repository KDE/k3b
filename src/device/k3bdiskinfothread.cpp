/*
 *
 * Copyright (C) 2003 Klaus-Dieter Krannich <kd@k3b.org>
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
  m_parent(p),m_device(d),m_info(i)
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

  m_device->readToc(m_info->toc);

  if (m_device->burner())
    fetchSizeInfo();

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

