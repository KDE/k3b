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
#include "k3bdeviceglobals.h"
#include "k3btrack.h"
#include "k3btoc.h"
#include "k3bdiskinfo.h"
#include "k3bmmc.h"

#include <qstringlist.h>
#include <qfile.h>
#include <qglobal.h>

#include <kdebug.h>
#include <kprocess.h>
#include <klocale.h>

#include <sys/types.h>
#include <sys/ioctl.h>

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

#include <config.h>

#ifdef HAVE_RESMGR
extern "C" {
#include <resmgr.h>
}
#endif

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



int K3bCdDevice::openDevice( const char* name )
{
  int fd = -1;
#ifdef HAVE_RESMGR
  // first try resmgr
  fd = ::rsm_open_device( name, O_RDONLY | O_NONBLOCK );
  kdDebug() << "(K3bCdDevice) resmgr open: " << fd << endl;
#endif

  if( fd < 0 )
    fd = ::open( name, O_RDONLY | O_NONBLOCK );

  if( fd < 0 )
    fd = -1;

  return fd;
}



class K3bCdDevice::CdDevice::Private
{
public:
  Private()
    : deviceType(0),
      supportedProfiles(0),
      deviceFd(-1),
      burnfree(false) {
  }
  
  QString blockDeviceName;
  QString genericDevice;
  int deviceType;
  int supportedProfiles;
  interface interfaceType;
  QString mountPoint;
  QString mountDeviceName;
  QStringList allNodes;
  int deviceFd;
  bool burnfree;
  bool writesDvdPlusR;
  bool writesDvdR;
};


K3bCdDevice::CdDevice::CdDevice( const QString& devname )
  : m_writeModes(0)
{
  d = new Private;

  d->interfaceType = OTHER;
  d->blockDeviceName = devname;
  d->allNodes.append(devname);
  d->writesDvdPlusR = d->writesDvdR = false;

  m_cdrdaoDriver = "auto";
  m_cdTextCapable = 0;
  m_maxWriteSpeed = 0;
  m_maxReadSpeed = 0;
  d->burnfree = false;
  m_burner = false;
  m_bWritesCdrw = false;
  m_bus = m_target = m_lun = -1;
}


K3bCdDevice::CdDevice::~CdDevice()
{
  delete d;
}


