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


#include "k3bdevice.h"
#include "k3btrack.h"
#include "k3btoc.h"

#include <qstring.h>
#include <qfile.h>

#include <kdebug.h>
#include <kprocess.h>

typedef Q_INT16 size16;
typedef Q_INT32 size32;


#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>

#include <fstab.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/hdreg.h>
#include <linux/../scsi/scsi.h> /* cope with silly includes */
#include <linux/cdrom.h>
#include <linux/major.h>
#include <linux/limits.h>

#ifndef IDE_DISK_MAJOR
#define IDE_DISK_MAJOR(M)       ((M) == IDE0_MAJOR || (M) == IDE1_MAJOR || \
                                (M) == IDE2_MAJOR || (M) == IDE3_MAJOR || \
                                (M) == IDE4_MAJOR || (M) == IDE5_MAJOR || \
                                (M) == IDE6_MAJOR || (M) == IDE7_MAJOR || \
                                (M) == IDE8_MAJOR || (M) == IDE9_MAJOR)
#endif /* #ifndef IDE_DISK_MAJOR */



const char* K3bCdDevice::CdDevice::cdrdao_drivers[] =
  { "auto", "plextor", "plextor-scan", "cdd2600", "generic-mmc",
    "generic-mmc-raw", "ricoh-mp6200", "sony-cdu920",
    "sony-cdu948", "taiyo-yuden", "teac-cdr55", "toshiba",
    "yamaha-cdr10x", 0
  };




class K3bCdDevice::CdDevice::Private
{
public:
  Private()
  {
    deviceFd = -1;
  }

  QString blockDeviceName;
  QString genericDevice;
  int deviceType;
  interface interfaceType;
  QString mountPoint;
  QString mountDeviceName;
  QStringList allNodes;
  int deviceFd;
};


K3bCdDevice::CdDevice::CdDevice( const QString& devname )
{
  d = new Private;

  d->interfaceType = OTHER;
  d->blockDeviceName = devname;

  d->allNodes.append(devname);

  m_cdrdaoDriver = "auto";
  m_cdTextCapable = 0;
  m_maxWriteSpeed = 0;
  m_maxReadSpeed = 0;
  m_burnproof = false;
  m_burner = false;
  m_bWritesCdrw = false;
  m_bus = m_target = m_lun = -1;

  //   QString model( drive->drive_model );

  //   // the cd_paranoia-lib puts vendor, model, and version in one string
  //   // we need to split it
  //   int i;
  //   if( (i = model.find("ATAPI")) != -1 )
  //     model.remove( i, 5 );
  //   if( (i = model.find("compatible")) != -1 )
  //     model.remove( i, 10 );

  //   model = model.stripWhiteSpace();

  //   // we assume that all letters up to the first white space
  //   // belong to the vendor string and the rest is the model
  //   // description

  //   m_vendor = model.left( model.find(' ') ).stripWhiteSpace();
  //   m_description = model.mid( model.find(' ') ).stripWhiteSpace();
}


K3bCdDevice::CdDevice::~CdDevice()
{
  delete d;
}


bool K3bCdDevice::CdDevice::init()
{
  if(open() < 0)
    return false;

  int drivetype = ::ioctl(d->deviceFd, CDROM_GET_CAPABILITY, CDSL_CURRENT);
  if( drivetype < 0 )
  {
    kdDebug() << "Error while retrieving capabilities." << endl;
    close();
    return false;
  }

  d->deviceType = CDROM;  // all drives should be able to read cdroms

  if (drivetype & CDC_CD_R)
  {
    d->deviceType |= CDR;
  }
  if (drivetype & CDC_CD_RW)
  {
    d->deviceType |= CDRW;
  }
  if (drivetype & CDC_DVD_R)
  {
    d->deviceType |= DVDR;
  }
  if (drivetype & CDC_DVD_RAM)
  {
    d->deviceType |= DVDRAM;
  }
  if (drivetype & CDC_DVD)
  {
    d->deviceType |= DVDROM;
  }

  close();

  d->interfaceType = interfaceType();

  return furtherInit();
}


bool K3bCdDevice::CdDevice::furtherInit()
{
  if (d->interfaceType == IDE)
  {
    if (open() < 0)
      return false;
    else
    {
      struct hd_driveid hdId;
      ::ioctl( d->deviceFd, HDIO_GET_IDENTITY, &hdId );

      m_description = QString::fromLatin1((const char*)hdId.model, 40).stripWhiteSpace();
      m_vendor = m_description.left( m_description.find( " " ) );
      m_description = m_description.mid( m_description.find(" ")+1 );
      m_version = QString::fromLatin1((const char*)hdId.fw_rev, 8).stripWhiteSpace();

      close();
      return true;
    }
  }
  else
    return true;
}

