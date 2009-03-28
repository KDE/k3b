/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config-k3b.h>

#include "k3bcdparanoialib.h"

#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3bmsf.h>

#include <kdebug.h>

// TODO replace dlopen/dlsym/dlclose by platform independent QLibrary 
#ifdef Q_OS_WIN32

#include <QLibrary>

inline void *dlopen(char *fileName, int b)
{
	static QLibrary lib;
	lib.setFileName(fileName);
	if (lib.isLoaded())
		return &lib;
	if (lib.load())
		return &lib;
	return 0;
}

inline void *dlsym(void *a, char *b)
{
	QLibrary *lib = (QLibrary *)a;
	return lib->resolve(b);
}

inline void dlclose(void *a)
{
	QLibrary *lib = (QLibrary *)a;
	lib->unload();
}

#define RTLD_GLOBAL 0
#define RTLD_NOW 0
#define RTLD_LAZY 0

typedef short int int16_t;
#else
#include <dlfcn.h>
#endif

#include <qfile.h>
#include <qmutex.h>

#ifdef Q_OS_WIN32
#define LIBCDIO_CDDA "cdio_cdda.dll"
#define LIBCDIO_PARANOIA "cdio_paranoia.dll"
#else
#define LIBCDIO_CDDA "libcdio_cdda.so"
#define LIBCDIO_PARANOIA "libcdio_paranoia.so.0"
#endif

static bool s_haveLibCdio = false;


void* K3b::CdparanoiaLib::s_libInterface = 0;
void* K3b::CdparanoiaLib::s_libParanoia = 0;
int K3b::CdparanoiaLib::s_counter = 0;


#define CDDA_IDENTIFY          s_haveLibCdio ? "cdio_cddap_identify" : "cdda_identify"
#define CDDA_CLOSE             s_haveLibCdio ? "cdio_cddap_close" : "cdda_close"
#define CDDA_OPEN              s_haveLibCdio ? "cdio_cddap_open" : "cdda_open"
#define CDDA_TRACK_FIRSTSECTOR s_haveLibCdio ? "cdio_cddap_track_firstsector" : "cdda_track_firstsector"
#define CDDA_TRACK_LASTSECTOR  s_haveLibCdio ? "cdio_cddap_track_lastsector" : "cdda_track_lastsector"
#define CDDA_VERBOSE_SET       s_haveLibCdio ? "cdio_cddap_verbose_set" : "cdda_verbose_set"
#define CDDA_DISC_FIRSTSECTOR  s_haveLibCdio ? "cdio_cddap_disc_firstsector" : "cdda_disc_firstsector"

#define PARANOIA_INIT          s_haveLibCdio ? "cdio_paranoia_init" : "paranoia_init"
#define PARANOIA_FREE          s_haveLibCdio ? "cdio_paranoia_free" : "paranoia_free"
#define PARANOIA_MODESET       s_haveLibCdio ? "cdio_paranoia_modeset" : "paranoia_modeset"
#define PARANOIA_SEEK          s_haveLibCdio ? "cdio_paranoia_seek" : "paranoia_seek"
#define PARANOIA_READ_LIMITED  s_haveLibCdio ? "cdio_paranoia_read_limited" : "paranoia_read_limited"


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



namespace K3b {
    /**
     * Internal class used by K3b::CdparanoiaLib
     */
    class CdparanoiaLibData
    {
    public:
        CdparanoiaLibData( K3b::Device::Device* dev )
            : m_device(dev),
              m_drive(0),
              m_paranoia(0),
              m_currentSector(0) {
            s_dataMap.insert( dev, this );
        }

        ~CdparanoiaLibData() {
            paranoiaFree();

            s_dataMap.remove( m_device );
        }

        K3b::Device::Device* device() const { return m_device; }
        void paranoiaModeSet( int );
        bool paranoiaInit();
        void paranoiaFree();
        int16_t* paranoiaRead( void(*callback)(long,int), int maxRetries );
        long paranoiaSeek( long, int );
        long firstSector( int );
        long lastSector( int );
        long sector() const { return m_currentSector; }

