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

#include <config.h>

#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3bscsicommand.h"
#include "k3bmmc.h"

#ifdef HAVE_HAL
#include "k3bhalconnection.h"
#endif

#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>

#include <kprocess.h>
#include <kapplication.h>
#include <kconfig.h>
#include <ktempfile.h>

#include <iostream>
#include <fstab.h>
#include <limits.h>
#include <assert.h>

#ifdef Q_OS_FREEBSD
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <osreldate.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>


#ifdef Q_OS_LINUX

/* Fix definitions for 2.5 kernels */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,70)
typedef unsigned char u8;
#endif

#undef __STRICT_ANSI__
#include <asm/types.h>
#define __STRICT_ANSI__

#include <scsi/scsi.h>
#include <linux/major.h>


#ifndef SCSI_DISK_MAJOR
#define SCSI_DISK_MAJOR(M) ((M) == SCSI_DISK0_MAJOR || \
  ((M) >= SCSI_DISK1_MAJOR && (M) <= SCSI_DISK7_MAJOR) || \
  ((M) >= SCSI_DISK8_MAJOR && (M) <= SCSI_DISK15_MAJOR))
#endif

#ifndef SCSI_BLK_MAJOR
#define SCSI_BLK_MAJOR(M) \
  (SCSI_DISK_MAJOR(M)   \
   || (M) == SCSI_CDROM_MAJOR)
#endif

#ifndef SCSI_GENERIC_MAJOR
#define SCSI_GENERIC_MAJOR 21
#endif

#endif // Q_OS_LINUX


#ifdef Q_OS_FREEBSD
#include <cam/cam.h>
#include <cam/scsi/scsi_pass.h>
#include <camlib.h>
#endif



class K3bDevice::DeviceManager::Private
{
public:
  QPtrList<K3bDevice::Device> allDevices;
  QPtrList<K3bDevice::Device> cdReader;
  QPtrList<K3bDevice::Device> cdWriter;
  QPtrList<K3bDevice::Device> dvdReader;
  QPtrList<K3bDevice::Device> dvdWriter;

  bool checkWritingModes;

#ifdef HAVE_HAL
  HalConnection hal;
#endif
};



K3bDevice::DeviceManager::DeviceManager( QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private;

#ifdef HAVE_HAL
    connect( &d->hal, SIGNAL(deviceAdded(const QString&)),
	     this, SLOT(addDevice(const QString&)) );
    connect( &d->hal, SIGNAL(deviceRemoved(const QString&)),
	     this, SLOT(removeDevice(const QString&)) );
#endif
}


K3bDevice::DeviceManager::~DeviceManager()
{
  d->allDevices.setAutoDelete( true );
  delete d;
}


void K3bDevice::DeviceManager::setCheckWritingModes( bool b )
{
  d->checkWritingModes = b;
}


K3bDevice::Device* K3bDevice::DeviceManager::deviceByName( const QString& name )
{
  return findDevice( name );
}