K3bCdDevice::CdDevice::interface K3bCdDevice::CdDevice::interfaceType()
{
  if (d->interfaceType == OTHER)
  {
    // if the device is already opened we do not close it
    // to allow fast multible method calls in a row
    bool needToClose = !isOpen();

    if (open() < 0)
      return OTHER;

    // stat the device
    struct stat cdromStat;
    ::fstat( d->deviceFd, &cdromStat );

    if( IDE_DISK_MAJOR( cdromStat.st_rdev>>8 ) )
    {
      d->interfaceType = IDE;
    }
    else if ( SCSI_BLK_MAJOR( cdromStat.st_rdev>>8 ) )
    {
      d->interfaceType = SCSI;
    }

    if( needToClose )
      close();
  }
  return d->interfaceType;
}

int K3bCdDevice::CdDevice::type() const
{
  return d->deviceType;
}


const QString& K3bCdDevice::CdDevice::devicename() const
{
  return blockDeviceName();
}


const QString& K3bCdDevice::CdDevice::ioctlDevice() const
{
  return blockDeviceName();
}


const QString& K3bCdDevice::CdDevice::blockDeviceName() const
{
  return d->blockDeviceName;
}


const QString& K3bCdDevice::CdDevice::genericDevice() const
{
  return d->genericDevice;
}


QString K3bCdDevice::CdDevice::busTargetLun() const
{
#ifdef SUPPORT_IDE
  if (d->interfaceType == IDE)
    return QString("ATAPI:%1").arg(devicename().ascii());
  else
#endif
    return QString("%1,%2,%3").arg(m_bus).arg(m_target).arg(m_lun);
}


int K3bCdDevice::CdDevice::cdTextCapable() const
{
  if( cdrdaoDriver() == "auto" )
    return 0;
  else
    return m_cdTextCapable;
}


void K3bCdDevice::CdDevice::setCdTextCapability( bool b )
{
  m_cdTextCapable = ( b ? 1 : 2 );
}


void K3bCdDevice::CdDevice::setMountPoint( const QString& mp )
{
  d->mountPoint = mp;
}

void K3bCdDevice::CdDevice::setMountDevice( const QString& md )
{
  d->mountDeviceName = md;
}


const QString& K3bCdDevice::CdDevice::mountDevice() const
{
  return d->mountDeviceName;
}


const QString& K3bCdDevice::CdDevice::mountPoint() const
{
  return d->mountPoint;
}

void K3bCdDevice::CdDevice::setBurnproof( bool b )
{
  m_burnproof = b;
}

K3bDiskInfo::type  K3bCdDevice::CdDevice::diskType()
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  int status;
  K3bDiskInfo::type ret = K3bDiskInfo::UNKNOWN;
  if (open() < 0)
    return K3bDiskInfo::UNKNOWN;

  if ( (status = ::ioctl(d->deviceFd,CDROM_DISC_STATUS)) >= 0 )
  {
    switch (status)
    {
    case CDS_AUDIO:   ret = K3bDiskInfo::AUDIO;
      break;
    case CDS_DATA_1:
    case CDS_DATA_2:  ret = K3bDiskInfo::DATA;
      break;
    case CDS_XA_2_1:
    case CDS_XA_2_2:
    case CDS_MIXED:   ret = K3bDiskInfo::MIXED;
      break;
    case CDS_NO_DISC: ret = K3bDiskInfo::NODISC;
      break;
    case CDS_NO_INFO: ret = K3bDiskInfo::UNKNOWN;
    }
  }
  if ( isDVD() )
    ret =  K3bDiskInfo::DVD;

  if( needToClose )
    close();
  return ret;
}

bool K3bCdDevice::CdDevice::isDVD()
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  bool ret = false;
  if (open() < 0)
    return ret;

  if( d->deviceType & (DVDR | DVDRAM | DVDROM) )
  {
    //     try to read the physical dvd-structure
    //     if this fails, we probably cannot take any further (usefull) dvd-action
    dvd_struct dvdinfo;
    ::memset(&dvdinfo,0,sizeof(dvd_struct));
    dvdinfo.type = DVD_STRUCT_PHYSICAL;
    if ( ::ioctl(d->deviceFd,DVD_READ_STRUCT,&dvdinfo) == 0 )
      ret = true;
  }

  if( needToClose )
    close();
  return ret;
}

