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



#include "k3bglobals.h"
#include <k3bversion.h>
#include <k3bdevice.h>
#include <k3bdeviceglobals.h>
#include <k3bexternalbinmanager.h>

#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kio/netaccess.h>
#include <kurl.h>

#include <qdatastream.h>
#include <qdir.h>
#include <qfile.h>

#include <cmath>
#include <sys/utsname.h>

#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/mount.h>
#else
#include <sys/vfs.h>
#endif



struct Sample {
  unsigned char msbLeft;
  unsigned char lsbLeft;
  unsigned char msbRight;
  unsigned char lsbRight;

  short left() const {
    return ( msbLeft << 8 ) | lsbLeft;
  } 
  short right() const {
    return ( msbRight << 8 ) | lsbRight;
  } 
  void left( short d ) {
    msbLeft = d >> 8;
    lsbLeft = d;
  } 
  void right( short d ) {
    msbRight = d >> 8;
    lsbRight = d;
  }
};

QString K3b::framesToString( int h, bool showFrames )
{
  int m = h / 4500;
  int s = (h % 4500) / 75;
  int f = h % 75;

  QString str;

  if( showFrames ) {
    // cdrdao needs the MSF format where 1 second has 75 frames!
    str.sprintf( "%.2i:%.2i:%.2i", m, s, f );
  }
  else
    str.sprintf( "%.2i:%.2i", m, s );

  return str;
}

QString K3b::sizeToTime(long size){
	int h = size / sizeof(Sample) / 588;
	return framesToString(h, false);
}


Q_INT16 K3b::swapByteOrder( Q_INT16 i )
{
  return ((i << 8) & 0xff00) | ((i >> 8 ) & 0xff);
}


Q_INT32 K3b::swapByteOrder( Q_INT32 i )
{
  return ((i << 24) & 0xff000000) | ((i << 8) & 0xff0000) | ((i >> 8) & 0xff00) | ((i >> 24) & 0xff );
}


int K3b::round( double d )
{
  return (int)( floor(d) + 0.5 <= d ? ceil(d) : floor(d) );
}


QString K3b::findUniqueFilePrefix( const QString& _prefix, const QString& path )
{
  QString url;
  if( path.isEmpty() || !QFile::exists(path) )
    url = defaultTempPath();
  else
    url = prepareDir( path );

  QString prefix = _prefix;
  if( prefix.isEmpty() )
    prefix = "k3b_";

  // now create the unique prefix
  QDir dir( url );
  QStringList entries = dir.entryList( QDir::DefaultFilter, QDir::Name );
  int i = 0;
  for( QStringList::iterator it = entries.begin(); 
       it != entries.end(); ++it ) {
    if( (*it).startsWith( prefix + QString::number(i) ) ) {
      i++;
      it = entries.begin();
    }
  }

  return url + prefix + QString::number(i);
}


QString K3b::findTempFile( const QString& ending, const QString& d )
{
  return findUniqueFilePrefix( "k3b_", d ) + ( ending.isEmpty() ? QString::null : (QString::fromLatin1(".") + ending) );
}


QString K3b::defaultTempPath()
{
  QString oldGroup = kapp->config()->group();
  kapp->config()->setGroup( "General Options" );
  QString url = kapp->config()->readPathEntry( "Temp Dir", KGlobal::dirs()->resourceDirs( "tmp" ).first() );
  kapp->config()->setGroup( oldGroup );
  return prepareDir(url);
}


QString K3b::prepareDir( const QString& dir )
{
  return (dir + (dir[dir.length()-1] != '/' ? "/" : ""));
}


QString K3b::parentDir( const QString& path )
{
  QString parent = path;
  if( path[path.length()-1] == '/' )
    parent.truncate( parent.length()-1 );

  int pos = parent.findRev( '/' );
  if( pos >= 0 )
    parent.truncate( pos+1 );
  else // relative path, do anything...
    parent = "/";

  return parent;
}


QString K3b::fixupPath( const QString& path )
{
  QString s;
  bool lastWasSlash = false;
  for( unsigned int i = 0; i < path.length(); ++i ) {
    if( path[i] == '/' ) {
      if( !lastWasSlash ) {
	lastWasSlash = true;
	s.append( "/" );
      }
    }
    else {
      lastWasSlash = false;
      s.append( path[i] );
    }
  }

  return s;
}


