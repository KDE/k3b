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

#ifdef Q_OS_LINUX
// Linux has all its headers alread included from the k3b headers
#endif

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
  bool closecam;
  struct cam_device  *cam;
  union ccb ccb;
#endif
  
  Private();
  ~Private();
  void clear();
};

K3bCdDevice::ScsiCommand::Private::Private()
{
#ifdef Q_OS_FREEBSD
  cam = 0;
#endif
  clear();
}

K3bCdDevice::ScsiCommand::Private::~Private()
{
#ifdef Q_OS_FREEBSD
  if (cam && closecam) {
    cam_close_device(cam);
    cam = 0;
  }
#endif
}

void K3bCdDevice::ScsiCommand::Private::clear()
{
#ifdef Q_OS_FREEBSD
  memset (&ccb,0,sizeof(ccb));
  if (!cam)
    return;
  ccb.ccb_h.path_id    = cam->path_id;
  ccb.ccb_h.target_id  = cam->target_id;
  ccb.ccb_h.target_lun = cam->target_lun;
#endif

#ifdef Q_OS_LINUX
  ::memset( &cmd, 0, sizeof(struct cdrom_generic_command) );
  ::memset( &sense, 0, sizeof(struct request_sense) );

  cmd.quiet = 1;
  cmd.sense = &sense;
#endif
}


K3bCdDevice::ScsiCommand::ScsiCommand( int fd )
  : d(new Private),
    m_fd(fd),
    m_device(0)
{
  clear();
#ifdef Q_OS_FREEBSD
  bool freebsd_incompatible_constructor = false;
  assert(freebsd_incompatible_constructor);
#endif
}


K3bCdDevice::ScsiCommand::ScsiCommand( const K3bCdDevice::CdDevice* dev )
  : d(new Private),
    m_fd(-1), // Reinitted later
    m_device(dev)
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  m_needToCloseDevice = !m_device->isOpen();

#ifdef Q_OS_LINUX
  m_fd = m_device->open();
#endif

#ifdef Q_OS_FREEBSD
  d->closecam = true;
  d->cam = cam_open_pass (m_device->m_passDevice.latin1(),O_RDWR,0 /* NULL */);
  kdDebug() << "(K3bCdDevice::ScsiCommand) open device " << m_device->m_passDevice << ((d->cam)?" succeeded.":" failed.") << endl;
  m_fd = 0;
#endif

  clear();
}


#ifdef Q_OS_FREEBSD
K3bCdDevice::ScsiCommand::ScsiCommand( const K3bCdDevice::CdDevice*dev , struct cam_device *cam)
  : d(0),
    m_fd(0),
    m_device(dev)
{
  d = new Private;
  d->closecam = false;
  d->cam = cam;
  clear();
}
#endif


K3bCdDevice::ScsiCommand::~ScsiCommand()
{
  delete d;
  if( m_device && m_needToCloseDevice )
    m_device->close();
}


void K3bCdDevice::ScsiCommand::clear()
{
  d->clear();
}


unsigned char& K3bCdDevice::ScsiCommand::operator[]( size_t i )
{
#ifdef Q_OS_FREEBSD
  d->ccb.csio.cdb_len = i+1;
  return d->ccb.csio.cdb_io.cdb_bytes[i];
#endif

#ifdef Q_OS_LINUX
  return d->cmd.cmd[i];
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
      .arg( MMC::commandString( d->cmd.cmd[0] ) )
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
  if (!d->cam)
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
  if ((ret = cam_send_ccb(d->cam, &d->ccb)) < 0)
    {
      kdDebug() << "(K3bCdDevice::ScsiCommand) transport failed: " << ret << endl;
      goto dump_error;
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
      ret = cam_send_ccb(d->cam, &d->ccb);

      // FIXME: remove this goto stuff! It has no place in a C++ application.

      d->ccb.csio.resid = resid;
      if (ret<0)
	{
	  kdDebug() << "(K3bCdDevice::ScsiCommand) transport failed (2): " << ret << endl;
	  ret = -1;
	  goto dump_error;
	}
      if ((d->ccb.ccb_h.status&CAM_STATUS_MASK) != CAM_REQ_CMP)
	{
	  kdDebug() << "(K3bCdDevice::ScsiCommand) transport failed (3): " << ret << endl;
	  errno=EIO,-1;
	  ret = -1;
	  goto dump_error;
	}

      memcpy(sense,_sense,sizeof(_sense));
    }

  ret = ERRCODE(sense);
  kdDebug() << "(K3bCdDevice::ScsiCommand) transport failed (4): " << ret << endl;
  if (ret == 0)
    ret = -1;
  else
    CREAM_ON_ERRNO(((unsigned char *)&d->ccb.csio.sense_data));
 dump_error:
  kdDebug() << "(K3bCdDevice::ScsiCommand) failed: " << endl
	    << "                           command:    " << QString("%1 (%2)")
    .arg( MMC::commandString( d->ccb.csio.cdb_io.cdb_bytes[0] ) )
    .arg( QString::number(d->ccb.csio.cdb_io.cdb_bytes[0], 16) ) << endl
	    << "                           errorcode:  " << QString::number(((struct scsi_sense_data *)sense)->error_code & SSD_ERRCODE, 16) << endl
	    << "                           sense key:  " << senseKeyToString(((struct scsi_sense_data *)sense)->flags & SSD_KEY) << endl
	    << "                           asc:        " << QString::number(((struct scsi_sense_data *)sense)->add_sense_code, 16) << endl
	    << "                           ascq:       " << QString::number(((struct scsi_sense_data *)sense)->add_sense_code_qual, 16) << endl;

  return ret;
#endif
}


