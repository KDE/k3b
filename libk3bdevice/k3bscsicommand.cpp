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

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <assert.h>

#ifdef Q_OS_FREEBSD
#include <cam/scsi/scsi_message.h>
#include <cam/scsi/scsi_pass.h>

#define ERRCODE(s)	((((s)[2]&0x0F)<<16)|((s)[12]<<8)|((s)[13]))
#define EMEDIUMTYPE	EINVAL
#define	ENOMEDIUM	ENODEV
#define CREAM_ON_ERRNO(s)	do {			\
    switch ((s)[12])					\
    {	case 0x04:	errno=EAGAIN;	break;		\
	case 0x20:	errno=ENODEV;	break;		\
	case 0x21:	if ((s)[13]==0)	errno=ENOSPC;	\
			else		errno=EINVAL;	\
			break;				\
	case 0x30:	errno=EMEDIUMTYPE;  break;	\
	case 0x3A:	errno=ENOMEDIUM;    break;	\
    }							\
} while(0)

static void dump_error(int b,struct scsi_sense_data *sense);

#endif


QString senseKeyToString( int key )
{
  switch( key ) {
  case 0x0:
    return "NO SENSE (2)";
  case 0x1:
    return "RECOVERED ERROR (1)";
  case 0x2:
    return "NOT READY (2)";
  case 0x3:
    return "MEDIUM ERROR (3)";
  case 0x4:
    return "HARDWARE ERROR (4)";
  case 0x5:
    return "ILLEGAL REQUEST (5)";
  case 0x6:
    return "UNIT ATTENTION (6)";
  case 0x7:
    return "DATA PROTECT (7)";
  case 0x8:
    return "BLANK CHECK (8)";
  case 0x9:
    return "VENDOR SPECIFIC (9)";
  case 0xA:
    return "COPY ABORTED (A)";
  case 0xB:
    return "ABORTED COMMAND (B)";
  case 0xC:
    return "0xC is obsolete... ??";
  }

  return "unknown";
}


class K3bCdDevice::ScsiCommand::Private
{
public:
#ifdef Q_OS_LINUX
  struct cdrom_generic_command cmd;
  struct request_sense sense;

#endif
#ifdef Q_OS_FREEBSD
  union ccb ccb;
#endif
};

K3bCdDevice::ScsiCommand::ScsiCommand( int fd )
  : d(new Private),
    m_fd(fd),
    m_device(0)
{
  clear();
}


K3bCdDevice::ScsiCommand::ScsiCommand( const K3bCdDevice::CdDevice* dev )
  : d(new Private),
    m_fd(-1),
    m_device(dev)
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  m_needToCloseDevice = !m_device->isOpen();

  m_fd = m_device->open();

  clear();
}


K3bCdDevice::ScsiCommand::~ScsiCommand()
{
  delete d;
  if( m_device && m_needToCloseDevice )
    m_device->close();
}


void K3bCdDevice::ScsiCommand::clear()
{
#ifdef Q_OS_LINUX
  ::memset( &d->cmd, 0, sizeof(struct cdrom_generic_command) );
  ::memset( &d->sense, 0, sizeof(struct request_sense) );

  d->cmd.quiet = 1;
  d->cmd.sense = &sense;
#endif
#ifdef Q_OS_FREEBSD
  memset (&d->ccb,0,sizeof(ccb));
  if (!m_device || !m_device->cam()) return;
  d->ccb.ccb_h.path_id    = m_device->cam()->path_id;
  d->ccb.ccb_h.target_id  = m_device->cam()->target_id;
  d->ccb.ccb_h.target_lun = m_device->cam()->target_lun;
#endif
}


unsigned char& K3bCdDevice::ScsiCommand::operator[]( size_t i )
{
#ifdef Q_OS_LINUX
  return d->cmd.cmd[i];
#endif
#ifdef Q_OS_FREEBSD
  d->ccb.csio.cdb_len = i+1;
  return d->ccb.csio.cdb_io.cdb_bytes[i];
#endif
}