bool K3bCdDevice::CdDevice::init()
{
  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << ": init()" << endl;

  if(open() < 0)
    return false;

  //
  // We probe all features of the device. Since not all devices support the GET CONFIGURATION command
  // we also query the mode page 2A and use the cdrom.h stuff to get as much information as possible
  //

  d->deviceType = 0;
  d->supportedProfiles = 0;

  struct cdrom_generic_command cmd;
  unsigned char header[8];
  ::memset( &cmd, 0, sizeof(struct cdrom_generic_command) );
  ::memset( header, 0, 8 );
  cmd.cmd[0] = 0x46;	// GET CONFIGURATION
  cmd.cmd[8] = 8;
  cmd.buffer = header;
  cmd.buflen = 8;
  cmd.data_direction = CGC_DATA_READ;
  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) ) {
    kdDebug() << "(K3bCdDevice) " << blockDeviceName() << ": GET_CONFIGURATION failed." << endl;
  }
  else {

    //
    // Now that we know the length of the returned data we just do it again with the
    // correct buffer size
    //

    int len = header[0]<<24 | header[1]<<16 | header[2]<<8 | header[3];
    unsigned char* profiles = new unsigned char[len];
    ::memset( profiles, 0, len );
    cmd.cmd[6] = len>>16;
    cmd.cmd[7] = len>>8;
    cmd.cmd[8] = len;
    cmd.buffer = profiles;
    cmd.buflen = len;
    if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) ) {
      kdDebug() << "(K3bCdDevice) " << blockDeviceName() << ": GET_CONFIGURATION with correct size failed." << endl;
    }
    else {
      for( int i = 8; i < len; ) {
	short feature = profiles[i]<<8 | profiles[i+1];
	int featureLen = profiles[i+3];
	i+=4; // skip feature header

	//
	// now i indexes the first byte of the feature dependant data
	//

	switch( feature ) {
	case 0x000: // Profile List
	  for( int j = 0; j < featureLen; j+=4 ) {
	    short profile = profiles[i+j]<<8|profiles[i+1+j];

	    switch (profile) {
	    case 0x10: d->supportedProfiles |= MEDIA_DVD_ROM;
	    case 0x11: d->supportedProfiles |= MEDIA_DVD_R_SEQ;
	    case 0x12: d->supportedProfiles |= MEDIA_DVD_RAM;
	    case 0x13: d->supportedProfiles |= MEDIA_DVD_RW_OVWR; 
	    case 0x14: d->supportedProfiles |= MEDIA_DVD_RW_SEQ;
	    case 0x1A: d->supportedProfiles |= MEDIA_DVD_PLUS_RW;
	    case 0x1B: d->supportedProfiles |= MEDIA_DVD_PLUS_R;
	    case 0x08: d->supportedProfiles |= MEDIA_CD_ROM;
	    case 0x09: d->supportedProfiles |= MEDIA_CD_R;
	    case 0x0A: d->supportedProfiles |= MEDIA_CD_RW;
	    default: 
	      kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " unknown profile: " 
			<< profile << endl;
	    }
	  }
	  break;

	case 0x001: // Core
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Core" << endl;
	  break;

	case 0x002: // Morphing
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Morphing" << endl;
	  break;

	case 0x003: // Removable Medium
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Removable Medium" << endl;
	  break;

	case 0x004: // Write Protect
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Write Protect" << endl;
	  break;

	  // 0x05 - 0x0F reserved

	case 0x010: // Random Readable
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Random Readable" << endl;
	  break;

	  // 0x11 - 0x1C reserved

	case 0x01D: // Multi-Read
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Multi-Read" << endl;
	  break;

	case 0x01E: // CD Read
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "CD Read" << endl;
	  break;

	case 0x01F: // DVD Read
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "DVD Read" << endl;
	  d->deviceType |= DVD;
	  break;

	case 0x020: // Random Writable
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Random Writable" << endl;
	  break;

	case 0x021: // Incremental Streaming Writable
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Incremental Streaming Writable" << endl;
	  break;

	case 0x022: // Sector Erasable
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Sector Erasable" << endl;
	  break;

	case 0x023: // Formattable
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Formattable" << endl;
	  break;

	case 0x024: // Defect Management
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Defect Management" << endl;
	  break;

	case 0x025: // Write Once
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Write Once" << endl;
	  break;

	case 0x026: // Restricted Overwrite
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Restricted Overwrite" << endl;
	  break;

	case 0x027: // CD-RW CAV Write
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "CD-RW CAV Write" << endl;
	  d->deviceType |= CDRW;
	  break;

	case 0x028: // MRW
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "MRW" << endl;
	  break;

	case 0x029: // Enhanced Defect Reporting
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Enhanced Defect Reporting" << endl;
	  break;

	case 0x02A: // DVD+RW
	  {
	    kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "DVD+RW" << endl;
#if __BYTE_ORDER == __BIG_ENDIAN
	    struct dvd_plus_rw_feature {
	      unsigned char reserved1   : 7;
	      unsigned char write       : 1;
	      unsigned char reserved2   : 6;
	      unsigned char quick_start : 1;
	      unsigned char close_only  : 1;
	      // and some stuff we do not use here...
	    };
#else
	    struct dvd_plus_rw_feature {
	      unsigned char write       : 1;
	      unsigned char reserved1   : 7;
	      unsigned char close_only  : 1;
	      unsigned char quick_start : 1;
	      unsigned char reserved2   : 6;
	      // and some stuff we do not use here...
	    };
#endif
	    
	    struct dvd_plus_rw_feature* p = (struct dvd_plus_rw_feature*)&profiles[i];
	    if( p->write ) d->deviceType |= DVDPRW;
	    break;
	  }

	case 0x02B: // DVD+R
	  {
	    kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "DVD+R" << endl;
#if __BYTE_ORDER == __BIG_ENDIAN
	    struct dvd_plus_r_feature {
	      unsigned char reserved1 : 7;
	      unsigned char write     : 1;
	      unsigned char reserved2[3];
	      // and some stuff we do not use here...
	    };
#else
	    struct dvd_plus_r_feature {
	      unsigned char write     : 1;
	      unsigned char reserved1 : 7;
	      unsigned char reserved2[3];
	      // and some stuff we do not use here...
	    };
#endif
	    struct dvd_plus_r_feature* p = (struct dvd_plus_r_feature*)&profiles[i];
	    if( p->write ) d->deviceType |= DVDPR;
	    break;
	  }

	case 0x02C: // Rigid Restricted Overwrite
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Rigid Restricted Overwrite" << endl;
	  break;

	case 0x02D: // CD Track At Once
	  {
	    kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "CD Track At Once" << endl;
#if __BYTE_ORDER == __BIG_ENDIAN
	    struct cd_track_at_once_feature {
	      unsigned char reserved1 : 1;
	      unsigned char BUF       : 1;  // Burnfree
	      unsigned char reserved2 : 1;
	      unsigned char rw_raw    : 1;  // Writing R-W subcode in Raw mode
	      unsigned char rw_pack   : 1;  // Writing R-W subcode in Packet mode
	      unsigned char testwrite : 1;  // Simulation write support
	      unsigned char cd_rw     : 1;  // CD-RW support
	      unsigned char rw_sub    : 1;  // Write R-W sub channels with user data
	      unsigned char reserved3;
	      unsigned char data_type[2];
	    };
#else
	    struct cd_track_at_once_feature {
	      unsigned char rw_sub    : 1;  // Write R-W sub channels with user data
	      unsigned char cd_rw     : 1;  // CD-RW support
	      unsigned char testwrite : 1;  // Simulation write support
	      unsigned char rw_pack   : 1;  // Writing R-W subcode in Packet mode
	      unsigned char rw_raw    : 1;  // Writing R-W subcode in Raw mode
	      unsigned char reserved2 : 1;
	      unsigned char BUF       : 1;  // Burnfree
	      unsigned char reserved1 : 1;
	      unsigned char reserved3;
	      unsigned char data_type[2];
	    };
#endif

	    struct cd_track_at_once_feature* p = (struct cd_track_at_once_feature*)&profiles[i];
	    m_writeModes |= TAO;
	    if( p->BUF ) d->burnfree = true;
	    d->deviceType |= CDR;
	    if( p->cd_rw ) d->deviceType |= CDRW;
	    break;
	  }

	case 0x02E: // CD Mastering
	  {
	    kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "CD Mastering" << endl;
#if __BYTE_ORDER == __BIG_ENDIAN
	    struct cd_mastering_feature {
	      unsigned char reserved1 : 1;
	      unsigned char BUF       : 1;  // Burnfree
	      unsigned char SAO       : 1;  // Session At Once writing
	      unsigned char raw_ms    : 1;  // Writing Multisession in Raw Writing Mode
	      unsigned char raw       : 1;  // Writing in RAW mode
	      unsigned char testwrite : 1;  // Simulation write support
	      unsigned char cd_rw     : 1;  // CD-RW support
	      unsigned char rw_sub    : 1;  // Write R-W sub channels with user data
	      unsigned char max_cue_length[3];
	    };
#else
	    struct cd_mastering_feature {
	      unsigned char rw_sub    : 1;  // Write R-W sub channels with user data
	      unsigned char cd_rw     : 1;  // CD-RW support
	      unsigned char testwrite : 1;  // Simulation write support
	      unsigned char raw       : 1;  // Writing in RAW mode
	      unsigned char raw_ms    : 1;  // Writing Multisession in Raw Writing Mode
	      unsigned char SAO       : 1;  // Session At Once writing
	      unsigned char BUF       : 1;  // Burnfree
	      unsigned char reserved1 : 1;
	      unsigned char max_cue_length[3];
	    };
#endif

	    struct cd_mastering_feature* p = (struct cd_mastering_feature*)&profiles[i];
	    if( p->BUF ) d->burnfree = true;
	    d->deviceType |= CDR;
	    if( p->cd_rw ) d->deviceType |= CDRW;
	    if( p->SAO ) m_writeModes |= SAO;
	    if( p->raw || p->raw_ms ) m_writeModes |= RAW;  // perhaps we should extend this to RAW/R96P|R
	    break;
	  }

	case 0x02F: // DVD-R/-RW Write
	  {
	    kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "DVD-R/-RW Write" << endl;
#if __BYTE_ORDER == __BIG_ENDIAN
	    struct dvd_r_rw_write_feature {
	      unsigned char reserved1 : 1;
	      unsigned char BUF       : 1;  // Burnfree
	      unsigned char reserved2 : 3;
	      unsigned char testwrite : 1;  // Simulation write support
	      unsigned char dvd_rw    : 1;  // DVD-RW Writing
	      unsigned char reserved3 : 1;
	      unsigned char reserved4[3];
	    };
#else
	    struct dvd_r_rw_write_feature {
	      unsigned char reserved3 : 1;
	      unsigned char dvd_rw    : 1;  // DVD-RW Writing
	      unsigned char testwrite : 1;  // Simulation write support
	      unsigned char reserved2 : 3;
	      unsigned char BUF       : 1;  // Burnfree
	      unsigned char reserved1 : 1;
	      unsigned char reserved4[3];
	    };
#endif

	    struct dvd_r_rw_write_feature* p = (struct dvd_r_rw_write_feature*)&profiles[i];
	    if( p->BUF ) d->burnfree = true;
	    d->deviceType |= DVDR;
	    if( p->dvd_rw ) d->deviceType |= DVDRW;

	    // TODO: p->testwrite tells us if the writer supports DVD-R(W) dummy writes!!!
	    break;
	  }

	case 0x030: // DDCD Read
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "DDCD Read" << endl;
	  break;

	case 0x031: // DDCD-R Write
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "DDCD-R Write" << endl;
	  break;

	case 0x032: // DDCD-RW Write
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "DDCD-RW Write" << endl;
	  break;

	  // 0x33 0x38

	case 0x037: // CD-RW Media Write Support
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "CD-RW Media Write Support" << endl;
	  d->deviceType |= CDRW;
	  break;

	  // 0x38- 0xFF reserved

	case 0x100: // Power Management
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Power Management" << endl;
	  break;

	  // 0x101 reserved

	case 0x102: // Embedded Changer
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Embedded Changer" << endl;
	  break;

	case 0x103: // CD Audio analog play
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "CD Audio analog play" << endl;
	  break;

	case 0x104: // Microcode Upgrade
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Microcode Upgrade" << endl;
	  break;

	case 0x105: // Timeout
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Timeout" << endl;
	  break;

	case 0x106: // DVD-CSS
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "DVD-CSS" << endl;
	  break;

	case 0x107: // Read Time Streaming
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Read Time Streaming" << endl;
	  break;

	case 0x108: // Logical Unit Serial Number
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Logical Unit Serial Number" << endl;
	  break;

	  // 0x109 reserved

	case 0x10A: // Disc Control Blocks
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Disc Control Blocks" << endl;
	  break;

	case 0x10B: // DVD CPRM
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "DVD CPRM" << endl;
	  break;

	  //  0x10C - 0x1FE reserved

	case 0x1FF: // Firmware Date
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "Firmware Date" << endl;
	  break;

	  // 0x200 - 0xFEFF reserved

	  // 0xFF00 - 0xFFFF vendor specific

	default:
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << ": unknown feature: " 
		    << feature << endl;
	  break;
	}

	// skip feature dependent data
	i += featureLen;
      }
    }

    delete [] profiles;
  }



  //
  // Most current drives support the 2A mode page
  // Here we can get some more information (cdrecord -prcap does exactly this)
  //
  mm_cap_page_2A mm_p;
  if( readModePage2A( &mm_p ) ) {
    if( mm_p.BUF ) d->burnfree = true;
    if( mm_p.cd_rw_write ) d->deviceType |= CDRW;
    m_maxWriteSpeed = (int)( (mm_p.max_write_speed[0]<<8 | mm_p.max_write_speed[1]) * 1000.0 / ( 2352.0 * 75.0 ) );
    m_maxReadSpeed = (int)( (mm_p.max_read_speed[0]<<8 | mm_p.max_read_speed[1]) * 1000.0 / ( 2352.0 * 75.0 ) );
  }
  else {
    kdDebug() << "(K3bCdDevice) " << blockDeviceName() << ": read mode page 2A failed!" << endl;
  }



  //
  // This is the backup if the drive does not support the GET CONFIGURATION command
  // In this case we do not as much information
  //
  int drivetype = ::ioctl(d->deviceFd, CDROM_GET_CAPABILITY, CDSL_CURRENT);
  if( drivetype < 0 ) {
    kdDebug() << "Error while retrieving capabilities." << endl;
    close();
    return false;
  }
    
  d->deviceType |= CDROM;  // all drives should be able to read cdroms
  
  if (drivetype & CDC_CD_R) {
    d->deviceType |= CDR;
  }
  if (drivetype & CDC_CD_RW) {
    d->deviceType |= CDRW;
  }
  if (drivetype & CDC_DVD_R) {
    d->deviceType |= DVDR;
  }
  if (drivetype & CDC_DVD_RAM) {
    d->deviceType |= DVDRAM;
  }
  if (drivetype & CDC_DVD) {
    d->deviceType |= DVD;
  }
  

  if( burner() )
    checkWriteModes();


  // inquiry
  unsigned char inq[36];
  ::memset( &cmd, 0, sizeof(struct cdrom_generic_command) );
  ::memset( inq, 0, sizeof(inq) );
  cmd.cmd[0] = GPCMD_INQUIRY; // 0x12
  cmd.cmd[4] = sizeof(inq);
  cmd.cmd[5] = 0;
  cmd.buffer = inq;
  cmd.buflen = sizeof(inq);
  cmd.data_direction = CGC_DATA_READ;
  if( ::ioctl( open(), CDROM_SEND_PACKET, &cmd) ) {
    kdError() << "(K3bCdDevice) Unable to do inquiry." << endl;
    close();
    return false;
  }
  else {
    m_vendor = QString::fromLocal8Bit( (char*)(inq+8), 8 ).stripWhiteSpace();
    m_description = QString::fromLocal8Bit( (char*)(inq+16), 16 ).stripWhiteSpace();
    m_version = QString::fromLocal8Bit( (char*)(inq+32), 4 ).stripWhiteSpace();
  }

  close();

  d->interfaceType = interfaceType();

  return furtherInit();
}