        static K3b::CdparanoiaLibData* data( K3b::Device::Device* dev ) {
            QMap<K3b::Device::Device*, K3b::CdparanoiaLibData*>::const_iterator it = s_dataMap.constFind( dev );
            if( it == s_dataMap.constEnd() )
                return new K3b::CdparanoiaLibData( dev );
            else
                return *it;
        }

        static void freeAll() {
            // clean up all K3b::CdparanoiaLibData instances
            qDeleteAll( s_dataMap );
            s_dataMap.clear();
        }

    private:
        //
        // We have exactly one instance of K3b::CdparanoiaLibData per device
        //
        static QMap<K3b::Device::Device*, K3b::CdparanoiaLibData*> s_dataMap;

        K3b::Device::Device* m_device;

        cdrom_drive* m_drive;
        cdrom_paranoia* m_paranoia;

        long m_currentSector;

        QMutex mutex;
    };
}

QMap<K3b::Device::Device*, K3b::CdparanoiaLibData*> K3b::CdparanoiaLibData::s_dataMap;

bool K3b::CdparanoiaLibData::paranoiaInit()
{
    mutex.lock();

    if( m_drive )
        paranoiaFree();

    // since we use cdparanoia to open the device it is important to close
    // the device here
    m_device->close();

    m_drive = cdda_cdda_identify( QFile::encodeName(m_device->blockDeviceName()), 0, 0 );
    if( m_drive == 0 ) {
        mutex.unlock();
        return false;
    }

    //  cdda_cdda_verbose_set( m_drive, 1, 1 );

    cdda_cdda_open( m_drive );
    m_paranoia = cdda_paranoia_init( m_drive );
    if( m_paranoia == 0 ) {
        mutex.unlock();
        paranoiaFree();
        return false;
    }

    m_currentSector = 0;

    mutex.unlock();

    return true;
}


void K3b::CdparanoiaLibData::paranoiaFree()
{
    mutex.lock();

    if( m_paranoia ) {
        cdda_paranoia_free( m_paranoia );
        m_paranoia = 0;
    }
    if( m_drive ) {
        cdda_cdda_close( m_drive );
        m_drive = 0;
    }

    mutex.unlock();
}


void K3b::CdparanoiaLibData::paranoiaModeSet( int mode )
{
    mutex.lock();
    cdda_paranoia_modeset( m_paranoia, mode );
    mutex.unlock();
}


int16_t* K3b::CdparanoiaLibData::paranoiaRead( void(*callback)(long,int), int maxRetries )
{
    if( m_paranoia ) {
        mutex.lock();
        int16_t* data = cdda_paranoia_read_limited( m_paranoia, callback, maxRetries );
        if( data )
            m_currentSector++;
        mutex.unlock();
        return data;
    }
    else
        return 0;
}


long K3b::CdparanoiaLibData::firstSector( int track )
{
    if( m_drive ) {
        mutex.lock();
        long sector = cdda_cdda_track_firstsector( m_drive, track );
        mutex.unlock();
        return sector;
    }
    else
        return -1;
}

long K3b::CdparanoiaLibData::lastSector( int track )
{
    if( m_drive ) {
        mutex.lock();
        long sector = cdda_cdda_track_lastsector(m_drive, track );
        mutex.unlock();
        return sector;
    }
    else
        return -1;
}


long K3b::CdparanoiaLibData::paranoiaSeek( long sector, int mode )
{
    if( m_paranoia ) {
        mutex.lock();
        m_currentSector = cdda_paranoia_seek( m_paranoia, sector, mode );
        mutex.unlock();
        return m_currentSector;
    }
    else
        return -1;
}



class K3b::CdparanoiaLib::Private
{
public:
    Private()
        : device(0),
          currentSector(0),
          startSector(0),
          lastSector(0),
          status(S_OK),
          paranoiaLevel(0),
          neverSkip(true),
          maxRetries(5),
          data(0) {
    }

