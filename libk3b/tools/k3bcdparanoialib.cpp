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

#include "k3bcdparanoialib.h"

#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3bmsf.h>

#include <kdebug.h>

#include <dlfcn.h>
#include <qfile.h>

void* K3bCdparanoiaLib::s_libInterface = 0;
void* K3bCdparanoiaLib::s_libParanoia = 0;
int K3bCdparanoiaLib::s_counter = 0;

// from cdda_paranoia.h
#define PARANOIA_CB_READ           0
#define PARANOIA_CB_VERIFY         1
#define PARANOIA_CB_FIXUP_EDGE     2
#define PARANOIA_CB_FIXUP_ATOM     3
#define PARANOIA_CB_SCRATCH        4
#define PARANOIA_CB_REPAIR         5
#define PARANOIA_CB_SKIP           6
#define PARANOIA_CB_DRIFT          7
#define PARANOIA_CB_BACKOFF        8
#define PARANOIA_CB_OVERLAP        9
#define PARANOIA_CB_FIXUP_DROPPED 10
#define PARANOIA_CB_FIXUP_DUPED   11
#define PARANOIA_CB_READERR       12



static void paranoiaCallback( long, int status )
{
  // do nothing so far....
  return;

  switch( status ) {
  case -1:
    break;
  case -2:
    break;
  case PARANOIA_CB_READ:
    // no problem
    // does only this mean that the sector has been read?
//     m_lastReadSector = sector;  // this seems to be rather useless
//     m_readSectors++;
    break;
  case PARANOIA_CB_VERIFY:
    break;
  case PARANOIA_CB_FIXUP_EDGE:
    break;
  case PARANOIA_CB_FIXUP_ATOM:
    break;
  case PARANOIA_CB_SCRATCH:
    // scratch detected
    break;
  case PARANOIA_CB_REPAIR:
    break;
  case PARANOIA_CB_SKIP:
    // skipped sector
    break;
  case PARANOIA_CB_DRIFT:
    break;
  case PARANOIA_CB_BACKOFF:
    break;
  case PARANOIA_CB_OVERLAP:
    // sector does not seem to contain the current
    // sector but the amount of overlapped data
    //    m_overlap = sector;
    break;
  case PARANOIA_CB_FIXUP_DROPPED:
    break;
  case PARANOIA_CB_FIXUP_DUPED:
    break;
  case PARANOIA_CB_READERR:
    break;
  } 
}



extern "C" {
  struct cdrom_drive;
  struct cdrom_paranoia;

  // HINT: these pointers must NOT have the same name like the actual methods!
  //       I added "cdda_" as prefix
  //       Before doing that K3b crashed in cdda_open!
  //       Can anyone please explain that to me?

  // cdda_interface
  cdrom_drive* (*cdda_cdda_identify)(const char*, int, char**);
  int (*cdda_cdda_open)(cdrom_drive *d);
  int (*cdda_cdda_close)(cdrom_drive *d);
  long (*cdda_cdda_track_firstsector)( cdrom_drive*, int );
  long (*cdda_cdda_track_lastsector)( cdrom_drive*, int );
  long (*cdda_cdda_disc_firstsector)(cdrom_drive *d);
  void (*cdda_cdda_verbose_set)(cdrom_drive *d,int err_action, int mes_action);

  // cdda_paranoia
  cdrom_paranoia* (*cdda_paranoia_init)(cdrom_drive*);
  void (*cdda_paranoia_free)(cdrom_paranoia *p);
  void (*cdda_paranoia_modeset)(cdrom_paranoia *p, int mode);
  int16_t* (*cdda_paranoia_read_limited)(cdrom_paranoia *p, void(*callback)(long,int), int);
  long (*cdda_paranoia_seek)(cdrom_paranoia *p,long seek,int mode);
}

// from cdda_paranoia.h
#define PARANOIA_MODE_FULL        0xff
#define PARANOIA_MODE_DISABLE     0

#define PARANOIA_MODE_VERIFY      1
#define PARANOIA_MODE_FRAGMENT    2
#define PARANOIA_MODE_OVERLAP     4
#define PARANOIA_MODE_SCRATCH     8
#define PARANOIA_MODE_REPAIR      16
#define PARANOIA_MODE_NEVERSKIP   32




class K3bCdparanoiaLib::Private
{
public:
  Private() 
    : drive(0),
      paranoia(0),
      paranoiaMode(1),
      neverSkip(false),
      maxRetries(20),
      device(0),
      currentSector(0),
      startSector(0),
      lastSector(0),
      status(S_OK) {
  }

