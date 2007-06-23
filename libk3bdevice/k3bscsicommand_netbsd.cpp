/*
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Mark Davies <mark@mcs.vuw.ac.nz>
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


#include "k3bscsicommand.h"
#include "k3bdevice.h"

#include <k3bdebug.h>

#include <sys/ioctl.h>
#include <sys/scsiio.h>
// #include <sys/cdio.h>
// #include <sys/dkio.h>

#include <unistd.h>
#include <sys/types.h>


class K3bDevice::ScsiCommand::Private
{
public:
  struct scsireq cmd;
};


void K3bDevice::ScsiCommand::clear()
{
  ::memset( &d->cmd, 0, sizeof(struct scsireq ) );
}


unsigned char& K3bDevice::ScsiCommand::operator[]( size_t i )
{
  if( d->cmd.cmdlen < i+1 )
    d->cmd.cmdlen = i+1;
  return d->cmd.cmd[i];
}


int K3bDevice::ScsiCommand::transport( TransportDirection dir,
				       void* data,
				       size_t len )
{
  bool needToClose = false;
  if( m_device ) {
      m_device->usageLock();
      if( !m_device->isOpen() ) {
          needToClose = true;
      }
      m_device->open( dir == TR_DIR_WRITE );
      m_deviceHandle = m_device->handle();
  }

  if( m_deviceHandle == -1 ) {
      if ( m_device ) {
          m_device->usageUnlock();
      }

      return -1;
  }

  d->cmd.timeout = 10000;
  d->cmd.databuf = (caddr_t) data;
  d->cmd.datalen = len;
  //  d->cmd.datalen_used = len;
  d->cmd.senselen = SENSEBUFLEN;
  switch (dir)
  {
  case TR_DIR_READ:
      d->cmd.flags = SCCMD_READ;
      break;
  case TR_DIR_WRITE:
      d->cmd.flags = SCCMD_WRITE;
      break;
  default:
      d->cmd.flags = SCCMD_READ;
      break;
  }

  int i = ::ioctl( m_deviceHandle, SCIOCCOMMAND, &d->cmd );

  if ( m_device ) {
      if( needToClose )
          m_device->close();
      m_device->usageUnlock();
  }

  if( i || (d->cmd.retsts != SCCMD_OK)) {
      debugError( d->cmd.cmd[0],
                  d->cmd.retsts,
                  d->cmd.sense[2],
                  d->cmd.sense[12],
                  d->cmd.sense[13] );

      return 1;
  }
  else
      return 0;
}