    ~Private() {
    }

    void updateParanoiaMode() {
        // from cdrdao 1.1.7
        int paranoiaMode = PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP;

        switch( paranoiaLevel ) {
        case 0:
            paranoiaMode = PARANOIA_MODE_DISABLE;
            break;

        case 1:
            paranoiaMode |= PARANOIA_MODE_OVERLAP;
            paranoiaMode &= ~PARANOIA_MODE_VERIFY;
            break;

        case 2:
            paranoiaMode &= ~(PARANOIA_MODE_SCRATCH|PARANOIA_MODE_REPAIR);
            break;
        }

        if( neverSkip )
            paranoiaMode |= PARANOIA_MODE_NEVERSKIP;

        data->paranoiaModeSet( paranoiaMode );
    }

    // high-level api
    K3b::Device::Device* device;
    K3b::Device::Toc toc;
    long currentSector;
    long startSector;
    long lastSector;
    int status;
    unsigned int currentTrack;
    int paranoiaLevel;
    bool neverSkip;
    int maxRetries;

    K3b::CdparanoiaLibData* data;
};


K3b::CdparanoiaLib::CdparanoiaLib()
{
    d = new Private();
    s_counter++;
}


K3b::CdparanoiaLib::~CdparanoiaLib()
{
    delete d;
    s_counter--;
    if( s_counter == 0 ) {
        K3b::CdparanoiaLibData::freeAll();

        // cleanup the dynamically loaded lib
        dlclose( s_libInterface );
        dlclose( s_libParanoia );
        s_libInterface = 0;
        s_libParanoia = 0;
    }
}


bool K3b::CdparanoiaLib::load()
{
    cdda_cdda_identify = (cdrom_drive* (*) (const char*, int, char**))dlsym( s_libInterface, CDDA_IDENTIFY );
    cdda_cdda_open = (int (*) (cdrom_drive*))dlsym( s_libInterface, CDDA_OPEN );
    cdda_cdda_close = (int (*) (cdrom_drive*))dlsym( s_libInterface, CDDA_CLOSE );
    cdda_cdda_track_firstsector = (long (*)(cdrom_drive*, int))dlsym( s_libInterface, CDDA_TRACK_FIRSTSECTOR );
    cdda_cdda_track_lastsector = (long (*)(cdrom_drive*, int))dlsym( s_libInterface, CDDA_TRACK_LASTSECTOR );
    cdda_cdda_verbose_set = (void (*)(cdrom_drive *d,int err_action, int mes_action))dlsym( s_libInterface, CDDA_VERBOSE_SET );
    cdda_cdda_disc_firstsector = (long (*)(cdrom_drive *d))dlsym( s_libInterface, CDDA_DISC_FIRSTSECTOR );

    cdda_paranoia_init = (cdrom_paranoia* (*)(cdrom_drive*))dlsym( s_libParanoia, PARANOIA_INIT );
    cdda_paranoia_free = (void (*)(cdrom_paranoia *p))dlsym( s_libParanoia, PARANOIA_FREE );
    cdda_paranoia_modeset = (void (*)(cdrom_paranoia *p, int mode))dlsym( s_libParanoia, PARANOIA_MODESET );
    cdda_paranoia_read_limited = (int16_t* (*)(cdrom_paranoia *p, void(*callback)(long,int), int))dlsym( s_libParanoia, PARANOIA_READ_LIMITED );
    cdda_paranoia_seek = (long (*)(cdrom_paranoia *p,long seek,int mode))dlsym( s_libParanoia, PARANOIA_SEEK );

    // check if all symbols could be resoled
    if( cdda_cdda_identify == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'cdda_identify'";
        return false;
    }
    if( cdda_cdda_open == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'cdda_open'";
        return false;
    }
    if( cdda_cdda_close == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'cdda_close'";
        return false;
    }
    if( cdda_cdda_track_firstsector == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'cdda_track_firstsector'";
        return false;
    }
    if( cdda_cdda_track_lastsector == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'cdda_track_lastsector'";
        return false;
    }
    if( cdda_cdda_disc_firstsector == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'cdda_disc_firstsector'";
        return false;
    }
    if( cdda_cdda_verbose_set == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'cdda_verbose_set'";
        return false;
    }

    if( cdda_paranoia_init == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'paranoia_init'";
        return false;
    }
    if( cdda_paranoia_free == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'paranoia_free'";
        return false;
    }
    if( cdda_paranoia_modeset == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'paranoia_modeset'";
        return false;
    }
    if( cdda_paranoia_read_limited == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'paranoia_read_limited'";
        return false;
    }
    if( cdda_paranoia_seek == 0 ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve 'paranoia_seek'";
        return false;
    }

    return true;
}