  ~Private() {
  }

  cdrom_drive* drive;
  cdrom_paranoia* paranoia;
  int paranoiaMode;
  bool neverSkip;
  int maxRetries;

  // high-level api
  K3bDevice::Device* device;
  K3bDevice::Toc toc;
  long currentSector;
  long startSector;
  long lastSector;
  int status;
  unsigned int currentTrack;
};


bool K3bCdparanoiaLib::paranoiaInit( const QString& devicename )
{
  if( d->drive )
    paranoiaFree();

  d->drive = cdda_cdda_identify( QFile::encodeName(devicename), 0, 0 );
  if( d->drive == 0 )
    return false;

  //  cdda_cdda_verbose_set( d->drive, 1, 1 );

  cdda_cdda_open( d->drive );
  d->paranoia = cdda_paranoia_init( d->drive );
  if( d->paranoia == 0 ) {
    paranoiaFree();
    return false;
  }

  setParanoiaMode( d->paranoiaMode );

  return true;
}


void K3bCdparanoiaLib::paranoiaFree()
{
  if( d->paranoia ) {
    cdda_paranoia_free( d->paranoia );
    d->paranoia = 0;
  }
  if( d->drive ) {
    cdda_cdda_close( d->drive );
    d->drive = 0;
  }
}


void K3bCdparanoiaLib::setParanoiaMode( int mode )
{
  // from cdrdao 1.1.7
  d->paranoiaMode = PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP;

  switch (mode) {
  case 0:
    d->paranoiaMode = PARANOIA_MODE_DISABLE;
    break;

  case 1:
    d->paranoiaMode |= PARANOIA_MODE_OVERLAP;
    d->paranoiaMode &= ~PARANOIA_MODE_VERIFY;
    break;

  case 2:
    d->paranoiaMode &= ~(PARANOIA_MODE_SCRATCH|PARANOIA_MODE_REPAIR);
    break;
  }

  if( d->neverSkip )
    d->paranoiaMode |= PARANOIA_MODE_NEVERSKIP;

  if( d->paranoia )
    cdda_paranoia_modeset( d->paranoia, d->paranoiaMode );
}


void K3bCdparanoiaLib::setNeverSkip( bool b )
{
  d->neverSkip = b;
  setParanoiaMode( d->paranoiaMode );
}


  /** default: 20 */
void K3bCdparanoiaLib::setMaxRetries( int m )
{
  d->maxRetries = m;
}


int16_t* K3bCdparanoiaLib::paranoiaRead( void(*callback)(long,int) )
{
  if( d->paranoia ) {
    return cdda_paranoia_read_limited( d->paranoia, callback, d->maxRetries );
  }
  else
    return 0;
}


long K3bCdparanoiaLib::firstSector( int track )
{
  if( d->drive )
    return cdda_cdda_track_firstsector( d->drive, track );
  else
    return -1;
}

long K3bCdparanoiaLib::lastSector( int track )
{
  if( d->drive )
    return cdda_cdda_track_lastsector(d->drive, track );
  else
   return -1;
}


long K3bCdparanoiaLib::paranoiaSeek( long sector, int mode )
{
  if( d->paranoia )
    return cdda_paranoia_seek( d->paranoia, sector, mode );
  else
    return -1;
}


K3bCdparanoiaLib::K3bCdparanoiaLib()
{
  d = new Private();
  s_counter++;
}

K3bCdparanoiaLib::~K3bCdparanoiaLib()
{
  delete d;
  s_counter--;
  if( s_counter == 0 ) {
    dlclose( s_libInterface );
    dlclose( s_libParanoia );
    s_libInterface = 0;
    s_libParanoia = 0;
  }
}


