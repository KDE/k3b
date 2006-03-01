/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3blibdvdcss.h"

#include <k3bdevice.h>
#include <k3biso9660.h>
#include <k3biso9660backend.h>

#include <qfile.h>
#include <qcstring.h>
#include <qvaluevector.h>
#include <qpair.h>

#include <dlfcn.h>


void* K3bLibDvdCss::s_libDvdCss = 0;
int K3bLibDvdCss::s_counter = 0;


extern "C" {
  struct dvdcss_s;
  typedef struct dvdcss_s* dvdcss_t;

  dvdcss_t (*dvdcss_open)(char*);
  int (*dvdcss_close)( dvdcss_t );
  int (*dvdcss_seek)( dvdcss_t, int, int );
  int (*dvdcss_read)( dvdcss_t, void*, int, int );
}



class K3bLibDvdCss::Private
{
public:
  Private()
    :dvd(0) {
  }

  dvdcss_t dvd;
  K3bDevice::Device* device;
  QValueVector< QPair<int,int> > titleOffsets;
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
  d->dvd = dvdcss_open( const_cast<char*>( QFile::encodeName(dev->blockDeviceName()).data() ) );
  d->currentSector = 0;
  d->currentSectorInTitle = false;
  return ( d->dvd != 0 );
}


void K3bLibDvdCss::close()
{
  if( d->dvd )
    dvdcss_close( d->dvd );
  d->dvd = 0;
}


int K3bLibDvdCss::seek( int sector, int flags )
{
  return dvdcss_seek( d->dvd, sector, flags );
}


int K3bLibDvdCss::read( void* buffer, int sectors, int flags )
{
  return dvdcss_read( d->dvd, buffer, sectors, flags );
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
      kdDebug() << "(K3bLibDvdCss) title start inside of sector range (" 
		<< firstSector << "-" << (firstSector+sectors-1) 
		<< "). only reading " << (titleStart - firstSector) << " sectors up to title offset "
		<< (titleStart-1) << endl;
      sectors = titleStart - firstSector;
    }
    
    if( firstSector < titleEnd && firstSector+sectors > titleEnd ) {
      kdDebug() << "(K3bLibDvdCss) title end inside of sector range (" 
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

    kdDebug() << "(K3bLibDvdCss) need to seek from " << d->currentSector << " to " << firstSector << " with " << flags << endl;

    d->currentSector = seek( firstSector, flags );
    if( d->currentSector != firstSector ) {
      kdDebug() << "(K3bLibDvdCss) seek failed: " << d->currentSector << endl;
      return -1;
    }

    kdDebug() << "(K3bLibDvdCss) seek done: " << d->currentSector << endl;
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
  kdDebug() << "(K3bLibDvdCss) cracking all keys." << endl;

  d->titleOffsets.clear();

  K3bIso9660 iso( new K3bIso9660DeviceBackend( d->device ) );
  iso.setPlainIso9660( true );
  if( !iso.open() ) {
    kdDebug() << "(K3bLibDvdCss) could not open iso9660 fs." << endl;
    return false;
  }

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
      d->titleOffsets.append( qMakePair( (int)file->startSector(), (int)file->size() / 2048 ) );
      kdDebug() << "(K3bLibDvdCss) Get key for /" << filename << " at " << file->startSector() << endl;
      if( seek( (int)file->startSector(), DVDCSS_SEEK_KEY ) < 0 ) {
	kdDebug() << "(K3bLibDvdCss) unable to seek to " << file->startSector() << endl;
	return false;
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
	    kdError() << "(K3bLibDvdCss) FILESIZE % 2048 != 0!!!" << endl;
	  if( vob == 1 ) {
	    p.first = file->startSector();
	    p.second = file->size() / 2048;
	    kdDebug() << "(K3bLibDvdCss) Get key for /" << filename << " at " << file->startSector() << endl;
	    if( seek( (int)file->startSector(), DVDCSS_SEEK_KEY ) < 0 ) {
	      kdDebug() << "(K3bLibDvdCss) unable to seek to " << file->startSector() << endl;
	      return false;
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

      kdDebug() << "(K3bLibDvdCss) Title " << title << " " << vob << " vobs with length " << p.second << endl;
      d->titleOffsets.append( p );
    }
  }

  --title;

  kdDebug() << "(K3bLibDvdCss) found " << title << " titles." << endl;

  return (title > 0);
}


K3bLibDvdCss* K3bLibDvdCss::create()
{
  if( s_libDvdCss == 0 ) {
    s_libDvdCss = dlopen( "libdvdcss.so.2", RTLD_LAZY|RTLD_GLOBAL );
    if( s_libDvdCss ) {
      dvdcss_open = (dvdcss_t (*)(char*))dlsym( s_libDvdCss, "dvdcss_open" );
      dvdcss_close = (int (*)( dvdcss_t ))dlsym( s_libDvdCss, "dvdcss_close" );
      dvdcss_seek = (int (*)( dvdcss_t, int, int ))dlsym( s_libDvdCss, "dvdcss_seek" );
      dvdcss_read = (int (*)( dvdcss_t, void*, int, int ))dlsym( s_libDvdCss, "dvdcss_read" );

      if( !dvdcss_open || !dvdcss_close || !dvdcss_seek || !dvdcss_read ) {
	kdDebug() << "(K3bLibDvdCss) unable to resolve libdvdcss." << endl;
	dlclose( s_libDvdCss );
	s_libDvdCss = 0;
      }
    }
    else
      kdDebug() << "(K3bLibDvdCss) unable to load libdvdcss." << endl;
  }

  if( s_libDvdCss )
    return new K3bLibDvdCss();
  else
    return 0;
}