int K3bCdDevice::CdDevice::isReady() const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  int drive_status,ret;
  ret = 1;
  if(open() < 0)
    return ret;

  if( (drive_status = ::ioctl(d->deviceFd,CDROM_DRIVE_STATUS)) < 0 )
  {
    kdDebug() << "(K3bCdDevice) Error: could not get drive status" << endl;
    ret = 1;
  }
  else if ( drive_status == CDS_DISC_OK )
    ret = 0;
  else if ( drive_status == CDS_NO_DISC || drive_status == CDS_TRAY_OPEN )
    ret = 3;

  if( needToClose )
    close();
  return ret;
}


int K3bCdDevice::CdDevice::isEmpty()
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  int ret = NO_INFO;
  if (open() < 0)
    return NO_INFO;

  int drive_status = ::ioctl(d->deviceFd,CDROM_DRIVE_STATUS);
  if ( drive_status < 0 )
  {
    kdDebug() << "(K3bCdDevice) Error: could not get drive status" << endl;
    ret = NO_INFO;
  }
  else if ( drive_status == CDS_NO_DISC || drive_status == CDS_TRAY_OPEN )
  {
    // kernel bug ?? never seen CDS_NO_DISC
    kdDebug() << "(K3bCdDevice)  Error: No disk in drive" << endl;
    ret = NO_DISK;
  }
  else
  {
    struct cdrom_generic_command cmd;

    unsigned char inf[32];

    ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
    ::memset(inf,0,32);
    cmd.cmd[0] = GPCMD_READ_DISC_INFO;
    cmd.cmd[8] = 32;
    cmd.buffer = inf;
    cmd.buflen = 32;
    cmd.data_direction = CGC_DATA_READ;
    //
    // Disc Information Block
    // ======================
    // Byte   0-1: Disk Information Length
    // Byte     2: Bits 0-1 Disc Status: 0-empty,1-appendable,2-complete,3-other
    //             Bits 2-3 State of last Session: 0-empty,1-incomplete,2-reserved,3-complete
    //             Bit    4 Erasable, 1-cd-rw present
    //             Bits 5-7 Reserved
    // Byte     3: Number of first Track on Disk
    // Byte     4: Number of Sessions (LSB)
    // Byte     5: First Track in Last Session (LSB)
    // Byte     6: Last Track in Last Session (LSB)
    // Byte     7: Bits 0-4 Reserved
    //             Bit    5 URU Unrestricted Use Bit
    //             Bit    6 DBC_V Disc Bar Code Field is Valid
    //             Bit    7 DID_V Disc Id Field is valid
    // Byte     8: Disc Type: 0x00 CD-DA or CD-ROM, 0x10 CD-I, 0x20 CD-ROM XA, 0xFF undefined
    //                        all other values reserved
    // Byte     9: Number of Sessions (MSB)
    // Byte    10: First Track in Last Session (MSB)
    // Byte    11: Last Track in Last Session (MSB)
    // Byte 12-15: Disc Identification
    // Byte 16-19: Last Session Lead-In Start Time MSF (16-MSB 19-LSB)
    // Byte 20-23: Last Possible Start Time for Start of Lead-Out MSF (20-MSB 23-LSB)
    // Byte 24-31: Disc Bar Code
    //
    if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) == 0 )
    {
      ret = (inf[2] & 0x03);
    }
    else
    {
      kdDebug() << "(K3bCdDevice) could not get disk info !" << endl;
      ret = NO_INFO;
    }
  }

  if( needToClose )
    close();
  return ret;

}

K3b::Msf K3bCdDevice::CdDevice::discSize()
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  K3b::Msf ret(0);
  if (open() < 0)
    return ret;

  struct cdrom_generic_command cmd;

  unsigned char inf[32];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(inf,0,32);
  cmd.cmd[0] = GPCMD_READ_DISC_INFO;
  cmd.cmd[8] = 32;
  cmd.buffer = inf;
  cmd.buflen = 32;
  cmd.data_direction = CGC_DATA_READ;
  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) == 0 )
  {
    if ( inf[21] != 0xFF && inf[22] != 0xFF && inf[23] != 0xFF )
      ret.setMinutes((int)inf[21]);
    ret.setSeconds((int)inf[22]);
    ret.setFrames((int)inf[23]);
  }
  else
    kdDebug() << "(K3bCdDevice) could not get disk info !" << endl;

  if( needToClose )
    close();
  return ret;

}

