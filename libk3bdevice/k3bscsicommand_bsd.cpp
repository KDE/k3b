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

#include <stdio.h>
#include <camlib.h>
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



class K3bCdDevice::ScsiCommand::Private
{
public:
  union ccb ccb;
};


void K3bCdDevice::ScsiCommand::clear()
{
  memset (&d->ccb,0,sizeof(ccb));
  if (!m_device || !m_device->cam()) return;
  d->ccb.ccb_h.path_id    = m_device->cam()->path_id;
  d->ccb.ccb_h.target_id  = m_device->cam()->target_id;
  d->ccb.ccb_h.target_lun = m_device->cam()->target_lun;
}


unsigned char& K3bCdDevice::ScsiCommand::operator[]( size_t i )
{
  d->ccb.csio.cdb_len = i+1;
  return d->ccb.csio.cdb_io.cdb_bytes[i];
}

int K3bCdDevice::ScsiCommand::transport( TransportDirection dir,
					 void* data,
					 size_t len )
{
  if( !m_device || !m_device->cam() )
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
      struct scsi_sense_data* senset = (struct scsi_sense_data*)sense;
      debugError( d->ccb.csio.cdb_io.cdb_bytes[0],
		  senset->error_code & SSD_ERRCODE,
		  senset->flags & SSD_KEY,
		  senset->flags & SSD_KEY,
		  senset->add_sense_code,
		  senset->add_sense_code_qual );
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
	  struct scsi_sense_data* senset = (struct scsi_sense_data*)sense;
	  debugError( d->ccb.csio.cdb_io.cdb_bytes[0],
		      senset->error_code & SSD_ERRCODE,
		      senset->flags & SSD_KEY,
		      senset->flags & SSD_KEY,
		      senset->add_sense_code,
		      senset->add_sense_code_qual );
	  return -1;
	}
      if ((d->ccb.ccb_h.status&CAM_STATUS_MASK) != CAM_REQ_CMP)
	{
	  kdDebug() << "(K3bCdDevice::ScsiCommand) transport failed (3): " << ret << endl;
	  errno=EIO,-1;
	  ret = -1;
	  struct scsi_sense_data* senset = (struct scsi_sense_data*)sense;
	  debugError( d->ccb.csio.cdb_io.cdb_bytes[0],
		      senset->error_code & SSD_ERRCODE,
		      senset->flags & SSD_KEY,
		      senset->flags & SSD_KEY,
		      senset->add_sense_code,
		      senset->add_sense_code_qual );
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
  struct scsi_sense_data* senset = (struct scsi_sense_data*)sense;
  debugError( d->ccb.csio.cdb_io.cdb_bytes[0],
	      senset->error_code & SSD_ERRCODE,
	      senset->flags & SSD_KEY,
	      senset->flags & SSD_KEY,
	      senset->add_sense_code,
	      senset->add_sense_code_qual );
  return ret;
}
