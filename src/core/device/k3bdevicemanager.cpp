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


#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3bmmc.h"
#include "k3bscsicommand.h"
#include <k3bexternalbinmanager.h>
#include <k3bglobals.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>

#include <kprocess.h>
#include <kapplication.h>
#include <kconfig.h>

#include <iostream>
#include <fstab.h>
#include <limits.h>

#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

/* Fix definitions for 2.5 kernels */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,70)
typedef unsigned char u8;
#endif
#undef __STRICT_ANSI__
#include <asm/types.h>
#define __STRICT_ANSI__

#include <linux/../scsi/scsi.h> /* cope with silly includes */
#include <linux/major.h>




#ifndef SCSI_DISK_MAJOR
#define SCSI_DISK_MAJOR(M) ((M) == SCSI_DISK0_MAJOR || \
  ((M) >= SCSI_DISK1_MAJOR && (M) <= SCSI_DISK7_MAJOR) || \
  ((M) >= SCSI_DISK8_MAJOR && (M) <= SCSI_DISK15_MAJOR))
#endif /* #ifndef SCSI_DISK_MAJOR */

#ifndef SCSI_BLK_MAJOR
#define SCSI_BLK_MAJOR(M) \
  (SCSI_DISK_MAJOR(M)   \
   || (M) == SCSI_CDROM_MAJOR)
#endif /* #ifndef SCSI_BLK_MAJOR */

class K3bCdDevice::DeviceManager::Private
{
public:
  QPtrList<K3bDevice> allDevices;
  QPtrList<K3bDevice> cdReader;
  QPtrList<K3bDevice> cdWriter;
  QPtrList<K3bDevice> dvdReader;
  QPtrList<K3bDevice> dvdWriter;
};

K3bCdDevice::DeviceManager::DeviceManager( K3bExternalBinManager* externalBinManager,
					   QObject* parent, const char* name )
  : QObject( parent, name ),
    m_externalBinManager( externalBinManager )
{
  d = new Private;

  d->allDevices.setAutoDelete( true );
}


K3bCdDevice::DeviceManager::~DeviceManager()
{
  delete d;
}


K3bDevice* K3bCdDevice::DeviceManager::deviceByName( const QString& name )
{
  return findDevice( name );
}


K3bDevice* K3bCdDevice::DeviceManager::findDevice( int bus, int id, int lun )
{
  QPtrListIterator<K3bDevice> it( d->allDevices );
  while( it.current() )
  {
    if( it.current()->scsiBus() == bus &&
        it.current()->scsiId() == id &&
        it.current()->scsiLun() == lun )
      return it.current();

    ++it;
  }

  return 0;
}


K3bDevice* K3bCdDevice::DeviceManager::findDevice( const QString& devicename )
{
  if( devicename.isEmpty() )
  {
    kdDebug() << "(K3bDeviceManager) request for empty device!" << endl;
    return 0;
  }
  QPtrListIterator<K3bDevice> it( d->allDevices );
  while( it.current() )
  {
    if( it.current()->deviceNodes().contains(devicename) )
      return it.current();

    ++it;
  }

  return 0;
}


QPtrList<K3bDevice>& K3bCdDevice::DeviceManager::cdWriter()
{
  return d->cdWriter;
}

QPtrList<K3bDevice>& K3bCdDevice::DeviceManager::cdReader()
{
  return d->cdReader;
}

QPtrList<K3bDevice>& K3bCdDevice::DeviceManager::dvdWriter()
{
  return d->dvdWriter;
}

QPtrList<K3bDevice>& K3bCdDevice::DeviceManager::dvdReader()
{
  return d->dvdReader;
}


QPtrList<K3bDevice>& K3bCdDevice::DeviceManager::burningDevices()
{
  return cdWriter();
}


QPtrList<K3bDevice>& K3bCdDevice::DeviceManager::readingDevices()
{
  return cdReader();
}


QPtrList<K3bDevice>& K3bCdDevice::DeviceManager::allDevices()
{
  return d->allDevices;
}


