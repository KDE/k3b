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

#include <config.h>


#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3btrack.h"
#include "k3btoc.h"
#include "k3bdiskinfo.h"
#include "k3bmmc.h"
#include "k3bscsicommand.h"
#include "k3bcrc.h"

#include <qstringlist.h>
#include <qfile.h>
#include <qglobal.h>
#include <qvaluevector.h>

#include <kdebug.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>


#ifdef Q_OS_LINUX

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,70)
typedef unsigned char u8;
#endif

#undef __STRICT_ANSI__
#include <linux/cdrom.h>
#define __STRICT_ANSI__

#endif // Q_OS_LINUX


#ifdef HAVE_RESMGR
extern "C" {
#include <resmgr.h>
}
#endif



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
  int deviceType;
  int supportedProfiles;
  QString mountPoint;
  QString mountDeviceName;
  QStringList allNodes;
  int deviceFd;
  bool burnfree;
};


K3bCdDevice::CdDevice::CdDevice( const QString& devname )
  : m_writeModes(0),
    m_supermount(false)
{
  d = new Private;

  d->blockDeviceName = devname;
  d->allNodes.append(devname);

  m_cdrdaoDriver = "auto";
  m_cdTextCapable = 0;
  m_maxWriteSpeed = 0;
  m_maxReadSpeed = 0;
  d->burnfree = false;
  m_bus = m_target = m_lun = -1;
  m_dvdMinusTestwrite = true;
  m_bufferSize = 0;
}


K3bCdDevice::CdDevice::~CdDevice()
{
  delete d;
}


