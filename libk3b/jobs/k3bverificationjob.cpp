/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bverificationjob.h"

#include "k3bdevice.h"
#include "k3bdevicehandler.h"
#include "k3bglobals.h"
#include "k3bdatatrackreader.h"
#include "k3bchecksumpipe.h"
#include "k3biso9660.h"

#include <kdebug.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kio/netaccess.h>

#include <qapplication.h>
#include <qlist.h>
#include <qpair.h>


namespace {
    class VerificationJobTrackEntry
    {
    public:
        VerificationJobTrackEntry()
            : trackNumber(0) {
        }

        VerificationJobTrackEntry( int tn, const QByteArray& cs, const K3b::Msf& msf )
            : trackNumber(tn),
              checksum(cs),
              length(msf) {
        }

        int trackNumber;
        QByteArray checksum;
        K3b::Msf length;
    };

    class NullSinkChecksumPipe : public K3b::ChecksumPipe
    {
    protected:
        qint64 writeData( const char* data, qint64 max ) {
            ChecksumPipe::writeData( data, max );
            return max;
        }
    };
}


class K3b::VerificationJob::Private
{
public:
    Private()
        : device(0),
          dataTrackReader(0) {
    }

    void reloadMedium();

    bool canceled;
    K3b::Device::Device* device;

    K3b::Msf grownSessionSize;

    QList<VerificationJobTrackEntry> tracks;
    int currentTrackIndex;

    K3b::Device::DiskInfo diskInfo;
    K3b::Device::Toc toc;

    K3b::DataTrackReader* dataTrackReader;

    K3b::Msf currentTrackSize;
    K3b::Msf totalSectors;
    K3b::Msf alreadyReadSectors;

    NullSinkChecksumPipe pipe;

    bool readSuccessful;

    bool mediumHasBeenReloaded;

    VerificationJob* q;
};


void K3b::VerificationJob::Private::reloadMedium()
{
#ifdef _GNUC_
#warning FIXME: loks like the reload does not work
#endif
    // many drives need to reload the medium to return to a proper state
    mediumHasBeenReloaded = true;
    emit q->infoMessage( i18n( "Need to reload medium to return to proper state." ), MessageInfo );
    QObject::connect( K3b::Device::sendCommand( Device::DeviceHandler::CommandReload|Device::DeviceHandler::CommandMediaInfo, device ),
                      SIGNAL(finished(K3b::Device::DeviceHandler*)),
                      q,
                      SLOT(slotDiskInfoReady(K3b::Device::DeviceHandler*)) );
}


K3b::VerificationJob::VerificationJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::Job( hdl, parent )
{
    d = new Private();
    d->q = this;
}


K3b::VerificationJob::~VerificationJob()
{
    delete d;
}


void K3b::VerificationJob::cancel()
{
    d->canceled = true;
    if( d->dataTrackReader && d->dataTrackReader->active() ) {
        d->dataTrackReader->cancel();
    }
    else if( active() ) {
        emit canceled();
        jobFinished( false );
    }
}


void K3b::VerificationJob::addTrack( int trackNum, const QByteArray& checksum, const K3b::Msf& length )
{
    d->tracks.append( VerificationJobTrackEntry( trackNum, checksum, length ) );
}


void K3b::VerificationJob::clear()
{
    d->tracks.clear();
    d->grownSessionSize = 0;
}


void K3b::VerificationJob::setDevice( K3b::Device::Device* dev )
{
    d->device = dev;
}


void K3b::VerificationJob::setGrownSessionSize( const K3b::Msf& s )
{
    d->grownSessionSize = s;
}


void K3b::VerificationJob::start()
{
    jobStarted();

    d->canceled = false;
    d->currentTrackIndex = 0;
    d->alreadyReadSectors = 0;

    waitForMedia( d->device,
                  K3b::Device::STATE_COMPLETE|K3b::Device::STATE_INCOMPLETE,
                  K3b::Device::MEDIA_WRITABLE );

    // make sure the job is initialized
    if ( d->tracks.isEmpty() ) {
        emit infoMessage( i18n( "Internal Error: Verification job improperly initialized (%1)",
                                i18n("no tracks added") ), MessageError );
        jobFinished( false );
        return;
    }

    emit newTask( i18n("Checking medium") );

    d->mediumHasBeenReloaded = false;
    connect( K3b::Device::sendCommand( K3b::Device::DeviceHandler::CommandMediaInfo, d->device ),
             SIGNAL(finished(K3b::Device::DeviceHandler*)),
             this,
             SLOT(slotDiskInfoReady(K3b::Device::DeviceHandler*)) );
}


void K3b::VerificationJob::slotDiskInfoReady( K3b::Device::DeviceHandler* dh )
{
    if( d->canceled ) {
        // signal already emitted in cancel()
        return;
    }

    if ( !dh->success() ) {
        blockingInformation( i18n("Please reload the medium and press 'ok'"),
                             i18n("Failed to reload the medium") );
    }

    d->diskInfo = dh->diskInfo();
    d->toc = dh->toc();
    d->totalSectors = 0;

    // just to be sure check if we actually have all the tracks
    int i = 0;
    for( QList<VerificationJobTrackEntry>::iterator it = d->tracks.begin();
         it != d->tracks.end(); ++i, ++it ) {

        // 0 means "last track"
        if( it->trackNumber == 0 )
            it->trackNumber = d->toc.count();

        if( d->toc.count() < it->trackNumber ) {
            if ( d->mediumHasBeenReloaded ) {
                emit infoMessage( i18n("Internal Error: Verification job improperly initialized (%1)",
                                       i18n("specified track number '%1' not found on medium", it->trackNumber) ), MessageError );
                jobFinished( false );
                return;
            }
            else {
                d->reloadMedium();
                return;
            }
        }

        d->totalSectors += trackLength( i );
    }

    readTrack( 0 );
}