int K3bCdDevice::DeviceManager::scanbus()
{
  m_foundDevices = 0;

  QFile info("/proc/sys/dev/cdrom/info");
  QString line,devstring;
  info.open(IO_ReadOnly);
  info.readLine(line,80); // CD-ROM information, Id: cdrom.c 3.12 2000/10/18
  info.readLine(line,80); //

  while (info.readLine(line,80) > 0)
  {
    if (line.contains("drive name") > 0)
    {
      int i = 1;
      QString dev;
      QRegExp re("[\t\n:]+");
      while ( !(dev = line.section(re, i, i)).isEmpty() )
      {
        if( addDevice(QString("/dev/%1").arg(dev)) ) {
          devstring += dev + "|";
          m_foundDevices++;
        }
// according to the LINUX ALLOCATED DEVICES document (http://www.lanana.org/docs/device-list/),
// the official device names for SCSI-CDROM's (block major 11) are /dev/sr*, the
// prefix /dev/scd instead of /dev/sr has been used as well, and might make more sense.
// Since there should be one and only one device node (and maybe several symbolic links) for
// each physical device the next line should be better
//      else if ( dev.startsWith("sr") )
        if ( dev.startsWith("sr") )
          if( addDevice(QString("/dev/%1").arg(dev.replace(QRegExp("r"),"cd"))) ) {
            devstring += dev + "|";
            m_foundDevices++;
        }
        ++i;
      }
    }
    break;
  }
  info.close();

  // try to find symlinks
  QString cmd = QString("find /dev -type l -printf \"%p\t%l\n\" | egrep '%1cdrom|dvd|cdwriter|cdrecorder' | cut -f1").arg(devstring);
  FILE *fd = popen(QFile::encodeName(cmd),"r");
  if (fd) {
     QFile links;
     QString device;
     links.open(IO_ReadOnly,fd);
     while ( links.readLine(device,80) > 0) {
       device = device.stripWhiteSpace();
       K3bDevice *d = findDevice(resolveSymLink(device));
       if (d) {
         d->addDeviceNode(device);
         kdDebug() << "(K3bDeviceManager) Link: " << device << " -> " << d->devicename() << endl;
       }
     }
  }
  pclose(fd);

  // we also check all these nodes to make sure to get all links and stuff

//   static const char* devicenames[] = {
//     "/dev/hda",
//     "/dev/hdb",
//     "/dev/hdc",
//     "/dev/hdd",
//     "/dev/hde",
//     "/dev/hdf",
//     "/dev/hdg",
//     "/dev/hdh",
//     "/dev/hdi",
//     "/dev/hdj",
//     "/dev/hdk",
//     "/dev/hdl",
//     "/dev/dvd",
//     "/dev/cdrom",
//     "/dev/cdrecorder",
//     0
//   };
//   int i = 0;
//   while( devicenames[i] ) {
//     if( addDevice( devicenames[i] ) )
//       m_foundDevices++;
//     ++i;
//   }
//   for( int i = 0; i < 16; i++ ) {
//     if( addDevice( QString("/dev/scd%1").arg(i).ascii() ) )
//       m_foundDevices++;
//   }
//   for( int i = 0; i < 16; i++ ) {
//     if( addDevice( QString("/dev/sr%1").arg(i).ascii() ) )
//       m_foundDevices++;
//   }

  scanFstab();

  return m_foundDevices;
}


void K3bCdDevice::DeviceManager::printDevices()
{
  kdDebug() << "Devices:" << endl
	    << "------------------------------" << endl;
  QPtrListIterator<CdDevice> it( allDevices() );
  for( ; *it; ++it ) {
    CdDevice* dev = *it;
    kdDebug() << "Blockdevice:    " << dev->blockDeviceName() << endl
	      << "Vendor:         " << dev->vendor() << endl
	      << "Description:    " << dev->description() << endl
	      << "Version:        " << dev->version() << endl
	      << "MountDevice:    " << dev->mountDevice() << endl
	      << "Mountpoint:     " << dev->mountPoint() << endl
	      << "Write speed:    " << dev->maxWriteSpeed() << endl
	      << "Profiles:       " << mediaTypeString( dev->supportedProfiles() ) << endl
	      << "Devicetype:     " << deviceTypeString( dev->type() ) << endl
	      << "Writing modes:  " << writingModeString( dev->writingModes() ) << endl
	      << "Reader aliases: " << dev->deviceNodes().join(", ") << endl
	      << "------------------------------" << endl;
  }
}


