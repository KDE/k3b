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

#include "k3bdatatrackreader.h"

#include <k3blibdvdcss.h>
#include <k3bdevice.h>
#include <k3bdeviceglobals.h>
#include <k3btrack.h>
#include <k3bthread.h>
#include <k3bcore.h>

#include <klocale.h>
#include <kdebug.h>

#include <qfile.h>

#include <unistd.h>



// FIXME: determine max DMA buffer size
static int s_bufferSizeSectors = 10;


class K3bDataTrackReader::Private
{
public:
    Private();
    ~Private();

    bool ignoreReadErrors;
    bool noCorrection;
    int retries;
    K3bDevice::Device* device;
    K3b::Msf firstSector;
    K3b::Msf lastSector;
    K3b::Msf nextReadSector;
    int fd;
    QString imagePath;
    int sectorSize;
    bool useLibdvdcss;
    K3bLibDvdCss* libcss;

    int oldErrorRecoveryMode;

    int errorSectorCount;

    int usedSectorSize;
};


K3bDataTrackReader::Private::Private()
    : ignoreReadErrors(false),
      noCorrection(false),
      retries(10),
      device(0),
      fd(-1),
      libcss(0)
{
}


K3bDataTrackReader::Private::~Private()
{
    delete libcss;
}




K3bDataTrackReader::K3bDataTrackReader( K3bJobHandler* jh, QObject* parent )
    : K3bThreadJob( jh, parent ),
      d( new Private() )
{
}


K3bDataTrackReader::~K3bDataTrackReader()
{
    delete d;
}


void K3bDataTrackReader::setDevice( K3bDevice::Device* dev )
{
    d->device = dev;
}


void K3bDataTrackReader::setSectorRange( const K3b::Msf& start, const K3b::Msf& end )
{
    d->firstSector = start;
    d->lastSector = end;
}


void K3bDataTrackReader::setRetries( int r )
{
    d->retries = r;
}


void K3bDataTrackReader::setIgnoreErrors( bool b )
{
    d->ignoreReadErrors = b;
}


void K3bDataTrackReader::setNoCorrection( bool b )
{
    d->noCorrection = b;
}


void K3bDataTrackReader::writeToFd( int fd )
{
    d->fd = fd;
}


void K3bDataTrackReader::setImagePath( const QString& p )
{
    d->imagePath = p;
    d->fd = -1;
}


void K3bDataTrackReader::setSectorSize( SectorSize size )
{
    d->sectorSize = size;
}