K3bDevice::Device* K3bDevice::DeviceManager::findDevice( int bus, int id, int lun )
{
  QPtrListIterator<K3bDevice::Device> it( d->allDevices );
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


K3bDevice::Device* K3bDevice::DeviceManager::findDevice( const QString& devicename )
{
  if( devicename.isEmpty() ) {
    kdDebug() << "(K3bDevice::DeviceManager) request for empty device!" << endl;
    return 0;
  }
  QPtrListIterator<K3bDevice::Device> it( d->allDevices );
  while( it.current() )
  {
    if( it.current()->deviceNodes().contains(devicename) )
      return it.current();

    ++it;
  }

  return 0;
}


QPtrList<K3bDevice::Device>& K3bDevice::DeviceManager::cdWriter() const
{
  return d->cdWriter;
}

QPtrList<K3bDevice::Device>& K3bDevice::DeviceManager::cdReader() const
{
  return d->cdReader;
}

QPtrList<K3bDevice::Device>& K3bDevice::DeviceManager::dvdWriter() const
{
  return d->dvdWriter;
}

QPtrList<K3bDevice::Device>& K3bDevice::DeviceManager::dvdReader() const
{
  return d->dvdReader;
}


QPtrList<K3bDevice::Device>& K3bDevice::DeviceManager::burningDevices() const
{
  return cdWriter();
}


QPtrList<K3bDevice::Device>& K3bDevice::DeviceManager::readingDevices() const
{
  return cdReader();
}


QPtrList<K3bDevice::Device>& K3bDevice::DeviceManager::allDevices() const
{
  return d->allDevices;
}


int K3bDevice::DeviceManager::scanBus()
{
  m_foundDevices = 0;

#ifdef HAVE_HAL
  d->hal.open();
#endif

#ifdef Q_OS_LINUX
  LinuxDeviceScan();
#endif
#ifdef Q_OS_FREEBSD
  BSDDeviceScan();
#endif
  scanFstab();

  return m_foundDevices;
}


void K3bDevice::DeviceManager::LinuxDeviceScan()
{
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

#ifdef HAVE_RESMGR
  int havenormaldevs = d->allDevices.count();
#endif

  // try to find symlinks
  QString cmd = QString("find /dev -type l -printf \"%p\t%l\n\" | egrep '%1cdrom|dvd|cdwriter|cdrecorder' | cut -f1").arg(devstring);
  FILE *fd = popen(QFile::encodeName(cmd),"r");
  if (fd) {
     QFile links;
     QString device;
     links.open(IO_ReadOnly,fd);
     while ( links.readLine(device,80) > 0) {
       device = device.stripWhiteSpace();
       K3bDevice::Device *d = findDevice(resolveSymLink(device));
       if (d) {
         d->addDeviceNode(device);
         kdDebug() << "(K3bDevice::DeviceManager) Link: " << device << " -> " << d->devicename() << endl;
       }
#ifdef HAVE_RESMGR
       // if we had no initial devices, add the symlinked devices.
       if (!d) {
         if (!havenormaldevs)
           addDevice(device);
       }
#endif
     }
  }
  pclose(fd);


#ifdef Q_OS_LINUX
  //
  // Scan the generic devices if we have scsi devices
  //
  kdDebug() << "(K3bDevice::DeviceManager) SCANNING FOR GENERIC DEVICES." << endl;
  for( int i = 0; i < 16; i++ ) {
    QString sgDev = resolveSymLink( QString("/dev/sg%1").arg(i) );
    int bus, id, lun;
    if( determineBusIdLun( sgDev, bus, id, lun ) ) {
      if( Device* dev = findDevice( bus, id, lun ) ) {
	dev->m_genericDevice = sgDev;
      }
    }
  }
  // FIXME: also scan /dev/scsi/hostX.... for devfs without symlinks
#endif
}


void K3bDevice::DeviceManager::BSDDeviceScan()
{
  // Unfortunately uses lots of FBSD-specific data structures
#ifndef Q_OS_FREEBSD
  bool bsdspecificcode = false;
  assert(bsdspecificcode);
#endif

#ifdef Q_OS_FREEBSD
  union ccb ccb;
  int fd;
  int need_close = 0;
  int skip_device = 0;
  int bus, target, lun;
  QString dev1, dev2;

  if ((fd = open(XPT_DEVICE, O_RDWR)) == -1)
    {
      kdDebug() << "couldn't open %s " << XPT_DEVICE << endl;
      return;
    }

  memset(&ccb, 0, sizeof(ccb));

  ccb.ccb_h.func_code = XPT_DEV_MATCH;
  char buffer[100*sizeof(struct dev_match_result)];
  ccb.cdm.match_buf_len = 100*sizeof(struct dev_match_result);
  ccb.cdm.matches = (struct dev_match_result *)buffer;
  ccb.cdm.num_matches = 0;
  ccb.cdm.num_patterns = 0;
  ccb.cdm.pattern_buf_len = 0;
  do {
    if (ioctl(fd, CAMIOCOMMAND, &ccb) == -1) {
      kdDebug() << "(BSDDeviceScan) error sending CAMIOCOMMAND ioctl: " << errno << endl;
      break;
    }

    if ((ccb.ccb_h.status != CAM_REQ_CMP)
	|| ((ccb.cdm.status != CAM_DEV_MATCH_LAST) && (ccb.cdm.status != CAM_DEV_MATCH_MORE))) {
      kdDebug() << "(BSDDeviceScan) got CAM error " << ccb.ccb_h.status << ", CDM error %d" << ccb.cdm.status << endl;
      break;
    }
    kdDebug() << "(BSDDeviceScan) number of matches " << (int)ccb.cdm.num_matches << endl;
    for (int i = 0; i < (int)ccb.cdm.num_matches; i++) {
      switch (ccb.cdm.matches[i].type) {
      case DEV_MATCH_DEVICE: {
	struct device_match_result *dev_result = &ccb.cdm.matches[i].result.device_result;

	if (dev_result->flags & DEV_RESULT_UNCONFIGURED)
	  {
	    skip_device = 1;
	    break;
	  }
	else
	  skip_device = 0;
	if (need_close)
	  {
	    QString pass = dev1;
	    QString dev = "/dev/" + dev2;
	    if (dev2.startsWith("pass"))
	      {
		pass = dev2;
		dev = "/dev/" + dev1;
	      }
#if __FreeBSD_version < 500100
	    dev += "c";
#endif
	    if (dev1 != "" && dev2 != "" && dev.startsWith("/dev/cd"))
	    {
	      Device* device = new Device(dev.latin1());
	      device->m_bus = bus;
	      device->m_target = target;
	      device->m_lun = lun;
	      device->m_passDevice = "/dev/" + pass;
	      kdDebug() << "(BSDDeviceScan) add device " << dev << ":" << bus << ":" << target << ":" << lun << endl;
	      addDevice(device);
	    }
	    need_close = 0;
	    dev1="";
	    dev2="";
	  }
	bus = dev_result->path_id;
	target = dev_result->target_id;
	lun = dev_result->target_lun;

	need_close = 1;

	break;
      }
      case DEV_MATCH_PERIPH: {
	struct periph_match_result *periph_result = &ccb.cdm.matches[i].result.periph_result;

	if (skip_device != 0)
	  break;

	if (need_close > 1)
	  dev1 = periph_result->periph_name + QString::number(periph_result->unit_number);
	else
	  dev2 = periph_result->periph_name + QString::number(periph_result->unit_number);

	need_close++;
	break;
      }
      case DEV_MATCH_BUS : {
	// bool cannotmatchbus = false;
	// assert(cannotmatchbus);
	break;
      }
      }
    }

  } while ((ccb.ccb_h.status == CAM_REQ_CMP)
	   && (ccb.cdm.status == CAM_DEV_MATCH_MORE));

  if (need_close)
    {
      QString pass = dev1;
      QString dev = "/dev/" + dev2;
      if (dev2.startsWith("pass"))
	{
	  pass = dev2;
	  dev = "/dev/" + dev1;
	}
#if __FreeBSD_version < 500100
      dev += "c";
#endif
      if (dev1 != "" && dev2 != "" && dev.startsWith("/dev/cd"))
      {
        Device* device = new Device(dev.latin1());
        device->m_bus = bus;
        device->m_target = target;
        device->m_lun = lun;
        device->m_passDevice = "/dev/" + pass;
        kdDebug() << "(BSDDeviceScan) add device " << dev << ":" << bus << ":" << target << ":" << lun << endl;
        addDevice(device);
      }
    }
  close(fd);
#endif
}


void K3bDevice::DeviceManager::printDevices()
{
  kdDebug() << "Devices:" << endl
	    << "------------------------------" << endl;
  QPtrListIterator<Device> it( allDevices() );
  for( ; *it; ++it ) {
    Device* dev = *it;
    kdDebug() << "Blockdevice:    " << dev->blockDeviceName() << endl
	      << "Generic device: " << dev->genericDevice() << endl
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


void K3bDevice::DeviceManager::clear()
{
  // clear current devices
  d->cdReader.clear();
  d->cdWriter.clear();
  d->dvdReader.clear();
  d->dvdWriter.clear();
  d->allDevices.clear();

  emit changed();
  emit changed( this );
}


bool K3bDevice::DeviceManager::readConfig( KConfig* c )
{
  //
  // New configuration format since K3b 0.11.94
  // for details see saveConfig()
  //

  m_foundDevices = 0;

  if( !c->hasGroup( "Devices" ) )
    return false;

  c->setGroup( "Devices" );

  QStringList deviceSearchPath = c->readListEntry( "device_search_path" );
  for( QStringList::const_iterator it = deviceSearchPath.constBegin();
       it != deviceSearchPath.constEnd(); ++it )
    addDevice( *it );

  //
  // Iterate over all devices and check if we have a config entry
  //
  for( QPtrListIterator<K3bDevice::Device> it( d->allDevices ); *it; ++it ) {
    K3bDevice::Device* dev = *it;

    QString configEntryName = dev->vendor() + " " + dev->description();
    QStringList list = c->readListEntry( configEntryName );
    if( !list.isEmpty() ) {
      kdDebug() << "(K3bDevice::DeviceManager) found config entry for devicetype: " << configEntryName << endl;

      dev->setMaxReadSpeed( list[0].toInt() );
      if( list.count() > 1 )
	dev->setMaxWriteSpeed( list[1].toInt() );
      if( list.count() > 2 )
	dev->setCdrdaoDriver( list[2] );
      if( list.count() > 3 )
	dev->setCdTextCapability( list[3] == "yes" );
    }
  }

  scanFstab();

  return true;
}


bool K3bDevice::DeviceManager::saveConfig( KConfig* c )
{
  //
  // New configuration format since K3b 0.11.94
  //
  // We save a device search path which contains all device nodes
  // where devices could be found including the old search path.
  // This way also for example a manually added USB device will be
  // found between sessions.
  // Then we do not save the device settings (writing speed, cdrdao driver)
  // for every single device but for every device type.
  // This also makes sure device settings are kept between sessions
  //

  c->setGroup( "Devices" );
  QStringList deviceSearchPath = c->readListEntry( "device_search_path" );
  // remove duplicate entries (caused by buggy old implementations)
  QStringList saveDeviceSearchPath;
  for( QStringList::const_iterator it = deviceSearchPath.constBegin(); it != deviceSearchPath.constEnd(); ++it )
    if( !saveDeviceSearchPath.contains( *it ) )
      saveDeviceSearchPath.append( *it );

  for( QPtrListIterator<K3bDevice::Device> it( d->allDevices ); *it; ++it ) {
    K3bDevice::Device* dev = *it;

    // update device search path
    if( !saveDeviceSearchPath.contains( dev->blockDeviceName() ) )
      saveDeviceSearchPath.append( dev->blockDeviceName() );

    // save the device type settings
    QString configEntryName = dev->vendor() + " " + dev->description();
    QStringList list;
    list << QString::number(dev->maxReadSpeed())
	 << QString::number(dev->maxWriteSpeed())
	 << dev->cdrdaoDriver();

    if( dev->cdrdaoDriver() != "auto" )
      list << ( dev->cdTextCapable() == 1 ? "yes" : "no" );
    else
      list << "auto";

    c->writeEntry( configEntryName, list );
  }

  c->writeEntry( "device_search_path", saveDeviceSearchPath );

  c->sync();

  return true;
}


bool K3bDevice::DeviceManager::testForCdrom(const QString& devicename)
{
#ifdef Q_OS_FREEBSD
  Q_UNUSED(devicename);
  return true;
#endif
#ifdef Q_OS_LINUX
  bool ret = false;
  int cdromfd = K3bDevice::openDevice( devicename.ascii() );
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
    cmd[0] = MMC_INQUIRY;
    cmd[4] = sizeof(buf);
    cmd[5] = 0;

    if( cmd.transport( TR_DIR_READ, buf, sizeof(buf) ) ) {
      kdError() << "(K3bDevice::Device) Unable to do inquiry." << endl;
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
#endif
}

K3bDevice::Device* K3bDevice::DeviceManager::addDevice( const QString& devicename )
{
#ifdef Q_OS_FREEBSD
  return 0;
#endif

  K3bDevice::Device* device = 0;

  // resolve all symlinks
  QString resolved = resolveSymLink( devicename );
  kdDebug() << devicename << " resolved to " << resolved << endl;

  if( !testForCdrom(resolved) ) {
#ifdef HAVE_RESMGR
    // With resmgr we might only be able to open the symlink name.
    if( testForCdrom(devicename) ) {
      resolved = devicename;
    } 
    else {
      return 0;
    }
#else
    return 0;
#endif
  }

  if ( K3bDevice::Device* oldDev = findDevice(resolved) ) {
    kdDebug() << "(K3bDevice::DeviceManager) dev " << resolved  << " already found" << endl;
    oldDev->addDeviceNode( devicename );
    return 0;
  }

  int bus = -1, target = -1, lun = -1;
  bool scsi = determineBusIdLun( resolved, bus, target, lun );
  if(scsi) {
    if ( K3bDevice::Device* oldDev = findDevice(bus, target, lun) ) {
      kdDebug() << "(K3bDevice::DeviceManager) dev " << resolved  << " already found" << endl;
      oldDev->addDeviceNode( devicename );
      return 0;
    }
  }

  device = new K3bDevice::Device(resolved);
  if( scsi ) {
    device->m_bus = bus;
    device->m_target = target;
    device->m_lun = lun;
  }

  return addDevice(device);
}


K3bDevice::Device* K3bDevice::DeviceManager::addDevice( K3bDevice::Device* device )
{
  const QString devicename = device->devicename();

  if( !device->init() ) {
    kdDebug() << "Could not initialize device " << devicename << endl;
    delete device;
    return 0;
  }

  if( device ) {
    d->allDevices.append( device );

    // not every drive is able to read CDs
    // there are some 1st generation DVD writer that cannot
    if( device->type() & K3bDevice::DEVICE_CD_ROM )
      d->cdReader.append( device );
    if( device->readsDvd() )
      d->dvdReader.append( device );
    if( device->writesCd() )
      d->cdWriter.append( device );
    if( device->writesDvd() )
      d->dvdWriter.append( device );

    if( device->writesCd() ) {
      // default to max write speed
      kdDebug() << "(K3bDevice::DeviceManager) setting current write speed of device "
		<< device->blockDeviceName()
		<< " to " << device->maxWriteSpeed() << endl;
      device->setCurrentWriteSpeed( device->maxWriteSpeed() );
    }

    emit changed();
    emit changed( this );
  }

  return device;
}


void K3bDevice::DeviceManager::removeDevice( const QString& dev )
{
  if( Device* device = findDevice( dev ) ) {
    d->cdReader.removeRef( device );
    d->dvdReader.removeRef( device );
    d->cdWriter.removeRef( device );
    d->dvdWriter.removeRef( device );
    d->allDevices.removeRef( device );

    emit changed();
    emit changed( this );

    delete device;
  }
}


void K3bDevice::DeviceManager::scanFstab()
{
  ::setfsent();

  // clear all mount-Infos
  for( QPtrListIterator<K3bDevice::Device> it( d->allDevices ); it.current(); ++it ) {
    it.current()->setMountPoint( QString::null );
    it.current()->setMountDevice( QString::null );
  }

  struct fstab * mountInfo = 0;
  while( (mountInfo = ::getfsent()) )
  {
    // check if the entry corresponds to a device
    QString md = QFile::decodeName( mountInfo->fs_spec );
    QString type = QFile::decodeName( mountInfo->fs_vfstype );

    bool automount = false;

    if( type == "supermount" || type == "subfs" ) {
      automount = true;

      // parse the device
      QStringList opts = QStringList::split( ",", QString::fromLocal8Bit(mountInfo->fs_mntops) );
      for( QStringList::const_iterator it = opts.constBegin(); it != opts.constEnd(); ++it ) {
	if( (*it).startsWith("dev=") ) {
	  md = (*it).mid( 4 );
	  break;
	}
      }
    }

    if( md == "none" )
      continue;

    kdDebug() << "(K3bDevice::DeviceManager) scanning fstab: " << md << endl;


    //
    // Try finding the device
    //
    // compare bus, id, lun since the same device can for example be
    // determined as /dev/srX or /dev/scdX
    //
    int bus = -1, id = -1, lun = -1;
    K3bDevice::Device* dev = findDevice( resolveSymLink(md) );
    if( !dev && determineBusIdLun( mountInfo->fs_spec, bus, id, lun ) )
      dev = findDevice( bus, id, lun );

    // FIXME: is this nessessary? Don't we resolve all symlinks on bsd, too?
    //        and shouldn't we do an addDevice anywhere here in case the fstab
    //        contains a device which we did not find before?
    if( !dev )
      dev = findDevice( md );

    //
    // Maybe the fstab contains a device we did not find before?
    //
    if( !dev )
      dev = addDevice( md );

    //
    // Did we find a device?
    //
    if( dev ) {
      bool isPreferredMountPoint = false;
      kdDebug() << "(K3bDevice::DeviceManager) found device for " << md << ": " << resolveSymLink(md) << endl;

#ifdef Q_OS_FREEBSD
      // Several mount points for one device might exist. If more than one are found, the one with
      // user permission should have a higher priority.
      struct stat filestat;
      if (mountInfo->fs_file &&
	  !stat(mountInfo->fs_file, &filestat) &&
	  filestat.st_uid == geteuid())	{
	isPreferredMountPoint = true;
      }
#else
      if( mountInfo->fs_file &&
	  QString::fromLocal8Bit(mountInfo->fs_mntops).contains("users") )
	isPreferredMountPoint = true;
#endif

      if( isPreferredMountPoint || dev->mountDevice().isEmpty() ) {
        dev->setMountPoint( mountInfo->fs_file );
        dev->setMountDevice( md );
	dev->m_automount = automount;
      }
    }
  } // while mountInfo

  ::endfsent();
}


bool K3bDevice::DeviceManager::determineBusIdLun( const QString& dev, int& bus, int& id, int& lun )
{
#ifdef Q_OS_FREEBSD
  Q_UNUSED(dev);
  Q_UNUSED(bus);
  Q_UNUSED(id);
  Q_UNUSED(lun);
  return false;
  /* NOTREACHED */
#endif

#ifdef Q_OS_LINUX
  int ret = false;
  int cdromfd = K3bDevice::openDevice( dev.ascii() );
  if (cdromfd < 0) {
    kdDebug() << "could not open device " << dev << " (" << strerror(errno) << ")" << endl;
    return false;
  }

  struct stat cdromStat;
  ::fstat( cdromfd, &cdromStat );

  if( SCSI_BLK_MAJOR( cdromStat.st_rdev>>8 ) ||
      SCSI_GENERIC_MAJOR == (cdromStat.st_rdev>>8) ) {
    struct ScsiIdLun
    {
      int id;
      int lun;
    };
    ScsiIdLun idLun;

    // in kernel 2.2 SCSI_IOCTL_GET_IDLUN does not contain the bus id
    if ( (::ioctl( cdromfd, SCSI_IOCTL_GET_IDLUN, &idLun ) < 0) ||
         (::ioctl( cdromfd, SCSI_IOCTL_GET_BUS_NUMBER, &bus ) < 0) ) {
      kdDebug() << "Need a filename that resolves to a SCSI device" << endl;
      ret = false;
    }
    else {
      id  = idLun.id & 0xff;
      lun = (idLun.id >> 8) & 0xff;
      kdDebug() << "bus: " << bus << ", id: " << id << ", lun: " << lun << endl;
      ret = true;
    }
  }

  ::close(cdromfd);
  return ret;
#endif
}


QString K3bDevice::DeviceManager::resolveSymLink( const QString& path )
{
  char resolved[PATH_MAX];
  if( !realpath( QFile::encodeName(path), resolved ) )
  {
    kdDebug() << "Could not resolve " << path << endl;
    return path;
  }

  return QString::fromLatin1( resolved );
}


#include "k3bdevicemanager.moc"