QString K3bCdDevice::MMC::commandString( const unsigned char& command )
{
  if( command == MMC::BLANK )
    return "BLANK";
  if( command == MMC::CLOSE_TRACK_SESSION )
    return "CLOSE TRACK/SESSION";
  if( command == MMC::ERASE )
    return "ERASE";
  if( command == MMC::FORMAT_UNIT )
    return "FORMAT UNIT";
  if( command == MMC::GET_CONFIGURATION )
    return "GET CONFIGURATION";
  if( command == MMC::GET_EVENT_STATUS_NOTIFICATION )
    return "GET EVENT STATUS NOTIFICATION";
  if( command == MMC::GET_PERFORMANCE )
    return "GET PERFORMANCE";
  if( command == MMC::INQUIRY )
    return "INQUIRY";
  if( command == MMC::LOAD_UNLOAD_MEDIUM )
    return "LOAD/UNLOAD MEDIUM";
  if( command == MMC::MECHANISM_STATUS )
    return "MECHANISM STATUS";
  if( command == MMC::MODE_SELECT )
    return "MODE SELECT";
  if( command == MMC::MODE_SENSE )
    return "MODE SENSE";
  if( command == MMC::PAUSE_RESUME )
    return "PAUSE/RESUME";
  if( command == MMC::PLAY_AUDIO_10 )
    return "PLAY AUDIO (10)";
  if( command == MMC::PLAY_AUDIO_12 )
    return "PLAY AUDIO (12)";
  if( command == MMC::PLAY_AUDIO_MSF )
    return "PLAY AUDIO (MSF)";
  if( command == MMC::PREVENT_ALLOW_MEDIUM_REMOVAL )
    return "PREVENT ALLOW MEDIUM REMOVAL";
  if( command == MMC::READ_10 )
    return "READ (10)";
  if( command == MMC::READ_12 )
    return "READ (12)";
  if( command == MMC::READ_BUFFER )
    return "READ BUFFER";
  if( command == MMC::READ_BUFFER_CAPACITY )
    return "READ BUFFER CAPACITY";
  if( command == MMC::READ_CAPACITY )
    return "READ CAPACITY";
  if( command == MMC::READ_CD )
    return "READ CD";
  if( command == MMC::READ_CD_MSF )
    return "READ CD MSF";
  if( command == MMC::READ_DISK_INFORMATION )
    return "READ DISK INFORMATION";
  if( command == MMC::READ_DVD_STRUCTURE )
    return "READ DVD STRUCTURE";
  if( command == MMC::READ_FORMAT_CAPACITIES )
    return "READ FORMAT CAPACITIES";
  if( command == MMC::READ_SUB_CHANNEL )
    return "READ SUB-CHANNEL";
  if( command == MMC::READ_TOC_PMA_ATIP )
    return "READ TOC/PMA/ATIP";
  if( command == MMC::READ_TRACK_INFORMATION )
    return "READ TRACK INFORMATION";
  if( command == MMC::REPAIR_TRACK )
    return "REPAIR TRACK";
  if( command == MMC::REPORT_KEY )
    return "REPORT KEY";
  if( command == MMC::REQUEST_SENSE )
    return "REQUEST SENSE";
  if( command == MMC::RESERVE_TRACK )
    return "RESERVE TRACK";
  if( command == MMC::SCAN )
    return "SCAN";
  if( command == MMC::SEEK_10 )
    return "SEEK (10)";
  if( command == MMC::SEND_CUE_SHEET )
    return "SEND CUE SHEET";
  if( command == MMC::SEND_DVD_STRUCTURE )
    return "SEND DVD STRUCTURE";
  if( command == MMC::SEND_KEY )
    return "SEND KEY";
  if( command == MMC::SEND_OPC_INFORMATION )
    return "SEND OPC INFORMATION";
  if( command == MMC::SET_SPEED )
    return "SET SPEED";
  if( command == MMC::SET_READ_AHEAD )
    return "SET READ AHEAD";
  if( command == MMC::SET_STREAMING )
    return "SET STREAMING";
  if( command == MMC::START_STOP_UNIT )
    return "START STOP UNIT";
  if( command == MMC::STOP_PLAY_SCAN )
    return "STOP PLAY/SCAN";
  if( command == MMC::SYNCHRONIZE_CACHE )
    return "SYNCHRONIZE CACHE";
  if( command == MMC::TEST_UNIT_READY )
    return "TEST UNIT READY";
  if( command == MMC::VERIFY_10 )
    return "VERIFY (10)";
  if( command == MMC::WRITE_10 )
    return "WRITE (10)";
  if( command == MMC::WRITE_12 )
    return "WRITE (12)";
  if( command == MMC::WRITE_AND_VERIFY_10 )
    return "WRITE AND VERIFY (10)";
  if( command == MMC::WRITE_BUFFER )
    return "WRITE BUFFER";

  return "unknown";
}