bool K3bCdDevice::CdDevice::furtherInit()
{
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

    if( IDE_DISK_MAJOR( cdromStat.st_rdev>>8 ) ) {
      d->interfaceType = IDE;
    } else if ( SCSI_BLK_MAJOR( cdromStat.st_rdev>>8 ) ) {
      d->interfaceType = SCSI;
    }

    if( needToClose )
      close();
  }
  return d->interfaceType;
}


bool K3bCdDevice::CdDevice::dao() const 
{
  return m_writeModes & SAO;
}


bool K3bCdDevice::CdDevice::burner() const
{
  return d->deviceType & CDR;
}


bool K3bCdDevice::CdDevice::writesCdrw() const
{
  return d->deviceType & CDRW;
}


bool K3bCdDevice::CdDevice::writesDvd() const
{
  return d->deviceType & (DVDR|DVDPR|DVDRW|DVDPRW);
}

bool K3bCdDevice::CdDevice::readsDvd() const
{
  return d->deviceType & (DVD|DVDR|DVDRAM);
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
  d->burnfree = b;
}


bool K3bCdDevice::CdDevice::burnproof() const
{
  return burnfree();
}


bool K3bCdDevice::CdDevice::burnfree() const
{
  return d->burnfree;
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

  if ( (status = ::ioctl(d->deviceFd,CDROM_DISC_STATUS)) >= 0 ) {
    switch (status) {
    case CDS_AUDIO:   ret = K3bDiskInfo::AUDIO;
      break;
    case CDS_DATA_1:
    case CDS_DATA_2:  ret = K3bDiskInfo::DATA;
      break;
    case CDS_MIXED:   ret = K3bDiskInfo::MIXED;
      break;
    case CDS_NO_DISC: ret = K3bDiskInfo::NODISC;
      break;
    case CDS_NO_INFO: ret = K3bDiskInfo::UNKNOWN;
    }
  }
  if ( isDVD() )
    ret =  K3bDiskInfo::DVD;

  if (tocType() == 0x20)
    kdDebug() << "(K3bCdDevice) CD_XA disc found !" << endl;

  if( needToClose )
    close();
  return ret;
}

bool K3bCdDevice::CdDevice::isDVD() const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  bool ret = false;
  if (open() < 0)
    return ret;

  if( d->deviceType & (DVDR | DVDRAM | DVD) ) {
    //     try to read the physical dvd-structure
    //     if this fails, we probably cannot take any further (useful) dvd-action
    dvd_struct dvdinfo;
    ::memset(&dvdinfo,0,sizeof(dvd_struct));
    dvdinfo.type = DVD_STRUCT_PHYSICAL;
    if ( ::ioctl(d->deviceFd,DVD_READ_STRUCT,&dvdinfo) == 0 )
      ret = true;
    else
      kdDebug() << "(K3bCdDevice::CdDevice) no DVD" << endl;
  }
  else
    kdDebug() << "(K3bCdDevice::CdDevice) no DVD drive" << endl;

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

  if( (drive_status = ::ioctl(d->deviceFd,CDROM_DRIVE_STATUS)) < 0 ) {
    kdDebug() << "(K3bCdDevice) Error: could not get drive status" << endl;
    ret = 1;
  } else if ( drive_status == CDS_DISC_OK )
    ret = 0;
  else if ( drive_status == CDS_NO_DISC || drive_status == CDS_TRAY_OPEN )
    ret = 3;

  if( needToClose )
    close();
  return ret;
}


