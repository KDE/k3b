/*
 *
 * k3bscsicommand_win32.cpp
 * Copyright (C) 2007 Jeremy C. Andrus <jeremy@jeremya.com>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * Parts of this file are inspired (and copied) from various source
 * files in the cdrdao project (C) J. Schilling, Andreas Mueller 
 * and many others.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bscsicommand.h"
#include "k3bdevice.h"

#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "winspti.h"


class K3b::Device::ScsiCommand::Private
{
  public:
    SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER m_cmd;
    
    SCSI_SENSE_DATA m_senseData;
};


void K3b::Device::ScsiCommand::clear()
{
  ::memset( &d->m_cmd, 0, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) );
}


unsigned char& K3b::Device::ScsiCommand::operator[]( size_t i )
{
  if( d->m_cmd.spt.CdbLength < i+1 )
    d->m_cmd.spt.CdbLength = i+1;
  return d->m_cmd.spt.Cdb[i];
}


int K3b::Device::ScsiCommand::transport( TransportDirection dir,
                                       void* data,
                                       size_t len )
{
  bool needToClose = false;
  ULONG returned = 0;
  BOOL status = TRUE;

  BYTE * buf;

  if( m_device ) {
    needToClose = !m_device->isOpen();
    m_device->open( dir == TR_DIR_WRITE );
    m_deviceHandle = m_device->handle();
  }

  if( m_deviceHandle == INVALID_HANDLE_VALUE )
  {
    kDebug() << "(K3bScsiCommand::transport) could not perform Win32 IOCTL on invalid handle value" << endl;
    return -1;
  }

  if ( dir == TR_DIR_READ )
    d->m_cmd.spt.DataIn           = SCSI_IOCTL_DATA_IN;
  else if ( dir == TR_DIR_WRITE )
    d->m_cmd.spt.DataIn           = SCSI_IOCTL_DATA_OUT;
  else
    d->m_cmd.spt.DataIn           = SCSI_IOCTL_DATA_IN;

  buf = new BYTE[len];

  d->m_cmd.spt.Length             = sizeof(SCSI_PASS_THROUGH_DIRECT);
  d->m_cmd.spt.SenseInfoLength    = SENSE_LEN_SPTI;
  d->m_cmd.spt.DataTransferLength = len;
  d->m_cmd.spt.TimeOutValue       = 2;
  d->m_cmd.spt.DataBuffer         = buf;
  d->m_cmd.spt.SenseInfoOffset    = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);

  status = DeviceIoControl( m_deviceHandle,
                            IOCTL_SCSI_PASS_THROUGH_DIRECT,
                            &(d->m_cmd), sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),
                            &(d->m_cmd), sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),
                            &returned, NULL);

  memcpy( data, buf, len );
  delete [] buf;

  if( needToClose )
    m_device->close();

  // get the sense data on error
  if ( !status )
  {
    kDebug() << "(K3bScsiCommand::transport) ioctl failed: " << GetLastError() << ", returned=" << returned << endl;
    ::memcpy( &(d->m_senseData), d->m_cmd.ucSenseBuf, SENSE_LEN_SPTI );
	
    debugError( d->m_cmd.spt.Cdb[0],
                d->m_senseData.SD_Error,
                d->m_senseData.SD_SenseKey,
                d->m_senseData.SD_ASC,
                d->m_senseData.SD_ASCQ );

    int errCode =
      (d->m_senseData.SD_Error << 24)    & 0xF000 |
      (d->m_senseData.SD_SenseKey << 16) & 0x0F00 |
      (d->m_senseData.SD_ASC << 8)       & 0x00F0 |
      (d->m_senseData.SD_ASCQ)           & 0x000F;

    return ( errCode != 0 ? errCode : 1 );
  }

  return 0;
}