K3bVersion K3b::kernelVersion()
{
  // initialize kernel version
  K3bVersion v;
  utsname unameinfo;
  if( ::uname(&unameinfo) == 0 ) {
    v = QString::fromLocal8Bit( unameinfo.release, sizeof(unameinfo.release) );
    kdDebug() << "linux kernel version: " << v << endl;
  }
  else
    kdError() << "could not determine Linux kernel version." << endl;
  return v;
}


K3bVersion K3b::simpleKernelVersion()
{
  return kernelVersion().simplify();
}


QString K3b::systemName()
{
  QString v;
  utsname unameinfo;
  if( ::uname(&unameinfo) == 0 ) {
    v = QString::fromLocal8Bit( unameinfo.sysname, sizeof(unameinfo.sysname) );;
  }
  else
    kdError() << "could not determine system name." << endl;
  return v;
}


bool K3b::kbFreeOnFs( const QString& path, unsigned long& size, unsigned long& avail )
{
  struct statfs fs;

  if( ::statfs( QFile::encodeName(path), &fs ) == 0 ) {
    unsigned long kBfak = fs.f_bsize/1024;

    size = fs.f_blocks*kBfak;
    avail = fs.f_bavail*kBfak;

    return true;
  }
  else
    return false;
}


KIO::filesize_t K3b::filesize( const KURL& url )
{
  KIO::filesize_t fSize = 0;
  // we use KIO since QFileInfo does provide the size as unsigned int wich is way too small for DVD images
  KIO::UDSEntry uds;
  KIO::NetAccess::stat( url, uds, 0 );
  for( KIO::UDSEntry::const_iterator it = uds.begin(); it != uds.end(); ++it ) {
    if( (*it).m_uds == KIO::UDS_SIZE ) {
      fSize = (*it).m_long;
      break;
    }
  }

  return fSize;
}


QString K3b::cutFilename( const QString& name, unsigned int len )
{
  if( name.length() > len ) {
    QString ret = name;

    // determine extension (we think of an extension to be at most 5 chars in length)
    int pos = name.find( '.', -6 );
    if( pos > 0 )
      len -= (name.length() - pos);

    ret.truncate( len );

    if( pos > 0 )
      ret.append( name.mid( pos ) );

    return ret;
  }
  else
    return name;
}


QString K3b::appendNumberToFilename( const QString& name, int num, unsigned int maxlen )
{
  // determine extension (we think of an extension to be at most 5 chars in length)
  QString result = name;
  QString ext;
  int pos = name.find( '.', -6 );
  if( pos > 0 ) {
    ext = name.mid(pos);
    result.truncate( pos );
  }

  ext.prepend( QString::number(num) );
  result.truncate( maxlen - ext.length() );

  return result + ext;
}


bool K3b::plainAtapiSupport()
{
  // IMPROVEME!!!
  return ( K3b::simpleKernelVersion() >= K3bVersion( 2, 5, 40 ) );
}


bool K3b::hackedAtapiSupport()
{
  // IMPROVEME!!!
  // FIXME: since when does the kernel support this?
  return ( K3b::simpleKernelVersion() >= K3bVersion( 2, 4, 0 ) );
}


QString K3b::externalBinDeviceParameter( K3bDevice::Device* dev, const K3bExternalBin* bin )
{
  if( dev->interfaceType() == K3bDevice::SCSI )
    return dev->busTargetLun();
  else if( (plainAtapiSupport() && bin->hasFeature("plain-atapi") ) )
    return dev->blockDeviceName();
  else
    return QString("ATAPI:%1").arg(dev->blockDeviceName());
}


int K3b::writingAppFromString( const QString& s )
{
  if( s.lower() == "cdrdao" )
    return K3b::CDRDAO;
  else if( s.lower() == "cdrecord" )
    return K3b::CDRECORD;
  else if( s.lower() == "dvdrecord" )
    return K3b::DVDRECORD;
  else if( s.lower() == "growisofs" )
    return K3b::GROWISOFS;
  else if( s.lower() == "dvd+rw-format" )
    return K3b::DVD_RW_FORMAT;
  else
    return K3b::DEFAULT;
}


QString K3b::writingModeString( int mode )
{
  if( mode == WRITING_MODE_AUTO )
    return i18n("Auto");
  else
    return K3bDevice::writingModeString( mode );
}
