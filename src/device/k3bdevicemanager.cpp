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
#include "../tools/k3bexternalbinmanager.h"
#include "../tools/k3bglobals.h"

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
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
/* Fix definitions for 2.5 kernels */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,50)
typedef unsigned long long __u64;
typedef unsigned char u8;
#endif
#include <linux/../scsi/scsi.h> /* cope with silly includes */
#include <linux/cdrom.h>
#include <linux/major.h>
#include <linux/limits.h>


typedef Q_INT16 size16;
typedef Q_INT32 size32;


#ifndef IDE_DISK_MAJOR
/*
 * Tests for IDE devices
 */
#define IDE_DISK_MAJOR(M)       ((M) == IDE0_MAJOR || (M) == IDE1_MAJOR || \
                                (M) == IDE2_MAJOR || (M) == IDE3_MAJOR || \
                                (M) == IDE4_MAJOR || (M) == IDE5_MAJOR || \
                                (M) == IDE6_MAJOR || (M) == IDE7_MAJOR || \
                                (M) == IDE8_MAJOR || (M) == IDE9_MAJOR)
#endif /* #ifndef IDE_DISK_MAJOR */


K3bCdDevice::DeviceManager::DeviceManager( )
    : QObject()
{
  m_externalBinManager = K3bExternalBinManager::self();
  m_reader.setAutoDelete( true );
  m_writer.setAutoDelete( true );
  m_allDevices.setAutoDelete( false );
}


K3bDevice* K3bCdDevice::DeviceManager::deviceByName( const QString& name )
{
  return findDevice( name );
}


K3bDevice* K3bCdDevice::DeviceManager::findDevice( int bus, int id, int lun )
{
  QPtrListIterator<K3bDevice> it( m_allDevices );
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
  QPtrListIterator<K3bDevice> it( m_allDevices );
  while( it.current() )
  {
    if( it.current()->deviceNodes().contains(devicename) )
      return it.current();

    ++it;
  }

  return 0;
}


K3bCdDevice::DeviceManager::~DeviceManager() {}


QPtrList<K3bDevice>& K3bCdDevice::DeviceManager::burningDevices()
{
  return m_writer;
}


QPtrList<K3bDevice>& K3bCdDevice::DeviceManager::readingDevices()
{
  return m_reader;
}


QPtrList<K3bDevice>& K3bCdDevice::DeviceManager::allDevices()
{
  return m_allDevices;
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
  FILE *fd = popen(cmd.ascii(),"r");
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

  static const char* devicenames[] = {
    "/dev/hda",
    "/dev/hdb",
    "/dev/hdc",
    "/dev/hdd",
    "/dev/hde",
    "/dev/hdf",
    "/dev/hdg",
    "/dev/hdh",
    "/dev/hdi",
    "/dev/hdj",
    "/dev/hdk",
    "/dev/hdl",
    "/dev/dvd",
    "/dev/cdrom",
    "/dev/cdrecorder",
    0
  };
  int i = 0;
  while( devicenames[i] ) {
    if( addDevice( devicenames[i] ) )
      m_foundDevices++;
    ++i;
  }
  for( int i = 0; i < 16; i++ ) {
    if( addDevice( QString("/dev/scd%1").arg(i).ascii() ) )
      m_foundDevices++;
  }
  for( int i = 0; i < 16; i++ ) {
    if( addDevice( QString("/dev/sr%1").arg(i).ascii() ) )
      m_foundDevices++;
  }

  scanFstab();

  return m_foundDevices;
}


void K3bCdDevice::DeviceManager::printDevices()
{
  kdDebug() << "\nReader:" << endl;
  for( K3bDevice * dev = m_reader.first(); dev != 0; dev = m_reader.next() ) {
    kdDebug() << "  " << ": "
	      << dev->ioctlDevice() << " "
	      << dev->blockDeviceName() << " "
	      << dev->vendor() << " "
	      << dev->description() << " "
	      << dev->version() << endl
	      << "    "
	      << dev->mountDevice() << " "
	      << dev->mountPoint() << endl;
    kdDebug() << "Reader aliases: " << endl;
    for(QStringList::ConstIterator it = dev->deviceNodes().begin(); it != dev->deviceNodes().end(); ++it )
      kdDebug() << "     " << *it << endl;
  }
  kdDebug() << "\nWriter:" << endl;
  for( K3bDevice * dev = m_writer.first(); dev != 0; dev = m_writer.next() ) {
    kdDebug() << "  " << ": "
	      << dev->ioctlDevice() << " "
	      << dev->blockDeviceName() << " "
	      << dev->vendor() << " "
	      << dev->description() << " "
	      << dev->version() << " "
	      << dev->maxWriteSpeed() << endl
	      << "    "
	      << dev->mountDevice() << " "
	      << dev->mountPoint() << endl;
    kdDebug() << "Writer aliases: " << endl;
    for(QStringList::ConstIterator it = dev->deviceNodes().begin(); it != dev->deviceNodes().end(); ++it )
      kdDebug() << "     " << *it << endl;

  }
}