int K3bCdDevice::ScsiCommand::transport( TransportDirection dir,
					 void* data,
					 size_t len )
{
  if( m_fd == -1 ) {
    return -1;
  }

#ifdef Q_OS_LINUX
  d->cmd.buffer = (unsigned char*)data;
  d->cmd.buflen = len;
  if( dir == TR_DIR_READ )
    d->cmd.data_direction = CGC_DATA_READ;
  else if( dir == TR_DIR_WRITE )
    d->cmd.data_direction = CGC_DATA_WRITE;
  else
    d->cmd.data_direction = CGC_DATA_NONE;

  if( ::ioctl( m_fd, CDROM_SEND_PACKET, &d->cmd ) ) {
    kdDebug() << "(K3bCdDevice::ScsiCommand) failed: fd: " << m_fd << endl
	      << "                           command:    " << QString("%1 (%2)")
      .arg( commandString( d->cmd.cmd[0] ) )
      .arg( QString::number(d->cmd.cmd[0], 16) ) << endl
	      << "                           errorcode:  " << QString::number((int)d->sense.error_code, 16) << endl
	      << "                           sense key:  " << senseKeyToString(d->sense.sense_key) << endl
	      << "                           asc:        " << QString::number((int)d->sense.asc, 16) << endl
	      << "                           ascq:       " << QString::number((int)d->sense.ascq, 16) << endl;

    return( d->sense.error_code != 0 ? d->sense.error_code : -1 );
  }
  else
    return 0;
#endif

#ifdef Q_OS_FREEBSD
  if (!m_device || !m_device->cam())
    return -1;
  kdDebug() << "(K3bCdDevice::ScsiCommand) transport command " << QString::number((int)d->ccb.csio.cdb_io.cdb_bytes[0], 16) << ", length: " << (int)d->ccb.csio.cdb_len << endl;
  int ret=0;
  int direction = CAM_DEV_QFRZDIS;
  if (!len)
    direction |= CAM_DIR_NONE;
  else
    direction |= (dir & TR_DIR_READ)?CAM_DIR_IN : CAM_DIR_OUT;
  cam_fill_csio (&(d->ccb.csio), 1, 0 /* NULL */, direction | CAM_DEV_QFRZDIS, MSG_SIMPLE_Q_TAG, (u_int8_t *)data, len, sizeof(d->ccb.csio.sense_data), d->ccb.csio.cdb_len, 30*1000);
  unsigned char * sense = (unsigned char *)&d->ccb.csio.sense_data;
  if ((ret = cam_send_ccb(m_device->cam(), &d->ccb)) < 0)
    {
      kdDebug() << "(K3bCdDevice::ScsiCommand) transport failed: " << ret << endl;
      dump_error(d->ccb.csio.cdb_io.cdb_bytes[0],(struct scsi_sense_data *)sense);
      return ret;
    }
  if ((d->ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP)
    return 0;

  errno = EIO;
  // FreeBSD 5-CURRENT since 2003-08-24, including 5.2 fails to
  // pull sense data automatically, at least for ATAPI transport,
  // so I reach for it myself...
  if ((d->ccb.csio.scsi_status==SCSI_STATUS_CHECK_COND) &&
      !(d->ccb.ccb_h.status&CAM_AUTOSNS_VALID))
    {
      u_int8_t  _sense[18];
      u_int32_t resid=d->ccb.csio.resid;

      memset(_sense,0,sizeof(_sense));

      operator[](0)      = 0x03;	// REQUEST SENSE
      d->ccb.csio.cdb_io.cdb_bytes[4] = sizeof(_sense);
      d->ccb.csio.cdb_len   = 6;
      d->ccb.csio.ccb_h.flags |= CAM_DIR_IN|CAM_DIS_AUTOSENSE;
      d->ccb.csio.data_ptr  = _sense;
      d->ccb.csio.dxfer_len = sizeof(_sense);
      d->ccb.csio.sense_len = 0;
      ret = cam_send_ccb(m_device->cam(), &d->ccb);

      d->ccb.csio.resid = resid;
      if (ret<0)
	{
	  kdDebug() << "(K3bCdDevice::ScsiCommand) transport failed (2): " << ret << endl;
	  ret = -1;
	  dump_error(d->ccb.csio.cdb_io.cdb_bytes[0],(struct scsi_sense_data *)sense);
	  return -1;
	}
      if ((d->ccb.ccb_h.status&CAM_STATUS_MASK) != CAM_REQ_CMP)
	{
	  kdDebug() << "(K3bCdDevice::ScsiCommand) transport failed (3): " << ret << endl;
	  errno=EIO,-1;
	  ret = -1;
	  dump_error(d->ccb.csio.cdb_io.cdb_bytes[0],(struct scsi_sense_data *)sense);
	  return -1;
	}

      memcpy(sense,_sense,sizeof(_sense));
    }

  ret = ERRCODE(sense);
  kdDebug() << "(K3bCdDevice::ScsiCommand) transport failed (4): " << ret << endl;
  if (ret == 0)
    ret = -1;
  else
    CREAM_ON_ERRNO(((unsigned char *)&d->ccb.csio.sense_data));
  dump_error(d->ccb.csio.cdb_io.cdb_bytes[0],(struct scsi_sense_data *)sense);
  return ret;
#endif
}


QString K3bCdDevice::commandString( const unsigned char& command )
{
  if( command == MMC_BLANK )
    return "BLANK";
  if( command == MMC_CLOSE_TRACK_SESSION )
    return "CLOSE TRACK/SESSION";
  if( command == MMC_ERASE )
    return "ERASE";
  if( command == MMC_FORMAT_UNIT )
    return "FORMAT UNIT";
  if( command == MMC_GET_CONFIGURATION )
    return "GET CONFIGURATION";
  if( command == MMC_GET_EVENT_STATUS_NOTIFICATION )
    return "GET EVENT STATUS NOTIFICATION";
  if( command == MMC_GET_PERFORMANCE )
    return "GET PERFORMANCE";
  if( command == MMC_INQUIRY )
    return "INQUIRY";
  if( command == MMC_LOAD_UNLOAD_MEDIUM )
    return "LOAD/UNLOAD MEDIUM";
  if( command == MMC_MECHANISM_STATUS )
    return "MECHANISM STATUS";
  if( command == MMC_MODE_SELECT )
    return "MODE SELECT";
  if( command == MMC_MODE_SENSE )
    return "MODE SENSE";
  if( command == MMC_PAUSE_RESUME )
    return "PAUSE/RESUME";
  if( command == MMC_PLAY_AUDIO_10 )
    return "PLAY AUDIO (10)";
  if( command == MMC_PLAY_AUDIO_12 )
    return "PLAY AUDIO (12)";
  if( command == MMC_PLAY_AUDIO_MSF )
    return "PLAY AUDIO (MSF)";
  if( command == MMC_PREVENT_ALLOW_MEDIUM_REMOVAL )
    return "PREVENT ALLOW MEDIUM REMOVAL";
  if( command == MMC_READ_10 )
    return "READ (10)";
  if( command == MMC_READ_12 )
    return "READ (12)";
  if( command == MMC_READ_BUFFER )
    return "READ BUFFER";
  if( command == MMC_READ_BUFFER_CAPACITY )
    return "READ BUFFER CAPACITY";
  if( command == MMC_READ_CAPACITY )
    return "READ CAPACITY";
  if( command == MMC_READ_CD )
    return "READ CD";
  if( command == MMC_READ_CD_MSF )
    return "READ CD MSF";
  if( command == MMC_READ_DISK_INFORMATION )
    return "READ DISK INFORMATION";
  if( command == MMC_READ_DVD_STRUCTURE )
    return "READ DVD STRUCTURE";
  if( command == MMC_READ_FORMAT_CAPACITIES )
    return "READ FORMAT CAPACITIES";
  if( command == MMC_READ_SUB_CHANNEL )
    return "READ SUB-CHANNEL";
  if( command == MMC_READ_TOC_PMA_ATIP )
    return "READ TOC/PMA/ATIP";
  if( command == MMC_READ_TRACK_INFORMATION )
    return "READ TRACK INFORMATION";
  if( command == MMC_REPAIR_TRACK )
    return "REPAIR TRACK";
  if( command == MMC_REPORT_KEY )
    return "REPORT KEY";
  if( command == MMC_REQUEST_SENSE )
    return "REQUEST SENSE";
  if( command == MMC_RESERVE_TRACK )
    return "RESERVE TRACK";
  if( command == MMC_SCAN )
    return "SCAN";
  if( command == MMC_SEEK_10 )
    return "SEEK (10)";
  if( command == MMC_SEND_CUE_SHEET )
    return "SEND CUE SHEET";
  if( command == MMC_SEND_DVD_STRUCTURE )
    return "SEND DVD STRUCTURE";
  if( command == MMC_SEND_KEY )
    return "SEND KEY";
  if( command == MMC_SEND_OPC_INFORMATION )
    return "SEND OPC INFORMATION";
  if( command == MMC_SET_SPEED )
    return "SET SPEED";
  if( command == MMC_SET_READ_AHEAD )
    return "SET READ AHEAD";
  if( command == MMC_SET_STREAMING )
    return "SET STREAMING";
  if( command == MMC_START_STOP_UNIT )
    return "START STOP UNIT";
  if( command == MMC_STOP_PLAY_SCAN )
    return "STOP PLAY/SCAN";
  if( command == MMC_SYNCHRONIZE_CACHE )
    return "SYNCHRONIZE CACHE";
  if( command == MMC_TEST_UNIT_READY )
    return "TEST UNIT READY";
  if( command == MMC_VERIFY_10 )
    return "VERIFY (10)";
  if( command == MMC_WRITE_10 )
    return "WRITE (10)";
  if( command == MMC_WRITE_12 )
    return "WRITE (12)";
  if( command == MMC_WRITE_AND_VERIFY_10 )
    return "WRITE AND VERIFY (10)";
  if( command == MMC_WRITE_BUFFER )
    return "WRITE BUFFER";

  return "unknown";
}

#ifdef Q_OS_FREEBSD
static void dump_error(int b,struct scsi_sense_data *sense)
{
  kdDebug() << "(K3bCdDevice::ScsiCommand) failed: " << endl
	    << "                           command:    " << QString("%1 (%2)")
    .arg( K3bCdDevice::commandString( b ) )
    .arg( QString::number(b, 16) ) << endl
	    << "                           errorcode:  " << QString::number(sense->error_code & SSD_ERRCODE, 16) << endl
	    << "                           sense key:  " << senseKeyToString(sense->flags & SSD_KEY) << endl
	    << "                           asc:        " << QString::number(sense->add_sense_code, 16) << endl
	    << "                           ascq:       " << QString::number(sense->add_sense_code_qual, 16) << endl;
}
#endif


