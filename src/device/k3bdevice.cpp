/***************************************************************************
                          k3bdevice.cpp  -  description
                             -------------------
    begin                : Tue May 14 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


const char* K3bDevice::cdrdao_drivers[] = { "auto", "plextor", "plextor-scan", "cdd2600", "generic-mmc", 
					    "generic-mmc-raw", "ricoh-mp6200", "sony-cdu920", 
					    "sony-cdu948", "taiyo-yuden", "teac-cdr55", "toshiba", 
					    "yamaha-cdr10x", 0};




K3bDevice::K3bDevice( const QString& devname )
{
  d = new Private;

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


K3bDevice::~K3bDevice()
{
  delete d;
}


bool K3bDevice::init()
{
  int cdromfd = ::open( blockDeviceName().ascii(), O_RDONLY | O_NONBLOCK );
  if(cdromfd < 0) {
    kdDebug() << "could not open device " << blockDeviceName() << " (" << strerror(errno) << ")" << endl;
    return false;
  }

  int drivetype = ::ioctl(cdromfd, CDROM_GET_CAPABILITY, CDSL_CURRENT);
  if( drivetype < 0 ) {
    kdDebug() << "Error while retrieving capabilities." << endl;
    ::close( cdromfd );
    return false;
  }

  d->deviceType = CDROM;  // all drives should be able to read cdroms

  if (drivetype & CDC_CD_R) {
    d->deviceType |= CDR;
  }
  if (drivetype & CDC_CD_RW) {
    d->deviceType |= CDRW;
  }
  if (drivetype & CDC_DVD_R){
    d->deviceType |= DVDR;
  }
  if (drivetype & CDC_DVD_RAM) {
    d->deviceType |= DVDRAM;
  }
  if (drivetype & CDC_DVD) {
    d->deviceType |= DVDROM;
  }

  ::close( cdromfd );

  return furtherInit();
}


bool K3bDevice::furtherInit()
{
  return true;
}


int K3bDevice::type() const
{
  return d->deviceType;
}


const QString& K3bDevice::devicename() const
{
  return blockDeviceName();
}


const QString& K3bDevice::ioctlDevice() const
{
  return blockDeviceName();
}


const QString& K3bDevice::blockDeviceName() const
{
  return d->blockDeviceName;
}


const QString& K3bDevice::genericDevice() const
{
  return d->genericDevice;
}


QString K3bDevice::busTargetLun() const
{
  return QString("%1,%2,%3").arg(m_bus).arg(m_target).arg(m_lun);
}


int K3bDevice::cdTextCapable() const
{
  if( cdrdaoDriver() == "auto" )
    return 0;
  else
    return m_cdTextCapable;
}


void K3bDevice::setCdTextCapability( bool b )
{
  m_cdTextCapable = ( b ? 1 : 2 );
}


void K3bDevice::setMountPoint( const QString& mp )
{
  d->mountPoint = mp;
}

void K3bDevice::setMountDevice( const QString& md ) 
{ 
  d->mountDeviceName = md; 
}


const QString& K3bDevice::mountDevice() const
{ 
  return d->mountDeviceName; 
}


const QString& K3bDevice::mountPoint() const 
{
  return d->mountPoint; 
}

void K3bDevice::setBurnproof( bool b )
{
  m_burnproof = b;
}

K3bDiskInfo::type  K3bDevice::diskType() {

  int status;
  K3bDiskInfo::type ret = K3bDiskInfo::UNKNOWN;
  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
    return K3bDiskInfo::UNKNOWN;
  }

  if ( (status = ::ioctl(cdromfd,CDROM_DISC_STATUS)) >= 0 ) {
    switch (status) {
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
    
  ::close(cdromfd);
  return ret;

}

bool K3bDevice::isDVD() {
  bool ret = false;
  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
    return ret;
  }
  if ( d->deviceType & (DVDR | DVDRAM | DVDROM) ) {
//     try to read the physical dvd-structure
//     if this fails, we probably cannot take any further (usefull) dvd-action
     dvd_struct dvdinfo;
     ::memset(&dvdinfo,0,sizeof(dvd_struct));
     dvdinfo.type = DVD_STRUCT_PHYSICAL;
     if ( ::ioctl(cdromfd,DVD_READ_STRUCT,&dvdinfo) == 0 ) 
       ret = true;
  }
  ::close(cdromfd);
  return ret;
}

int K3bDevice::isReady() const
{
  int drive_status,ret;
  ret = 1;
  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
    return ret;
  }
  
  if ( (drive_status = ::ioctl(cdromfd,CDROM_DRIVE_STATUS)) < 0 ) {
    kdDebug() << "(K3bDevice) Error: could not get drive status" << endl;
    ret = 1;
  } 
  else if ( drive_status == CDS_DISC_OK ) 
    ret = 0;
  else if ( drive_status == CDS_NO_DISC || drive_status == CDS_TRAY_OPEN ) 
    ret = 3;
  
  ::close(cdromfd);
  return ret;
}


int K3bDevice::isEmpty()
{
  int ret = NO_INFO;
  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
    return NO_INFO;
  }

  int drive_status;
  if ( (drive_status = ::ioctl(cdromfd,CDROM_DRIVE_STATUS)) < 0 ) {
    kdDebug() << "(K3bDevice) Error: could not get drive status" << endl;
    ret = NO_INFO;
  } else if ( drive_status == CDS_NO_DISC || drive_status == CDS_TRAY_OPEN ) { // kernel bug ?? never seen CDS_NO_DISC
    kdDebug() << "(K3bDevice)  Error: No disk in drive" << endl;
    ret = NO_DISK;
  } else {
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
    if ( ::ioctl(cdromfd,CDROM_SEND_PACKET,&cmd) == 0 ) {
      ret = (inf[2] & 0x03);
    } else { 
      kdDebug() << "(K3bDevice) could not get disk info !" << endl;
      ret = NO_INFO;
    }
  }  
  
  ::close( cdromfd );
  return ret;
  
}

int K3bDevice::discSize() {
  int ret = -1;
  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
    return ret;
  }

  struct cdrom_generic_command cmd;

  unsigned char inf[32];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(inf,0,32);
  cmd.cmd[0] = GPCMD_READ_DISC_INFO;
  cmd.cmd[8] = 32;
  cmd.buffer = inf;
  cmd.buflen = 32;
  cmd.data_direction = CGC_DATA_READ;
  if ( ::ioctl(cdromfd,CDROM_SEND_PACKET,&cmd) == 0 ) {
    if ( inf[21] != 0xFF && inf[22] != 0xFF && inf[23] != 0xFF ) 
      ret = (int)(inf[21] << 16) | (int)(inf[22] << 8) | (int)inf[23];
  } else 
    kdDebug() << "(K3bDevice) could not get disk info !" << endl;
  
  ::close( cdromfd );
  return ret;
  
}

int K3bDevice::remainingSize() {
  int ret = -1;
  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
    return ret;
  }

  struct cdrom_generic_command cmd;

  unsigned char inf[32];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(inf,0,32);
  cmd.cmd[0] = GPCMD_READ_DISC_INFO;
  cmd.cmd[8] = 32;
  cmd.buffer = inf;
  cmd.buflen = 32;
  cmd.data_direction = CGC_DATA_READ;
  if ( ::ioctl(cdromfd,CDROM_SEND_PACKET,&cmd) == 0 ) {
    if ( inf[17] != 0xFF && inf[18] != 0xFF && inf[19] != 0xFF ) 
      ret = (int)(inf[17] << 16) |  (int)(inf[18] << 8) | (int)inf[19];
  } else 
    kdDebug() << "(K3bDevice) could not get disk info !" << endl;
  
  ::close( cdromfd );
  return ret;
  
}

int K3bDevice::numSessions() {

  int ret=-1;
  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
    return ret;
  }

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
  if ( ::ioctl(cdromfd,CDROM_SEND_PACKET,&cmd) == 0 )
     ret = dat[3];
  else 
    kdDebug() << "(K3bDevice) could not get session info !" << endl;

  ::close( cdromfd );
  return ret;
}


bool K3bDevice::block( bool b) const
{
  bool ret = false;
  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
    return ret;
  }
  
  if ( ::ioctl(cdromfd,CDROM_LOCKDOOR, b ? 1 : 0 ) < 0 ) 
    kdDebug() << "(K3bDevice) Cannot block/unblock device " << devicename() << endl;
  else
    ret = true;

  ::close(cdromfd);
  return ret;   
}

bool K3bDevice::rewritable() {
  bool ret = false;
  if ( !burner() )  // no chance to detect empty discs in readers
    return false;
  if ( isReady() != 0 )
    return false;

  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
    return false;
  }
  
  struct cdrom_generic_command cmd;

  unsigned char inf[32];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(inf,0,32);
  cmd.cmd[0] = GPCMD_READ_DISC_INFO;
  cmd.cmd[8] = 32;
  cmd.buffer = inf;
  cmd.buflen = 32;
  cmd.data_direction = CGC_DATA_READ;
  if ( ::ioctl(cdromfd,CDROM_SEND_PACKET,&cmd) == 0 ) 
    ret = (inf[2] >> 4 ) & 0x01;
  else
    kdDebug() << "(K3bDevice) could not get disk info !" << endl;

  ::close( cdromfd );
  return ret;
}

void K3bDevice::eject() const
{
  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
  }
  else {
    ::ioctl( cdromfd, CDROMEJECT );
    ::close( cdromfd );
  }
}


void K3bDevice::load() const
{
  int cdromfd = ::open( devicename().ascii(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDevice) Error: could not open device." << endl;
  }
  else {
    ::ioctl( cdromfd, CDROMCLOSETRAY );
    ::close( cdromfd );
  }
}

void K3bDevice::addDeviceNode( const QString& n )
{
  if( !d->allNodes.contains( n ) )
    d->allNodes.append( n );
}


const QStringList& K3bDevice::deviceNodes() const
{
  return d->allNodes;
}


bool K3bDevice::supportsWriteMode( K3bDevice::WriteMode w )
{
  return (m_writeModes & w);
}