void K3bCdDevice::DeviceManager::clear()
{
  // clear current devices
  m_reader.clear();
  m_writer.clear();
  m_allDevices.clear();
}


bool K3bCdDevice::DeviceManager::readConfig( KConfig* c )
{
  m_foundDevices = 0;

  if( !c->hasGroup( "Devices" ) )
  {
    return false;
  }

  c->setGroup( "Devices" );

  // read Readers
  QStringList list = c->readListEntry( "Reader1" );
  int devNum = 1;
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
        dev->setCdrdaoDriver( list[2] );
    }

    if( dev == 0 )
      kdDebug() << "(K3bDeviceManager) Could not detect saved device " << list[0] << "." << endl;

    devNum++;
    list = c->readListEntry( QString( "Reader%1" ).arg( devNum ) );
  }

  // read Writers
  list = c->readListEntry( "Writer1" );
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
        dev->setWritesCdrw( list[5] == "yes" );
      if( list.count() > 6 )
        dev->setBurnproof( list[6] == "yes" );
      if( list.count() > 7 )
        dev->setBufferSize( list[7].toInt() );
      if( list.count() > 8 )
        dev->setCurrentWriteSpeed( list[8].toInt() );
      if( list.count() > 9 )
        dev->setDao( list[9] == "yes" );
    }

    if( dev == 0 )
      kdDebug() << "(K3bDeviceManager) Could not detect saved device " << list[0] << "." << endl;

    devNum++;
    list = c->readListEntry( QString( "Writer%1" ).arg( devNum ) );
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
    // remove all old device entrys
    c->deleteGroup("Devices");
  }


  c->setGroup( "Devices" );

  int i = 1;
  K3bDevice* dev = m_reader.first();
  while( dev != 0 )
  {
    QStringList list;
    list << dev->blockDeviceName()
    << QString::number(dev->maxReadSpeed())
    << dev->cdrdaoDriver();

    c->writeEntry( QString("Reader%1").arg(i), list );

    i++;
    dev = m_reader.next();
  }

  i = 1;
  dev = m_writer.first();
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

    list << ( dev->writesCdrw() ? "yes" : "no" )
    << ( dev->burnproof() ? "yes" : "no" )
    << QString::number( dev->bufferSize() )
    << QString::number( dev->currentWriteSpeed() )
    << ( dev->dao() ? "yes" : "no" );

    c->writeEntry( QString("Writer%1").arg(i), list );

    i++;
    dev = m_writer.next();
  }

  c->sync();

  return true;
}

