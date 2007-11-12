/*
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config.h>

#include "k3blibdvdcss.h"

#include <k3bdevice.h>
#include <k3biso9660.h>
#include <k3biso9660backend.h>

#include <qfile.h>
#include <q3cstring.h>
#include <q3valuevector.h>
#include <qpair.h>

#include <dlfcn.h>


void* K3bLibDvdCss::s_libDvdCss = 0;
int K3bLibDvdCss::s_counter = 0;


extern "C" {
  struct dvdcss_s;
  typedef struct dvdcss_s* dvdcss_t;

  dvdcss_t (*k3b_dvdcss_open)(char*);
  int (*k3b_dvdcss_close)( dvdcss_t );
  int (*k3b_dvdcss_seek)( dvdcss_t, int, int );
  int (*k3b_dvdcss_read)( dvdcss_t, void*, int, int );
}



class K3bLibDvdCss::Private
{
public:
  Private()
    :dvd(0) {
  }

  dvdcss_t dvd;
  K3bDevice::Device* device;
  Q3ValueVector< QPair<int,int> > titleOffsets;
  int currentSector;
  bool currentSectorInTitle;
};

K3bLibDvdCss::K3bLibDvdCss()
{
  d = new Private();
  s_counter++;
}


K3bLibDvdCss::~K3bLibDvdCss()
{
  close();
  delete d;
  s_counter--;
  if( s_counter == 0 ) {
    dlclose( s_libDvdCss );
    s_libDvdCss = 0;
  }
}


bool K3bLibDvdCss::open( K3bDevice::Device* dev )
{
  d->device = dev;
  dev->close();
  d->dvd = k3b_dvdcss_open( const_cast<char*>( QFile::encodeName(dev->blockDeviceName()).data() ) );
  d->currentSector = 0;
  d->currentSectorInTitle = false;
  return ( d->dvd != 0 );
}


void K3bLibDvdCss::close()
{
  if( d->dvd )
    k3b_dvdcss_close( d->dvd );
  d->dvd = 0;
}


int K3bLibDvdCss::seek( int sector, int flags )
{
  return k3b_dvdcss_seek( d->dvd, sector, flags );
}


int K3bLibDvdCss::read( void* buffer, int sectors, int flags )
{
  return k3b_dvdcss_read( d->dvd, buffer, sectors, flags );
}


int K3bLibDvdCss::readWrapped( void* buffer, int firstSector, int sectors )
{
  // 1. are we in a title?
  // 2. does a new title start in the read sector area?
  //    - see below, set title if firstSector is the first sector of a new title
  // 3. does a title end in the read sector area?
  //    3.1 does a previous title end
  //    3.2 does the title from 2. already end

  // we need to seek to the first sector. Otherwise we get faulty data.
  bool needToSeek = ( firstSector != d->currentSector || firstSector == 0 );
  bool inTitle = false;
  bool startOfTitle = false;

  //
  // Make sure we never read encrypted and unencrypted data at once since libdvdcss
  // only decrypts the whole area of read sectors or nothing at all.
  //
  for( unsigned int i = 0; i < d->titleOffsets.count(); ++i ) {
    int titleStart = d->titleOffsets[i].first;
    int titleEnd = titleStart + d->titleOffsets[i].second - 1;

    // update key when entrering a new title
    // FIXME: we also need this if we seek into a new title (not only the start of the title)
    if( titleStart == firstSector )
      startOfTitle = needToSeek = inTitle = true;

    // check if a new title or non-title area starts inside the read sector range
    if( firstSector < titleStart && firstSector+sectors > titleStart ) {
      kDebug() << "(K3bLibDvdCss) title start inside of sector range ("
		<< firstSector << "-" << (firstSector+sectors-1)
		<< "). only reading " << (titleStart - firstSector) << " sectors up to title offset "
		<< (titleStart-1) << endl;
      sectors = titleStart - firstSector;
    }

    if( firstSector < titleEnd && firstSector+sectors > titleEnd ) {
      kDebug() << "(K3bLibDvdCss) title end inside of sector range ("
		<< firstSector << "-" << (firstSector+sectors-1)
		<< "). only reading " << (titleEnd - firstSector + 1) << " sectors up to title offset "
		<< titleEnd << endl;
      sectors = titleEnd - firstSector + 1;
      inTitle = true;
    }

    // is our read range part of one title
    if( firstSector >= titleStart && firstSector+sectors-1 <= titleEnd )
      inTitle = true;
  }

  if( needToSeek ) {
    int flags = DVDCSS_NOFLAGS;
    if( startOfTitle )
      flags = DVDCSS_SEEK_KEY;
    else if( inTitle )
      flags = DVDCSS_SEEK_MPEG;

    kDebug() << "(K3bLibDvdCss) need to seek from " << d->currentSector << " to " << firstSector << " with " << flags;

    d->currentSector = seek( firstSector, flags );
    if( d->currentSector != firstSector ) {
      kDebug() << "(K3bLibDvdCss) seek failed: " << d->currentSector;
      return -1;
    }

    kDebug() << "(K3bLibDvdCss) seek done: " << d->currentSector;
  }


  int flags = DVDCSS_NOFLAGS;
  if( inTitle )
    flags = DVDCSS_READ_DECRYPT;

  int ret = read( buffer, sectors, flags );
  if( ret >= 0 )
    d->currentSector += ret;
  else
    d->currentSector = 0; // force a seek the next time

  return ret;
}


bool K3bLibDvdCss::crackAllKeys()
{
  //
  // Loop over all titles and crack the keys (inspired by libdvdread)
  //
  kDebug() << "(K3bLibDvdCss) cracking all keys.";

  d->titleOffsets.clear();

  K3bIso9660 iso( new K3bIso9660DeviceBackend( d->device ) );
  iso.setPlainIso9660( true );
  if( !iso.open() ) {
    kDebug() << "(K3bLibDvdCss) could not open iso9660 fs.";
    return false;
  }

#ifdef K3B_DEBUG
  iso.debug();
#endif

  const K3bIso9660Directory* dir = iso.firstIsoDirEntry();

  int title = 0;
  for( ; title < 100; ++title ) {
    QString filename;

    // first we get the menu vob
    if( title == 0 )
      filename.sprintf( "VIDEO_TS/VIDEO_TS.VOB" );
    else
      filename.sprintf( "VIDEO_TS/VTS_%02d_%d.VOB", title, 0 );

    const K3bIso9660File* file = dynamic_cast<const K3bIso9660File*>( dir->entry( filename ) );
    if( file && file->size() > 0 ) {
      d->titleOffsets.append( qMakePair( (int)file->startSector(), (int)(file->size() / 2048U) ) );
      kDebug() << "(K3bLibDvdCss) Get key for /" << filename << " at " << file->startSector();
      if( seek( (int)file->startSector(), DVDCSS_SEEK_KEY ) < 0 ) {
        kDebug() << "(K3bLibDvdCss) failed to crash key for " << filename << " at " << file->startSector();
      }
    }

    if( title > 0 ) {
      QPair<int,int> p;
      int vob = 1;
      for( ; vob < 100; ++vob ) {
	filename.sprintf( "VIDEO_TS/VTS_%02d_%d.VOB", title, vob );
	file = dynamic_cast<const K3bIso9660File*>( dir->entry( filename ) );
	if( file ) {
	  if( file->size() % 2048 )
	    kError() << "(K3bLibDvdCss) FILESIZE % 2048 != 0!!!" << endl;
	  if( vob == 1 ) {
	    p.first = file->startSector();
	    p.second = file->size() / 2048;
	    kDebug() << "(K3bLibDvdCss) Get key for /" << filename << " at " << file->startSector();
	    if( seek( (int)file->startSector(), DVDCSS_SEEK_KEY ) < 0 ) {
                kDebug() << "(K3bLibDvdCss) failed to crash key for " << filename << " at " << file->startSector();
	    }
	  }
	  else {
	    p.second += file->size() / 2048;
	  }
	}
	else {
	  // last vob
	  break;
	}
      }
      --vob;

      // last title
      if( vob == 0 )
	break;

      kDebug() << "(K3bLibDvdCss) Title " << title << " " << vob << " vobs with length " << p.second;
      d->titleOffsets.append( p );
    }
  }

  --title;

  kDebug() << "(K3bLibDvdCss) found " << title << " titles.";

  return (title > 0);
}


K3bLibDvdCss* K3bLibDvdCss::create()
{
  if( s_libDvdCss == 0 ) {
    s_libDvdCss = dlopen( "libdvdcss.so.2", RTLD_LAZY|RTLD_GLOBAL );
    if( s_libDvdCss ) {
      k3b_dvdcss_open = (dvdcss_t (*)(char*))dlsym( s_libDvdCss, "dvdcss_open" );
      k3b_dvdcss_close = (int (*)( dvdcss_t ))dlsym( s_libDvdCss, "dvdcss_close" );
      k3b_dvdcss_seek = (int (*)( dvdcss_t, int, int ))dlsym( s_libDvdCss, "dvdcss_seek" );
      k3b_dvdcss_read = (int (*)( dvdcss_t, void*, int, int ))dlsym( s_libDvdCss, "dvdcss_read" );

      if( !k3b_dvdcss_open || !k3b_dvdcss_close || !k3b_dvdcss_seek || !k3b_dvdcss_read ) {
	kDebug() << "(K3bLibDvdCss) unable to resolve libdvdcss.";
	dlclose( s_libDvdCss );
	s_libDvdCss = 0;
      }
    }
    else
      kDebug() << "(K3bLibDvdCss) unable to load libdvdcss.";
  }

  if( s_libDvdCss )
    return new K3bLibDvdCss();
  else
    return 0;
}