K3b::Msf K3bCdDevice::CdDevice::remainingSize()
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  K3b::Msf ret(0);
  K3b::Msf size(0);
  if (open() < 0)
    return ret;

  struct cdrom_generic_command cmd;

  unsigned char inf[32];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(inf,0,32);
  cmd.cmd[0] = GPCMD_READ_DISC_INFO;
  cmd.cmd[8] = 32;
  cmd.buffer = inf;
  cmd.buflen = 32;
  cmd.data_direction = CGC_DATA_READ;
  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) == 0 )
  {
    if ( inf[17] != 0xFF && inf[18] != 0xFF && inf[19] != 0xFF )
    {
      ret.setMinutes((int)inf[17]);
      ret.setSeconds((int)inf[18]);
      ret.setFrames((int)inf[19]);
    }
    if ( inf[21] != 0xFF && inf[22] != 0xFF && inf[23] != 0xFF )
    {
      size.setMinutes((int)inf[21]);
      size.setSeconds((int)inf[22]);
      size.setFrames((int)inf[23]);
    }
  }
  else
    kdDebug() << "(K3bCdDevice) could not get disk info !" << endl;

  if( needToClose )
    close();
  return size-ret;
}

int K3bCdDevice::CdDevice::numSessions()
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  int ret=-1;
  if (open() < 0)
    return ret;

  struct cdrom_generic_command cmd;
  unsigned char dat[4];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(dat,0,4);
  cmd.cmd[0] = GPCMD_READ_TOC_PMA_ATIP;
  // Format Field: 0-TOC, 1-Session Info, 2-Full TOC, 3-PMA, 4-ATIP, 5-CD-TEXT
  cmd.cmd[2] = 1;
  cmd.cmd[8] = 4;
  cmd.buffer = dat;
  cmd.buflen = 4;
  cmd.data_direction = CGC_DATA_READ;
  //
  // Session Info
  // ============
  // Byte 0-1: Data Length
  // Byte   2: First Complete Session Number (Hex) - always 1
  // Byte   3: Last Complete Session Number (Hex)
  //
  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) == 0 )
    ret = dat[3];
  else
    kdDebug() << "(K3bCdDevice) could not get session info !" << endl;

  if( needToClose )
    close();
  return ret;
}

int K3bCdDevice::CdDevice::getTrackDataMode(int track)
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  int ret=-1;
  if (open() < 0)
    return ret;

  struct cdrom_generic_command cmd;
  unsigned char dat[8];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(dat,0,8);
  cmd.cmd[0] = GPCMD_READ_TRACK_RZONE_INFO;
  cmd.cmd[1] = 1; // track
  cmd.cmd[4] = (track & 0xFF00 ) >> 8;
  cmd.cmd[5] = (track & 0x00FF );

  cmd.cmd[8] = 8;
  cmd.quiet = 1;
  cmd.buffer = dat;
  cmd.buflen = 8;
  cmd.data_direction = CGC_DATA_READ;

  //
  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) == 0 )
    ret = dat[6] & 0x0F;
  else
    kdDebug() << "(K3bCdDevice) could not get track info !" << endl;

  if( needToClose )
    close();
  return ret;
}

