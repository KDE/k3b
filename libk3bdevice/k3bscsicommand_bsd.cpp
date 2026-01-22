/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2011 Andriy Gapon <avg@FreeBSD.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bscsicommand.h"
#include "k3bdevice.h"

#include <QDebug>

#include <stdio.h>
#include <errno.h>
#include <camlib.h>
#include <cam/scsi/scsi_message.h>
#include <cam/scsi/scsi_pass.h>

namespace /*anonymous*/
{
    inline int sense_to_err( const struct scsi_sense_data& s )
    {
        int errorCode, senseKey, addSenseCode, addSenseCodeQual;
        scsi_extract_sense( (struct scsi_sense_data*) &s, &errorCode,
                            &senseKey, &addSenseCode, &addSenseCodeQual );
        return (errorCode << 24) | (senseKey << 16) |
	       (addSenseCode << 8) | addSenseCodeQual;
    }
}


class K3b::Device::ScsiCommand::Private
{
    typedef union ccb CCB;

public:
    Private();
    int transport( const Device* device, TransportDirection dir, void* data, size_t len );
    unsigned char& operator[]( size_t i );
    void clear();
    const CCB& get_ccb() { return ccb; }

private:
    CCB ccb;
};


void K3b::Device::ScsiCommand::clear()
{
    d->clear();
}

unsigned char& K3b::Device::ScsiCommand::operator[]( size_t i )
{
    return (*d)[i];
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

    int ret = d->transport( m_device, dir, data, len );
    if( ret != 0 ) {
        const struct scsi_sense_data& s = d->get_ccb().csio.sense_data;
        int errorCode, senseKey, addSenseCode, addSenseCodeQual;
        scsi_extract_sense( (struct scsi_sense_data*) &s, &errorCode, &senseKey,
                            &addSenseCode, &addSenseCodeQual );
        debugError( d->get_ccb().csio.cdb_io.cdb_bytes[0],
                    errorCode,
                    senseKey,
                    addSenseCode,
                    addSenseCodeQual );
    }

    if( needToClose )
        m_device->close();
    m_device->usageUnlock();

    return ret;
}

K3b::Device::ScsiCommand::Private::Private()
{
    clear();
}

void K3b::Device::ScsiCommand::Private::clear()
{
    memset( &ccb, 0, sizeof(ccb) );
}

unsigned char& K3b::Device::ScsiCommand::Private::operator[]( size_t i )
{
    if( ccb.csio.cdb_len < i + 1 )
        ccb.csio.cdb_len = i + 1;
    return ccb.csio.cdb_io.cdb_bytes[i];
}

int K3b::Device::ScsiCommand::Private::transport( const Device* device, TransportDirection dir, void* data, size_t len )
{
    ccb.ccb_h.path_id    = device->handle()->path_id;
    ccb.ccb_h.target_id  = device->handle()->target_id;
    ccb.ccb_h.target_lun = device->handle()->target_lun;

    qDebug() << "(K3b::Device::ScsiCommand) transport command " << commandString(ccb.csio.cdb_io.cdb_bytes[0])
             << " (" << QString::number((int)ccb.csio.cdb_io.cdb_bytes[0], 16) << "), length: " << (int)ccb.csio.cdb_len;
    int direction = CAM_DEV_QFRZDIS;
    if (!len)
        direction |= CAM_DIR_NONE;
    else
        direction |= (dir & TR_DIR_READ) ? CAM_DIR_IN : CAM_DIR_OUT;

    cam_fill_csio( &(ccb.csio), 1, nullptr, direction, MSG_SIMPLE_Q_TAG, (uint8_t*)data, len, sizeof(ccb.csio.sense_data), ccb.csio.cdb_len, 30*1000 );
    int ret = cam_send_ccb( device->handle(), &ccb );
    if( ret < 0 ) {
        qCritical() << "(K3b::Device::ScsiCommand) transport cam_send_ccb failed: ret = " << ret
                 << ", errno = " << errno << ", cam_errbuf = " << cam_errbuf;
        return 1;
    }
    else if( (ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP ) {
        qDebug() << "(K3b::Device::ScsiCommand) transport succeeded";
        return 0;
    }

    qDebug() << "(K3b::Device::ScsiCommand) transport command failed: scsi_status = " << QString::number(ccb.csio.scsi_status, 16);

    if( ccb.csio.scsi_status == SCSI_STATUS_CHECK_COND &&
        !(ccb.ccb_h.status & CAM_AUTOSNS_VALID) &&
        ccb.csio.cdb_io.cdb_bytes[0] != MMC_REQUEST_SENSE )
    {
        qDebug() << "(K3b::Device::ScsiCommand) transport requesting sense data";

        struct scsi_sense_data sense;
        ScsiCommand::Private cmd;
        cmd[0] = MMC_REQUEST_SENSE;
        cmd[4] = SSD_MIN_SIZE;
        cmd[5] = 0; // Necessary to set the proper command length

        memset( &sense, 0, sizeof(sense) );
        ret = cmd.transport( device, TR_DIR_READ, &sense, SSD_MIN_SIZE );
        if( ret < 0 )
        {
            qWarning() << "(K3b::Device::ScsiCommand) transport getting sense data failed: " << ret;
            return 1;
        }

        ccb.csio.sense_data = sense;
        ccb.ccb_h.status |= CAM_AUTOSNS_VALID;
    }

    if( !(ccb.ccb_h.status & CAM_AUTOSNS_VALID) )
        qDebug() << "(K3b::Device::ScsiCommand) sense data is not available";

    ret = sense_to_err(ccb.csio.sense_data);
    if( ret == 0 )
        ret = 1;
    qDebug() << "(K3b::Device::ScsiCommand) transport failed: " << ret;
    return ret;
}