bool K3bCdparanoiaLib::load()
{
  cdda_cdda_identify = (cdrom_drive* (*) (const char*, int, char**))dlsym( s_libInterface, "cdda_identify");
  cdda_cdda_open = (int (*) (cdrom_drive*))dlsym( s_libInterface, "cdda_open");
  cdda_cdda_close = (int (*) (cdrom_drive*))dlsym( s_libInterface, "cdda_close");
  cdda_cdda_track_firstsector = (long (*)(cdrom_drive*, int))dlsym( s_libInterface, "cdda_track_firstsector");
  cdda_cdda_track_lastsector = (long (*)(cdrom_drive*, int))dlsym( s_libInterface, "cdda_track_lastsector");
  cdda_cdda_verbose_set = (void (*)(cdrom_drive *d,int err_action, int mes_action))dlsym( s_libInterface, "cdda_verbose_set");
  cdda_cdda_disc_firstsector = (long (*)(cdrom_drive *d))dlsym( s_libInterface, "cdda_disc_firstsector");

  cdda_paranoia_init = (cdrom_paranoia* (*)(cdrom_drive*))dlsym( s_libParanoia, "paranoia_init");
  cdda_paranoia_free = (void (*)(cdrom_paranoia *p))dlsym( s_libParanoia, "paranoia_free");
  cdda_paranoia_modeset = (void (*)(cdrom_paranoia *p, int mode))dlsym( s_libParanoia, "paranoia_modeset");
  cdda_paranoia_read_limited = (int16_t* (*)(cdrom_paranoia *p, void(*callback)(long,int), int))dlsym( s_libParanoia, "paranoia_read_limited");
  cdda_paranoia_seek = (long (*)(cdrom_paranoia *p,long seek,int mode))dlsym( s_libParanoia, "paranoia_seek");

  // check if all symbols could be resoled
  if( cdda_cdda_identify == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'cdda_identify'" << endl;
    return false;
  }
  if( cdda_cdda_open == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'cdda_open'" << endl;
    return false;
  }
  if( cdda_cdda_close == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'cdda_close'" << endl;
    return false;
  }
  if( cdda_cdda_track_firstsector == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'cdda_track_firstsector'" << endl;
    return false;
  }
  if( cdda_cdda_track_lastsector == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'cdda_track_lastsector'" << endl;
    return false;
  }
  if( cdda_cdda_disc_firstsector == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'cdda_disc_firstsector'" << endl;
    return false;
  }
  if( cdda_cdda_verbose_set == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'cdda_verbose_set'" << endl;
    return false;
  }

  if( cdda_paranoia_init == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'paranoia_init'" << endl;
    return false;
  }
  if( cdda_paranoia_free == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'paranoia_free'" << endl;
    return false;
  }
  if( cdda_paranoia_modeset == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'paranoia_modeset'" << endl;
    return false;
  }
  if( cdda_paranoia_read_limited == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'paranoia_read_limited'" << endl;
    return false;
  }
  if( cdda_paranoia_seek == 0 ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve 'paranoia_seek'" << endl;
    return false;
  }

  return true;
}



K3bCdparanoiaLib* K3bCdparanoiaLib::create()
{
  // check if libcdda_interface is avalilable
  if( s_libInterface == 0 ) {
    s_libInterface = dlopen( "libcdda_interface.so.0", RTLD_NOW|RTLD_GLOBAL );
      // try the redhat & Co. location
    if( s_libInterface == 0 )
      s_libInterface = dlopen( "cdda/libcdda_interface.so.0", RTLD_NOW|RTLD_GLOBAL );

    if( s_libInterface == 0 ) {
      kdDebug() << "(K3bCdparanoiaLib) Error while loading libcdda_interface. " << endl;
      return 0;
    }

    s_libParanoia = dlopen( "libcdda_paranoia.so.0", RTLD_NOW );
    if( s_libParanoia == 0 )
      s_libParanoia = dlopen( "cdda/libcdda_paranoia.so.0", RTLD_NOW );

    if( s_libParanoia == 0 ) {
      kdDebug() << "(K3bCdparanoiaLib) Error while loading libcdda_paranoia. " << endl;
      dlclose( s_libInterface );
      s_libInterface = 0;
      return 0;
    }
  }

  K3bCdparanoiaLib* lib = new K3bCdparanoiaLib();
  if( !lib->load() ) {
    kdDebug() << "(K3bCdparanoiaLib) Error: could not resolve all symbols!" << endl;
    delete lib;
    return 0;
  }
  return lib;
}


bool K3bCdparanoiaLib::initParanoia( K3bDevice::Device* dev, const K3bDevice::Toc& toc )
{
  if( !dev ) {
    kdError() << "(K3bCdparanoiaLib::initParanoia) dev = 0!" << endl;
    return false;
  }

  paranoiaFree();

  // since we use cdparanoia to open the device it is important to close
  // the device here
  dev->close();

  d->device = dev;
  d->toc = toc;
  if( d->toc.isEmpty() ) {
    kdDebug() << "(K3bCdparanoiaLib) empty toc." << endl;
    cleanup();
    return false;
  }

  if( d->toc.contentType() == K3bDevice::DATA ) {
    kdDebug() << "(K3bCdparanoiaLib) No audio tracks found." << endl;
    cleanup();
    return false;
  }

  if( paranoiaInit( dev->blockDeviceName() ) ) {
    d->startSector = d->currentSector = d->lastSector = 0;

    return true;
  }
  else {
    cleanup();
    return false;
  }
}


