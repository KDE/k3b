/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2009 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bverificationjob.h"

#include "k3bdevice.h"
#include "k3bdevicehandler.h"
#include "k3bglobals.h"
#include "k3bdatatrackreader.h"
#include "k3bchecksumpipe.h"
#include "k3biso9660.h"
#include "k3b_i18n.h"

#include <QDebug>
#include <QList>


namespace {
    class TrackEntry
    {
    public:
        TrackEntry()
            : trackNumber(0) {
        }

        TrackEntry( int tn, const QByteArray& cs, const K3b::Msf& msf )
            : trackNumber(tn),
              checksum(cs),
              length(msf) {
        }

        int trackNumber;
        QByteArray checksum;
        mutable K3b::Msf length; // it's a cache, let's make it modifiable
    };

    typedef QList<TrackEntry> TrackEntries;

    class NullSinkChecksumPipe : public K3b::ChecksumPipe
    {
    protected:
        qint64 writeData( const char* data, qint64 max ) override {
            ChecksumPipe::writeData( data, max );
            return max;
        }
    };
}


class K3b::VerificationJob::Private
{
public:
    Private( VerificationJob* job )
        : device(0),
          dataTrackReader(0),
          q(job){
    }

    void reloadMedium();
    Msf trackLength( const TrackEntry& trackEntry );

    bool canceled;
    K3b::Device::Device* device;

    K3b::Msf grownSessionSize;