bool K3bCdDevice::CdDevice::init()
{
  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << ": init()" << endl;

  //
  // they all should read CD-ROM.
  //
  d->deviceType = CDROM;

  d->supportedProfiles = 0;

  if(open() < 0)
    return false;


  //
  // inquiry
  // use a 36 bytes buffer since not all devices return the full inquiry struct
  //
  ScsiCommand cmd( this );
  unsigned char buf[36];
  cmd.clear();
  ::memset( buf, 0, sizeof(buf) );
  struct inquiry* inq = (struct inquiry*)buf;
  cmd[0] = MMC::INQUIRY;
  cmd[4] = sizeof(buf);
  cmd[5] = 0;
  if( cmd.transport( TR_DIR_READ, buf, sizeof(buf) ) ) {
    kdError() << "(K3bCdDevice) Unable to do inquiry." << endl;
    close();
    return false;
  }
  else {
    m_vendor = QString::fromLocal8Bit( (char*)(inq->vendor), 8 ).stripWhiteSpace();
    m_description = QString::fromLocal8Bit( (char*)(inq->product), 16 ).stripWhiteSpace();
    m_version = QString::fromLocal8Bit( (char*)(inq->revision), 4 ).stripWhiteSpace();
  }



  //
  // We probe all features of the device. Since not all devices support the GET CONFIGURATION command
  // we also query the mode page 2A and use the cdrom.h stuff to get as much information as possible
  //

  unsigned char header[8];
  ::memset( header, 0, 8 );
  cmd[0] = MMC::GET_CONFIGURATION;
  cmd[8] = 8;
  if( cmd.transport( TR_DIR_READ, header, 8 ) ) {
    kdDebug() << "(K3bCdDevice) " << blockDeviceName() << ": GET_CONFIGURATION failed." << endl;
  }
  else {

    //
    // Now that we know the length of the returned data we just do it again with the
    // correct buffer size
    //

    int len = from4Byte( header );
    unsigned char* profiles = new unsigned char[len];
    ::memset( profiles, 0, len );
    cmd[6] = len>>16;
    cmd[7] = len>>8;
    cmd[8] = len;
    if( cmd.transport( TR_DIR_READ, profiles, len ) ) {
      kdDebug() << "(K3bCdDevice) " << blockDeviceName() << ": GET_CONFIGURATION with correct size failed." << endl;
    }
    else {
      for( int i = 8; i < len; ) {
	short feature = from2Byte( &profiles[i] );
	int featureLen = profiles[i+3];
	i+=4; // skip feature header

	//
	// now i indexes the first byte of the feature dependant data
	//

	switch( feature ) {
	case 0x000: // Profile List
	  for( int j = 0; j < featureLen; j+=4 ) {
	    short profile = from2Byte( &profiles[i+j] );

	    switch (profile) {
	    case 0x10: d->supportedProfiles |= MEDIA_DVD_ROM; break;
	    case 0x11: d->supportedProfiles |= MEDIA_DVD_R_SEQ; break;
	    case 0x12: d->supportedProfiles |= MEDIA_DVD_RAM; break;
	    case 0x13: d->supportedProfiles |= MEDIA_DVD_RW_OVWR; break;
	    case 0x14: d->supportedProfiles |= MEDIA_DVD_RW_SEQ; break;
	    case 0x1A: d->supportedProfiles |= MEDIA_DVD_PLUS_RW; break;
	    case 0x1B: d->supportedProfiles |= MEDIA_DVD_PLUS_R; break;
	    case 0x08: d->supportedProfiles |= MEDIA_CD_ROM; break;
	    case 0x09: d->supportedProfiles |= MEDIA_CD_R; break;
	    case 0x0A: d->supportedProfiles |= MEDIA_CD_RW; break;
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
	  d->deviceType |= CDROM;
	  break;

	case 0x01E: // CD Read
	  kdDebug() << "(K3bCdDevice) " << blockDeviceName() << " feature: " << "CD Read" << endl;
	  d->deviceType |= CDROM;
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
	      unsigned char reserved1      : 7;
	      unsigned char write          : 1;
	      unsigned char reserved2[3];
	      unsigned char reserved3      : 6;
	      unsigned char write_4x_max   : 1;
	      unsigned char write_2_4x_max : 1;
	      // and some stuff we do not use here...
	    };
#else
	    struct dvd_plus_r_feature {
	      unsigned char write     : 1;
	      unsigned char reserved1 : 7;
	      unsigned char reserved2[3];
	      unsigned char write_2_4x_max : 1;
	      unsigned char write_4x_max   : 1;
	      unsigned char reserved3      : 6;
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

	    m_dvdMinusTestwrite = p->testwrite;
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
  // Check the supported write modes (TAO, SAO, RAW) by trying to set them
  // We do this before checking mode page 2A in case some readers allow changin
  // the write parameter page
  //
  checkWriteModes();

  //
  // Most current drives support the 2A mode page
  // Here we can get some more information (cdrecord -prcap does exactly this)
  //
  unsigned char* mm_cap_buffer = 0;
  int mm_cap_len = 0;
  if( modeSense( &mm_cap_buffer, mm_cap_len, 0x2A ) ) {
    mm_cap_page_2A* mm_p = (mm_cap_page_2A*)(mm_cap_buffer+8);
    if( mm_p->BUF )
      d->burnfree = true;

    if( mm_p->cd_r_write )
      d->deviceType |= CDR;
    else
      d->deviceType &= ~CDR;

    if( mm_p->cd_rw_write )
      d->deviceType |= CDRW;
    else
      d->deviceType &= ~CDRW;

    if( mm_p->dvd_r_write )
      d->deviceType |= DVDR;
    else
      d->deviceType &= ~DVDR;

    if( mm_p->dvd_rom_read || mm_p->dvd_r_read )
      d->deviceType |= DVD;

    m_maxReadSpeed = from2Byte(mm_p->max_read_speed);
    m_bufferSize = from2Byte( mm_p->buffer_size );

    delete [] mm_cap_buffer;
  }
  else {
    kdDebug() << "(K3bCdDevice) " << blockDeviceName() << ": read mode page 2A failed!" << endl;
  }

  m_maxWriteSpeed = determineMaximalWriteSpeed();


  //
  // Check Just-Link via Ricoh mode page 0x30
  //
  if( !d->burnfree ) {
    unsigned char* ricoh = 0;
    int ricohLen = 0;
    if( modeSense( &ricoh, ricohLen, 0x30 ) ) {

      //
      // 8 byte mode header + 6 byte page data
      //

      if( ricohLen >= 14 ) {
	ricoh_mode_page_30* rp = (ricoh_mode_page_30*)(ricoh+8);
	d->burnfree = rp->BUEFS;
      }

      delete [] ricoh;
    }
  }


  //
  // Support for some very old drives
  //
  checkForAncientWriters();

  close();

  return furtherInit();
}


bool K3bCdDevice::CdDevice::furtherInit()
{
#ifdef Q_OS_LINUX

  //
  // Since all CDR drives at least support TAO, all CDRW drives should support 
  // mode page 2a and all DVD writer should support mode page 2a or the GET CONFIGURATION
  // command this is redundant and may be removed for BSD ports or even completely
  //
  // We just keep it here because of the "should" in the sentence above. If someone can tell me
  // that the linux driver does nothing more we can remove it completely.
  //
  int drivetype = ::ioctl( open(), CDROM_GET_CAPABILITY, CDSL_CURRENT );
  if( drivetype < 0 ) {
    kdDebug() << "Error while retrieving capabilities." << endl;
    close();
    return false;
  }
    
  d->deviceType |= CDROM;

  if (drivetype & CDC_CD_R)
    d->deviceType |= CDR;
  if (drivetype & CDC_CD_RW)
    d->deviceType |= CDRW;
  if (drivetype & CDC_DVD_R)
    d->deviceType |= DVDR;
  if (drivetype & CDC_DVD_RAM)
    d->deviceType |= DVDRAM;
  if (drivetype & CDC_DVD)
    d->deviceType |= DVD;
   
  close();

#endif // Q_OS_LINUX
  return true;
}


void K3bCdDevice::CdDevice::checkForAncientWriters()
{
  if( vendor() == "TEAC" ) {
    if( description() == "R50S" || description() == "R55S" ) {
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() 
		<< " found ancient drive: " << vendor() << " " << description() << endl;

      m_writeModes = TAO|SAO;
      d->deviceType = CDROM|CDR;
      m_maxWriteSpeed = 4;
      m_maxReadSpeed = 12;
      m_bufferSize = 1024;
      d->burnfree = false;
    }
  }
}


K3bCdDevice::CdDevice::interface K3bCdDevice::CdDevice::interfaceType()
{
  if( m_bus != -1 && m_target != -1 && m_lun != -1 )
    return SCSI;
  else
    return IDE;
}


bool K3bCdDevice::CdDevice::dao() const 
{
  return m_writeModes & SAO;
}


bool K3bCdDevice::CdDevice::supportsRawWriting() const
{
  return( writingModes() & (RAW|RAW_R16|RAW_R96P|RAW_R96R) );
}


bool K3bCdDevice::CdDevice::writesCd() const
{
  return ( d->deviceType & CDR ) && ( m_writeModes & TAO );
}


bool K3bCdDevice::CdDevice::burner() const
{
  return ( writesCd() || writesDvd() );
}


bool K3bCdDevice::CdDevice::writesCdrw() const
{
  return d->deviceType & CDRW;
}


bool K3bCdDevice::CdDevice::writesDvd() const
{
  return ( writesDvdPlus() || writesDvdMinus() );
}


bool K3bCdDevice::CdDevice::writesDvdPlus() const
{
  return d->deviceType & (DVDPR|DVDPRW);
}


bool K3bCdDevice::CdDevice::writesDvdMinus() const
{
  return d->deviceType & (DVDR|DVDRW);
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


const QString& K3bCdDevice::CdDevice::blockDeviceName() const
{
  return d->blockDeviceName;
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



bool K3bCdDevice::CdDevice::burnproof() const
{
  return burnfree();
}


bool K3bCdDevice::CdDevice::burnfree() const
{
  return d->burnfree;
}


bool K3bCdDevice::CdDevice::isDVD() const
{
  if( d->deviceType & ( DVDR | DVDRAM | DVD ) ) {
    return ( dvdMediaType() >= 0 );
  }
  else
    return false;
}


bool K3bCdDevice::CdDevice::isReady() const
{
  ScsiCommand cmd( this );
  cmd[0] = MMC::TEST_UNIT_READY;
  return( cmd.transport() == 0 );
}


int K3bCdDevice::CdDevice::isEmpty() const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  int ret = NO_INFO;
  if (open() < 0)
    return NO_INFO;

  if( !isReady() )
    return NO_DISK;

  unsigned char* data = 0;
  int dataLen = 0;

  if( readDiscInfo( &data, dataLen ) ) {
    disc_info_t* inf = (disc_info_t*)data;
    switch( inf->status ) {
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

    delete [] data;
  }

  if( needToClose )
    close();

  return ret;
}


K3b::Msf K3bCdDevice::CdDevice::discSize() const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  K3b::Msf ret(0);
  if (open() < 0)
    return ret;

  unsigned char* data = 0;
  int dataLen = 0;

  if( readDiscInfo( &data, dataLen ) ) {
    disc_info_t* inf = (disc_info_t*)data;
    if ( inf->lead_out_m != 0xFF && inf->lead_out_s != 0xFF && inf->lead_out_f != 0xFF ) {
      ret = K3b::Msf( inf->lead_out_m, inf->lead_out_s, inf->lead_out_f );
      ret -= 150;
    }

    delete [] data;
  }

  if( ret == 0 ) {
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() 
	      << "READ DISC INFORMATION failed. getting disk size via toc." << endl;
    Toc toc = readToc();
    ret = toc.lastSector();
  }

  if( needToClose )
    close();

  return ret;
}


bool K3bCdDevice::CdDevice::readDiscInfo( unsigned char** data, int& dataLen ) const
{
  unsigned char header[2];
  ::memset( header, 0, 2 );

  ScsiCommand cmd( this );
  cmd[0] = MMC::READ_DISK_INFORMATION;
  cmd[8] = 2;

  if( cmd.transport( TR_DIR_READ, header, 2 ) == 0 ) {
    // again with real length
    dataLen = from2Byte( header ) + 2;

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[7] = dataLen>>8;
    cmd[8] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 )
      return true;
    else {
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ DISC INFORMATION with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else {
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ DISC INFORMATION length det failed" << endl;
  }

  return false;
}


K3b::Msf K3bCdDevice::CdDevice::remainingSize() const
{
  K3b::Msf ret, size;
  bool empty = false;
  bool appendable = false;

  unsigned char* data = 0;
  int dataLen = 0;

  if( readDiscInfo( &data, dataLen ) ) {
    disc_info_t* inf = (disc_info_t*)data;
    if ( inf->lead_in_m != 0xFF && inf->lead_in_s != 0xFF && inf->lead_in_f != 0xFF ) {
      ret = K3b::Msf( inf->lead_in_m, inf->lead_in_s, inf->lead_in_f );
    }
    if ( inf->lead_out_m != 0xFF && inf->lead_out_s != 0xFF && inf->lead_out_f != 0xFF ) {
      size = K3b::Msf( inf->lead_out_m, inf->lead_out_s, inf->lead_out_f );
    }

    empty = !inf->status;
    appendable = ( inf->status == 1 );

    delete [] data;
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
  // 
  // Althought disk_info should get the real value without ide-scsi
  // I keep getting wrong values (the value is too high. I think the leadout
  // gets counted as session sometimes :()
  //

  //
  // Session Info
  // ============
  // Byte 0-1: Data Length
  // Byte   2: First Complete Session Number (Hex) - always 1
  // Byte   3: Last Complete Session Number (Hex)
  //

  int ret = -1;

  unsigned char* dat = 0;
  int len = 0;
  if( readTocPmaAtip( &dat, len, 1, 0, 0 ) ) {
    ret = dat[3];

    delete [] dat;
  }
  else {
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": could not get session info !" << endl;
  }

  return ret;
}


int K3bCdDevice::CdDevice::getDataMode( const K3b::Msf& sector ) const
{
  bool needToClose = !isOpen();

  int ret = Track::UNKNOWN;

  if (open() < 0)
    return ret;

  unsigned char data[2352];
  bool readSuccess = readSectorsRaw( data, sector.lba(), 1 );

#ifdef Q_OS_LINUX
  if( !readSuccess ) {
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName()
	      << ": MMC RAW READ failed. falling back to cdrom.h." << endl;

    data[0] = sector.minutes();
    data[1] = sector.seconds();
    data[2] = sector.frames();
    if( ::ioctl(d->deviceFd,CDROMREADRAW,data) == -1 ) {
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName()
		<< ": could not get track header, (lba " << sector.lba() << ") ! "  << strerror(errno) << endl;
      readSuccess = false;
    }
  }
#endif // Q_OS_LINUX

  if( readSuccess ) {
    if ( data[15] == 0x1 )
      ret = Track::MODE1;
    else if ( data[15] == 0x2 )
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



int K3bCdDevice::CdDevice::getTrackDataMode( const K3bCdDevice::Track& track ) const
{
  return getDataMode( track.firstSector() );
}


K3bCdDevice::Toc K3bCdDevice::CdDevice::readToc() const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  Toc toc;

  if( open() == -1 )
    return toc;

  if( isDVD() ) {
    int mediaType = dvdMediaType();
    switch( mediaType ) {

    case MEDIA_DVD_ROM:
    case MEDIA_DVD_RW_OVWR:
    case MEDIA_DVD_PLUS_RW:
      {
	K3b::Msf size;
	if( readCapacity( size ) ) {
	  Track track;
	  track.m_firstSector = 0;
	  track.m_lastSector = size.lba();
	  track.m_session = 1;
	  track.m_type = Track::DATA;
	  track.m_mode = Track::DVD;
	  track.m_copyPermitted = ( mediaType != MEDIA_DVD_ROM );
	  track.m_preEmphasis = ( mediaType != MEDIA_DVD_ROM );

	  toc.append( track );
	}
	else {
	  kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << "READ CAPACITY failed." << endl;
	  readFormattedToc( toc );
	}
      }
      break;

    case MEDIA_DVD_R:
    case MEDIA_DVD_R_SEQ:
      // multisession possible
      readFormattedToc( toc, true );
      break;

    case MEDIA_DVD_RW:
    case MEDIA_DVD_RW_SEQ:
      // is multisession possible??
      readFormattedToc( toc, true );
      break;

    case MEDIA_DVD_PLUS_R:
      //
      // a DVD+R disk may have multible sessions
      // every session may contain up to 16 fragments
      // if the disk is open there is one open session
      // every closed session is viewed as a track whereas
      // every fragment of the open session is viewed as a track
      //
      // We may use 
      // READ DISK INFORMATION
      // READ TRACK INFORMATION: track number FFh refers to the incomplete fragment (track)
      // READ TOC/PMA/ATIP: form 0 refers to all closed sessions
      //                    form 1 refers to the last closed session
      //
      readFormattedToc( toc, true );
      break;

    case MEDIA_DVD_RAM:
      kdDebug() << "(K3bCdDevice::readDvdToc) no dvdram support" << endl;
      break;
    default:
      kdDebug() << "(K3bCdDevice::readDvdToc) no dvd." << endl;
    }
  }
  else {
    bool success = readRawToc( toc );
    if( !success ) {
      kdDebug() << "(K3bCdDevice::CdDevice) MMC READ RAW TOC failed." << endl;

      success = readFormattedToc( toc );

#ifdef Q_OS_LINUX
      if( !success ) {
	kdDebug() << "(K3bCdDevice::CdDevice) MMC READ TOC failed. falling back to cdrom.h." << endl;
	readTocLinux(toc);
      }
#endif

      if( success )
	fixupToc( toc );
    }

    if( success ) {
      // read MCN and ISRC of all tracks
      QCString mcn;
      if( readMcn( mcn ) ) {
	toc.setMcn( mcn );
	kdDebug() << "(K3bCdDevice::CdDevice) found MCN: " << mcn << endl;
      }
      else
	kdDebug() << "(K3bCdDevice::CdDevice) no MCN found." << endl;

      for( unsigned int i = 1; i <= toc.count(); ++i ) {
	QCString isrc;
	if( readIsrc( i, isrc ) ) {
	  kdDebug() << "(K3bCdDevice::CdDevice) found ISRC for track " << i << ": " << isrc << endl;
	  toc[i-1].setIsrc( isrc );
	}
	else
	  kdDebug() << "(K3bCdDevice::CdDevice) no ISRC found for track " << i << endl;
      }
    }
  }

  if( needToClose )
    close();

  return toc;
}


bool K3bCdDevice::CdDevice::readFormattedToc( K3bCdDevice::Toc& toc, bool dvd ) const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  bool success = false;

  toc.clear();

  unsigned char* data = 0;
  int dataLen = 0;
  if( readTocPmaAtip( &data, dataLen, 0, 0, 1 ) ) {
    int lastTrack = data[3];
    toc_track_descriptor* td = (toc_track_descriptor*)&data[4];
    for( int i = 0; i < lastTrack; ++i ) {

      Track track;
      unsigned int control = 0;

      unsigned char* trackData = 0;
      int trackDataLen = 0;
      if( readTrackInformation( &trackData, trackDataLen, 1, i+1 ) ) {
	track_info_t* trackInfo = (track_info_t*)trackData;

	track.m_firstSector = from4Byte( trackInfo->track_start );
	track.m_lastSector = track.m_firstSector + from4Byte( trackInfo->track_size ) - 1;
	track.m_session = (int)(trackInfo->session_number_m<<8 & 0xf0 |
				trackInfo->session_number_l & 0x0f);  //FIXME: is this BCD??

	control = trackInfo->track_mode;

	delete [] trackData;
      }
      else {
	//
	// READ TRACK INFORMATION failed
	// no session number info
	// no track length and thus possibly incorrect last sector for
	// multisession disks
	//
	track.m_firstSector = from4Byte( td[i].start_adr );
	track.m_lastSector = from4Byte( td[i+1].start_adr ) - 1;
	control = td[i].control;
      }

      if( dvd ) {
	track.m_type = Track::DATA;
	track.m_mode = Track::DVD;
      }
      else {
	track.m_type = (control & 0x4) ? Track::DATA : Track::AUDIO;
	track.m_mode = getTrackDataMode( track );
      }
      track.m_copyPermitted = ( control & 0x2 );
      track.m_preEmphasis = ( control & 0x1 );

      toc.append( track );
    }
    
    delete [] data;

    success = true;
  }


  if( needToClose )
    close();

  return success;
}


bool K3bCdDevice::CdDevice::readRawToc( K3bCdDevice::Toc& toc ) const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  bool success = false;

  toc.clear();

  if( open() != -1 ) {
    //
    // Read Raw TOC (format: 0010b)
    //
    // For POINT from 01h-63h we get all the tracks
    // POINT a1h gices us the last track number in the session in PMIN
    // POINT a2h gives the start of the session lead-out in PMIN,PSEC,PFRAME
    //

    unsigned char* data = 0;
    int dataLen = 0;

    if( readTocPmaAtip( &data, dataLen, 2, false, 1 ) ) {
      success = true;

      toc_raw_track_descriptor* tr = (toc_raw_track_descriptor*)&data[4];

      K3b::Msf sessionLeadOut;

	kdDebug() << "Session |  ADR   | CONTROL|  TNO   | POINT  |  Min   |  Sec   | Frame  |  Zero  |  PMIN  |  PSEC  | PFRAME |" << endl;
      for( int i = 0; i < (dataLen-4)/11; ++i ) {
	QString s;
	s += QString( " %1 |" ).arg( (int)tr[i].session_number, 6 );
	s += QString( " %1 |" ).arg( (int)tr[i].adr, 6 );
	s += QString( " %1 |" ).arg( (int)tr[i].control, 6 );
	s += QString( " %1 |" ).arg( (int)tr[i].tno, 6 );
	s += QString( " %1 |" ).arg( (int)tr[i].point, 6, 16 );
	s += QString( " %1 |" ).arg( (int)tr[i].min, 6 );
	s += QString( " %1 |" ).arg( (int)tr[i].sec, 6 );
	s += QString( " %1 |" ).arg( (int)tr[i].frame, 6 );
	s += QString( " %1 |" ).arg( (int)tr[i].zero, 6, 16 );
	s += QString( " %1 |" ).arg( (int)tr[i].p_min, 6 );
	s += QString( " %1 |" ).arg( (int)tr[i].p_sec, 6 );
	s += QString( " %1 |" ).arg( (int)tr[i].p_frame, 6 );
	kdDebug() << s << endl;

	if( tr[i].adr == 1 && tr[i].point <= 0x63 ) {
	  // track
	  K3bTrack track;
	  track.m_session = tr[i].session_number;
	  track.m_firstSector = K3b::Msf( tr[i].p_min, tr[i].p_sec, tr[i].p_frame ) - 150; // :( We use 00:00:00 == 0 lba)
	  track.m_type = ( tr[i].control & 0x4 ? Track::DATA : Track::AUDIO );
	  track.m_mode = ( track.type() == Track::DATA ? getTrackDataMode(track) : Track::UNKNOWN );
	  track.m_copyPermitted = ( tr[i].control & 0x2 );
	  track.m_preEmphasis = ( tr[i].control & 0x1 );

	  //
	  // only do this within a session because otherwise we already set the last sector with the session leadout
	  //
	  if( !toc.isEmpty() )
	    if( toc[toc.count()-1].session() == track.session() )
	      toc[toc.count()-1].m_lastSector = track.firstSector() - 1;

	  toc.append(track);
	}
	else if( tr[i].point == 0xa2 ) {
	  //
	  // since the session is always reported before the tracks this is where we do this:
	  // set the previous session's last tracks's last sector to the first sector of the
	  // session leadout (which was reported before the tracks)
	  //
	  // This only happens on multisession CDs
	  //
	  if( !toc.isEmpty() )
	    toc[toc.count()-1].m_lastSector = sessionLeadOut - 1;

	  // this is save since the descriptors are reported in ascending order of the session number
	  sessionLeadOut = K3b::Msf( tr[i].p_min, tr[i].p_sec, tr[i].p_frame ) - 150; // :( We use 00:00:00 == 0 lba)
	}
      }

      // set the last track's last sector
      if( !toc.isEmpty() )
	toc[toc.count()-1].m_lastSector = sessionLeadOut - 1;

      delete [] data;
    }
  }

  if( needToClose )
    close();

  return success;
}


K3bCdDevice::AlbumCdText K3bCdDevice::CdDevice::readCdText( unsigned int trackCount ) const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  bool success = false;

  K3bCdDevice::AlbumCdText textData;

  if( open() != -1 ) {
    unsigned char* data = 0;
    int dataLen = 0;
    
    if( trackCount <= 0 ) {
      // we need to determine the number of tracks first
      if( readTocPmaAtip( &data, dataLen, 0, 0, 1 ) ) {
	trackCount = data[3];

	delete [] data;
      }
      else
	kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": unable to determine number of tracks." << endl;
    }

    // if we failed to determine the trackCount it's still <= 0
    if( trackCount > 0 ) {
      if( readTocPmaAtip( &data, dataLen, 5, false, 0 ) ) {
	if( dataLen > 4 ) {
	  textData.resize( trackCount );

	  cdtext_pack* pack = (cdtext_pack*)&data[4];

	  kdDebug() << endl << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": CD_TEXT:" << endl;
	  kdDebug() << " id1    | id2    | id3    | charps | blockn | dbcc | data           | crc |" << endl;
      
	  for( int i = 0; i < (dataLen-4)/18; ++i ) {
	    QString s;
	    s += QString( " %1 |" ).arg( pack[i].id1, 6, 16 );
	    s += QString( " %1 |" ).arg( pack[i].id2, 6 );
	    s += QString( " %1 |" ).arg( pack[i].id3, 6 );
	    s += QString( " %1 |" ).arg( pack[i].charpos, 6 );
	    s += QString( " %1 |" ).arg( pack[i].blocknum, 6 );
	    s += QString( " %1 |" ).arg( pack[i].dbcc, 4 );
	    char str[12];
	    sprintf( str, "%c%c%c%c%c%c%c%c%c%c%c%c",
		     pack[i].data[0] == '\0' ? '°' : pack[i].data[0],
		     pack[i].data[1] == '\0' ? '°' : pack[i].data[1],
		     pack[i].data[2] == '\0' ? '°' : pack[i].data[2],
		     pack[i].data[3] == '\0' ? '°' : pack[i].data[3],
		     pack[i].data[4] == '\0' ? '°' : pack[i].data[4],
		     pack[i].data[5] == '\0' ? '°' : pack[i].data[5],
		     pack[i].data[6] == '\0' ? '°' : pack[i].data[6],
		     pack[i].data[7] == '\0' ? '°' : pack[i].data[7],
		     pack[i].data[8] == '\0' ? '°' : pack[i].data[8],
		     pack[i].data[9] == '\0' ? '°' : pack[i].data[9],
		     pack[i].data[10] == '\0' ? '°' : pack[i].data[10],
		     pack[i].data[11] == '\0' ? '°' : pack[i].data[11] );
	    s += QString( " %1 |" ).arg( "'" + QCString(str,13) + "'", 14 );
	    //      s += QString( " %1 |" ).arg( QString::fromLatin1( (char*)pack[i].crc, 2 ), 3 );
	    kdDebug() << s << endl;


	    //
	    // pack.data has a length of 12
	    //
	    // id1 tells us the tracknumber of the data (0 for global)
	    // data may contain multible \0. In that case after every \0 the track number increases 1
	    //

	    char* nullPos = (char*)pack[i].data - 1;
	
	    unsigned int trackNo = pack[i].id2;
	    while( nullPos && trackNo <= trackCount ) {
	      char* nextNullPos = (char*)::memchr( nullPos+1, '\0', 11 - (nullPos - (char*)pack[i].data) );
	      QString txtstr;	    
	      if( nextNullPos ) // take all chars up to the next null
		txtstr = QString::fromLocal8Bit( (char*)nullPos+1, nextNullPos - nullPos - 1 );
	      else // take all chars to the end of the pack data (12 bytes)
		txtstr = QString::fromLocal8Bit( (char*)nullPos+1, 11 - (nullPos - (char*)pack[i].data) );
	  
	      switch( pack[i].id1 ) {
	      case 0x80: // Title
		if( trackNo == 0 )
		  textData.m_title.append( txtstr );
		else
		  textData.m_trackCdText[trackNo-1].m_title.append( txtstr );
		break;

	      case 0x81: // Performer
		if( trackNo == 0 )
		  textData.m_performer.append( txtstr );
		else
		  textData.m_trackCdText[trackNo-1].m_performer.append( txtstr );
		break;

	      case 0x82: // Writer
		if( trackNo == 0 )
		  textData.m_songwriter.append( txtstr );
		else
		  textData.m_trackCdText[trackNo-1].m_songwriter.append( txtstr );
		break;

	      case 0x83: // Composer
		if( trackNo == 0 )
		  textData.m_composer.append( txtstr );
		else
		  textData.m_trackCdText[trackNo-1].m_composer.append( txtstr );
		break;

	      case 0x84: // Arranger
		if( trackNo == 0 )
		  textData.m_arranger.append( txtstr );
		else
		  textData.m_trackCdText[trackNo-1].m_arranger.append( txtstr );
		break;

	      case 0x85: // Message
		if( trackNo == 0 )
		  textData.m_message.append( txtstr );
		else
		  textData.m_trackCdText[trackNo-1].m_message.append( txtstr );
		break;

	      case 0x86: // Disc identification
		// only global
		if( trackNo == 0 )
		  textData.m_discId.append( txtstr );
		break;

	      case 0x8e: // Upc or isrc
		if( trackNo == 0 )
		  textData.m_upcEan.append( txtstr );
		else
		  textData.m_trackCdText[trackNo-1].m_isrc.append( txtstr );
		break;

		// TODO: support for binary data
		// 0x88: TOC 
		// 0x89: second TOC
		// 0x8f: Size information

	      default:
		break;
	      }
	  
	      trackNo++;
	      nullPos = nextNullPos;
	    }
	  }
	}
	else
	  kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << " zero-sized CD-TEXT: " << dataLen << endl; 

	delete [] data;

	success = true;
      }
    } // valid trackCount

    if( needToClose )
      close();
  }

  return textData;
}


#ifdef Q_OS_LINUX
// fallback
bool K3bCdDevice::CdDevice::readTocLinux( K3bCdDevice::Toc& toc ) const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  bool success = true;

  toc.clear();

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
      success = false;
    } 
    else {
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

        if( ::ioctl(d->deviceFd,CDROMREADTOCENTRY,&tocentry) ) {
          kdDebug() << "(K3bCdDevice) error reading tocentry " << i << endl;
	  success = false;
	  break;
	}

        int startSec = tocentry.cdte_addr.lba;
        int control  = tocentry.cdte_ctrl & 0x0f;
        int mode     = tocentry.cdte_datamode;
        if( i > tochdr.cdth_trk0 ) {
	  Track track( lastTrack.firstSector(), startSec-1, lastTrack.type(), lastTrack.mode() );
	  track.m_preEmphasis = control & 0x1;
	  track.m_copyPermitted = control & 0x2;
          toc.append( track );
        }
        int trackType = 0;
        int trackMode = Track::UNKNOWN;
        if( (control & 0x04 ) && (tocentry.cdte_track != CDROM_LEADOUT) ) {
          trackType = Track::DATA;
          if( mode == 1 )
            trackMode = Track::MODE1;
          else if( mode == 2 )
            trackMode = Track::MODE2;

          mode = getDataMode(startSec);
          if( mode != Track::UNKNOWN )
            trackMode = mode;
        } 
	else
          trackType = Track::AUDIO;

        lastTrack = Track( startSec, startSec, trackType, trackMode );
      }
    }

    if( needToClose )
      close();
  }
  else
    success = false;

  return success;
}
#endif // Q_OS_LINUX


bool K3bCdDevice::CdDevice::fixupToc( K3bCdDevice::Toc& toc ) const
{
  bool success = false;

  //
  // This is a very lame method of fixing the TOC of an Advanced Audio CD
  // (a CD with two sessions: one with audio tracks and one with the data track)
  // If a drive does not support reading raw toc or reading track info we only
  // get every track's first sector. But between sessions there is a gap which is used
  // for ms stuff. In this case it's 11400 sectors in size. When ripping ausio we would 
  // include these 11400 sectors which would result in a strange ending audio file.
  //
  if( numSessions() > 1 || toc.contentType() == MIXED ) {
    kdDebug() << "(K3bCdDevice::CdDevice) fixup multisession toc..." << endl;

    //
    // we need to update the last sector of every last track in every session
    // for now we only update the track before the last session...
    // This is the most often case: Advanced Audio CD
    //

    unsigned char* data = 0;
    int dataLen = 0;
    if( readTocPmaAtip( &data, dataLen, 1, false, 0 ) ) {

      //
      // data[6]    - first track number in last complete session
      // data[8-11] - start adress of first track in last session
      //

      toc[(unsigned int)data[6]-2].m_lastSector = from4Byte( &data[8] ) - 11400 - 1;

      delete [] data;
      success = true;
    }
    else
      kdDebug() << "(K3bCdDevice::CdDevice) FIXUP TOC failed." << endl;
  }

  return success;
}


bool K3bCdDevice::CdDevice::block( bool b ) const
{
  ScsiCommand cmd( this );
  cmd[0] = MMC::PREVENT_ALLOW_MEDIUM_REMOVAL;
  cmd[4] = b ? 0x1 : 0x0;
  int r = cmd.transport();

  if( r )
    kdDebug() << "(K3bCdDevice::CdDevice) MMC ALLOW MEDIA REMOVAL failed." << endl;

  return ( r == 0 );
}

bool K3bCdDevice::CdDevice::rewritable() const
{
  unsigned char* data = 0;
  int dataLen = 0;

  if( readDiscInfo( &data, dataLen ) ) {
    disc_info_t* inf = (disc_info_t*)data;
    bool e = inf->erasable;

    delete [] data;

    return e;
  }
  else
    return false;
}

bool K3bCdDevice::CdDevice::eject() const
{
  block(false);

  ScsiCommand cmd( this );
  cmd[0] = MMC::START_STOP_UNIT;

  // Since all other eject methods I saw also start the unit before ejecting
  // we do it also although I don't know why...
  cmd[4] = 0x1;      // Start unit
  cmd.transport();

  cmd[4] = 0x2;    // LoEj = 1, Start = 0

  return !cmd.transport();
}


bool K3bCdDevice::CdDevice::load() const
{
  ScsiCommand cmd( this );
  cmd[0] = MMC::START_STOP_UNIT;
  cmd[4] = 0x3;    // LoEj = 1, Start = 1
  return !cmd.transport();
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

  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  DiskInfo info;
  info.device = this;

  if( open() != -1 ) {
    info.mediaType = 0;  // removed the mediaType method. use ngDiskInfo
    bool ready = isReady();
    if( ready ) {
      info.toc = readToc();
      switch( info.toc.contentType() ) {
      case AUDIO:
	info.tocType = DiskInfo::AUDIO;
	break;
      case DATA:
	if( isDVD() )
	  info.tocType = DiskInfo::DVD;
	else
	  info.tocType = DiskInfo::DATA;
	break;
      case MIXED:
	  info.tocType = DiskInfo::MIXED;
	  break;
      default:
	info.tocType = DiskInfo::UNKNOWN;
	break;
      }
      info.valid = true;

      if( info.tocType == DiskInfo::NODISC ) {
        kdDebug() << "(K3bCdDevice::CdDevice::diskInfo) no disk." << endl;
        info.noDisk = true;
      } else if( info.tocType != DiskInfo::UNKNOWN ) {
        kdDebug() << "(K3bCdDevice::CdDevice::diskInfo) valid disk." << endl;
        info.noDisk = false;
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
    }
    else {  // no disk or tray open
      kdDebug() << "(K3bCdDevice::CdDevice::diskInfo) no disk or tray open." << endl;
      info.valid = true;
      info.noDisk = true;
    }
  }

  if( needToClose )
    close();

  return info;
}


int K3bCdDevice::CdDevice::supportedProfiles() const
{
  return d->supportedProfiles;
}


int K3bCdDevice::CdDevice::currentProfile() const
{
  unsigned char profileBuf[8];
  ::memset( profileBuf, 0, 8 );

  ScsiCommand cmd( this );
  cmd[0] = MMC::GET_CONFIGURATION;
  cmd[1] = 1;
  cmd[8] = 8;

  if( cmd.transport( TR_DIR_READ, profileBuf, 8 ) ) {
    kdDebug() << "(K3bCdDevice) GET_CONFIGURATION failed." << endl;

    return -1;
  }
  else {
    short profile = from2Byte( &profileBuf[6] );
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


K3bCdDevice::NextGenerationDiskInfo K3bCdDevice::CdDevice::ngDiskInfo() const
{
  NextGenerationDiskInfo inf;
  inf.m_diskState = STATE_UNKNOWN;

  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  if( open() != -1 ) {

    //
    // The first thing to do should be: checking if a media is loaded
    // We do this with requesting the current profile. If it is 0 no media
    // should be loaded. On an error we just go on.
    //
    if( !isReady() ) {
      // no disk or tray open
      inf.m_diskState = STATE_NO_MEDIA;
      inf.m_mediaType = MEDIA_NONE;
    }
    else {
      int profile = currentProfile();
      if( profile == MEDIA_NONE ) {
	inf.m_diskState = STATE_NO_MEDIA;
	inf.m_mediaType = MEDIA_NONE;
      }
      inf.m_currentProfile = profile;
    }

    if( inf.diskState() != STATE_NO_MEDIA ) {

      unsigned char* data = 0;
      int dataLen = 0;
      
      if( readDiscInfo( &data, dataLen ) ) {
	disc_info_t* dInf = (disc_info_t*)data;
	//
	// Copy the needed values from the disk_info struct
	//
	switch( dInf->status ) {
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

	switch( dInf->border ) {
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

	inf.m_rewritable = dInf->erasable;

	// we set it here so we have the info even if the call to READ TOC/PMA/ATIP failes
	inf.m_numSessions = (dInf->n_sessions_l & 0x00FF) | ((dInf->n_sessions_m << 8) & 0xFF00);

	//	inf.m_numTracks = ( (dInf->last_track_m<<8) | dInf->last_track_l);

	// MMC-4 says that only CD-R(W) should return proper sizes here. 
	// Does that means that lead_out_r is rather useless?
	inf.m_capacity = K3b::Msf( dInf->lead_out_m + dInf->lead_out_r*60, 
				   dInf->lead_out_s, 
				   dInf->lead_out_f ) - 150;

	// Why do we need to substract 4500 again??
	if( inf.appendable() )
	  inf.m_remaining = inf.capacity() - K3b::Msf( dInf->lead_in_m + dInf->lead_in_r*60, 
						       dInf->lead_in_s, 
						       dInf->lead_in_f ) - 4500;

	delete [] data;
      }

      //
      // Now we determine the size:
      // for empty and appendable CD-R(W) and DVD+R media this should be in the dInf->lead_out_X fields
      // for all empty and appendable media READ FORMAT CAPACITIES should return the proper unformatted size
      // for complete disks we may use the READ_CAPACITY command or the start sector from the leadout
      //

      if( inf.diskState() == STATE_EMPTY ||
	  inf.diskState() == STATE_INCOMPLETE ) {
	K3b::Msf readFrmtCap;
	if( readFormatCapacity( readFrmtCap ) ) {
	  kdDebug() << "(K3bCdDevice::CdDevice) READ FORMAT CAPACITY: " << readFrmtCap.toString()
		    << " other capacity: " << inf.m_capacity.toString() << endl;
	  if( readFrmtCap > 0 )
	    inf.m_capacity = readFrmtCap;
	}
      }
      else {
	K3b::Msf readCap;
	if( readCapacity( readCap ) ) {
	  kdDebug() << "(K3bCdDevice::CdDevice) READ CAPACITY: " << readCap.toString()
		    << " other capacity: " << inf.m_capacity.toString() << endl;

	  //
	  // READ CAPACITY returns the last written sector
	  // that means the size is actually readCap + 1
	  //
	  inf.m_capacity = readCap + 1;
	}
	else {
	  kdDebug() << "(K3bCdDevice) READ CAPACITY failed. Falling back to READ TRACK INFO." << endl;

	  unsigned char* trackData = 0;
	  int trackDataLen = 0;
	  if( readTrackInformation( &trackData, trackDataLen, 0x1, 0xff ) ) {
	    track_info_t* trackInfo = (track_info_t*)trackData;
	    inf.m_capacity = from4Byte( trackInfo->track_start );

	    delete [] data;
	  }
	  else {
	    kdDebug() << "(K3bCdDevice) Falling back to readToc." << endl;
	    inf.m_capacity = readToc().length();
	  }
	}
      }

      // 
      // The mediatype needs to be set
      //

      // no need to test for dvd if the device does not support it
      if( readsDvd() )
	inf.m_mediaType = dvdMediaType();
      else
	inf.m_mediaType = -1;

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
	kdDebug() << "(K3bCdDevice) could not get session info via READ TOC/PMA/ATIP." << endl;
	if( inf.empty() ) {
	  // just to fix the info for empty disks
	  inf.m_numSessions = 0;
	}
      }
    }
   
    if( needToClose )
      close();
  }
  
  return inf;
  }


int K3bCdDevice::CdDevice::cdMediaType() const
{
  int m = -1;

  unsigned char* data = 0;
  int dataLen = 0;
  if( readTocPmaAtip( &data, dataLen, 4, false, 0 ) ) {
    if( (data[6]>>6)&1 )
      m = MEDIA_CD_RW;
    else
      m = MEDIA_CD_R;

    delete [] data;
  }
  else
    m = MEDIA_CD_ROM;

  return m;
}


int K3bCdDevice::CdDevice::dvdMediaType() const
{
  int m = -1;

  unsigned char dvdheader[20];
  ::memset( dvdheader, 0, 20 );
  ScsiCommand cmd( this );
  cmd[0] = MMC::READ_DVD_STRUCTURE;
  cmd[9] = 20;
  if( cmd.transport( TR_DIR_READ, dvdheader, 20 ) ) {
    kdDebug() << "(K3bCdDevice::CdDevice) Unable to read DVD structure." << endl;
  }
  else {
    switch( dvdheader[4]&0xF0 ) {
    case 0x00: m = MEDIA_DVD_ROM; break;
    case 0x10: m = MEDIA_DVD_RAM; break;
    case 0x20: m = MEDIA_DVD_R; break;
    case 0x30: m = MEDIA_DVD_RW; break;
    case 0x90: m = MEDIA_DVD_PLUS_RW; break;
    case 0xA0: m = MEDIA_DVD_PLUS_R; break;
    default: m = -1; break; // unknown
    }
  }

  return m;
}


// does only make sense for complete media
bool K3bCdDevice::CdDevice::readCapacity( K3b::Msf& r ) const
{
  ScsiCommand cmd( this );
  cmd[0] = MMC::READ_CAPACITY;
  unsigned char buf[8];
  ::memset( buf, 0, 8 );
  if( cmd.transport( TR_DIR_READ, buf, 8 ) == 0 ) {
    r = from4Byte( buf );
    return true;
  }
  else
    return false;
}


bool K3bCdDevice::CdDevice::readFormatCapacity( K3b::Msf& r ) const
{
  bool success = false;

  unsigned char header[4]; // for reading the size of the returned data
  ::memset( header, 0, 4 );

  ScsiCommand cmd( this );
  cmd[0] = MMC::READ_FORMAT_CAPACITIES;
  cmd[8] = 4;
  if( cmd.transport( TR_DIR_READ, header, 4 ) == 0 ) {
    int realLength = header[3] + 4;

    unsigned char* buffer = new unsigned char[realLength];
    ::memset( buffer, 0, realLength );

    cmd[7] = realLength >> 8;
    cmd[8] = realLength & 0xFF;
    if( cmd.transport( TR_DIR_READ, buffer, realLength ) == 0 ) {
      //
      // now find the 00h format type since that contains the number of adressable blocks
      // and the block size used for formatting the whole media.
      // There may be multible occurences of this descriptor (MMC4 says so) but I think it's
      // sufficient to read the first one
      // 00h may not be supported by the unit (e.g. CD-RW)
      // for this case we fall back to the first descriptor (the current/maximum descriptor)
      //
      for( int i = 12; i < realLength-4; ++i ) {
	if( (buffer[i+4]>>2) == 0 ) {
	  // found the descriptor
	  r = from4Byte( &buffer[i] );
	  success = true;
	  break;
	}
      }

      if( !success ) {
	// try the current/maximum descriptor
	int descType = buffer[8] & 0x03;
	if( descType == 1 || descType == 2 ) {
	  // 1: unformatted :)
	  // 2: formatted. Here we get the used capacity (lead-in to last lead-out/border-out)
	  r = from4Byte( &buffer[4] );
	  success = true;

	  // FIXME: we assume a blocksize of 2048 here. Is that correct? Wouldn' it be better to use
	  //        the blocksize from the descriptor?
	}
      }
    }

    delete [] buffer;
  }
  
  return success;
}


bool K3bCdDevice::CdDevice::readSectorsRaw(unsigned char *buf, int start, int count) const
{
  return readCd( buf, count*2352,
		 0,      // all sector types
		 false,  // no dap
		 start, 
		 count,
		 true, // SYNC
		 true, // HEADER
		 true, // SUBHEADER
		 true, // USER DATA
		 true, // EDC/ECC
		 0,    // no c2 info
		 0 );
}


bool K3bCdDevice::CdDevice::modeSense( unsigned char** pageData, int& pageLen, int page ) const
{
  unsigned char header[8];
  ::memset( header, 0, 8 );

  ScsiCommand cmd( this );
  cmd[0] = MMC::MODE_SENSE;
  cmd[1] = 0x08;        // Disable Block Descriptors
  cmd[2] = page;
  cmd[8] = 8;           // first we determine the data length
  if( cmd.transport( TR_DIR_READ, header, 8 ) == 0 ) {
    // again with real length
    pageLen = from2Byte( header ) + 2;

    *pageData = new unsigned char[pageLen];
    ::memset( *pageData, 0, pageLen );

    cmd[7] = pageLen>>8;
    cmd[8] = pageLen;
    if( cmd.transport( TR_DIR_READ, *pageData, pageLen ) == 0 )
      return true;
    else
      delete [] *pageData;
  }

  return false;
}


bool K3bCdDevice::CdDevice::modeSelect( unsigned char* page, int pageLen, bool pf, bool sp ) const
{
  ScsiCommand cmd( this );
  cmd[0] = MMC::MODE_SELECT;
  cmd[1] = ( sp ? 1 : 0 ) | ( pf ? 0x10 : 0 );
  cmd[7] = pageLen>>8;
  cmd[8] = pageLen;
  cmd[9] = 0;
  return( cmd.transport( TR_DIR_WRITE, page, pageLen ) == 0 );
}


void K3bCdDevice::CdDevice::checkWriteModes()
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  if (open() < 0)
    return;

  // header size is 8
  unsigned char* buffer = 0;
  int dataLen = 0;

  if( !modeSense( &buffer, dataLen, 0x05 ) ) {
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": modeSense 0x05 failed!" << endl;
  }
  else {
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": dataLen: " << dataLen << endl;

    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": modesense data: " << endl;
    //    debugBitfield( buffer, dataLen );

    wr_param_page_05* mp = (struct wr_param_page_05*)(buffer+8);

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

    //    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": modeselect TAO data: " << endl;
    //    debugBitfield( buffer, dataLen );


    //
    // if a writer does not support TAO it surely does not support SAO or RAW writing since TAO is the minimal
    // requirement
    //

    if( modeSelect( buffer, dataLen, 1, 0 ) ) {
      m_writeModes |= TAO;
      d->deviceType |= CDR;

      // SAO
      mp->write_type = 0x02; // Session-at-once

      if( modeSelect( buffer, dataLen, 1, 0 ) )
	m_writeModes |= SAO;

      // RAW
      mp->write_type = 0x03; // RAW
      mp->dbtype = 1;        // Raw data with P and Q Sub-channel (2368 bytes)
      if( modeSelect( buffer, dataLen, 1, 0 ) ) {
	m_writeModes |= RAW;
	m_writeModes |= RAW_R16;
      }

      mp->dbtype = 2;        // Raw data with P-W Sub-channel (2448 bytes)
      if( modeSelect( buffer, dataLen, 1, 0 ) ) {
	m_writeModes |= RAW;
	m_writeModes |= RAW_R96P;
      }

      mp->dbtype = 3;        // Raw data with P-W raw Sub-channel (2368 bytes)
      if( modeSelect( buffer, dataLen, 1, 0 ) ) {
	m_writeModes |= RAW;
	m_writeModes |= RAW_R96R;
      }
    }
    else {
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": modeSelect with TAO failed. No writer" << endl;
    }


    delete [] buffer;
  }
    
  if( needToClose )
    close();
}


bool K3bCdDevice::CdDevice::readTocPmaAtip( unsigned char** data, int& dataLen, int format, bool time, int track ) const
{
  unsigned char header[2];
  ::memset( header, 0, 2 );

  ScsiCommand cmd( this );
  cmd[0] = MMC::READ_TOC_PMA_ATIP;
  cmd[1] = ( time ? 0x2 : 0x0 );
  cmd[2] = format & 0x0F;
  cmd[6] = track;
  cmd[7] = 0;
  cmd[8] = 2; // we only read the length first

  if( cmd.transport( TR_DIR_READ, header, 2 ) == 0 ) {
    // again with real length
    dataLen = from2Byte( header ) + 2;

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[7] = dataLen>>8;
    cmd[8] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 )
      return true;
    else {
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ TOC/PMA/ATIP format "
		<< format << " with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ TOC/PMA/ATIP length det failed." << endl;

  return false;
}


bool K3bCdDevice::CdDevice::mechanismStatus( unsigned char** data, int& dataLen ) const
{
  unsigned char header[8];
  ::memset( header, 0, 8 );

  ScsiCommand cmd( this );
  cmd[0] = MMC::MECHANISM_STATUS;
  cmd[9] = 8;     // first we read the header
  if( cmd.transport( TR_DIR_READ, header, 8 ) == 0 ) {
    // again with real length
    dataLen = from4Byte( &header[6] ) + 8;

    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": MECHANISM STATUS " 
	      << (int)header[5] << " slots." << endl;

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[8] = dataLen>>8;
    cmd[9] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 ) {
      return true;
    }
    else {
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": MECHANISM STATUS with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": MECHANISM STATUS length det failed." << endl;

  return false;
}


int K3bCdDevice::CdDevice::determineMaximalWriteSpeed() const
{
  int ret = 0;

  QValueList<int> list = determineSupportedWriteSpeeds();
  for( QValueList<int>::iterator it = list.begin(); it != list.end(); ++it )
    ret = QMAX( ret, *it );

  return ret;
}


QValueList<int> K3bCdDevice::CdDevice::determineSupportedWriteSpeeds() const 
{
  QValueList<int> ret;

  if( burner() ) {
    unsigned char* data = 0;
    int dataLen = 0;
    if( modeSense( &data, dataLen, 0x2A ) ) {
      mm_cap_page_2A* mm = (mm_cap_page_2A*)&data[8];

      if( dataLen > 32 ) {
	// we have descriptors
	int numDes = from2Byte( mm->num_wr_speed_des );
	cd_wr_speed_performance* wr = (cd_wr_speed_performance*)mm->wr_speed_des;      

	kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ":  Number of supported write speeds: "
		  << numDes << endl;
		

	for( int i = 0; i < numDes; ++i ) {
	  // sort the list
	  int s = from2Byte( wr[i].wr_speed_supp );
	  QValueList<int>::iterator it = ret.begin();
	  while( it != ret.end() && *it < s )
	    ++it;
	  ret.insert( it, s );
	}
      }
      else if( dataLen > 19 ) {
	// MMC1 used byte 18 and 19 for the max write speed
	ret.append( from2Byte( mm->max_write_speed ) );
      }
      else
	kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": no writing speed info. No MMC drive?" << endl;

      delete [] data;
    }
  }

  return ret;
}


bool K3bCdDevice::CdDevice::read10( unsigned char* data,
				    int dataLen,
				    unsigned long startAdress,
				    unsigned int length,
				    bool fua ) const
{
  ::memset( data, 0, dataLen );

  ScsiCommand cmd( this );
  cmd[0] = MMC::READ_10;
  cmd[1] = ( fua ? 0x8 : 0x0 );
  cmd[2] = startAdress>>24;
  cmd[3] = startAdress>>16;
  cmd[4] = startAdress>>8;
  cmd[5] = startAdress;
  cmd[7] = length>>8;
  cmd[8] = length;

  if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ 10 failed!" << endl;
    return false;
  }
  else
    return true;
}


bool K3bCdDevice::CdDevice::read12( unsigned char* data,
				    int dataLen,
				    unsigned long startAdress,
				    unsigned long length,
				    bool streaming,
				    bool fua ) const
{
  ::memset( data, 0, dataLen );

  ScsiCommand cmd( this );
  cmd[0] = MMC::READ_12;
  cmd[1] = ( fua ? 0x8 : 0x0 );
  cmd[2] = startAdress>>24;
  cmd[3] = startAdress>>16;
  cmd[4] = startAdress>>8;
  cmd[5] = startAdress;
  cmd[6] = length>>24;
  cmd[7] = length>>16;
  cmd[8] = length>>8;
  cmd[9] = length;
  cmd[10] = (streaming ? 0x80 : 0 );

  if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ 12 failed!" << endl;
    return false;
  }
  else
    return true;
}


bool K3bCdDevice::CdDevice::readCd( unsigned char* data, 
				    int dataLen,
				    int sectorType,
				    bool dap,
				    unsigned long startAdress,
				    unsigned long length,
				    bool sync,
				    bool header,
				    bool subHeader,
				    bool userData,				    
				    bool edcEcc,
				    int c2,
				    int subChannel ) const
{
  ::memset( data, 0, dataLen );

  ScsiCommand cmd( this );
  cmd[0] = MMC::READ_CD;
  cmd[1] = (sectorType<<2 & 0x1c) | ( dap ? 0x2 : 0x0 );
  cmd[2] = startAdress>>24;
  cmd[3] = startAdress>>16;
  cmd[4] = startAdress>>8;
  cmd[5] = startAdress;
  cmd[6] = length>>16;
  cmd[7] = length>>8;
  cmd[8] = length;
  cmd[9] = ( ( sync      ? 0x80 : 0x0 ) | 
	     ( subHeader ? 0x40 : 0x0 ) | 
	     ( header    ? 0x20 : 0x0 ) |
	     ( userData  ? 0x10 : 0x0 ) |
	     ( edcEcc    ? 0x8  : 0x0 ) |
	     ( c2<<1 & 0x6 ) );
  cmd[10] = subChannel & 0x7;

  if( cmd.transport( TR_DIR_READ, data, dataLen ) ) {
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ CD failed!" << endl;
    return false;
  }
  else
    return true;
}


bool K3bCdDevice::CdDevice::readSubChannel( unsigned char** data, int& dataLen,
					    unsigned int subchannelParam,
					    unsigned int trackNumber ) const
{
  unsigned char header[4];
  ::memset( header, 0, 4 );

  ScsiCommand cmd( this );
  cmd[0] = MMC::READ_SUB_CHANNEL;
  cmd[2] = 0x40;    // SUBQ
  cmd[3] = subchannelParam;
  cmd[6] = trackNumber;   // only used when subchannelParam == 03h (ISRC)
  cmd[8] = 4;      // first we read the header
  if( cmd.transport( TR_DIR_READ, header, 4 ) == 0 ) {
    // again with real length
    dataLen = from2Byte( &header[2] ) + 4;

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[7] = dataLen>>8;
    cmd[8] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 ) {
      return true;
    }
    else {
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ SUB-CHANNEL with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ SUB-CHANNEL length det failed." << endl;

  return false;
}


bool K3bCdDevice::CdDevice::readTrackInformation( unsigned char** data, int& dataLen, int type, unsigned long value ) const
{
  unsigned char header[4];
  ::memset( header, 0, 4 );

  ScsiCommand cmd( this );
  cmd[0] = MMC::READ_TRACK_INFORMATION;

  switch( type ) {
  case 0:
  case 1:
  case 2:
    cmd[1] = type & 0x3;
    cmd[2] = value>>24;
    cmd[3] = value>>16;
    cmd[4] = value>>8;
    cmd[5] = value;
    break;
  default:
    kdDebug() << "(K3bCdDevice::readTrackInformation) wrong type parameter: " << type << endl;
    return false;
  }

  cmd[8] = 4;      // first we read the header
  if( cmd.transport( TR_DIR_READ, header, 4 ) == 0 ) {
    // again with real length
    dataLen = from2Byte( header ) + 2;

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[7] = dataLen>>8;
    cmd[8] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 ) {
      return true;
    }
    else {
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ TRACK INFORMATION with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": READ TRACK INFORMATION length det failed." << endl;

  return false;
}


int K3bCdDevice::CdDevice::getIndex( unsigned long lba ) const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  if( open() < 0 )
    return -1;

  int ret = -1;

  //
  // first try readCd
  //
  unsigned char readData[16];
  ::memset( readData, 0, 16 );

  //
  // The index is found in the Mode-1 Q which occupies at least 9 out of 10 successive CD frames
  // It can be indentified by ADR == 1
  //
  // So if the current sector does not provide Mode-1 Q subchannel we try the previous.
  //
  
  if( readCd( readData, 
	      16,
	      1, // CD-DA
	      0, // no DAP
	      lba,
	      1,
	      false,
	      false,
	      false,
	      false,
	      false,
	      0,
	      2 // Q-Subchannel
	      ) ) {
    // byte 0: 4 bits CONTROL (MSB) + 4 bits ADR (LSB) 
    if( (readData[0]&0x0f) == 0x1 )
      ret = readData[2];

    // search previous sector for Mode1 Q Subchannel
    else if( readCd( readData, 
	      16,
	      1, // CD-DA
	      0, // no DAP
	      lba-1,
	      1,
	      false,
	      false,
	      false,
	      false,
	      false,
	      0,
	      2 // Q-Subchannel
	      ) ) {
      if( (readData[0]&0x0f) == 0x1 )
	ret = readData[2];
      else
	ret = -2;
    }
  }

  else {
    kdDebug() << "(K3bCdDevice::CdDevice::getIndex) readCd failed. Trying seek." << endl;

    unsigned char* data = 0;
    int dataLen = 0;
    if( seek( lba ) && readSubChannel( &data, dataLen, 1, 0 ) ) {
      // byte 5: 4 bits ADR (MSB) + 4 bits CONTROL (LSB)
      if( dataLen > 7 && (data[5]>>4 & 0x0F) == 0x1 ) {
	ret = data[7];
      }
      else if( seek( lba-1 ) && readSubChannel( &data, dataLen, 1, 0 ) ) {
	if( dataLen > 7 && (data[5]>>4 & 0x0F) == 0x1 )
	  ret = data[7];
	else
	  ret = -2;
      }
      
      delete [] data;
    }
    else
      kdDebug() << "(K3bCdDevice::CdDevice::getIndex) seek or readSubChannel failed." << endl;
  }

  if( needToClose )
    close();
  
  return ret;
}


bool K3bCdDevice::CdDevice::seek( unsigned long lba ) const
{
  ScsiCommand cmd( this );
  cmd[0] = MMC::SEEK_10;
  cmd[2] = lba>>24;
  cmd[3] = lba>>16;
  cmd[4] = lba>>8;
  cmd[5] = lba;

  return !cmd.transport();
}


bool K3bCdDevice::CdDevice::searchIndex0( unsigned long startSec,
					  unsigned long endSec,
					  long& pregapStart ) const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  if( open() < 0 )
    return false;

  bool ret = false;

  int lastIndex = getIndex( endSec );
  if( lastIndex == 0 ) {
    // there is a pregap
    // let's find the position where the index turns to 0
    // we jump in 1 sec steps backwards until we find an index > 0
    unsigned long sector = endSec;
    while( lastIndex == 0 && sector > startSec ) {
      sector -= 75;
      if( sector < startSec )
	sector = startSec;
      lastIndex = getIndex(sector);
    }

    if( lastIndex == 0 ) {
      kdDebug() << "(K3bCdDevice::CdDevice) warning: no index != 0 found." << endl;
    }
    else {
      // search forward to the first index = 0
      while( getIndex( sector ) != 0 && sector < endSec )
	sector++;

      pregapStart = sector;
      ret = true;
    }
  }
  else if( lastIndex > 0 ) {
    // no pregap
    pregapStart = -1;
    ret = true;
  }
  
  if( needToClose )
    close();

  return ret;
}


bool K3bCdDevice::CdDevice::indexScan( K3bCdDevice::Toc& toc ) const
{
  // if the device is already opened we do not close it
  // to allow fast multible method calls in a row
  bool needToClose = !isOpen();

  if( open() < 0 )
    return false;

  bool ret = true;

  for( Toc::iterator it = toc.begin(); it != toc.end(); ++it ) {
    Track& track = *it;
    if( track.type() == Track::AUDIO ) {
      track.m_indices.clear();
      long index0 = -1;
      if( searchIndex0( track.firstSector().lba(), track.lastSector().lba(), index0 ) ) {
	kdDebug() << "(K3bCdDevice::CdDevice) found index 0: " << index0 << endl;
      }
      track.m_indices.append( index0 );
      if( index0 > 0 )
	searchIndexTransitions( track.firstSector().lba(), index0-1, track );
      else
	searchIndexTransitions( track.firstSector().lba(), track.lastSector().lba(), track );
    }
  }

  if( needToClose )
    close();

  return ret;
}


void K3bCdDevice::CdDevice::searchIndexTransitions( long start, long end, K3bCdDevice::Track& track ) const
{
  kdDebug() << "(K3bCdDevice::CdDevice) searching for index transitions betweeen " 
	    << start << " and " << end << endl;
  int startIndex = getIndex( start );
  int endIndex = getIndex( end );

  if( startIndex < 0 || endIndex < 0 ) {
    kdDebug() << "(K3bCdDevice::CdDevice) could not retrieve index values." << endl;
  }
  
  kdDebug() << "(K3bCdDevice::CdDevice) indices: " << start << " - " << startIndex
	    << " and " << end << " - " << endIndex << endl;

  if( startIndex != endIndex ) {
    if( start+1 == end ) {
      kdDebug() << "(K3bCdDevice::CdDevice) found index transition: " << endIndex << " " << end << endl;
      track.m_indices.resize( endIndex+1 );
      track.m_indices[endIndex] = end;
    }
    else {
      searchIndexTransitions( start, start+(end-start)/2, track );
      searchIndexTransitions( start+(end-start)/2, end, track );
    }
  }
}


bool K3bCdDevice::CdDevice::getFeature( unsigned char** data, int& dataLen, unsigned int feature ) const
{
  unsigned char header[8];
  ::memset( header, 0, 8 );

  ScsiCommand cmd( this );
  cmd[0] = MMC::GET_CONFIGURATION;
  cmd[1] = 2;      // read only specified feature
  cmd[2] = feature>>8;
  cmd[3] = feature;
  cmd[8] = 8;      // we only read the data length first
  if( cmd.transport( TR_DIR_READ, header, 8 ) ) {
    // again with real length
    dataLen = from4Byte( header ) + 4;

    *data = new unsigned char[dataLen];
    ::memset( *data, 0, dataLen );

    cmd[7] = dataLen>>8;
    cmd[8] = dataLen;
    if( cmd.transport( TR_DIR_READ, *data, dataLen ) == 0 ) {
      return true;
    }
    else {
      kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": GET CONFIGURATION with real length "
		<< dataLen << " failed." << endl;
      delete [] *data;
    }
  }
  else
    kdDebug() << "(K3bCdDevice::CdDevice) " << blockDeviceName() << ": GET CONFIGURATION length det failed." << endl;

  return false; 
}


bool K3bCdDevice::CdDevice::supportsFeature( unsigned int feature ) const
{
  unsigned char* data = 0;
  int dataLen = 0;
  if( getFeature( &data, dataLen, feature ) ) {
    bool success = false;
    if( dataLen >= 11 )
      success = ( data[10]&1 );  // check the current flag

    delete [] data;      

    return success;
  }
  else
    return false;
}


bool K3bCdDevice::CdDevice::readIsrc( unsigned int track, QCString& isrc ) const
{
  unsigned char* data = 0;
  int dataLen = 0;

  if( readSubChannel( &data, dataLen, 0x3, track ) ) {
    bool isrcValid = false;

    if( dataLen >= 8+18 ) {
      isrcValid = (data[8+4]>>7 & 0x1);
      
      if( isrcValid ) {
	isrc = QCString( reinterpret_cast<char*>(data[8+5]), 13 );

	// TODO: check the range of the chars
	
      }
    }
    
    delete [] data;

    return isrcValid;
  }
  else
    return false;
}


bool K3bCdDevice::CdDevice::readMcn( QCString& mcn ) const
{
  unsigned char* data = 0;
  int dataLen = 0;

  if( readSubChannel( &data, dataLen, 0x2, 0 ) ) {
    bool mcnValid = false;

    if( dataLen >= 8+18 ) {
      mcnValid = (data[8+4]>>7 & 0x1);
      
      if( mcnValid )
	mcn = QCString( reinterpret_cast<char*>(data[8+5]), 14 );
    }
    
    delete [] data;

    return mcnValid;
  }
  else
    return false;
}
