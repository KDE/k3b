/*
 *
 * $Id: k3bisoimageverificationjob.cpp 597651 2006-10-21 08:04:01Z trueg $
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bverificationjob.h"

#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bmd5job.h>
#include <k3bglobals.h>
#include <k3bdatatrackreader.h>
#include <k3bpipe.h>
#include <k3biso9660.h>

#include <kdebug.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kio/netaccess.h>

#include <qcstring.h>
#include <qapplication.h>
#include <qvaluelist.h>
#include <qpair.h>


class K3bVerificationJobTrackEntry
{
public:
  K3bVerificationJobTrackEntry()
    : trackNumber(0) {
  }

  K3bVerificationJobTrackEntry( int tn, const QCString& cs, const K3b::Msf& msf )
    : trackNumber(tn),
      checksum(cs),
      length(msf) {
  }

  int trackNumber;
  QCString checksum;
  K3b::Msf length;
};


class K3bVerificationJob::Private
{
public:
  Private()
    : md5Job(0),
      device(0),
      dataTrackReader(0) {
  }

  bool canceled;
  K3bMd5Job* md5Job;
  K3bDevice::Device* device;

  K3b::Msf grownSessionSize;

  QValueList<K3bVerificationJobTrackEntry> tracks;
  int currentTrackIndex;

  K3bDevice::DiskInfo diskInfo;
  K3bDevice::Toc toc;

  K3bDataTrackReader* dataTrackReader;

  K3b::Msf currentTrackSize;
  K3b::Msf totalSectors;
  K3b::Msf alreadyReadSectors;

  K3bPipe pipe;

  bool readSuccessful;

  bool mediumHasBeenReloaded;
};


K3bVerificationJob::K3bVerificationJob( K3bJobHandler* hdl, QObject* parent, const char* name )
  : K3bJob( hdl, parent, name )
{
  d = new Private();

  d->md5Job = new K3bMd5Job( this );
  connect( d->md5Job, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( d->md5Job, SIGNAL(finished(bool)), this, SLOT(slotMd5JobFinished(bool)) );
  connect( d->md5Job, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
}


K3bVerificationJob::~K3bVerificationJob()
{
  delete d;
}


void K3bVerificationJob::cancel()
{
  d->canceled = true;
  if( d->md5Job && d->md5Job->active() )
    d->md5Job->cancel();
  if( d->dataTrackReader && d->dataTrackReader->active() )
    d->dataTrackReader->cancel();
}


void K3bVerificationJob::addTrack( int trackNum, const QCString& checksum, const K3b::Msf& length )
{
  d->tracks.append( K3bVerificationJobTrackEntry( trackNum, checksum, length ) );
}


void K3bVerificationJob::clear()
{
  d->tracks.clear();
  d->grownSessionSize = 0;
}


void K3bVerificationJob::setDevice( K3bDevice::Device* dev )
{
  d->device = dev;
}


void K3bVerificationJob::setGrownSessionSize( const K3b::Msf& s )
{
  d->grownSessionSize = s;
}


void K3bVerificationJob::start()
{
  jobStarted();

  d->canceled = false;
  d->currentTrackIndex = 0;
  d->alreadyReadSectors = 0;

  emit newTask( i18n("Checking medium") );

  d->mediumHasBeenReloaded = false;
  connect( K3bDevice::sendCommand( K3bDevice::DeviceHandler::DISKINFO, d->device ),
           SIGNAL(finished(K3bDevice::DeviceHandler*)),
           this,
           SLOT(slotDiskInfoReady(K3bDevice::DeviceHandler*)) );
}


void K3bVerificationJob::slotMediaReloaded( bool /*success*/ )
{
    // we always need to wait for the medium. Otherwise the diskinfo below
    // may run before the drive is ready!
    waitForMedia( d->device,
                  K3bDevice::STATE_COMPLETE|K3bDevice::STATE_INCOMPLETE,
                  K3bDevice::MEDIA_WRITABLE );

  d->mediumHasBeenReloaded = true;

  emit newTask( i18n("Checking medium") );

  connect( K3bDevice::sendCommand( K3bDevice::DeviceHandler::DISKINFO, d->device ),
           SIGNAL(finished(K3bDevice::DeviceHandler*)),
           this,
           SLOT(slotDiskInfoReady(K3bDevice::DeviceHandler*)) );
}


void K3bVerificationJob::slotDiskInfoReady( K3bDevice::DeviceHandler* dh )
{
  if( d->canceled ) {
    emit canceled();
    jobFinished(false);
  }

  d->diskInfo = dh->diskInfo();
  d->toc = dh->toc();
  d->totalSectors = 0;

  // just to be sure check if we actually have all the tracks
  int i = 0;
  for( QValueList<K3bVerificationJobTrackEntry>::iterator it = d->tracks.begin();
       it != d->tracks.end(); ++i, ++it ) {

    // 0 means "last track"
    if( (*it).trackNumber == 0 )
      (*it).trackNumber = d->toc.count();

    if( (int)d->toc.count() < (*it).trackNumber ) {
        if ( d->mediumHasBeenReloaded ) {
            emit infoMessage( i18n("Internal Error: Verification job improperly initialized (%1)")
                              .arg( "Specified track number not found on medium" ), ERROR );
            jobFinished( false );
            return;
        }
        else {
            // many drives need to reload the medium to return to a proper state
            emit newTask( i18n("Reloading the medium") );
            connect( K3bDevice::reload( d->device ),
                     SIGNAL(finished(bool)),
                     this,
                     SLOT(slotMediaReloaded(bool)) );
            return;
        }
    }

    d->totalSectors += trackLength( i );
  }

  readTrack( 0 );
}


