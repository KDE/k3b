/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bscsicommand.h"
#include "k3bdevice.h"

#include <kdebug.h>

#include <sys/ioctl.h>
#undef __STRICT_ANSI__
#include <linux/cdrom.h>
#define __STRICT_ANSI__


#include <unistd.h>
#include <sys/types.h>


class K3bCdDevice::ScsiCommand::Private
{
public:
  struct cdrom_generic_command cmd;
  struct request_sense sense;
};


void K3bCdDevice::ScsiCommand::clear()
{
  ::memset( &d->cmd, 0, sizeof(struct cdrom_generic_command) );
  ::memset( &d->sense, 0, sizeof(struct request_sense) );

  d->cmd.quiet = 1;
  d->cmd.sense = &d->sense;
}


unsigned char& K3bCdDevice::ScsiCommand::operator[]( size_t i )
{
  return d->cmd.cmd[i];
}


int K3bCdDevice::ScsiCommand::transport( TransportDirection dir,
					 void* data,
					 size_t len )
{
  if( m_fd == -1 ) {
    return -1;
  }

  d->cmd.buffer = (unsigned char*)data;
  d->cmd.buflen = len;
  if( dir == TR_DIR_READ )
    d->cmd.data_direction = CGC_DATA_READ;
  else if( dir == TR_DIR_WRITE )
    d->cmd.data_direction = CGC_DATA_WRITE;
  else
    d->cmd.data_direction = CGC_DATA_NONE;

  if( ::ioctl( m_fd, CDROM_SEND_PACKET, &d->cmd ) ) {
    debugError( d->cmd.cmd[0],
		d->sense.error_code,
		d->sense.sense_key,
		d->sense.asc,
		d->sense.ascq );

    return( d->sense.error_code != 0 ? d->sense.error_code : -1 );
  }
  else
    return 0;
}