bool K3bDataTrackReader::run()
{
    if( !d->device->open() ) {
        emit infoMessage( i18n("Could not open device %1",d->device->blockDeviceName()), K3bJob::ERROR );
        return false;
    }

    // 1. determine sector size by checking the first sectors mode
    //    if impossible or MODE2 (mode2 formless) finish(false)

    d->useLibdvdcss = false;
    d->usedSectorSize = d->sectorSize;

    int mediaType = d->device->mediaType();

    if( K3bDevice::isDvdMedia( mediaType ) ) {
        d->usedSectorSize = MODE1;

        //
        // In case of an encrypted VideoDVD we read with libdvdcss which takes care of decrypting the vobs
        //
        if( d->device->copyrightProtectionSystemType() == K3bDevice::COPYRIGHT_PROTECTION_CSS ) {

            // close the device for libdvdcss
            d->device->close();

            kDebug() << "(K3bDataTrackReader::WorkThread) found encrypted dvd. using libdvdcss.";

            // open the libdvdcss stuff
            if( !d->libcss )
                d->libcss = K3bLibDvdCss::create();
            if( !d->libcss ) {
                emit infoMessage( i18n("Unable to open libdvdcss."), K3bJob::ERROR );
                return false;
            }

            if( !d->libcss->open(d->device) ) {
                emit infoMessage( i18n("Could not open device %1",d->device->blockDeviceName()), K3bJob::ERROR );
                return false;
            }

            emit infoMessage( i18n("Retrieving all CSS keys. This might take a while."), K3bJob::INFO );
            if( !d->libcss->crackAllKeys() ) {
                d->libcss->close();
                emit infoMessage( i18n("Failed to retrieve all CSS keys."), K3bJob::ERROR );
                emit infoMessage( i18n("Video DVD decryption failed."), K3bJob::ERROR );
                return false;
            }

            d->useLibdvdcss = true;
        }
    }
    else if ( K3bDevice::isBdMedia( mediaType ) ) {
        d->usedSectorSize = MODE1;
    }
    else {
        if( d->usedSectorSize == AUTO ) {
            switch( d->device->getDataMode( d->firstSector ) ) {
            case K3bDevice::Track::MODE1:
            case K3bDevice::Track::DVD:
                d->usedSectorSize = MODE1;
                break;
            case K3bDevice::Track::XA_FORM1:
                d->usedSectorSize = MODE2FORM1;
                break;
            case K3bDevice::Track::XA_FORM2:
                d->usedSectorSize = MODE2FORM2;
                break;
            case K3bDevice::Track::MODE2:
                emit infoMessage( i18n("No support for reading formless Mode2 sectors."), K3bJob::ERROR );
            default:
                emit infoMessage( i18n("Unsupported sector type."), K3bJob::ERROR );
                d->device->close();
                return false;
            }
        }
    }

    emit infoMessage( i18n("Reading with sector size %1.",d->usedSectorSize), K3bJob::INFO );
    emit debuggingOutput( "K3bDataTrackReader",
                          QString("reading sectors %1 to %2 with sector size %3. Length: %4 sectors, %5 bytes.")
                          .arg( d->firstSector.lba() )
                          .arg( d->lastSector.lba() )
                          .arg( d->usedSectorSize )
                          .arg( d->lastSector.lba() - d->firstSector.lba() + 1 )
                          .arg( quint64(d->usedSectorSize) * (quint64)(d->lastSector.lba() - d->firstSector.lba() + 1) ) );

    QFile file;
    if( d->fd == -1 ) {
        file.setName( d->imagePath );
        if( !file.open( QIODevice::WriteOnly ) ) {
            d->device->close();
            if( d->useLibdvdcss )
                d->libcss->close();
            emit infoMessage( i18n("Unable to open '%1' for writing.",d->imagePath), K3bJob::ERROR );
            return false;
        }
    }

    k3bcore->blockDevice( d->device );
    d->device->block( true );

    //
    // set the error recovery mode to 0x21 or 0x20 depending on d->ignoreReadErrors
    // TODO: should we also set RC=1 in d->ignoreReadErrors mode (0x11 because TB is ignored)
    //
    setErrorRecovery( d->device, d->noCorrection ? 0x21 : 0x20 );

    //
    // Let the drive determine the optimal reading speed
    //
    d->device->setSpeed( 0xffff, 0xffff );

    s_bufferSizeSectors = 128;
    unsigned char* buffer = new unsigned char[d->usedSectorSize*s_bufferSizeSectors];
    while( s_bufferSizeSectors > 0 && read( buffer, d->firstSector.lba(), s_bufferSizeSectors ) < 0 ) {
        kDebug() << "(K3bDataTrackReader) determine max read sectors: "
                 << s_bufferSizeSectors << " too high." << endl;
        s_bufferSizeSectors--;
    }
    kDebug() << "(K3bDataTrackReader) determine max read sectors: "
             << s_bufferSizeSectors << " is max." << endl;

    //    s_bufferSizeSectors = K3bDevice::determineMaxReadingBufferSize( d->device, d->firstSector );
    if( s_bufferSizeSectors <= 0 ) {
        emit infoMessage( i18n("Error while reading sector %1.",d->firstSector.lba()), K3bJob::ERROR );
        d->device->block( false );
        k3bcore->unblockDevice( d->device );
        return false;
    }

    kDebug() << "(K3bDataTrackReader) using buffer size of " << s_bufferSizeSectors << " blocks.";
    emit debuggingOutput( "K3bDataTrackReader", QString("using buffer size of %1 blocks.").arg( s_bufferSizeSectors ) );

    // 2. get it on
    K3b::Msf currentSector = d->firstSector;
    K3b::Msf totalReadSectors;
    d->nextReadSector = 0;
    d->errorSectorCount = 0;
    bool writeError = false;
    bool readError = false;
    int lastPercent = 0;
    unsigned long lastReadMb = 0;
    int bufferLen = s_bufferSizeSectors*d->usedSectorSize;
    while( !canceled() && currentSector <= d->lastSector ) {

        int maxReadSectors = qMin( bufferLen/d->usedSectorSize, d->lastSector.lba()-currentSector.lba()+1 );

        int readSectors = read( buffer,
                                currentSector.lba(),
                                maxReadSectors );
        if( readSectors < 0 ) {
            if( !retryRead( buffer,
                            currentSector.lba(),
                            maxReadSectors ) ) {
                readError = true;
                break;
            }
            else
                readSectors = maxReadSectors;
        }

        totalReadSectors += readSectors;

        int readBytes = readSectors * d->usedSectorSize;

        if( d->fd != -1 ) {
            if( ::write( d->fd, reinterpret_cast<void*>(buffer), readBytes ) != readBytes ) {
                kDebug() << "(K3bDataTrackReader::WorkThread) error while writing to fd " << d->fd
                         << " current sector: " << (currentSector.lba()-d->firstSector.lba()) << endl;
                emit debuggingOutput( "K3bDataTrackReader",
                                      QString("Error while writing to fd %1. Current sector is %2.")
                                      .arg(d->fd).arg(currentSector.lba()-d->firstSector.lba()) );
                writeError = true;
                break;
            }
        }
        else {
            if( file.write( reinterpret_cast<char*>(buffer), readBytes ) != readBytes ) {
                kDebug() << "(K3bDataTrackReader::WorkThread) error while writing to file " << d->imagePath
                         << " current sector: " << (currentSector.lba()-d->firstSector.lba()) << endl;
                emit debuggingOutput( "K3bDataTrackReader",
                                      QString("Error while writing to file %1. Current sector is %2.")
                                      .arg(d->imagePath).arg(currentSector.lba()-d->firstSector.lba()) );
                writeError = true;
                break;
            }
        }

        currentSector += readSectors;

        int currentPercent = 100 * (currentSector.lba() - d->firstSector.lba() + 1 ) /
                             (d->lastSector.lba() - d->firstSector.lba() + 1 );

        if( currentPercent > lastPercent ) {
            lastPercent = currentPercent;
            emit percent( currentPercent );
        }

        unsigned long readMb = (currentSector.lba() - d->firstSector.lba() + 1) / 512;
        if( readMb > lastReadMb ) {
            lastReadMb = readMb;
            emit processedSize( readMb, ( d->lastSector.lba() - d->firstSector.lba() + 1 ) / 512 );
        }
    }

    if( d->errorSectorCount > 0 )
        emit infoMessage( i18np("Ignored %n erroneous sector.", "Ignored a total of %n erroneous sectors.", d->errorSectorCount ),
                          K3bJob::ERROR );

    // reset the error recovery mode
    setErrorRecovery( d->device, d->oldErrorRecoveryMode );

    d->device->block( false );
    k3bcore->unblockDevice( d->device );

    // cleanup
    if( d->useLibdvdcss )
        d->libcss->close();
    d->device->close();
    delete [] buffer;

    emit debuggingOutput( "K3bDataTrackReader",
                          QString("Read a total of %1 sectors (%2 bytes)")
                          .arg(totalReadSectors.lba())
                          .arg((quint64)totalReadSectors.lba()*(quint64)d->usedSectorSize) );

    return( !canceled() && !writeError && !readError );
}