int K3bCdDevice::CdDevice::isEmpty() const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  int ret = NO_INFO;
  if (open() < 0)
    return NO_INFO;

  int drive_status = ::ioctl(d->deviceFd,CDROM_DRIVE_STATUS);
  if ( drive_status < 0 ) {
    kdDebug() << "(K3bCdDevice) Error: could not get drive status" << endl;
    ret = NO_INFO;
  } else if ( drive_status == CDS_NO_DISC || drive_status == CDS_TRAY_OPEN ) {
    // kernel bug ?? never seen CDS_NO_DISC
    kdDebug() << "(K3bCdDevice)  Error: No disk in drive" << endl;
    ret = NO_DISK;
  } else {
    disc_info_t inf;
    if( getDiscInfo( &inf ) ) {
      switch( inf.status ) {
      case 0:
        ret = EMPTY;
        break;
      case 1:
        ret = APPENDABLE;
        break;
      case 2:
        ret = COMPLETE;
        break;
      default:
        ret = NO_INFO;
        break;
      }
    }
    else {
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

  disc_info_t inf;
  if( getDiscInfo( &inf ) ) {
    if ( inf.lead_out_m != 0xFF && inf.lead_out_s != 0xFF && inf.lead_out_f != 0xFF ) {
      ret = K3b::Msf( inf.lead_out_m, inf.lead_out_s, inf.lead_out_f );
      ret -= 150;
    }
  }

  if( ret == 0 ) {
    kdDebug() << "(K3bCdDevice) getting disk size via toc." << endl;
    struct cdrom_tocentry tocentry;
    tocentry.cdte_track = CDROM_LEADOUT;
    tocentry.cdte_format = CDROM_LBA;
    if( ::ioctl(d->deviceFd,CDROMREADTOCENTRY,&tocentry) )
      kdDebug() << "(K3bCdDevice) error reading lead out " << endl;
    else {
      ret = tocentry.cdte_addr.lba;
      ret -= 1;  // we need the last sector of the last track, not the first from the lead-out
    }
  }

  if( needToClose )
    close();

  return ret;
}


bool K3bCdDevice::CdDevice::getDiscInfo( K3bCdDevice::disc_info_t* info ) const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();
  bool success = true;

  if (open() < 0)
    return false;

  struct cdrom_generic_command cmd;

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(info, 0, sizeof(disc_info_t));

  cmd.cmd[0] = GPCMD_READ_DISC_INFO;
  cmd.cmd[8] = sizeof(disc_info_t);
  cmd.buffer = (unsigned char*)info;
  cmd.buflen = sizeof(disc_info_t);
  cmd.data_direction = CGC_DATA_READ;

  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) ) {
    success = false;
    kdDebug() << "(K3bCdDevice::CdDevice) could not get disk info (size: "
	      << sizeof(disc_info_t) << ")" << endl;
  }

  if( needToClose )
    close();

  return success;
}


K3b::Msf K3bCdDevice::CdDevice::remainingSize()
{
  K3b::Msf ret, size;
  bool empty = false;
  bool appendable = false;

  disc_info_t inf;

  if( getDiscInfo( &inf ) ) {
    if ( inf.lead_in_m != 0xFF && inf.lead_in_s != 0xFF && inf.lead_in_f != 0xFF ) {
      ret = K3b::Msf( inf.lead_in_m, inf.lead_in_s, inf.lead_in_f );
    }
    if ( inf.lead_out_m != 0xFF && inf.lead_out_s != 0xFF && inf.lead_out_f != 0xFF ) {
      size = K3b::Msf( inf.lead_out_m, inf.lead_out_s, inf.lead_out_f );
    }

    empty = !inf.status;
    appendable = ( inf.status == 1 );
  }
  else
    return 0;

  if( empty )
    return size - 150;
  else if( appendable )
    return size - ret - 4650;
  else
    return 0;
}

int K3bCdDevice::CdDevice::numSessions() const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  int ret=-1;
  if (open() < 0)
    return ret;

  // 
  // Althought disk_info should get the real value without ide-scsi
  // I keep getting wrong values (the value is too high. I think the leadout
  // gets counted as session)
  //

//   disc_info_t inf;
//   if( getDiscInfo( &inf ) ) {
//     ret = inf.n_sessions_l | (inf.n_sessions_m << 8);
//   }
//   else {
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
    //  }

  if( needToClose )
    close();
  return ret;
}

int K3bCdDevice::CdDevice::tocType() const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  int ret=-1;
  if (open() < 0)
    return ret;

  struct cdrom_generic_command cmd;
  unsigned char dat[15];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(dat,0,15);
  cmd.cmd[0] = GPCMD_READ_TOC_PMA_ATIP;
  // Format Field: 0-TOC, 1-Session Info, 2-Full TOC, 3-PMA, 4-ATIP, 5-CD-TEXT
  cmd.cmd[1] = 2;
  cmd.cmd[2] = 2;
  cmd.cmd[8] = 15;
  cmd.buffer = dat;
  cmd.buflen = 15;
  cmd.data_direction = CGC_DATA_READ;
  //
  // Full Toc
  // ============
  // Byte 0-1: Data Length
  // Byte   2: First Complete Session Number (Hex) - always 1
  // Byte   3: Last Complete Session Number (Hex)
  //   TOC Track Descriptors
  // Byte   4: Session Number
  // Byte   5: ADR | CTRL
  // Byte   6: TNO
  // Byte   7; POINT
  // Byte   8: Min
  // Byte   9: Sec
  // Byte  10: Frame
  // Byte  11: Zero
  // Byte  12: PMIN
  // Byte  13: PSEC
  // Byte  14: PFRAME
  //
  // We are interested in POINT A0 (always first) PSEC field
  // 0x00 - CD_DA or CD_ROM
  // 0x10 - CD-I
  // 0x20 - CD_XA
  //

  // WHY DON'T WE DO THIS WITH DISC_INFO???

  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) == 0 )
    if ( dat[7] == 0xA0 )
      ret = dat[13];
    else
      kdDebug() << "(K3bCdDevice) could not get toc type !" << endl;

  if( needToClose )
    close();
  return ret;
}