void K3b::VerificationJob::readTrack( int trackIndex )
{
    d->currentTrackIndex = trackIndex;
    d->readSuccessful = true;

    d->currentTrackSize = trackLength( trackIndex );
    if( d->currentTrackSize == 0 ) {
        jobFinished(false);
        return;
    }

    emit newTask( i18n("Verifying track %1", d->tracks[trackIndex].trackNumber ) );

    K3b::Device::Track& track = d->toc[d->tracks[trackIndex].trackNumber-1];

    d->pipe.open();

    if( track.type() == K3b::Device::Track::TYPE_DATA ) {
        if( !d->dataTrackReader ) {
            d->dataTrackReader = new K3b::DataTrackReader( this );
            connect( d->dataTrackReader, SIGNAL(percent(int)), this, SLOT(slotReaderProgress(int)) );
            //      connect( d->dataTrackReader, SIGNAL(processedSize(int, int)), this, SLOT(slotReaderProcessedSize(int, int)) );
            connect( d->dataTrackReader, SIGNAL(finished(bool)), this, SLOT(slotReaderFinished(bool)) );
            connect( d->dataTrackReader, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
            connect( d->dataTrackReader, SIGNAL(newTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
            connect( d->dataTrackReader, SIGNAL(debuggingOutput(const QString&, const QString&)),
                     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
        }

        d->dataTrackReader->setDevice( d->device );
        d->dataTrackReader->setIgnoreErrors( false );
        d->dataTrackReader->setSectorSize( K3b::DataTrackReader::MODE1 );
        d->dataTrackReader->writeTo( &d->pipe );

        // in case a session was grown the track size does not say anything about the verification data size
        if( d->diskInfo.mediaType() & (K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR) &&
            d->grownSessionSize > 0 ) {
            K3b::Iso9660 isoF( d->device );
            if( isoF.open() ) {
                int firstSector = isoF.primaryDescriptor().volumeSpaceSize - d->grownSessionSize.lba();
                d->dataTrackReader->setSectorRange( firstSector,
                                                    isoF.primaryDescriptor().volumeSpaceSize -1 );
            }
            else {
                emit infoMessage( i18n("Unable to determine the ISO9660 filesystem size."), MessageError );
                jobFinished( false );
                return;
            }
        }
        else
            d->dataTrackReader->setSectorRange( track.firstSector(),
                                                track.firstSector() + d->currentTrackSize -1 );

        d->pipe.open();
        d->dataTrackReader->start();
    }
    else {
        // FIXME: handle audio tracks
    }
}


void K3b::VerificationJob::slotReaderProgress( int p )
{
    emit subPercent( p );

    emit percent( 100 * ( d->alreadyReadSectors.lba() + ( p*d->currentTrackSize.lba()/100 ) ) / d->totalSectors.lba() );
}


void K3b::VerificationJob::slotReaderFinished( bool success )
{
    d->readSuccessful = success;
    if( d->readSuccessful && !d->canceled ) {
        d->alreadyReadSectors += trackLength( d->currentTrackIndex );

        d->pipe.close();

        // compare the two sums
        if( d->tracks[d->currentTrackIndex].checksum != d->pipe.checksum() ) {
            emit infoMessage( i18n("Written data in track %1 differ from original.", d->tracks[d->currentTrackIndex].trackNumber), MessageError );
            jobFinished(false);
        }
        else {
            emit infoMessage( i18n("Written data verified."), MessageSuccess );
            ++d->currentTrackIndex;
            if( d->currentTrackIndex < (int)d->tracks.count() )
                readTrack( d->currentTrackIndex );
            else
                jobFinished(true);
        }
    }
    else {
        jobFinished( false );
    }
}


K3b::Msf K3b::VerificationJob::trackLength( int trackIndex )
{
    K3b::Msf& trackSize = d->tracks[trackIndex].length;
    const int& trackNum = d->tracks[trackIndex].trackNumber;

    K3b::Device::Track& track = d->toc[trackNum-1];

    if( trackSize == 0 ) {
        trackSize = track.length();

        if( d->diskInfo.mediaType() & (K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR) ) {
            K3b::Iso9660 isoF( d->device, track.firstSector().lba() );
            if( isoF.open() ) {
                trackSize = isoF.primaryDescriptor().volumeSpaceSize;
            }
            else {
                emit infoMessage( i18n("Unable to determine the ISO9660 filesystem size."), MessageError );
                return 0;
            }
        }

        //
        // A data track recorded in TAO mode has two run-out blocks which cannot be read and contain
        // zero data anyway. The problem is that I do not know of a valid method to determine if a track
        // was written in TAO (the control nibble does definitely not work, I never saw one which did not
        // equal 4).
        // So the solution for now is to simply try to read the last sector of a data track. If this is not
        // possible we assume it was written in TAO mode and reduce the length by 2 sectors
        //
        if( track.type() == K3b::Device::Track::TYPE_DATA &&
            d->diskInfo.mediaType() & K3b::Device::MEDIA_CD_ALL ) {
            // we try twice just to be sure
            unsigned char buffer[2048];
            if( !d->device->read10( buffer, 2048, track.lastSector().lba(), 1 ) &&
                !d->device->read10( buffer, 2048, track.lastSector().lba(), 1 ) ) {
                trackSize -= 2;
                kDebug() << "(K3b::CdCopyJob) track " << trackNum << " probably TAO recorded.";
            }
        }
    }

    return trackSize;
}


#include "k3bverificationjob.moc"