bool K3bCdparanoiaLib::initParanoia( K3bDevice::Device* dev )
{
  return initParanoia( dev, dev->readToc() );
}


void K3bCdparanoiaLib::close()
{
  cleanup();
}


void K3bCdparanoiaLib::cleanup()
{
  paranoiaFree();
  d->device = 0;
  d->currentSector = 0;
}


bool K3bCdparanoiaLib::initReading()
{
  if( d->device ) {
    // find first audio track
    K3bDevice::Toc::const_iterator trackIt = d->toc.begin();
    while( (*trackIt).type() != K3bDevice::Track::AUDIO ) {
      ++trackIt;
    }

    long start = (*trackIt).firstSector().lba();

    // find last audio track
    while( trackIt != d->toc.end() && (*trackIt).type() == K3bDevice::Track::AUDIO )
      ++trackIt;
    --trackIt;

    long end = (*trackIt).lastSector().lba();

    return initReading( start, end );
  }
  else {
    kdDebug() << "(K3bCdparanoiaLib) initReading without initParanoia." << endl;
    return false;
  }
}


bool K3bCdparanoiaLib::initReading( unsigned int track )
{
  if( d->device ) {
    if( track <= d->toc.count() ) {
      const K3bDevice::Track& k3bTrack = d->toc[track-1];
      if( k3bTrack.type() == K3bDevice::Track::AUDIO ) {
	return initReading( k3bTrack.firstSector().lba(), k3bTrack.lastSector().lba() );
      }
      else {
	kdDebug() << "(K3bCdparanoiaLib) Track " << track << " no audio track." << endl;
	return false;
      }
    }
    else {
      kdDebug() << "(K3bCdparanoiaLib) Track " << track << " too high." << endl;
      return false;
    }
  }
  else {
    kdDebug() << "(K3bCdparanoiaLib) initReading without initParanoia." << endl;
    return false;
  }
}


bool K3bCdparanoiaLib::initReading( long start, long end )
{
  kdDebug() << "(K3bCdparanoiaLib) initReading( " << start << ", " << end << " )" << endl;

  if( d->device ) {
    if( d->toc.firstSector().lba() <= start &&
	d->toc.lastSector().lba() >= end ) {
      d->startSector = d->currentSector = start;
      d->lastSector = end;

      // determine track number
      d->currentTrack = 1;
      while( d->toc[d->currentTrack-1].lastSector() < start )
	d->currentTrack++;

      // let the paranoia stuff point to the startSector
      paranoiaSeek( start, SEEK_SET );
      return true;
    }
    else {
      kdDebug() << "(K3bCdparanoiaLib) " << start << " and " << end << " out of range." << endl;
      return false;
    }
  }
  else {
    kdDebug() << "(K3bCdparanoiaLib) initReading without initParanoia." << endl;
    return false;
  }
}


char* K3bCdparanoiaLib::read( int* statusCode, unsigned int* track, bool littleEndian )
{
  if( d->currentSector > d->lastSector ) {
    kdDebug() << "(K3bCdparanoiaLib) finished ripping. read " 
	      << (d->currentSector - d->startSector) << " sectors." << endl
	      << "                   current sector: " << d->currentSector << endl;
    d->status = S_OK;
    if( statusCode )
      *statusCode = d->status;
    return 0;
  }

  Q_INT16* data = paranoiaRead( paranoiaCallback );

  char* charData = reinterpret_cast<char*>(data);

#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
  if( littleEndian ) {
#else
  if( !littleEndian ) {
#endif
    for( int i = 0; i < CD_FRAMESIZE_RAW-1; i+=2 ) {
      char b = charData[i];
      charData[i] = charData[i+1];
      charData[i+1] = b;
    }
  }


  if( data )
    d->status = S_OK;
  else
    d->status = S_ERROR; // We may skip this sector if we'd like...

  if( statusCode )
    *statusCode = d->status;

  if( track )
    *track = d->currentTrack;

  d->currentSector++;

  if( d->toc[d->currentTrack-1].lastSector() < d->currentSector )
    d->currentTrack++;

  return charData;
}


int K3bCdparanoiaLib::status() const
{
  return d->status;
}


const K3bDevice::Toc& K3bCdparanoiaLib::toc() const
{
  return d->toc;
}


long K3bCdparanoiaLib::rippedDataLength() const
{
  return d->lastSector - d->startSector + 1;
}