int K3bDataTrackReader::read( unsigned char* buffer, unsigned long sector, unsigned int len )
{
    //
    // Encrypted DVD reading with libdvdcss
    //
    if( d->useLibdvdcss ) {
        return d->libcss->readWrapped( reinterpret_cast<void*>(buffer), sector, len );
    }

    //
    // Standard reading
    //
    else {
        bool success = false;
        //      setErrorRecovery( d->device, d->ignoreReadErrors ? 0x21 : 0x20 );
        if( d->usedSectorSize == 2048 )
            success = d->device->read10( buffer, len*2048, sector, len );
        else
            success = d->device->readCd( buffer,
                                        len*d->usedSectorSize,
                                        0,     // all sector types
                                        false, // no dap
                                        sector,
                                        len,
                                        false, // no sync
                                        false, // no header
                                        d->usedSectorSize != MODE1,  // subheader
                                        true,  // user data
                                        false, // no edc/ecc
                                        0,     // no c2 error info... FIXME: should we check this??
                                        0      // no subchannel data
                );

        if( success )
            return len;
        else
            return -1;
    }
}


// here we read every single sector for itself to find the troubleing ones
bool K3bDataTrackReader::retryRead( unsigned char* buffer, unsigned long startSector, unsigned int len )
{
    emit debuggingOutput( "K3bDataTrackReader", QString( "Problem while reading. Retrying from sector %1.").arg(startSector) );
    emit infoMessage( i18n("Problem while reading. Retrying from sector %1.",startSector), K3bJob::WARNING );

    int sectorsRead = -1;
    bool success = true;
    for( unsigned long sector = startSector; sector < startSector+len; ++sector ) {
        int retry = d->retries;
        while( !canceled() && retry && (sectorsRead = read( &buffer[( sector - startSector ) * d->usedSectorSize], sector, 1 )) < 0 )
            --retry;

        success = ( sectorsRead > 0 );

        if( canceled() )
            return false;

        if( !success ) {
            if( d->ignoreReadErrors ) {
                emit infoMessage( i18n("Ignoring read error in sector %1.",sector), K3bJob::ERROR );
                emit debuggingOutput( "K3bDataTrackReader", QString( "Ignoring read error in sector %1.").arg(sector) );

                ++d->errorSectorCount;
                //	  ::memset( &buffer[i], 0, 1 );
                success = true;
            }
            else {
                emit infoMessage( i18n("Error while reading sector %1.",sector), K3bJob::ERROR );
                emit debuggingOutput( "K3bDataTrackReader", QString( "Read error in sector %1.").arg(sector) );
                break;
            }
        }
    }

    return success;
}


bool K3bDataTrackReader::setErrorRecovery( K3bDevice::Device* dev, int code )
{
    unsigned char* data = 0;
    unsigned int dataLen = 0;
    if( !dev->modeSense( &data, dataLen, 0x01 ) )
        return false;

    // in MMC1 the page has 8 bytes (12 in MMC4 but we only need the first 3 anyway)
    if( dataLen < 8+8 ) {
        kDebug() << "(K3bDataTrackReader) modepage 0x01 data too small: " << dataLen;
        delete [] data;
        return false;
    }

    d->oldErrorRecoveryMode = data[8+2];
    data[8+2] = code;

    if( d->oldErrorRecoveryMode != code )
        kDebug() << "(K3bDataTrackReader) changing data recovery mode from " << d->oldErrorRecoveryMode << " to " << code;

    bool success = dev->modeSelect( data, dataLen, true, false );

    delete [] data;

    return success;
}