void K3bCdDevice::DeviceManager::clear()
{
  // clear current devices
  d->cdReader.clear();
  d->cdWriter.clear();
  d->dvdReader.clear();
  d->dvdWriter.clear();
  d->allDevices.clear();
}


bool K3bCdDevice::DeviceManager::readConfig( KConfig* c )
{
  m_foundDevices = 0;

  if( !c->hasGroup( "Devices" ) )
  {
    return false;
  }

  c->setGroup( "Devices" );

  int devNum = 1;
  QStringList list = c->readListEntry( "Device1" );
  devNum = 1;
  while( !list.isEmpty() )
  {
    K3bDevice *dev;
    dev = deviceByName( list[0] );

    if( dev == 0 )
      dev = addDevice( list[0] );

    if( dev != 0 )
    {
      // device found, apply changes
      if( list.count() > 1 )
        dev->setMaxReadSpeed( list[1].toInt() );
      if( list.count() > 2 )
        dev->setMaxWriteSpeed( list[2].toInt() );
      if( list.count() > 3 )
        dev->setCdrdaoDriver( list[3] );
      if( list.count() > 4 )
        dev->setCdTextCapability( list[4] == "yes" );
      if( list.count() > 5 )
        dev->setBurnproof( list[5] == "yes" );
      if( list.count() > 6 )
        dev->setBufferSize( list[6].toInt() );
      if( list.count() > 7 )
        dev->setCurrentWriteSpeed( list[7].toInt() );
    }

    if( dev == 0 )
      kdDebug() << "(K3bDeviceManager) Could not detect saved device " << list[0] << "." << endl;

    devNum++;
    list = c->readListEntry( QString( "Device%1" ).arg( devNum ) );
  }

  scanFstab();

  return true;
}


bool K3bCdDevice::DeviceManager::saveConfig( KConfig* c )
{
  //////////////////////////////////
  // Clear config
  /////////////////////////////////

  if( c->hasGroup( "Devices" ) )
  {
    // remove all old device entries
    c->deleteGroup("Devices");
  }


  c->setGroup( "Devices" );

  int i = 1;
  CdDevice* dev = d->allDevices.first();
  while( dev != 0 )
  {
    QStringList list;
    list << dev->blockDeviceName()
    << QString::number(dev->maxReadSpeed())
    << QString::number(dev->maxWriteSpeed())
    << dev->cdrdaoDriver();

    if( dev->cdrdaoDriver() != "auto" )
      list << ( dev->cdTextCapable() == 1 ? "yes" : "no" );
    else
      list << "auto";

    list 
      << ( dev->burnproof() ? "yes" : "no" )
      << QString::number( dev->bufferSize() )
      << QString::number( dev->currentWriteSpeed() );

    c->writeEntry( QString("Device%1").arg(i), list );

    i++;
    dev = d->allDevices.next();
  }

  c->sync();

  return true;
}