K3b::CdparanoiaLib* K3b::CdparanoiaLib::create()
{
    // check if libcdda_interface is avalilable
    if( s_libInterface == 0 ) {
        s_haveLibCdio = false;
#ifndef Q_OS_WIN32
        s_libInterface = dlopen( "libcdda_interface.so.0", RTLD_NOW|RTLD_GLOBAL );

        // try the redhat & Co. location
        if( s_libInterface == 0 )
            s_libInterface = dlopen( "cdda/libcdda_interface.so.0", RTLD_NOW|RTLD_GLOBAL );
#endif
        // try the new cdio lib
        if( s_libInterface == 0 ) {
            s_libInterface = dlopen( LIBCDIO_CDDA, RTLD_NOW|RTLD_GLOBAL );
            s_haveLibCdio = true;
        }

        if( s_libInterface == 0 ) {
            kDebug() << "(K3b::CdparanoiaLib) Error while loading libcdda_interface. ";
            return 0;
        }

#ifndef Q_OS_WIN32
        s_libParanoia = dlopen( "libcdda_paranoia.so.0", RTLD_NOW );

        // try the redhat & Co. location
        if( s_libParanoia == 0 )
            s_libParanoia = dlopen( "cdda/libcdda_paranoia.so.0", RTLD_NOW );
#endif
        // try the new cdio lib
        if( s_haveLibCdio && s_libParanoia == 0 )
            s_libParanoia = dlopen( LIBCDIO_PARANOIA, RTLD_NOW );

        if( s_libParanoia == 0 ) {
            kDebug() << "(K3b::CdparanoiaLib) Error while loading libcdda_paranoia. ";
            dlclose( s_libInterface );
            s_libInterface = 0;
            return 0;
        }
    }

    K3b::CdparanoiaLib* lib = new K3b::CdparanoiaLib();
    if( !lib->load() ) {
        kDebug() << "(K3b::CdparanoiaLib) Error: could not resolve all symbols!";
        delete lib;
        return 0;
    }
    return lib;
}


bool K3b::CdparanoiaLib::initParanoia( K3b::Device::Device* dev, const K3b::Device::Toc& toc )
{
    if( !dev ) {
        kError() << "(K3b::CdparanoiaLib::initParanoia) dev = 0!" << endl;
        return false;
    }

    close();

    d->device = dev;
    d->toc = toc;
    if( d->toc.isEmpty() ) {
        kDebug() << "(K3b::CdparanoiaLib) empty toc.";
        cleanup();
        return false;
    }

    if( d->toc.contentType() == K3b::Device::DATA ) {
        kDebug() << "(K3b::CdparanoiaLib) No audio tracks found.";
        cleanup();
        return false;
    }

    //
    // Get the appropriate data instance for this device
    //
    d->data = K3b::CdparanoiaLibData::data( dev );

    if( d->data->paranoiaInit() ) {
        d->startSector = d->currentSector = d->lastSector = 0;

        return true;
    }
    else {
        cleanup();
        return false;
    }
}


bool K3b::CdparanoiaLib::initParanoia( K3b::Device::Device* dev )
{
    return initParanoia( dev, dev->readToc() );
}


void K3b::CdparanoiaLib::close()
{
    cleanup();
}