int K3bCdDevice::CdDevice::getTrackDataMode(int lba)
{
//   bool needToClose = !isOpen();
//
//   int ret = Track::UNKNOWN;
//   if (open() < 0)
//     return ret;
//
//   struct cdrom_generic_command cmd;
//   unsigned char dat[8];
//
//   ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
//   ::memset(dat,0,8);
//   cmd.cmd[0] = GPCMD_READ_HEADER;
//   cmd.cmd[1] = 0; // lba
//   cmd.cmd[3] = (lba & 0xFF0000 ) >> 16;
//   cmd.cmd[4] = (lba & 0x00FF00 ) >> 8;
//   cmd.cmd[5] = (lba & 0x0000FF );
//
//   cmd.cmd[8] = 8;
//   cmd.quiet = 1;
//   cmd.buffer = dat;
//   cmd.buflen = 8;
//   cmd.timeout = 4*60*1000;
//   cmd.data_direction = CGC_DATA_READ;
//
//   //
//   if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) == 0 ) {
//     switch( (int)dat[0] ) {
//     case 1:
//       ret = Track::MODE1;
//       break;
//     case 2:
//       ret = Track::MODE2;
//       break;
//     default:
//       ret = Track::UNKNOWN;
//     }
//   } else
//     kdDebug() << "(K3bCdDevice) could not get track header, (lba " << lba << ") ! "  << strerror(errno) << endl;
//   if( needToClose )
//     close();
//
//   return ret;
  bool needToClose = !isOpen();

  int ret = Track::UNKNOWN;
  if (open() < 0)
    return ret;

  K3b::Msf msf(lba + CD_MSF_OFFSET);
  unsigned char data[CD_FRAMESIZE_RAW];
  data[0] = msf.minutes();
  data[1] = msf.seconds();
  data[2] = msf.frames();

  if ( ::ioctl(d->deviceFd,CDROMREADRAW,data) == -1 )
    kdDebug() << "(K3bCdDevice) could not get track header, (lba " << lba << ") ! "  << strerror(errno) << endl;
  else {
    if ( data[15] == 1 )
      ret = Track::MODE1;
    else if ( data[15] == 2 )
      ret = Track::MODE2;
    if ( ret == Track::MODE2 ) {
      if ( data[16] == data[20] &&
           data[17] == data[21] &&
           data[18] == data[22] &&
           data[19] == data[23] ) {
          if ( data[18] & 0x20 )
            ret = Track::XA_FORM2;
          else
            ret = Track::XA_FORM1;
       }
    }
  }
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

  if (open() != -1) {
    //
    // CDROMREADTOCHDR ioctl returns:
    // cdth_trk0: First Track Number
    // cdth_trk1: Last Track Number
    //
    if( ::ioctl(d->deviceFd,CDROMREADTOCHDR,&tochdr) ) {
      kdDebug() << "(K3bCdDevice) could not get toc header !" << endl;
    } else {
      Track lastTrack;
      for (int i = tochdr.cdth_trk0; i <= tochdr.cdth_trk1 + 1; i++) {
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
        if( i > tochdr.cdth_trk0 ) {
          toc.append( Track( lastTrack.firstSector(), startSec-1, lastTrack.type(), lastTrack.mode() ) );
        }
        int trackType = 0;
        int trackMode = Track::UNKNOWN;
        if( (control & 0x04 ) && (tocentry.cdte_track != CDROM_LEADOUT) ) {
          trackType = Track::DATA;
          if( mode == 1 )
            trackMode = Track::MODE1;
          else if( mode == 2 )
            trackMode = Track::MODE2;

          mode = getTrackDataMode(startSec);
          if( mode != Track::UNKNOWN )
            trackMode = mode;
        } else
          trackType = Track::AUDIO;

        lastTrack = Track( startSec, startSec, trackType, trackMode );
      }
    }

    //
    // we probaly need to fixup the toc for multisession mixed-mode cds 
    // since the last audio track's last sector is reported to be in the second
    // session. This code is based on the FixupTOC stuff from the audiocd kioslave
    // Hopefully it works. TODO: we need something better here!
    //
    if( numSessions() > 1 && toc.contentType() == MIXED ) {
      kdDebug() << "(K3bCdDevice::CdDevice) fixup multisession toc..." << endl;

      // we need to update the last secotor of every last track in every session
      // for now we only update the track before the last session...

      struct cdrom_multisession ms;
      ms.addr_format = CDROM_LBA;
      if( ::ioctl( d->deviceFd, CDROMMULTISESSION, &ms ) == 0 ) {
	if( ms.xa_flag ) {
	  toc[toc.count()-2].setLastSector( ms.addr.lba - 11400 -1 ); // -1 because we are settings the last secotor, not the first of the next track as in the kioslave
	  kdDebug() << "(K3bCdDevice::CdDevice) success." << endl;
	}
	else
	  kdDebug() << "(K3bCdDevice::CdDevice) no xa." << endl;
      }
      else
	kdDebug() << "(K3bCdDevice::CdDevice) CDROMMULTISESSION failed." << endl;
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

bool K3bCdDevice::CdDevice::rewritable() const
{
  if( !burner() )  // no chance to detect empty discs in readers
    return false;

  if( isReady() != 0 )
    return false;

  disc_info_t inf;

  if( getDiscInfo( &inf ) )
    return inf.erasable;
  else
    return false;
}

bool K3bCdDevice::CdDevice::eject()
{
  if ( !KIO::findDeviceMountPoint(d->mountDeviceName).isEmpty() )
    unmount();
  if(open() != -1 ) {
    int r = ::ioctl( d->deviceFd, CDROMEJECT );
    close();
    return (r == 0);
  }
  else
    return false;
}


bool K3bCdDevice::CdDevice::load()
{
  if( open() != -1 ) {
    int r = ::ioctl( d->deviceFd, CDROMCLOSETRAY );
    close();
    return (r == 0);
  }
  return false;
}

int K3bCdDevice::CdDevice::mount()
{
  int ret = -1;
  if( !KIO::findDeviceMountPoint(d->mountDeviceName).isEmpty() )
    return 0;

  QString cmd("/bin/mount ");
  cmd += KProcess::quote(d->mountPoint);
  if ( ::system(QFile::encodeName(cmd)) == 0 )
    ret = 1;

  return ret;
}

int K3bCdDevice::CdDevice::unmount()
{
  int ret = -1;
  if( KIO::findDeviceMountPoint(d->mountDeviceName).isEmpty() )
    return 0;

  QString cmd("/bin/umount ");
  cmd += KProcess::quote(d->mountPoint);
  if (::system(QFile::encodeName(cmd)) == 0)
    ret = 0;

  return ret;
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
    d->deviceFd = openDevice( QFile::encodeName(devicename()) );
  if (d->deviceFd < 0)
  {
    kdDebug() << "(K3bCdDevice) Error: could not open device." << endl;
    d->deviceFd = -1;
  }

  return d->deviceFd;
}


void K3bCdDevice::CdDevice::close() const
{
  if( d->deviceFd != -1 ) {
    ::close( d->deviceFd );
    d->deviceFd = -1;
  }
}


bool K3bCdDevice::CdDevice::isOpen() const
{
  return ( d->deviceFd != -1 );
}


K3bCdDevice::DiskInfo K3bCdDevice::CdDevice::diskInfo()
{
  kdDebug() << "(K3bCdDevice) DEPRECATED! USE NextGenerationDiskInfo!" << endl;

  DiskInfo info;
  info.device = this;

  if( open() != -1 ) {
    info.mediaType = 0;  // removed the mediaType method. use ngDiskInfo
    int ready = isReady();
    if( ready == 0 ) {
      info.tocType = diskType();
      info.valid = true;

      if( info.tocType == DiskInfo::NODISC ) {
        kdDebug() << "(K3bCdDevice::CdDevice::diskInfo) no disk." << endl;
        info.noDisk = true;
      } else if( info.tocType != DiskInfo::UNKNOWN ) {
        kdDebug() << "(K3bCdDevice::CdDevice::diskInfo) valid disk." << endl;
        info.noDisk = false;
        info.toc = readToc();
        info.sessions = numSessions();
        if( burner() ) {
          kdDebug() << "(K3bCdDevice::CdDevice::diskInfo) burner." << endl;
          int empty = isEmpty();
          info.appendable = (empty == APPENDABLE);
          info.empty = (empty == EMPTY);
          info.cdrw = rewritable();
          info.size = discSize();
          info.remaining = remainingSize();
        }
      } else  {  // info.tocType == DiskInfo::UNKNOWN, maybe empty disc
        if( burner() ) {
          if ( isEmpty() == EMPTY ) {
            info.noDisk = false;
            info.empty = true;
            info.appendable = true;
            info.size = info.remaining = discSize();
            info.cdrw = rewritable();
            info.sessions = 0;
          }
        }
      }
    } else if( ready == 3 ) {  // no disk or tray open
      kdDebug() << "(K3bCdDevice::CdDevice::diskInfo) no disk or tray open." << endl;
      info.valid = true;
      info.noDisk = true;
    }
  }


  ngDiskInfo().debug();

  close();
  return info;
}


int K3bCdDevice::CdDevice::supportedProfiles() const
{
  return d->supportedProfiles;
}


int K3bCdDevice::CdDevice::currentProfile()
{
  struct cdrom_generic_command cmd;
  unsigned char profileBuf[8];
  ::memset( &cmd, 0, sizeof(struct cdrom_generic_command) );
  ::memset( profileBuf, 0, 8 );
  cmd.cmd[0] = 0x46;	// GET CONFIGURATION
  cmd.cmd[1] = 1;
  cmd.cmd[8] = 8;
  cmd.buffer = profileBuf;
  cmd.buflen = 8;
  cmd.data_direction = CGC_DATA_READ;
  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) ) {
    kdDebug() << "(K3bCdDevice) GET_CONFIGURATION failed." << endl;
    return -1;
  }
  else {
    unsigned short profile = profileBuf[6]<<8 | profileBuf[7];
    switch (profile) {
    case 0x00: return MEDIA_NONE;
    case 0x10: return MEDIA_DVD_ROM;
    case 0x11: return MEDIA_DVD_R_SEQ;
    case 0x12: return MEDIA_DVD_RAM;
    case 0x13: return MEDIA_DVD_RW_OVWR; 
    case 0x14: return MEDIA_DVD_RW_SEQ;
    case 0x1A: return MEDIA_DVD_PLUS_RW;
    case 0x1B: return MEDIA_DVD_PLUS_R;
    case 0x08: return MEDIA_CD_ROM;
    case 0x09: return MEDIA_CD_R;
    case 0x0A: return MEDIA_CD_RW;
    default: return -1;
    }
  }
}


K3bCdDevice::NextGenerationDiskInfo K3bCdDevice::CdDevice::ngDiskInfo()
{
  NextGenerationDiskInfo inf;
  inf.m_diskState = STATE_UNKNOWN;

  if( open() != -1 ) {

    //
    // The first thing to do should be: checking if a media is loaded
    // We do this with requesting the current profile. If it is 0 no media
    // should be loaded. On an error we just go on.
    //
    int profile = currentProfile();
    if( profile == MEDIA_NONE ) {
      inf.m_diskState = STATE_NO_MEDIA;
      inf.m_mediaType = MEDIA_NONE;
    }
    inf.m_currentProfile = profile;

    struct cdrom_generic_command cmd;

    if( inf.diskState() != STATE_NO_MEDIA ) {
      disc_info_t dInf;
      if( getDiscInfo( &dInf ) ) {

	//
	// Copy the needed values from the disk_info struct
	//
	switch( dInf.status ) {
	case 0:
	  inf.m_diskState = STATE_EMPTY;
	  break;
	case 1:
	  inf.m_diskState = STATE_INCOMPLETE;
	  break;
	case 2:
	  inf.m_diskState = STATE_COMPLETE;
	  break;
	default:
	  inf.m_diskState = STATE_UNKNOWN;
	  break;
	}

	switch( dInf.border ) {
	case 0x00:
	  inf.m_lastSessionState = STATE_EMPTY;
	  break;
	case 0x01:
	  inf.m_lastSessionState = STATE_INCOMPLETE;
	  break;
	case 0x11:
	  inf.m_lastSessionState = STATE_COMPLETE;
	  break;
	default:
	  inf.m_lastSessionState = STATE_UNKNOWN;
	  break;
	}

	inf.m_rewritable = dInf.erasable;

	// we set it here so we have the info even if the call to GPCMD_READ_TOC_PMA_ATIP failes
	inf.m_numSessions = dInf.n_sessions_l | (dInf.n_sessions_m << 8);

	//	inf.m_numTracks = ( (dInf.last_track_m<<8) | dInf.last_track_l);

	// MMC-4 says that only CD-R(W) should return proper sizes here. 
	// Does that means that lead_out_r is rather useless?
	inf.m_capacity = K3b::Msf( dInf.lead_out_m + dInf.lead_out_r*60, 
				   dInf.lead_out_s, 
				   dInf.lead_out_f ) - 150;

	// Why do we need to substract 4500 again??
	if( inf.appendable() )
	  inf.m_remaining = inf.capacity() - K3b::Msf( dInf.lead_in_m + dInf.lead_in_r*60, 
						       dInf.lead_in_s, 
						       dInf.lead_in_f ) - 4500;
      }

      //
      // Now we determine the size:
      // for empty and appendable CD-R(W) and DVD+R media this should be in the dInf.lead_out_X fields
      // for all empty and appendable media READ FORMAT CAPACITIES should return the proper unformatted size
      // for complete disks we may use the READ_CAPACITY command or the start sector from the leadout
      //

      if( inf.diskState() == STATE_EMPTY ||
	  inf.diskState() == STATE_INCOMPLETE ) {

	// try the READ FORMAT CAPACITIES command
	unsigned char buffer[254]; // for reading the size of the returned data
	::memset( &cmd, 0, sizeof(struct cdrom_generic_command) );
	::memset( buffer, 0, 12 );
	cmd.cmd[0] = GPCMD_READ_FORMAT_CAPACITIES;
	cmd.cmd[8] = 12;
	cmd.cmd[9] = 0;
	cmd.buffer = buffer;
	cmd.buflen = 12;
	cmd.data_direction = CGC_DATA_READ;
	if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) ) {
	  kdDebug() << "(K3bCdDevice) READ_FORMAT_CAPACITIES failed." << endl;
	}
	else {
	  int realLength = buffer[3];
	  if( realLength + 4 > 256 ) {
	    kdDebug() << "(K3bCdDevice) ERROR: READ_FORMAT_CAPACITIES data length > 256!" << endl;
	    realLength = 251;
	  }
	  ::memset( buffer, 0, realLength+4 );
	  cmd.cmd[7] = (realLength+4) >> 8;
	  cmd.cmd[8] = (realLength+4) & 0xFF;
	  cmd.buffer = buffer;
	  cmd.buflen = realLength+4;
	  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) ) {
	    kdDebug() << "(K3bCdDevice) READ_FORMAT_CAPACITIES failed." << endl;
	  }
	  else {

	    //
	    // now find the 00h format type since that contains the number of adressable blocks
	    // and the block size used for formatting the whole media.
	    // There may be multible occurences of this descriptor (MMC4 says so) but I think it's
	    // sufficient to read the first one
	    //
	    for( int i = 12; i < realLength; ++i ) {
	      if( (buffer[i+4]>>2) == 0 /*00h*/ ) {
		// found the descriptor
		inf.m_capacity = buffer[i]<<24 | buffer[i+1]<<16 | buffer[i+2]<<8 | buffer[i+3];
		break;
	      }
	    }
	  }
	}
      }
      else {

	//
	// We first try READ TRACK INFO MMC command since the cdrom.h toc stuff seems not to work
	// properly on DVD media.
	//
	//	if( inf.numTracks() == 1 ) {
	  // read the last track's last sector
	  unsigned char trackHeader[32];
	  ::memset( &cmd, 0, sizeof(struct cdrom_generic_command) );
	  ::memset( trackHeader, 0, 32 );
	  cmd.cmd[0] = 0x52;	// READ TRACK INFORMATION
	  cmd.cmd[1] = 1; // T_inv - the invisible or incomplete track
	  cmd.cmd[2] = 0xFF;
	  cmd.cmd[3] = 0xFF;
	  cmd.cmd[4] = 0xFF;
	  cmd.cmd[5] = 0xFF;
	  cmd.cmd[8] = 32;
	  cmd.cmd[9] = 0;
	  cmd.buffer = trackHeader;
	  cmd.data_direction = CGC_DATA_READ;
	  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) ) {
	    kdDebug() << "(K3bCdDevice) READ_TRACK_INFORMATION failed." << endl;

	    kdDebug() << "(K3bCdDevice) getting disk size via toc." << endl;
	    struct cdrom_tocentry tocentry;
	    tocentry.cdte_track = CDROM_LEADOUT;
	    tocentry.cdte_format = CDROM_LBA;
	    if( ::ioctl(d->deviceFd,CDROMREADTOCENTRY,&tocentry) )
	      kdDebug() << "(K3bCdDevice) error reading lead out " << endl;
	    else {
	      inf.m_capacity = tocentry.cdte_addr.lba;
	      inf.m_capacity -= 1;  // we need the last sector of the last track, not the first from the lead-out
	    }
	  }
	  else {
	    // not sure about this....
	    inf.m_capacity = trackHeader[8]<<24|trackHeader[9]<<16|trackHeader[10]<<8|trackHeader[11];
	  }
	  //	}
      }


      // 
      // The mediatype needs to be set
      //

      // no need to test for dvd if the device does not support it
      inf.m_mediaType = -1;
      if( readsDvd() ) {
	unsigned char dvdheader[20];
	::memset( &cmd, 0, sizeof(struct cdrom_generic_command) );
	::memset( dvdheader, 0, 20 );
	cmd.cmd[0] = GPCMD_READ_DVD_STRUCTURE;
	cmd.cmd[9] = 20;
	cmd.buffer = dvdheader;
	cmd.buflen = 20;
	cmd.data_direction = CGC_DATA_READ;
	if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) ) {
	  kdDebug() << "(K3bCdDevice::CdDevice) Unable to read DVD structure." << endl;
	}
	else {
	  switch( dvdheader[4]&0xF0 ) {
	  case 0x00: inf.m_mediaType = MEDIA_DVD_ROM; break;
	  case 0x10: inf.m_mediaType = MEDIA_DVD_RAM; break;
	  case 0x20: inf.m_mediaType = MEDIA_DVD_R; break;
	  case 0x30: inf.m_mediaType = MEDIA_DVD_RW; break;
	  case 0x90: inf.m_mediaType = MEDIA_DVD_PLUS_RW; break;
	  case 0xA0: inf.m_mediaType = MEDIA_DVD_PLUS_R; break;
	  default: inf.m_mediaType = -1; break; // unknown
	  }

	  //
	  // There is some other information which we can gain here:
	  // remaining blocks for appendable DVD disks or the size of the user data
	  //

	  long userDataStartingSector = dvdheader[4+5]<<16 | dvdheader[4+6]<<8 | dvdheader[4+7];
	  long userDataEndSector = dvdheader[4+9]<<16 | dvdheader[4+10]<<8 | dvdheader[4+11];
	  if( inf.appendable() ) {
	    inf.m_remaining = inf.capacity() - userDataEndSector + userDataStartingSector;
	  }
	  else if( !inf.empty() ) {
	    // in case the disk is complete (or if we have no info which may happen with some DVD-ROMs,
	    // that's why we do not use diskState() == COMPLETE here)
	    inf.m_capacity = userDataEndSector - userDataStartingSector;
	  }
	}
      }

      if( inf.m_mediaType == -1 ) {
	// probably it is a CD
	if( inf.rewritable() )
	  inf.m_mediaType = MEDIA_CD_RW;
	else if( inf.empty() || inf.appendable() )
	  inf.m_mediaType = MEDIA_CD_R;
	else
	  inf.m_mediaType = MEDIA_CD_ROM;
      }
    

      // fix the number of sessions (since I get wrong values from disk_info with non-ide-scsi emulated drives
      int sessions = numSessions();
      if( sessions >= 0 ) {
	inf.m_numSessions = sessions;
      }
      else {
	kdDebug() << "(K3bCdDevice) could not get session info via GPCMD_READ_TOC_PMA_ATIP." << endl;
	if( inf.empty() ) {
	  // just to fix the info for empty disks
	  inf.m_numSessions = 0;
	}
      }
    }
   

    close();
  }

  return inf;
}



