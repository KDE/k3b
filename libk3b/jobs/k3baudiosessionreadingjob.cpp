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

#include "k3baudiosessionreadingjob.h"

#include <k3bthread.h>
#include <k3btoc.h>
#include <k3bcdparanoialib.h>
#include <k3bwavefilewriter.h>
#include <k3bglobals.h>
#include <k3bdevice.h>
#include <k3bcore.h>

#include <kdebug.h>
#include <klocale.h>

#include <unistd.h>


class K3bAudioSessionReadingJob::Private
{
public:
    Private();
    ~Private();

    int fd;
    K3bCdparanoiaLib* paranoia;
    K3bDevice::Device* device;
    K3bDevice::Toc toc;
    K3bWaveFileWriter* waveFileWriter;
    QStringList filenames;
    int paranoiaMode;
    int retries;
    bool neverSkip;
};


K3bAudioSessionReadingJob::Private::Private()
    : fd(-1),
      paranoia(0),
      waveFileWriter(0),
      paranoiaMode(0),
      retries(50),
      neverSkip(false)
{
}


K3bAudioSessionReadingJob::Private::~Private()
{
    delete waveFileWriter;
    delete paranoia;
}


K3bAudioSessionReadingJob::K3bAudioSessionReadingJob( K3bJobHandler* jh, QObject* parent )
    : K3bThreadJob( jh, parent ),
      d( new Private() )
{
}


K3bAudioSessionReadingJob::~K3bAudioSessionReadingJob()
{
    delete d;
}


void K3bAudioSessionReadingJob::setDevice( K3bDevice::Device* dev )
{
    d->device = dev;
    d->toc = K3bDevice::Toc();
}


void K3bAudioSessionReadingJob::setToc( const K3bDevice::Toc& toc )
{
    d->toc = toc;
}


void K3bAudioSessionReadingJob::writeToFd( int fd )
{
    d->fd = fd;
}

void K3bAudioSessionReadingJob::setImageNames( const QStringList& l )
{
    d->filenames = l;
    d->fd = -1;
}


void K3bAudioSessionReadingJob::setParanoiaMode( int m )
{
    d->paranoiaMode = m;
}


void K3bAudioSessionReadingJob::setReadRetries( int r )
{
    d->retries = r;
}

void K3bAudioSessionReadingJob::setNeverSkip( bool b )
{
    d->neverSkip = b;
}


void K3bAudioSessionReadingJob::start()
{
    k3bcore->blockDevice( d->device );
    K3bThreadJob::start();
}


void K3bAudioSessionReadingJob::jobFinished( bool success )
{
    k3bcore->unblockDevice( d->device );
    K3bThreadJob::jobFinished( success );
}


bool K3bAudioSessionReadingJob::run()
{
    if( !d->paranoia )
        d->paranoia = K3bCdparanoiaLib::create();

    if( !d->paranoia ) {
        emit infoMessage( i18n("Could not load libcdparanoia."), K3bJob::ERROR );
        return false;
    }

    if( d->toc.isEmpty() )
        d->toc = d->device->readToc();

    if( !d->paranoia->initParanoia( d->device, d->toc ) ) {
        emit infoMessage( i18n("Could not open device %1", d->device->blockDeviceName()),
                          K3bJob::ERROR );
        return false;
    }

    if( !d->paranoia->initReading() ) {
        emit infoMessage( i18n("Error while initializing audio ripping."), K3bJob::ERROR );
        return false;
    }

    d->device->block( true );

    // init settings
    d->paranoia->setMaxRetries( d->retries );
    d->paranoia->setParanoiaMode( d->paranoiaMode );
    d->paranoia->setNeverSkip( d->neverSkip );

    bool writeError = false;
    unsigned int trackNum = 1;
    unsigned int currentTrack = 0;
    unsigned long trackRead = 0;
    unsigned long totalRead = 0;
    unsigned int lastTrackPercent = 0;
    unsigned int lastTotalPercent = 0;
    bool newTrack = true;
    int status = 0;
    char* buffer = 0;
    while( !canceled() && (buffer = d->paranoia->read( &status, &trackNum, d->fd == -1 /*when writing to a wav be want little endian */ )) ) {

        if( currentTrack != trackNum ) {
            emit nextTrack( trackNum, d->paranoia->toc().count() );
            trackRead = 0;
            lastTrackPercent = 0;

            currentTrack = trackNum;
            newTrack = true;
        }

        if( d->fd > 0 ) {
            if( ::write( d->fd, buffer, CD_FRAMESIZE_RAW ) != CD_FRAMESIZE_RAW ) {
                kDebug() << "(K3bAudioSessionCopyJob::WorkThread) error while writing to fd " << d->fd;
                writeError = true;
                break;
            }
        }
        else {
            if( newTrack ) {
                newTrack = false;

                if( !d->waveFileWriter )
                    d->waveFileWriter = new K3bWaveFileWriter();

                if( d->filenames.count() < ( int )currentTrack ) {
                    kDebug() << "(K3bAudioSessionCopyJob) not enough image filenames given: " << currentTrack;
                    writeError = true;
                    break;
                }

                if( !d->waveFileWriter->open( d->filenames[currentTrack-1] ) ) {
                    emit infoMessage( i18n("Unable to open '%1' for writing.", d->filenames[currentTrack-1]), K3bJob::ERROR );
                    writeError = true;
                    break;
                }
            }

            d->waveFileWriter->write( buffer,
                                      CD_FRAMESIZE_RAW,
                                      K3bWaveFileWriter::LittleEndian );
        }

        trackRead++;
        totalRead++;

        unsigned int trackPercent = 100 * trackRead / d->toc[currentTrack-1].length().lba();
        if( trackPercent > lastTrackPercent ) {
            lastTrackPercent = trackPercent;
            emit subPercent( lastTrackPercent );
        }
        unsigned int totalPercent = 100 * totalRead / d->paranoia->rippedDataLength();
        if( totalPercent > lastTotalPercent ) {
            lastTotalPercent = totalPercent;
            emit percent( lastTotalPercent );
        }
    }

    if( d->waveFileWriter )
        d->waveFileWriter->close();

    d->paranoia->close();

    d->device->block( false );

    if( status != K3bCdparanoiaLib::S_OK ) {
        emit infoMessage( i18n("Unrecoverable error while ripping track %1.", trackNum), K3bJob::ERROR );
        return false;
    }

    return !writeError && !canceled();
}

#include "k3baudiosessionreadingjob.moc"