K3bCdDevice::Toc K3bCdDevice::CdDevice::readToc()
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  Toc toc;

  struct cdrom_tochdr tochdr;
  struct cdrom_tocentry tocentry;

  if (open() != -1)
  {
    //
    // CDROMREADTOCHDR ioctl returns:
    // cdth_trk0: First Track Number
    // cdth_trk1: Last Track Number
    //
    if( ::ioctl(d->deviceFd,CDROMREADTOCHDR,&tochdr) )
    {
      kdDebug() << "(K3bCdDevice) could not get toc header !" << endl;
    }
    else
    {
      Track lastTrack;
      for (int i = tochdr.cdth_trk0; i <= tochdr.cdth_trk1 + 1; i++)
      {
        ::memset(&tocentry,0,sizeof (struct cdrom_tocentry));
        // get Lead-Out Information too
        tocentry.cdte_track = (i<=tochdr.cdth_trk1) ? i : CDROM_LEADOUT;
        tocentry.cdte_format = CDROM_LBA;
        //
        // CDROMREADTOCENTRY ioctl returns:
        // cdte_addr.lba: Start Sector Number (LBA Format requested)
        // cdte_ctrl:     4 ctrl bits
        //                   00x0b: 2 audio Channels(no pre-emphasis)
        //                   00x1b: 2 audio Channels(pre-emphasis)
        //                   10x0b: audio Channels(no pre-emphasis),reserved in cd-rw
        //                   10x1b: audio Channels(pre-emphasis),reserved in cd-rw
        //                   01x0b: data track, recorded uninterrupted
        //                   01x1b: data track, recorded incremental
        //                   11xxb: reserved
        //                   xx0xb: digital copy prohibited
        //                   xx1xb: digital copy permitted
        // cdte_addr:     4 addr bits (type of Q-Subchannel data)
        //                   0000b: no Information
        //                   0001b: current position data
        //                   0010b: MCN
        //                   0011b: ISRC
        //                   0100b-1111b:  reserved
        // cdte_datamode:  0: Data Mode1
        //                 1: CD-I
        //                 2: CD-XA Mode2
        //

        if( ::ioctl(d->deviceFd,CDROMREADTOCENTRY,&tocentry) )
          kdDebug() << "(K3bCdDevice) error reading tocentry " << i << endl;

        int startSec = tocentry.cdte_addr.lba;
        int control  = tocentry.cdte_ctrl & 0x0f;
        int mode     = tocentry.cdte_datamode;
        if( !lastTrack.isEmpty() )
        {
          toc.append( Track( lastTrack.firstSector(), startSec-1, lastTrack.type(), lastTrack.mode() ) );
        }
        int trackType = 0;
        int trackMode = Track::UNKNOWN;
        if( control & 0x04 )
        {
          trackType = Track::DATA;
          if( mode == 1 )
            trackMode = Track::MODE1;
          else if( mode == 2 )
            trackMode = Track::MODE2;
          if ( (mode=getTrackDataMode(tocentry.cdte_track)) != -1 )
            switch (mode)
            {
            case 1: trackMode = Track::MODE1;break;
            case 2: trackMode = Track::MODE2;break;
            }
        }
        else
          trackType = Track::AUDIO;

        lastTrack = Track( startSec, startSec, trackType, trackMode );
      }
    }

    if( needToClose )
      close();
  }

  return toc;
}

bool K3bCdDevice::CdDevice::block( bool b) const
{
  bool ret = false;
  if (open() < 0)
    return ret;

  if( ::ioctl(d->deviceFd,CDROM_LOCKDOOR, b ? 1 : 0 ) < 0 )
    kdDebug() << "(K3bCdDevice) Cannot block/unblock device " << devicename() << endl;
  else
    ret = true;

  close();
  return ret;
}

bool K3bCdDevice::CdDevice::rewritable()
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  bool ret = false;
  if ( !burner() )  // no chance to detect empty discs in readers
    return false;
  if ( isReady() != 0 )
    return false;

  if(open() < 0)
    return false;

  struct cdrom_generic_command cmd;

  unsigned char inf[32];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(inf,0,32);
  cmd.cmd[0] = GPCMD_READ_DISC_INFO;
  cmd.cmd[8] = 32;
  cmd.buffer = inf;
  cmd.buflen = 32;
  cmd.data_direction = CGC_DATA_READ;
  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) == 0 )
    ret = (inf[2] >> 4 ) & 0x01;
  else
    kdDebug() << "(K3bCdDevice) could not get disk info !" << endl;

  if( needToClose )
    close();
  return ret;
}

void K3bCdDevice::CdDevice::eject() const
{
  if(open() != -1 )
  {
    ::ioctl( d->deviceFd, CDROMEJECT );
    close();
  }
}


void K3bCdDevice::CdDevice::load() const
{
  if( open() != -1 )
  {
    ::ioctl( d->deviceFd, CDROMCLOSETRAY );
    close();
  }
}

void K3bCdDevice::CdDevice::addDeviceNode( const QString& n )
{
  if( !d->allNodes.contains( n ) )
    d->allNodes.append( n );
}


const QStringList& K3bCdDevice::CdDevice::deviceNodes() const
{
  return d->allNodes;
}


bool K3bCdDevice::CdDevice::supportsWriteMode( WriteMode w )
{
  return (m_writeModes & w);
}


int K3bCdDevice::CdDevice::open() const
{
  if( d->deviceFd == -1 )
    d->deviceFd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (d->deviceFd < 0)
  {
    kdDebug() << "(K3bCdDevice) Error: could not open device." << endl;
    d->deviceFd = -1;
  }

  return d->deviceFd;
}


void K3bCdDevice::CdDevice::close() const
{
  if( d->deviceFd != -1 )
  {
    ::close( d->deviceFd );
    d->deviceFd = -1;
  }
}


bool K3bCdDevice::CdDevice::isOpen() const
{
  return ( d->deviceFd != -1 );
}