// this is stolen from cdrdao's GenericMMC driver
bool K3bCdDevice::CdDevice::getTrackIndex( long lba,
					   int *trackNr,
					   int *indexNr,
					   unsigned char *ctl )
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();


  if (open() != -1) {
    struct cdrom_generic_command cmd;
    unsigned short dataLen = 0x30;
    unsigned char data[0x30];
    int waitLoops = 10;
    int waitFailed = 0;

    // play one audio block
    ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
    cmd.cmd[0] = GPCMD_PLAY_AUDIO_10;
    cmd.cmd[2] = lba >> 24;
    cmd.cmd[3] = lba >> 16;
    cmd.cmd[4] = lba >> 8;
    cmd.cmd[5] = lba;
    cmd.cmd[7] = 0;
    cmd.cmd[8] = 1;

    if( ::ioctl( d->deviceFd, CDROM_SEND_PACKET, &cmd ) ) {
      kdError() << "(K3bCdDevice::CdDevice) Cannot play audio block." << endl;
      return false;
    }

    // wait until the play command finished
    ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
    cmd.cmd[0] = GPCMD_MECHANISM_STATUS;
    cmd.cmd[9] = 8;
    cmd.buffer = data;
    cmd.buflen = dataLen;
    cmd.data_direction = CGC_DATA_READ;

    while( waitLoops > 0 ) {
      if( ::ioctl( d->deviceFd, CDROM_SEND_PACKET, &cmd ) == 0 ) {
	if ((data[1] >> 5) == 1) // still playing?
	  waitLoops--;
	else
	  waitLoops = 0;
      }
      else {
	waitFailed = 1;
	waitLoops = 0;
      }
    }

    if(waitFailed) {
      // The play operation immediately returns success status and the waiting
      // loop above failed. Wait here for a while until the desired block is
      // played. It takes ~13 msecs to play a block but access time is in the
      // order of several 100 msecs
      ::usleep(300000);
    }

    // read sub channel information
    ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
    cmd.cmd[0] = GPCMD_READ_SUBCHANNEL;
    cmd.cmd[2] = 0x40; // get sub channel data
    cmd.cmd[3] = 0x01; // get sub Q channel data
    cmd.cmd[6] = 0;
    cmd.cmd[7] = dataLen >> 8;
    cmd.cmd[8] = dataLen;
    cmd.buffer = data;
    cmd.buflen = dataLen;
    cmd.data_direction = CGC_DATA_READ;

    if( ::ioctl( d->deviceFd, CDROM_SEND_PACKET, &cmd ) ) {
      kdError() << "(K3bCdDevice::CdDevice) Cannot read sub Q channel data." << endl;
      return false;
    }

    *trackNr = data[6];
    *indexNr = data[7];
    if (ctl != 0) {
      *ctl = data[5] & 0x0f;
    }

    if( needToClose )
      close();

    return true;
  }
  else
    return false;
}

