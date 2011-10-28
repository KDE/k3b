/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
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
#include <errno.h>
#include <camlib.h>
#include <cam/scsi/scsi_message.h>
#include <cam/scsi/scsi_pass.h>

#define ERRCODE(s)	((((s)[2]&0x0F)<<16)|((s)[12]<<8)|((s)[13]))
#define EMEDIUMTYPE	EINVAL
#define	ENOMEDIUM	ENODEV
#define CREAM_ON_ERRNO(s)	do {                    \
        switch ((s)[12])                            \
        {	case 0x04:	errno=EAGAIN;	break;		\
        case 0x20:	errno=ENODEV;	break;          \
        case 0x21:	if ((s)[13]==0)	errno=ENOSPC;	\
			else		errno=EINVAL;               \
			break;                                  \
        case 0x30:	errno=EMEDIUMTYPE;  break;      \
        case 0x3A:	errno=ENOMEDIUM;    break;      \
        }                                           \
    } while(0)



class K3b::Device::ScsiCommand::Private
{
public:
    union ccb ccb;
};


void K3b::Device::ScsiCommand::clear()
{
    memset (&d->ccb,0,sizeof(ccb));
}


unsigned char& K3b::Device::ScsiCommand::operator[]( size_t i )
{
    if( d->ccb.csio.cdb_len < i+1 )
        d->ccb.csio.cdb_len = i+1;
    return d->ccb.csio.cdb_io.cdb_bytes[i];
}

int K3b::Device::ScsiCommand::transport( TransportDirection dir,
                                         void* data,
                                         size_t len )
{
    if( !m_device )
        return -1;

    m_device->usageLock();

    bool needToClose = false;
    if( !m_device->isOpen() ) {
        needToClose = true;
    }

    if( !m_device->open( true ) ) {
        m_device->usageUnlock();
        return -1;
    }
    d->ccb.ccb_h.path_id    = m_device->handle()->path_id;
    d->ccb.ccb_h.target_id  = m_device->handle()->target_id;
    d->ccb.ccb_h.target_lun = m_device->handle()->target_lun;

    kDebug() << "(K3b::Device::ScsiCommand) transport command " << QString::number((int)d->ccb.csio.cdb_io.cdb_bytes[0], 16) << ", length: " << (int)d->ccb.csio.cdb_len;
    int ret=0;
    int direction = CAM_DEV_QFRZDIS;
    if (!len)
        direction |= CAM_DIR_NONE;
    else
        direction |= (dir & TR_DIR_READ)?CAM_DIR_IN : CAM_DIR_OUT;
    cam_fill_csio (&(d->ccb.csio), 1, 0 /* NULL */, direction, MSG_SIMPLE_Q_TAG, (u_int8_t *)data, len, sizeof(d->ccb.csio.sense_data), d->ccb.csio.cdb_len, 30*1000);
    unsigned char * sense = (unsigned char *)&d->ccb.csio.sense_data;

    ret = cam_send_ccb(m_device->handle(), &d->ccb);

    if (ret < 0) {
        kDebug() << "(K3b::Device::ScsiCommand) transport failed: " << ret;

        if( needToClose )
            m_device->close();

        m_device->usageUnlock();

        struct scsi_sense_data* senset = (struct scsi_sense_data*)sense;
        int errorCode, senseKey, addSenseCode, addSenseCodeQual;
        scsi_extract_sense( senset, &errorCode, &senseKey, &addSenseCode,
                            &addSenseCodeQual );
        debugError( d->ccb.csio.cdb_io.cdb_bytes[0], errorCode, senseKey,
                    addSenseCode, addSenseCodeQual );

        int result = ((errorCode<<24)    & 0xF000 |
                      (senseKey<<16)     & 0x0F00 |
                      (addSenseCode<<8)  & 0x00F0 |
                      (addSenseCodeQual) & 0x000F );

        return result ? result : ret;
    }

    else if ((d->ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP) {
        if( needToClose )
            m_device->close();
        m_device->usageUnlock();
        return 0;
    }

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

        ret = cam_send_ccb(m_device->handle(), &d->ccb);

        d->ccb.csio.resid = resid;
        if (ret<0)
        {
            kDebug() << "(K3b::Device::ScsiCommand) transport failed (2): " << ret;
            ret = -1;
            struct scsi_sense_data* senset = (struct scsi_sense_data*)sense;
            int errorCode, senseKey, addSenseCode, addSenseCodeQual;
            scsi_extract_sense( senset, &errorCode, &senseKey, &addSenseCode,
                                &addSenseCodeQual );
            debugError( d->ccb.csio.cdb_io.cdb_bytes[0], errorCode, senseKey,
                        addSenseCode, addSenseCodeQual );

            if( needToClose )
                m_device->close();
            m_device->usageUnlock();

            return -1;
        }
        if ((d->ccb.ccb_h.status&CAM_STATUS_MASK) != CAM_REQ_CMP)
        {
            kDebug() << "(K3b::Device::ScsiCommand) transport failed (3): " << ret;
            errno=EIO,-1;
            ret = -1;
            struct scsi_sense_data* senset = (struct scsi_sense_data*)sense;
            int errorCode, senseKey, addSenseCode, addSenseCodeQual;
            scsi_extract_sense( senset, &errorCode, &senseKey, &addSenseCode,
                                &addSenseCodeQual );
            debugError( d->ccb.csio.cdb_io.cdb_bytes[0], errorCode, senseKey,
                        addSenseCode, addSenseCodeQual );

            if( needToClose )
                m_device->close();
            m_device->usageUnlock();

            return -1;
        }

        memcpy(sense,_sense,sizeof(_sense));
    }

    ret = ERRCODE(sense);
    kDebug() << "(K3b::Device::ScsiCommand) transport failed (4): " << ret;
    if (ret == 0)
        ret = -1;
    else
        CREAM_ON_ERRNO(((unsigned char *)&d->ccb.csio.sense_data));
    struct scsi_sense_data* senset = (struct scsi_sense_data*)sense;
    int errorCode, senseKey, addSenseCode, addSenseCodeQual;
    scsi_extract_sense( senset, &errorCode, &senseKey, &addSenseCode,
                        &addSenseCodeQual );
    debugError( d->ccb.csio.cdb_io.cdb_bytes[0], errorCode, senseKey,
                addSenseCode, addSenseCodeQual );

    if( needToClose )
        m_device->close();
    m_device->usageUnlock();

    return ret;
}