void K3bCdDevice::DeviceManager::determineCapabilities(K3bDevice *dev)
{
#ifndef SUPPORT_IDE
  if (dev->interfaceType() == K3bDevice::IDE )
    return;
#endif
  if( m_externalBinManager->foundBin( "cdrecord" ) )
  {
    kdDebug() << "(K3bDeviceManager) probing capabilities for device " << dev->blockDeviceName() << endl;

    KProcess driverProc, capProc;
    driverProc << m_externalBinManager->binPath( "cdrecord" );
    driverProc << QString("dev=%1").arg(dev->busTargetLun());
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

    dev->setDao( dev->supportsWriteMode( K3bDevice::SAO ) );



    // check drive capabilities
    // does only work for generic-mmc drives
    capProc << m_externalBinManager->binPath( "cdrecord" );
    capProc << QString("dev=%1").arg(dev->busTargetLun());
    capProc << "-prcap";

    connect( &capProc, SIGNAL(receivedStdout(KProcess*, char*, int)),
             this, SLOT(slotCollectStdout(KProcess*, char*, int)) );

    m_processOutput = "";

    capProc.start( KProcess::Block, KProcess::Stdout );

    QStringList lines = QStringList::split( "\n", m_processOutput );

    // parse output
    for( QStringList::const_iterator it = lines.begin(); it != lines.end(); ++it )
    {
      const QString& line = *it;

      if( line.startsWith("  ") )
      {
        if( line.contains("write CD-R media") )
          dev->m_burner = !line.contains( "not" );

        else if( line.contains("write CD-RW media") )
          dev->m_bWritesCdrw = !line.contains( "not" );

        else if( line.contains("Buffer-Underrun-Free recording") ||
                 line.contains("support BURN-Proof") )
          dev->m_burnproof = !line.contains( "not" );

        else if( line.contains( "Maximum read  speed" ) ) //lukas: are there really two spaces? trueg: Yes, there are! ;)
          dev->m_maxReadSpeed = K3b::round( line.mid( line.find(":")+1 ).toDouble() * 1000.0 / ( 2352.0 * 75.0 ) );

        else if( line.contains( "Maximum write speed" ) )
          dev->m_maxWriteSpeed = K3b::round( line.mid( line.find(":")+1 ).toDouble() * 1000.0 / ( 2352.0 * 75.0 ) );

        else if( line.contains( "Buffer size" ) )
          dev->m_bufferSize = line.mid( line.find(":")+1 ).toInt();
      }
      else if( line.startsWith("Vendor_info") )
        dev->m_vendor = line.mid( line.find(":")+3, 8 ).stripWhiteSpace();
      else if( line.startsWith("Identifikation") )
        dev->m_description = line.mid( line.find(":")+3, 16 ).stripWhiteSpace();
      else if( line.startsWith("Revision") )
        dev->m_version = line.mid( line.find(":")+3, 4 ).stripWhiteSpace();
      else
        kdDebug() << "(K3bDeviceManager) unusable cdrecord output: " << line << endl;

    }
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

  if( /*!S_ISCHR( cdromStat.st_mode) && */!S_ISBLK( cdromStat.st_mode) )
  {
    kdDebug() << devicename << " is no "/* << "character or " */ << "block device" << endl;
  }
  else
  {
    kdDebug() << devicename << " is block device (" << (int)(cdromStat.st_rdev & 0xFF) << ")" << endl;
    if( ::ioctl(cdromfd, CDROM_CLEAR_OPTIONS, CDO_AUTO_CLOSE) < 0 )
    {
      kdDebug() << devicename << " seems not to be a cdrom device: " << strerror(errno) << endl;
    }
    else
    {
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

    if( device->burner() )
      m_writer.append( device );
    else
      m_reader.append( device );

    m_allDevices.append( device );
  }

  return device;
}


void K3bCdDevice::DeviceManager::scanFstab()
{
  FILE* fstabFile = setmntent( _PATH_FSTAB, "r" );
  if( !fstabFile )
  {
    kdDebug() << "(K3bDeviceManager) could not open " << _PATH_FSTAB << endl;
    return;
  }


  // clear all mount-Infos
  for( QPtrListIterator<K3bDevice> it( m_allDevices ); it.current(); ++it )
  {
    it.current()->setMountPoint( QString::null );
    it.current()->setMountDevice( QString::null );
  }


  struct mntent* mountInfo = 0;
  while( (mountInfo = ::getmntent( fstabFile )) )
  {
    // check if the entry corresponds to a device
    QString md = QFile::decodeName( mountInfo->mnt_fsname );
    if ( md == "none" )
      continue;

    kdDebug() << "(K3bDeviceManager) scanning fstab: " << md << endl;

    if( K3bDevice* dev = findDevice( resolveSymLink(md) ) )
    {
      kdDebug() << "(K3bDeviceManager) found device for " << md << ": " << resolveSymLink(md) << endl;
      if( dev->mountDevice().isEmpty() )
      {
        dev->setMountPoint( mountInfo->mnt_dir );
        dev->setMountDevice( md );
      }
    }
    else
    {
      // compare bus, id, lun since the same device can for example be
      // determined as /dev/srX or /dev/scdX
      int bus = -1, id = -1, lun = -1;
      if( determineBusIdLun( mountInfo->mnt_fsname, bus, id, lun ) )
      {
        if( K3bDevice* dev = findDevice( bus, id, lun ) )
        {
          if( dev->mountDevice().isEmpty() )
          {
            dev->setMountPoint( mountInfo->mnt_dir );
            dev->setMountDevice( md );
          }
        }
      }


    }
  } // while mountInfo

  endmntent( fstabFile );
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


K3bCdDevice::DeviceManager* K3bCdDevice::DeviceManager::self()
{
  static K3bDeviceManager* instance = 0;
  if( !instance )
    instance = new K3bDeviceManager();

  return instance;
}


#include "k3bdevicemanager.moc"