void K3bVerificationJob::readTrack( int trackIndex )
{
  d->currentTrackIndex = trackIndex;
  d->readSuccessful = true;

  d->currentTrackSize = trackLength( trackIndex );
  if( d->currentTrackSize == 0 ) {
    jobFinished(false);
    return;
  }

  emit newTask( i18n("Verifying track %1").arg( d->tracks[trackIndex].trackNumber ) );

  d->pipe.open();

  if( d->toc[d->tracks[trackIndex].trackNumber-1].type() == K3bDevice::Track::DATA ) {
    if( !d->dataTrackReader ) {
      d->dataTrackReader = new K3bDataTrackReader( this );
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
    d->dataTrackReader->setSectorSize( K3bDataTrackReader::MODE1 );

    // in case a session was grown the track size does not say anything about the verification data size
    if( d->diskInfo.mediaType() & (K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR) &&
	d->grownSessionSize > 0 ) {
      K3bIso9660 isoF( d->device );
      if( isoF.open() ) {
	int firstSector = isoF.primaryDescriptor().volumeSpaceSize - d->grownSessionSize.lba();
	d->dataTrackReader->setSectorRange( firstSector,
					    isoF.primaryDescriptor().volumeSpaceSize -1 );
      }
      else {
	emit infoMessage( i18n("Unable to determine the ISO9660 filesystem size."), ERROR );
	jobFinished( false );
	return;
      }
    }
    else
      d->dataTrackReader->setSectorRange( d->toc[d->tracks[trackIndex].trackNumber-1].firstSector(),
					  d->toc[d->tracks[trackIndex].trackNumber-1].firstSector() + d->currentTrackSize -1 );

    d->md5Job->setMaxReadSize( d->currentTrackSize.mode1Bytes() );

    d->dataTrackReader->writeToFd( d->pipe.in() );
    d->dataTrackReader->start();
  }
  else {
    // FIXME: handle audio tracks
  }

  d->md5Job->setFd( d->pipe.out() );
  d->md5Job->start();
}


void K3bVerificationJob::slotReaderProgress( int p )
{
  emit subPercent( p );

  emit percent( 100 * ( d->alreadyReadSectors.lba() + ( p*d->currentTrackSize.lba()/100 ) ) / d->totalSectors.lba() );
}


void K3bVerificationJob::slotMd5JobFinished( bool success )
{
  d->pipe.close();

  if( success && !d->canceled && d->readSuccessful ) {
    // compare the two sums
    if( d->tracks[d->currentTrackIndex].checksum != d->md5Job->hexDigest() ) {
      emit infoMessage( i18n("Written data in track %1 differs from original.").arg(d->tracks[d->currentTrackIndex].trackNumber), ERROR );
      jobFinished(false);
    }
    else {
      emit infoMessage( i18n("Written data verified."), SUCCESS );
      ++d->currentTrackIndex;
      if( d->currentTrackIndex < (int)d->tracks.count() )
	readTrack( d->currentTrackIndex );
      else
	jobFinished(true);
    }
  }
  else {
    // The md5job emitted an error message. So there is no need to do this again
    jobFinished(false);
  }
}


void K3bVerificationJob::slotReaderFinished( bool success )
{
  d->readSuccessful = success;
  if( !d->readSuccessful )
    d->md5Job->cancel();
  else {
    d->alreadyReadSectors += trackLength( d->currentTrackIndex );

    // close the pipe and let the md5 job finish gracefully
    d->pipe.closeIn();
    //    d->md5Job->stop();
  }
}


K3b::Msf K3bVerificationJob::trackLength( int trackIndex )
{
  K3b::Msf& trackSize = d->tracks[trackIndex].length;
  const int& trackNum = d->tracks[trackIndex].trackNumber;

  if( trackSize == 0 ) {
    trackSize = d->toc[trackNum-1].length();

    if( d->diskInfo.mediaType() & (K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR) ) {
      K3bIso9660 isoF( d->device, d->toc[trackNum-1].firstSector().lba() );
      if( isoF.open() ) {
	trackSize = isoF.primaryDescriptor().volumeSpaceSize;
      }
      else {
	emit infoMessage( i18n("Unable to determine the ISO9660 filesystem size."), ERROR );
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
    if( d->toc[trackNum-1].type() == K3bDevice::Track::DATA &&
	d->diskInfo.mediaType() & K3bDevice::MEDIA_CD_ALL ) {
      // we try twice just to be sure
      unsigned char buffer[2048];
      if( !d->device->read10( buffer, 2048, d->toc[trackNum-1].lastSector().lba(), 1 ) &&
	  !d->device->read10( buffer, 2048, d->toc[trackNum-1].lastSector().lba(), 1 ) ) {
	trackSize -= 2;
	kdDebug() << "(K3bCdCopyJob) track " << trackNum << " probably TAO recorded." << endl;
      }
    }
  }

  return trackSize;
}


#include "k3bverificationjob.moc"