void K3bCdDevice::DeviceManager::determineCapabilities(K3bDevice *dev)
{
  // we just need to do this for writers since we use it to determine the writing modes
  if( !dev->burner() )
    return;


  // we do not use the user configured cdrecord here since we want to make sure
  // to get all the capabilities of the system

  const K3bExternalBin* cdrecordBin = m_externalBinManager->mostRecentBinObject( "cdrecord" );
  if( !cdrecordBin ) {
    kdError() << "(K3bDeviceManager) Could not find cdrecord. No proper device initialization possible." << endl;
    return;
  }

  if (dev->interfaceType() == K3bDevice::IDE ) {
    // only if the kernel and cdrecord support atapi
    if( !(plainAtapiSupport() && cdrecordBin->hasFeature("plain-atapi") ) &&
	!(hackedAtapiSupport() && cdrecordBin->hasFeature("hacked-atapi")) ) {
      kdError() << "(K3bDeviceManager) no ATAPI support." << endl;
      return;
    }
  }


  kdDebug() << "(K3bDeviceManager) probing capabilities for device " << dev->blockDeviceName() << endl;

  //
  // This is just a temp solution for those drives that do not support the GET CONFIGURATION command
  // and thus do not give us their supported features
  //
  if( dev->m_writeModes == 0 ) {

    kdDebug() << "(K3bDeviceManager) calling cdrecord to determine supported writemodes for "
	      << dev->blockDeviceName() << endl;

    KProcess driverProc, capProc;
    driverProc << cdrecordBin->path;

    driverProc << QString("dev=%1").arg(externalBinDeviceParameter(dev, cdrecordBin));

    driverProc << "-checkdrive";
    connect( &driverProc, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotCollectStdout(KProcess*, char*, int)) );

    m_processOutput = "";

    driverProc.start( KProcess::Block, KProcess::Stdout );
    // this should work for all drives
    // so we are always able to say if a drive is a writer or not
    if( driverProc.exitStatus() == 0 )
      {
	dev->m_burner = true;
	dev->m_writeModes = 0;
	QStringList lines = QStringList::split( "\n", m_processOutput );
	for( QStringList::const_iterator it = lines.begin(); it != lines.end(); ++it )
	  {
	    const QString& line = *it;

	    // no info in cdrecord <= 1.10 !!!!!
	    if( line.startsWith( "Supported modes" ) )
	      {
		QStringList modes = QStringList::split( " ", line.mid(16) );
		if( modes.contains( "SAO" ) )
		  dev->m_writeModes |= K3bDevice::SAO;
		if( modes.contains( "TAO" ) )
		  dev->m_writeModes |= K3bDevice::TAO;
		if( modes.contains( "PACKET" ) )
		  dev->m_writeModes |= K3bDevice::PACKET;
		if( modes.contains( "SAO/R96R" ) )
		  dev->m_writeModes |= K3bDevice::SAO_R96R;
		if( modes.contains( "SAO/R96P" ) )
		  dev->m_writeModes |= K3bDevice::SAO_R96P;
		if( modes.contains( "RAW/R16" ) )
		  dev->m_writeModes |= K3bDevice::RAW_R16;
		if( modes.contains( "RAW/R96R" ) )
		  dev->m_writeModes |= K3bDevice::RAW_R96R;
		if( modes.contains( "RAW/R96P" ) )
		  dev->m_writeModes |= K3bDevice::RAW_R96P;
		break;
	      }
	  }
      }

    // default to dao and tao if no write modes info was available (cdrecord <= 1.10)
    // I include this hack because I think it's better to get an error:
    //   "mode not supported" when trying to write instead of never getting to choose DAO!
    if( dev->m_writeModes == 0 )
      dev->m_writeModes = K3bDevice::SAO|K3bDevice::TAO;
  }
}


