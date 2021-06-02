/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    Parts of this file are inspired (and copied) from transport.hxx
    from the dvd+rw-tools (C) Andy Polyakov <appro@fy.chalmers.se>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bscsicommand.h"
#include "k3bdevice.h"

#include <QDebug>

#include <sys/ioctl.h>
#undef __STRICT_ANSI__
#include <linux/cdrom.h>
#define __STRICT_ANSI__
#include <scsi/sg.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>


#if !defined(SG_FLAG_LUN_INHIBIT)
# if defined(SG_FLAG_UNUSED_LUN_INHIBIT)
#  define SG_FLAG_LUN_INHIBIT SG_FLAG_UNUSED_LUN_INHIBIT
# else
#  define SG_FLAG_LUN_INHIBIT 0
# endif
#endif

#ifdef SG_IO
static bool useSgIo()
{
    struct utsname buf;
    uname( &buf );
    // was CDROM_SEND_PACKET declared dead in 2.5?
    return ( strcmp( buf.release, "2.5.43" ) >=0 );
}
#endif


class K3b::Device::ScsiCommand::Private
{
public:
    struct cdrom_generic_command cmd;
    struct request_sense sense;

#ifdef SG_IO
    bool useSgIo;
    struct sg_io_hdr sgIo;
#endif
};


void K3b::Device::ScsiCommand::clear()
{
    ::memset( &d->cmd, 0, sizeof(struct cdrom_generic_command) );
    ::memset( &d->sense, 0, sizeof(struct request_sense) );

    d->cmd.quiet = 1;
    d->cmd.sense = &d->sense;

#ifdef SG_IO
    d->useSgIo = useSgIo();
    ::memset( &d->sgIo, 0, sizeof(struct sg_io_hdr) );
#endif
}


unsigned char& K3b::Device::ScsiCommand::operator[]( size_t i )
{
#ifdef SG_IO
    if( d->sgIo.cmd_len < i+1 )
        d->sgIo.cmd_len = i+1;
#endif
    return d->cmd.cmd[i];
}


int K3b::Device::ScsiCommand::transport( TransportDirection dir,
                                         void* data,
                                         size_t len )
{
    bool needToClose = false;
    int deviceHandle = -1;
    if( m_device ) {
        m_device->usageLock();
        if( !m_device->isOpen() ) {
            needToClose = true;
        }
        if ( !m_device->open( dir == TR_DIR_WRITE ) ) {
            m_device->usageUnlock();
            return -1;
        }
        deviceHandle = m_device->handle();
    }

    if( deviceHandle == -1 ) {
        return -1;
    }

    int i = -1;

#ifdef SG_IO
    if( d->useSgIo ) {
        d->sgIo.interface_id= 'S';
        d->sgIo.mx_sb_len = sizeof( struct request_sense );
        d->sgIo.cmdp      = d->cmd.cmd;
        d->sgIo.sbp       = (unsigned char*)&d->sense;
        d->sgIo.flags     = SG_FLAG_LUN_INHIBIT|SG_FLAG_DIRECT_IO;
        d->sgIo.dxferp    = data;
        d->sgIo.dxfer_len = len;
        d->sgIo.timeout   = 5000;
        if( dir == TR_DIR_READ )
            d->sgIo.dxfer_direction = SG_DXFER_FROM_DEV;
        else if( dir == TR_DIR_WRITE )
            d->sgIo.dxfer_direction = SG_DXFER_TO_DEV;
        else
            d->sgIo.dxfer_direction = SG_DXFER_NONE;

        i = ::ioctl( deviceHandle, SG_IO, &d->sgIo );

        if( ( d->sgIo.info&SG_INFO_OK_MASK ) != SG_INFO_OK )
            i = -1;
    }
    else {
#endif
        d->cmd.buffer = (unsigned char*)data;
        d->cmd.buflen = len;
        if( dir == TR_DIR_READ )
            d->cmd.data_direction = CGC_DATA_READ;
        else if( dir == TR_DIR_WRITE )
            d->cmd.data_direction = CGC_DATA_WRITE;
        else
            d->cmd.data_direction = CGC_DATA_NONE;

        i = ::ioctl( deviceHandle, CDROM_SEND_PACKET, &d->cmd );
#ifdef SG_IO
    }
#endif

    if( needToClose )
        m_device->close();

    if ( m_device ) {
        m_device->usageUnlock();
    }

    if( i ) {
        debugError( d->cmd.cmd[0],
                    d->sense.error_code,
                    d->sense.sense_key,
                    d->sense.asc,
                    d->sense.ascq );

        int errCode =
            ((d->sense.error_code<<24) & 0xF000) |
            ((d->sense.sense_key<<16)  & 0x0F00) |
            ((d->sense.asc<<8)         & 0x00F0) |
            ((d->sense.ascq)           & 0x000F);

        return( errCode != 0 ? errCode : 1 );
    }
    else
        return 0;
}
