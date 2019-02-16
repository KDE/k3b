/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009-2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3baudioripjob.h"

#include "k3bcdparanoialib.h"
#include "k3bcore.h"
#include "k3bdevice.h"
#include "k3btoc.h"
#include "k3btrack.h"

#include <KCddb/Cdinfo>

#include <KLocalizedString>


namespace K3b {


class AudioRipJob::Private
{
public:
    Private()
        : paranoiaRetries(5),
          neverSkip(false),
          paranoiaLib(0),
          device(0),
          useIndex0(false) {
    }
    int paranoiaMode;
    int paranoiaRetries;
    int neverSkip;

    CdparanoiaLib* paranoiaLib;

    Device::Toc toc;
    Device::Device* device;

    bool useIndex0;
};


namespace {

class AudioCdReader : public QIODevice
{
public:
    AudioCdReader( int trackIndex, AudioRipJob::Private* priv, QObject* parent = 0 );
    bool open( OpenMode mode ) override;
    bool isSequential() const override;
    qint64 size() const override;

protected:
    qint64 writeData( const char* data, qint64 len ) override;
    qint64 readData( char* data, qint64 maxlen ) override;

private:
    int m_trackIndex;
    AudioRipJob::Private* d;
};


AudioCdReader::AudioCdReader( int trackIndex, AudioRipJob::Private* priv, QObject* parent )
    : QIODevice( parent ),
      m_trackIndex( trackIndex ),
      d( priv )
{
}


bool AudioCdReader::open( OpenMode mode )
{
    if( !mode.testFlag( QIODevice::WriteOnly ) ) {

        const Device::Track& tt = d->toc[m_trackIndex-1];

        long endSec = ( (d->useIndex0 && tt.index0() > 0)
                        ? tt.firstSector().lba() + tt.index0().lba() - 1
                        : tt.lastSector().lba() );

        if( d->paranoiaLib->initReading( tt.firstSector().lba(), endSec ) ) {
            return QIODevice::open( mode );
        }
        else {
            setErrorString( i18n("Error while initializing audio ripping.") );
            return false;
        }
    }
    else {
        return false;
    }
}


bool AudioCdReader::isSequential() const
{
    return false;
}


qint64 AudioCdReader::size() const
{
    return d->toc[m_trackIndex-1].length().audioBytes();
}


qint64 AudioCdReader::writeData( const char* /*data*/, qint64 /*len*/ )
{
    return -1;
}


qint64 AudioCdReader::readData( char* data, qint64 /*maxlen*/ )
{
    int status = 0;
    char* buf = d->paranoiaLib->read( &status );
    if( status == CdparanoiaLib::S_OK ) {
        if( buf == 0 ) {
            return -1;
        }
        else {
            ::memcpy( data, buf, CD_FRAMESIZE_RAW );
            return CD_FRAMESIZE_RAW;
        }
    }
    else {
        setErrorString( i18n("Unrecoverable error while ripping track %1.",m_trackIndex) );
        return -1;
    }
}

} // namespace


AudioRipJob::AudioRipJob( JobHandler* hdl, QObject* parent )
    :  MassAudioEncodingJob( false, hdl, parent ),
       d( new Private )
{
}


AudioRipJob::~AudioRipJob()
{
    delete d->paranoiaLib;
}


void AudioRipJob::setParanoiaMode( int mode )
{
    d->paranoiaMode = mode;
}


void AudioRipJob::setMaxRetries( int r )
{
    d->paranoiaRetries = r;
}


void AudioRipJob::setNeverSkip( bool b )
{
    d->neverSkip = b;
}


void AudioRipJob::setUseIndex0( bool b )
{
    d->useIndex0 = b;
}


void AudioRipJob::setDevice( Device::Device* device )
{
    d->device = device;
}


QString AudioRipJob::jobDescription() const
{
    if( cddbEntry().get( KCDDB::Title ).toString().isEmpty() )
        return i18n( "Ripping Audio Tracks" );
    else
        return i18n( "Ripping Audio Tracks From '%1'",
                     cddbEntry().get( KCDDB::Title ).toString() );
}


QString AudioRipJob::jobSource() const
{
    if( d->device )
        return d->device->vendor() + ' ' + d->device->description();
    else
        return QString();
}


void AudioRipJob::start()
{
    k3bcore->blockDevice( d->device );
    MassAudioEncodingJob::start();
}


void AudioRipJob::jobFinished( bool success )
{
    k3bcore->unblockDevice( d->device );
    MassAudioEncodingJob::jobFinished( success );
}


bool AudioRipJob::init()
{
    emit newTask( i18n("Extracting Digital Audio")  );

    if( !d->paranoiaLib ) {
        d->paranoiaLib = CdparanoiaLib::create();
    }

    if( !d->paranoiaLib ) {
        emit infoMessage( i18n("Could not load libcdparanoia."), Job::MessageError );
        return false;
    }

    // try to open the device
    if( !d->device ) {
        return false;
    }

    d->device->block(true);

    emit infoMessage( i18n("Reading CD table of contents."), Job::MessageInfo );
    d->toc = d->device->readToc();

    if( !d->paranoiaLib->initParanoia( d->device, d->toc ) ) {
        emit infoMessage( i18n("Could not open device %1",d->device->blockDeviceName()),
                          Job::MessageError );
        d->device->block(false);

        return false;
    }

    d->paranoiaLib->setParanoiaMode( d->paranoiaMode );
    d->paranoiaLib->setNeverSkip( d->neverSkip );
    d->paranoiaLib->setMaxRetries( d->paranoiaRetries );


    if( d->useIndex0 ) {
        emit newSubTask( i18n("Searching index 0 for all tracks") );
        d->device->indexScan( d->toc );
    }

    emit infoMessage( i18n("Starting digital audio extraction (ripping)."), Job::MessageInfo );
    return true;
}


void AudioRipJob::cleanup()
{
    d->paranoiaLib->close();
    d->device->block(false);
}


Msf AudioRipJob::trackLength( int trackIndex ) const
{
    if( d->useIndex0 )
        return d->toc[trackIndex-1].realAudioLength();
    else
        return d->toc[trackIndex-1].length();
}


QIODevice* AudioRipJob::createReader( int trackIndex ) const
{
    return new AudioCdReader( trackIndex, d.data() );
}


void AudioRipJob::trackStarted(int trackIndex)
{
    if (!cddbEntry().track(trackIndex - 1).get(KCDDB::Artist).toString().isEmpty() &&
        !cddbEntry().track(trackIndex - 1).get(KCDDB::Title).toString().isEmpty()) {
        emit newSubTask(i18n("Ripping track %1 (%2 - %3)",
                        trackIndex,
                        cddbEntry().track(trackIndex - 1).get(KCDDB::Artist).toString(),
                        cddbEntry().track(trackIndex - 1).get(KCDDB::Title).toString().trimmed()));
    } else
        emit newSubTask(i18n("Ripping track %1", trackIndex));
}


void AudioRipJob::trackFinished( int trackIndex, const QString& filename )
{
    emit infoMessage( i18n("Successfully ripped track %1 to %2.", trackIndex, filename), Job::MessageInfo );
}


} // namespace K3b