bool K3bCdDevice::DeviceManager::testForCdrom(const QString& devicename)
{
  bool ret = false;
  int cdromfd = K3bCdDevice::openDevice( devicename.ascii() );
  if (cdromfd < 0) {
    kdDebug() << "could not open device " << devicename << " (" << strerror(errno) << ")" << endl;
    return ret;
  }

  // stat the device
  struct stat cdromStat;
  ::fstat( cdromfd, &cdromStat );

  if( !S_ISBLK( cdromStat.st_mode) ) {
    kdDebug() << devicename << " is no block device" << endl;
  }
  else {
    kdDebug() << devicename << " is block device (" << (int)(cdromStat.st_rdev & 0xFF) << ")" << endl;

    // inquiry
    // use a 36 bytes buffer since not all devices return the full inquiry struct
    unsigned char buf[36];
    struct inquiry* inq = (struct inquiry*)buf;
    ::memset( buf, 0, sizeof(buf) );

    ScsiCommand cmd( cdromfd );
    cmd[0] = 0x12;  // GPCMD_INQUIRY
    cmd[4] = sizeof(buf);
    cmd[5] = 0;

    if( cmd.transport( TR_DIR_READ, buf, sizeof(buf) ) ) {
      kdError() << "(K3bCdDevice) Unable to do inquiry." << endl;
    }
    else if( (inq->p_device_type&0x1f) != 0x5 ) {
      kdDebug() << devicename << " seems not to be a cdrom device: " << strerror(errno) << endl;
    }
    else {
      ret = true;
      kdDebug() << devicename << " seems to be cdrom" << endl;
    }
  }

  ::close( cdromfd );
  return ret;
}

K3bDevice* K3bCdDevice::DeviceManager::addDevice( const QString& devicename )
{
  K3bDevice* device = 0;

  // resolve all symlinks
  QString resolved = resolveSymLink( devicename );
  kdDebug() << devicename << " resolved to " << resolved << endl;

  if  ( !testForCdrom(resolved) )
    return 0;

  if ( K3bDevice* oldDev = findDevice(resolved) ) {
    kdDebug() << "(K3bDeviceManager) dev " << resolved  << " already found" << endl;
    oldDev->addDeviceNode( devicename );
    return 0;
  }

  int bus = -1, target = -1, lun = -1;
  bool scsi = determineBusIdLun( resolved, bus, target, lun );
  if(scsi) {
    if ( K3bDevice* oldDev = findDevice(bus, target, lun) ) {
      kdDebug() << "(K3bDeviceManager) dev " << resolved  << " already found" << endl;
      oldDev->addDeviceNode( devicename );
      return 0;
    }
  }

  device = new K3bDevice(resolved);
  if( scsi ) {
    device->m_bus = bus;
    device->m_target = target;
    device->m_lun = lun;
  }

  if( !device->init() ) {
    kdDebug() << "Could not initialize device " << devicename << endl;
    delete device;
    return 0;
  }

  if( device ) {
    determineCapabilities(device);

    d->allDevices.append( device );

    // not every drive is able to read CDs
    // there are some 1st generation DVD writer that cannot
    if( device->type() & CdDevice::CDROM )
      d->cdReader.append( device );
    if( device->readsDvd() )
      d->dvdReader.append( device );
    if( device->writesCd() )
      d->cdWriter.append( device );
    if( device->writesDvd() )
      d->dvdWriter.append( device );

    if( device->writesCd() ) {
      // default to max write speed
      kdDebug() << "(K3bDeviceManager) setting current write speed of device " 
		<< device->blockDeviceName() 
		<< " to " << device->maxWriteSpeed() << endl;
      device->setCurrentWriteSpeed( device->maxWriteSpeed() );
    }
  }

  return device;
}