void K3b::CdparanoiaLib::cleanup()
{
    if( d->data )
        d->data->paranoiaFree();
    d->device = 0;
    d->currentSector = 0;
}


bool K3b::CdparanoiaLib::initReading()
{
    if( d->device ) {
        // find first audio track
        K3b::Device::Toc::const_iterator trackIt = d->toc.constBegin();
        while( (*trackIt).type() != K3b::Device::Track::TYPE_AUDIO ) {
            ++trackIt;
        }

        long start = (*trackIt).firstSector().lba();

        // find last audio track
        while( trackIt != d->toc.constEnd() && (*trackIt).type() == K3b::Device::Track::TYPE_AUDIO )
            ++trackIt;
        --trackIt;

        long end = (*trackIt).lastSector().lba();

        return initReading( start, end );
    }
    else {
        kDebug() << "(K3b::CdparanoiaLib) initReading without initParanoia.";
        return false;
    }
}


bool K3b::CdparanoiaLib::initReading( int track )
{
    if( d->device ) {
        if( track <= d->toc.count() ) {
            const K3b::Device::Track& k3bTrack = d->toc[track-1];
            if( k3bTrack.type() == K3b::Device::Track::TYPE_AUDIO ) {
                return initReading( k3bTrack.firstSector().lba(), k3bTrack.lastSector().lba() );
            }
            else {
                kDebug() << "(K3b::CdparanoiaLib) Track " << track << " no audio track.";
                return false;
            }
        }
        else {
            kDebug() << "(K3b::CdparanoiaLib) Track " << track << " too high.";
            return false;
        }
    }
    else {
        kDebug() << "(K3b::CdparanoiaLib) initReading without initParanoia.";
        return false;
    }
}


bool K3b::CdparanoiaLib::initReading( long start, long end )
{
    kDebug() << "(K3b::CdparanoiaLib) initReading( " << start << ", " << end << " )";

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
            d->data->paranoiaSeek( start, SEEK_SET );
            return true;
        }
        else {
            kDebug() << "(K3b::CdparanoiaLib) " << start << " and " << end << " out of range.";
            return false;
        }
    }
    else {
        kDebug() << "(K3b::CdparanoiaLib) initReading without initParanoia.";
        return false;
    }
}


char* K3b::CdparanoiaLib::read( int* statusCode, unsigned int* track, bool littleEndian )
{
    if( d->currentSector > d->lastSector ) {
        kDebug() << "(K3b::CdparanoiaLib) finished ripping. read "
                 << (d->currentSector - d->startSector) << " sectors." << endl
                 << "                   current sector: " << d->currentSector << endl;
        d->status = S_OK;
        if( statusCode )
            *statusCode = d->status;
        return 0;
    }

    if( d->currentSector != d->data->sector() ) {
        kDebug() << "(K3b::CdparanoiaLib) need to seek before read. Looks as if we are reusing the paranoia instance.";
        if( !d->data->paranoiaSeek( d->currentSector, SEEK_SET ) )
            return 0;
    }

    //
    // The paranoia data could have been used by someone else before
    // and setting the paranoia mode is fast
    //
    d->updateParanoiaMode();

    qint16* data = d->data->paranoiaRead( paranoiaCallback, d->maxRetries );

    char* charData = reinterpret_cast<char*>(data);

    if(
#ifndef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
        !
#endif
        littleEndian ) {
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


int K3b::CdparanoiaLib::status() const
{
    return d->status;
}


const K3b::Device::Toc& K3b::CdparanoiaLib::toc() const
{
    return d->toc;
}


long K3b::CdparanoiaLib::rippedDataLength() const
{
    return d->lastSector - d->startSector + 1;
}


void K3b::CdparanoiaLib::setParanoiaMode( int m )
{
    d->paranoiaLevel = m;
}


void K3b::CdparanoiaLib::setNeverSkip( bool b )
{
    d->neverSkip = b;
}


void K3b::CdparanoiaLib::setMaxRetries( int r )
{
    d->maxRetries = r;
}