bool K3bCdDevice::CdDevice::readSectorsRaw(unsigned char *buf, int start, int count)
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();


  if (open() != -1) {
    struct cdrom_generic_command cmd;
    ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
    cmd.cmd[0] = GPCMD_READ_CD;
    cmd.cmd[1] = 0; // all types
    cmd.cmd[2] = (start >> 24) & 0xff;
    cmd.cmd[3] = (start >> 16) & 0xff;
    cmd.cmd[4] = (start >>  8) & 0xff;
    cmd.cmd[5] = start & 0xff;
    cmd.cmd[6] = (count >> 16) & 0xff;
    cmd.cmd[7] = (count >>  8) & 0xff;
    cmd.cmd[8] = count & 0xff;
    cmd.cmd[9] = 0x80 | 0x40 | 0x20 | 0x10 | 0x08;
                 // DATA_SYNC | DATA_ALLHEADER | DATA_USER | DATA_EDC;
    cmd.buffer = buf;
    cmd.buflen = count*2352;
    cmd.data_direction = CGC_DATA_READ;
    if( ::ioctl( d->deviceFd, CDROM_SEND_PACKET, &cmd ) ) {
      kdError() << "(K3bCdDevice::CdDevice) Cannot read raw sectors." << endl;
      return false;
    }


    if( needToClose )
      close();

    return true;
  }
  else
    return false;
}