void K3bCdDevice::DeviceManager::scanFstab()
{
  ::setfsent();

  // clear all mount-Infos
  for( QPtrListIterator<K3bDevice> it( d->allDevices ); it.current(); ++it )
  {
    it.current()->setMountPoint( QString::null );
    it.current()->setMountDevice( QString::null );
  }


  struct fstab * mountInfo = 0;
  while( (mountInfo = ::getfsent()) )
  {
    // check if the entry corresponds to a device
    QString md = QFile::decodeName( mountInfo->fs_spec );
    QString type = QFile::decodeName( mountInfo->fs_vfstype );

    bool supermount = false;

    if( type == "supermount" ) {
      supermount = true;

      // parse the device
      QStringList opts = QStringList::split( ",", QString::fromLocal8Bit(mountInfo->fs_mntops) );
      for( QStringList::const_iterator it = opts.begin(); it != opts.end(); ++it ) {
	if( (*it).startsWith("dev=") ) {
	  md = (*it).mid( 4 );
	  break;
	}
      }
    }

    if( md == "none" )
      continue;

    kdDebug() << "(K3bDeviceManager) scanning fstab: " << md << endl;

    if( K3bDevice* dev = findDevice( resolveSymLink(md) ) )
    {
      kdDebug() << "(K3bDeviceManager) found device for " << md << ": " << resolveSymLink(md) << endl;
      if( dev->mountDevice().isEmpty() ) {
        dev->setMountPoint( mountInfo->fs_file );
        dev->setMountDevice( md );
	dev->m_supermount = supermount;
      }
    }
    else
    {
      // compare bus, id, lun since the same device can for example be
      // determined as /dev/srX or /dev/scdX
      int bus = -1, id = -1, lun = -1;
      if( determineBusIdLun( mountInfo->fs_spec, bus, id, lun ) ) {
        if( K3bDevice* dev = findDevice( bus, id, lun ) ) {
          if( dev->mountDevice().isEmpty() ) {
            dev->setMountPoint( mountInfo->fs_file );
            dev->setMountDevice( md );
	    dev->m_supermount = supermount;
          }
        }
      }


    }
  } // while mountInfo

  ::endfsent();
}


void K3bCdDevice::DeviceManager::slotCollectStdout( KProcess*, char* data, int len )
{
  m_processOutput += QString::fromLocal8Bit( data, len );
}


bool K3bCdDevice::DeviceManager::determineBusIdLun( const QString& dev, int& bus, int& id, int& lun )
{
  int ret = false;
  int cdromfd = K3bCdDevice::openDevice( dev.ascii() );
  if (cdromfd < 0) {
    kdDebug() << "could not open device " << dev << " (" << strerror(errno) << ")" << endl;
    return false;
  }

  struct stat cdromStat;
  ::fstat( cdromfd, &cdromStat );

  if( SCSI_BLK_MAJOR( cdromStat.st_rdev>>8 ) )
  {
    struct ScsiIdLun
    {
      int id;
      int lun;
    };
    ScsiIdLun idLun;

    // in kernel 2.2 SCSI_IOCTL_GET_IDLUN does not contain the bus id
    if ( (::ioctl( cdromfd, SCSI_IOCTL_GET_IDLUN, &idLun ) < 0) ||
         (::ioctl( cdromfd, SCSI_IOCTL_GET_BUS_NUMBER, &bus ) < 0) )
    {
      kdDebug() << "Need a filename that resolves to a SCSI device" << endl;
      ret = false;
    }
    else
    {
      id  = idLun.id & 0xff;
      lun = (idLun.id >> 8) & 0xff;
      kdDebug() << "bus: " << bus << ", id: " << id << ", lun: " << lun << endl;
      ret = true;
    }
  }


  ::close(cdromfd);
  return ret;
}


QString K3bCdDevice::DeviceManager::resolveSymLink( const QString& path )
{
  char resolved[PATH_MAX];
  if( !realpath( QFile::encodeName(path), resolved ) )
  {
    kdDebug() << "Could not resolve " << path << endl;
    return path;
  }

  return QString::fromLatin1( resolved );
}


bool K3bCdDevice::plainAtapiSupport()
{
  // IMPROVEME!!!
  return ( K3b::kernelVersion() >= K3bVersion( 2, 5, 40 ) );
}


bool K3bCdDevice::hackedAtapiSupport()
{
  // IMPROVEME!!!
  // FIXME: since when does the kernel support this?
  return ( K3b::kernelVersion() >= K3bVersion( 2, 4, 0 ) );
}


QString K3bCdDevice::externalBinDeviceParameter( K3bDevice* dev, const K3bExternalBin* bin )
{
  if( dev->interfaceType() == K3bDevice::SCSI )
    return dev->busTargetLun();
  else if( (plainAtapiSupport() && bin->hasFeature("plain-atapi") ) )
    return dev->blockDeviceName();
  else
    return QString("ATAPI:%1").arg(dev->blockDeviceName());
}

#include "k3bdevicemanager.moc"
