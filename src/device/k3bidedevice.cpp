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

#include "k3bidedevice.h"

#include <stdlib.h>
#include <fcntl.h>		// O_RDONLY etc.
#include <linux/hdreg.h>
#include <sys/ioctl.h>		// ioctls

#include <kdebug.h>


K3bIdeDevice::K3bIdeDevice( const QString& drive )
  : K3bDevice( drive )
{
  m_burner = false;
  m_burnproof = false;
  m_maxWriteSpeed = -1;

  // we could use cdda_speed_set to test the reading speed
  // for example from 100 down to 1 until it returns TR_OK
}

K3bIdeDevice::~K3bIdeDevice()
{
}

#ifdef SUPPORT_IDE
QString K3bIdeDevice::busTargetLun() const
{
  return QString("ATAPI:%1").arg(devicename().ascii());
}
#endif

bool K3bIdeDevice::furtherInit()
{
 int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bIdeDevice) Error: could not open device." << endl;
    return false;
  }
  else {
    struct hd_driveid hdId;
    ::ioctl( cdromfd, HDIO_GET_IDENTITY, &hdId );

    m_description = QString::fromLatin1((const char*)hdId.model, 40).stripWhiteSpace();
    m_vendor = m_description.left( m_description.find( " " ) );
    m_description = m_description.mid( m_description.find(" ")+1 );
    m_version = QString::fromLatin1((const char*)hdId.fw_rev, 8).stripWhiteSpace();

    return true;
  }
 
}