bool K3bCdDevice::CdDevice::readModePage2A( struct mm_cap_page_2A* p ) const
{
  unsigned char data[22+8]; // MMC-1: 22 byte, header: 8 byte
  ::memset( data, 0, sizeof(struct mm_cap_page_2A) );

  // to be as compatible as posiible we just use the MMC-1 part of the
  // mode page. Since we do not use the new stuff yet this is not a problem at all.
  // the MMC-1 part is 22 bytesin size.

  // MM Capabilities and Mechanical Status Page: 0x2A
  if( modeSense( 0x2A, data, 22+8 ) ) {
    //
    // this might be not perfect regarding performance but this
    // way we do not need to keep the MODE SENSE header in the
    // mm_cap_page_2A struct
    //
    ::memcpy( p, data+8, 22 );

    return true;
  }
  else
    return false;
}


bool K3bCdDevice::CdDevice::modeSense( int page, unsigned char* pageData, int pageLen ) const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  if( open() < 0 )
    return false;

  bool ret = false;

  struct cdrom_generic_command cmd;
  ::memset( &cmd, 0, sizeof(struct cdrom_generic_command) );
  cmd.cmd[0] = 0x5A;	// MODE SENSE
  cmd.cmd[2] = page;
  cmd.cmd[7] = pageLen>>8;
  cmd.cmd[8] = pageLen;
  cmd.buffer = pageData;
  cmd.buflen = pageLen;
  cmd.data_direction = CGC_DATA_READ;
  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) == 0 ) {
    ret = true;
  }

  if( needToClose )
    close();

  return ret;
}


bool K3bCdDevice::CdDevice::modeSelect( unsigned char* page, int pageLen, bool pf, bool sp ) const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  if( open() < 0 )
    return false;

  bool ret = false;

  struct cdrom_generic_command cmd;
  ::memset( &cmd, 0, sizeof(struct cdrom_generic_command) );
  cmd.cmd[0] = 0x55;	// MODE SELECT
  cmd.cmd[1] = ( sp ? 1 : 0 ) | ( pf ? 0x10 : 0 );
  cmd.cmd[7] = pageLen>>8;
  cmd.cmd[8] = pageLen;
  cmd.buffer = page;
  cmd.buflen = pageLen;
  cmd.data_direction = CGC_DATA_READ;
  if( ::ioctl(d->deviceFd,CDROM_SEND_PACKET,&cmd) == 0 ) {
    ret = true;
  }

  if( needToClose )
    close();

  return ret;
}


void K3bCdDevice::CdDevice::checkWriteModes()
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  if (open() < 0)
    return;

  // header size is 8
  unsigned char buffer[100];
  ::memset( buffer, 0, 100 );

  if( !modeSense( 0x05, buffer, 100 ) ) {
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": modeSense 0x05 failed!" << endl;
  }
  else {
    int blockDescrLen = (buffer[6]<<8)&0xF0 | buffer[7];
    int dataLen = (buffer[0]<<8)&0xF0 | buffer[1];

    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": blockDescLen: " << blockDescrLen << endl
	      << "                        dataLen: " << dataLen << endl;

    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": modesense data: " << endl;
    debugBitfield( buffer, dataLen+8 );

    wr_param_page_05* mp = (struct wr_param_page_05*)(buffer+8+blockDescrLen);

    // reset some stuff to be on the safe side
    mp->multi_session = 0;
    mp->test_write = 0;
    mp->LS_V = 0;
    mp->copy = 0;
    mp->fp = 0;
    mp->host_appl_code= 0;
    mp->session_format = 0;
    mp->audio_pause_len[0] = (150 >> 8) & 0xFF;
    mp->audio_pause_len[1] = 150 & 0xFF;

    m_writeModes = 0;

    buffer[0] = 0;
    buffer[1] = 0;
//     buffer[2] = 0;
//     buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;

    // TAO
    mp->write_type = 0x01;  // Track-at-once
    mp->track_mode = 4;     // MMC-4 says: 5, cdrecord uses 4 ???
    mp->dbtype = 8;         // Mode 1

    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": modeselect TAO data: " << endl;
    debugBitfield( buffer, dataLen+8 );


    if( modeSelect( buffer, dataLen+2, 1, 0 ) )
      m_writeModes |= TAO;
    else
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": modeSelect with TAO failed." << endl;


    // SAO
    mp->write_type = 0x02; // Session-at-once

    if( modeSelect( buffer, dataLen+8, 1, 0 ) )
      m_writeModes |= SAO;
    else
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": modeSelect with SAO failed." << endl;

    // RAW
    mp->write_type = 0x03; // RAW
    mp->dbtype = 1;        // Raw data with P and Q Sub-channel (2368 bytes)
    if( modeSelect( buffer, dataLen+8, 1, 0 ) ) {
      m_writeModes |= RAW;
      m_writeModes |= RAW_R16;
    }

    mp->dbtype = 2;        // Raw data with P-W Sub-channel (2448 bytes)
    if( modeSelect( buffer, dataLen+8, 1, 0 ) ) {
      m_writeModes |= RAW;
      m_writeModes |= RAW_R96P;
    }

    mp->dbtype = 3;        // Raw data with P-W raw Sub-channel (2368 bytes)
    if( modeSelect( buffer, dataLen+8, 1, 0 ) ) {
      m_writeModes |= RAW;
      m_writeModes |= RAW_R96R;
    }
  }
    
  if( needToClose )
    close();
}
