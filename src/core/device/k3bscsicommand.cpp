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

#include "k3bscsicommand.h"
#include "k3bdevice.h"

#include <kdebug.h>

#include <string.h>
#include <sys/ioctl.h>


K3bCdDevice::ScsiCommand::ScsiCommand( int fd )
  : m_fd(fd),
    m_device(0)
{
  clear();
}


K3bCdDevice::ScsiCommand::ScsiCommand( const K3bCdDevice::CdDevice* dev )
  : m_device(dev)
{
  clear();

  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  m_needToCloseDevice = !m_device->isOpen();

  m_fd = m_device->open();
}


K3bCdDevice::ScsiCommand::~ScsiCommand()
{
  if( m_device && m_needToCloseDevice )
    m_device->close();
}


void K3bCdDevice::ScsiCommand::clear()
{
  ::memset( &m_cmd, 0, sizeof(struct cdrom_generic_command) );
  ::memset( &m_sense, 0, sizeof(struct request_sense) );

  m_cmd.quiet = 1;
  m_cmd.sense = &m_sense;
}


unsigned char& K3bCdDevice::ScsiCommand::operator[]( size_t i )
{
  return m_cmd.cmd[i];
}


int K3bCdDevice::ScsiCommand::transport( TransportDirection dir,
					 void* data,
					 size_t len )
{
  if( m_fd == -1 ) {
    return -1;
  }

  m_cmd.buffer = (unsigned char*)data;
  m_cmd.buflen = len;
  if( dir == TR_DIR_READ )
    m_cmd.data_direction = CGC_DATA_READ;
  else if( dir == TR_DIR_WRITE )
    m_cmd.data_direction = CGC_DATA_WRITE;
  else
    m_cmd.data_direction = CGC_DATA_NONE;

  if( ::ioctl( m_fd, CDROM_SEND_PACKET, &m_cmd ) ) {
    kdDebug() << "(K3bCdDevice::ScsiCommand) failed: fd: " << m_fd << endl
	      << "                           command:    " << QString::number(m_cmd.cmd[0], 16) << endl
	      << "                           errorcode:  " << QString::number((int)m_sense.error_code, 16) << endl
	      << "                           asc:        " << QString::number((int)m_sense.asc, 16) << endl
	      << "                           ascq:       " << QString::number((int)m_sense.ascq, 16) << endl;
    switch( m_sense.sense_key ) {
    case 0x0:
      kdDebug() << "                           sense key:  NO SENSE (2)" << endl;
      break;
    case 0x1:
      kdDebug() << "                           sense key:  RECOVERED ERROR (1)" << endl;
      break;
    case 0x2:
      kdDebug() << "                           sense key:  NOT READY (2)" << endl;
      break;
    case 0x3:
      kdDebug() << "                           sense key:  MEDIUM ERROR (3)" << endl;
      break;
    case 0x4:
      kdDebug() << "                           sense key:  HARDWARE ERROR (4)" << endl;
      break;
    case 0x5:
      kdDebug() << "                           sense key:  ILLEGAL REQUEST (5)" << endl;
      break;
    case 0x6:
      kdDebug() << "                           sense key:  UNIT ATTENTION (6)" << endl;
      break;
    case 0x7:
      kdDebug() << "                           sense key:  DATA PROTECT (7)" << endl;
      break;
    case 0x8:
      kdDebug() << "                           sense key:  BLANK CHECK (8)" << endl;
      break;
    case 0x9:
      kdDebug() << "                           sense key:  VENDOR SPECIFIC (9)" << endl;
      break;
    case 0xA:
      kdDebug() << "                           sense key:  COPY ABORTED (A)" << endl;
      break;
    case 0xB:
      kdDebug() << "                           sense key:  ABORTED COMMAND (B)" << endl;
      break;
    case 0xC:
      kdDebug() << "                           sense key:  0xC is obsolete... ??" << endl;
      break;
    }
    return( m_sense.error_code != 0 ? m_sense.error_code : -1 );
  }
  else
    return 0;
}