    TrackEntries trackEntries;
    TrackEntries::const_iterator currentTrackEntry;

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


K3b::Msf K3b::VerificationJob::Private::trackLength( const TrackEntry& trackEntry )
{
    K3b::Msf& trackSize = trackEntry.length;
    const int& trackNum = trackEntry.trackNumber;

    K3b::Device::Track& track = toc[trackNum-1];

    if( trackSize == 0 ) {
        trackSize = track.length();

        if( diskInfo.mediaType() & (K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR) ) {
            K3b::Iso9660 isoF( device, track.firstSector().lba() );
            if( isoF.open() ) {
                trackSize = isoF.primaryDescriptor().volumeSpaceSize;
            }
            else {
                emit q->infoMessage( i18n("Unable to determine the ISO 9660 filesystem size."), MessageError );
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
            diskInfo.mediaType() & K3b::Device::MEDIA_CD_ALL ) {
            // we try twice just to be sure
            unsigned char buffer[2048];
            if( !device->read10( buffer, 2048, track.lastSector().lba(), 1 ) &&
                !device->read10( buffer, 2048, track.lastSector().lba(), 1 ) ) {
                trackSize -= 2;
                qDebug() << "(K3b::CdCopyJob) track " << trackNum << " probably TAO recorded.";
            }
        }
    }

    return trackSize;
}


K3b::VerificationJob::VerificationJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::Job( hdl, parent )
{
    d = new Private( this );
    d->currentTrackEntry = d->trackEntries.end();
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
    d->trackEntries.append( TrackEntry( trackNum, checksum, length ) );
}


void K3b::VerificationJob::clear()
{
    d->trackEntries.clear();
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
    d->alreadyReadSectors = 0;

    waitForMedium( d->device,
                   K3b::Device::STATE_COMPLETE|K3b::Device::STATE_INCOMPLETE,
                   K3b::Device::MEDIA_WRITABLE );

    // make sure the job is initialized
    if( !d->trackEntries.isEmpty() ) {
        d->currentTrackEntry = d->trackEntries.begin();
    }
    else {
        emit infoMessage( i18n( "Internal Error: Verification job improperly initialized (%1)",
                                i18n("no tracks added") ), MessageError );
        jobFinished( false );
        return;
    }

    emit newTask( i18n("Checking medium") );

    d->mediumHasBeenReloaded = false;
    connect( K3b::Device::sendCommand( K3b::Device::DeviceHandler::CommandLoad, d->device ),
             SIGNAL(finished(K3b::Device::DeviceHandler*)),
             this,
             SLOT(slotMediaLoaded()) );
}


void K3b::VerificationJob::slotMediaLoaded()
{
    // we always need to wait for the medium. Otherwise the diskinfo below
    // may run before the drive is ready!
    waitForMedium( d->device,
                   K3b::Device::STATE_COMPLETE|K3b::Device::STATE_INCOMPLETE,
                   K3b::Device::MEDIA_WRITABLE );

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
        blockingInformation( i18n("Please reload the medium and press 'OK'"),
                             i18n("Failed to reload the medium") );
    }

    d->diskInfo = dh->diskInfo();
    d->toc = dh->toc();
    d->totalSectors = 0;

    // just to be sure check if we actually have all the tracks
    for( TrackEntries::iterator it = d->trackEntries.begin(); it != d->trackEntries.end(); ++it ) {

        // 0 means "last track"
        if( it->trackNumber == 0 )
            it->trackNumber = d->toc.count();

        if( it->trackNumber <= 0 || it->trackNumber > d->toc.count() ) {
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

        d->totalSectors += d->trackLength( *it );
    }

    Q_ASSERT( d->currentTrackEntry != d->trackEntries.end() );

    if( d->currentTrackEntry->trackNumber >= d->toc.count() ) {
        readTrack();
    }
    else if( !d->mediumHasBeenReloaded ) {
        d->reloadMedium();
    }
    else {
        emit infoMessage( i18n("Internal Error: Verification job improperly initialized (%1)",
                               i18n("specified track number '%1' not found on medium", d->currentTrackEntry->trackNumber) ), MessageError );
        jobFinished( false );
    }
}


void K3b::VerificationJob::readTrack()
{
    if( d->currentTrackEntry == d->trackEntries.end() ) {
        jobFinished(true);
        return;
    }

    d->readSuccessful = true;

    d->currentTrackSize = d->trackLength( *d->currentTrackEntry );
    if( d->currentTrackSize == 0 ) {
        jobFinished(false);
        return;
    }

    emit newTask( i18n("Verifying track %1", d->currentTrackEntry->trackNumber ) );

    K3b::Device::Track& track = d->toc[ d->currentTrackEntry->trackNumber-1 ];

    d->pipe.open();

    if( track.type() == K3b::Device::Track::TYPE_DATA ) {
        if( !d->dataTrackReader ) {
            d->dataTrackReader = new K3b::DataTrackReader( this );
            connect( d->dataTrackReader, SIGNAL(percent(int)), this, SLOT(slotReaderProgress(int)) );
            connect( d->dataTrackReader, SIGNAL(finished(bool)), this, SLOT(slotReaderFinished(bool)) );
            connect( d->dataTrackReader, SIGNAL(infoMessage(QString,int)), this, SIGNAL(infoMessage(QString,int)) );
            connect( d->dataTrackReader, SIGNAL(newTask(QString)), this, SIGNAL(newSubTask(QString)) );
            connect( d->dataTrackReader, SIGNAL(debuggingOutput(QString,QString)),
                     this, SIGNAL(debuggingOutput(QString,QString)) );
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
                emit infoMessage( i18n("Unable to determine the ISO 9660 filesystem size."), MessageError );
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
        d->alreadyReadSectors += d->trackLength( *d->currentTrackEntry );

        d->pipe.close();

        // compare the two sums
        if( d->currentTrackEntry->checksum != d->pipe.checksum() ) {
            emit infoMessage( i18n("Written data in track %1 differs from original.", d->currentTrackEntry->trackNumber), MessageError );
            jobFinished(false);
        }
        else {
            emit infoMessage( i18n("Written data verified."), MessageSuccess );

            ++d->currentTrackEntry;
            if( d->currentTrackEntry != d->trackEntries.end() )
                readTrack();
            else
                jobFinished(true);
        }
    }
    else {
        jobFinished( false );
    }
}



