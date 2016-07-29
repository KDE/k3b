/*
 *
 * Copyright (C) 2003-2010 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdvdcopyjob.h"
#include "k3blibdvdcss.h"

#include "k3breadcdreader.h"
#include "k3bdatatrackreader.h"
#include "k3bexternalbinmanager.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3bdevicehandler.h"
#include "k3bdiskinfo.h"
#include "k3bglobals.h"
#include "k3bcore.h"
#include "k3bgrowisofswriter.h"
#include "k3bcdrecordwriter.h"
#include "k3bversion.h"
#include "k3biso9660.h"
#include "k3bfilesplitter.h"
#include "k3bchecksumpipe.h"
#include "k3bverificationjob.h"
#include "k3bglobalsettings.h"

#include <kdebug.h>
#include <klocale.h>
#include <kio/global.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qapplication.h>


class K3b::DvdCopyJob::Private
{
public:
    Private()
        : doneCopies(0),
          running(false),
          canceled(false),
          writerJob(0),
          readcdReader(0),
          dataTrackReader(0),
          verificationJob(0),
          usedWritingMode(K3b::WritingModeAuto),
          verifyData(false) {
        outPipe.readFrom( &imageFile, true );
    }

    K3b::WritingApp usedWritingApp;

    int doneCopies;

    bool running;
    bool readerRunning;
    bool writerRunning;
    bool canceled;

    K3b::AbstractWriter* writerJob;
    K3b::ReadcdReader* readcdReader;
    K3b::DataTrackReader* dataTrackReader;
    K3b::VerificationJob* verificationJob;

    K3b::Device::DiskInfo sourceDiskInfo;

    K3b::Msf lastSector;

    K3b::WritingMode usedWritingMode;

    K3b::FileSplitter imageFile;
    K3b::ChecksumPipe inPipe;
    K3b::ActivePipe outPipe;

    bool verifyData;
};


K3b::DvdCopyJob::DvdCopyJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::BurnJob( hdl, parent ),
      m_writerDevice(0),
      m_readerDevice(0),
      m_onTheFly(false),
      m_removeImageFiles(false),
      m_simulate(false),
      m_speed(1),
      m_copies(1),
      m_onlyCreateImage(false),
      m_ignoreReadErrors(false),
      m_readRetries(128),
      m_writingMode( K3b::WritingModeAuto )
{
    d = new Private();
}


K3b::DvdCopyJob::~DvdCopyJob()
{
    delete d;
}


void K3b::DvdCopyJob::start()
{
    jobStarted();
    emit burning(false);

    d->canceled = false;
    d->running = true;
    d->readerRunning = d->writerRunning = false;

    emit newTask( i18n("Checking Source Medium") );

    if( m_onTheFly &&
        k3bcore->externalBinManager()->binObject( "growisofs" )->version() < K3b::Version( 5, 12 ) ) {
        m_onTheFly = false;
        emit infoMessage( i18n("K3b does not support writing on-the-fly with growisofs %1.",
                          k3bcore->externalBinManager()->binObject( "growisofs" )->version()), MessageError );
        emit infoMessage( i18n("Disabling on-the-fly writing."), MessageInfo );
    }

    emit newSubTask( i18n("Waiting for source medium") );

    // wait for a source disk
    if( waitForMedium( m_readerDevice,
                       K3b::Device::STATE_COMPLETE|K3b::Device::STATE_INCOMPLETE,
                       K3b::Device::MEDIA_WRITABLE_DVD|K3b::Device::MEDIA_DVD_ROM|K3b::Device::MEDIA_BD_ALL ) == Device::MEDIA_UNKNOWN ) {
        emit canceled();
        d->running = false;
        jobFinished( false );
        return;
    }

    emit newSubTask( i18n("Checking source medium") );

    connect( K3b::Device::sendCommand( K3b::Device::DeviceHandler::CommandMediaInfo, m_readerDevice ),
             SIGNAL(finished(K3b::Device::DeviceHandler*)),
             this,
             SLOT(slotDiskInfoReady(K3b::Device::DeviceHandler*)) );
}


void K3b::DvdCopyJob::slotDiskInfoReady( K3b::Device::DeviceHandler* dh )
{
    if( d->canceled ) {
        emit canceled();
        jobFinished(false);
        d->running = false;
    }

    d->sourceDiskInfo = dh->diskInfo();

    if( dh->diskInfo().empty() || dh->diskInfo().diskState() == K3b::Device::STATE_NO_MEDIA ) {
        emit infoMessage( i18n("No source medium found."), MessageError );
        jobFinished(false);
        d->running = false;
    }
    else {
        // first let's determine which application to use
        d->usedWritingApp = writingApp();
        if ( d->usedWritingApp == K3b::WritingAppAuto ) {
            // prefer growisofs to wodim, which doesn't work all that great for DVDs
            // (and doesn't support BluRay at all)
            if ( k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "wodim" ) )
                d->usedWritingApp = K3b::WritingAppGrowisofs;
            // otherwise, let's default to cdrecord for the time being
            // FIXME: use growisofs for non-dao and non-auto mode
            else {
                if ( K3b::Device::isBdMedia( d->sourceDiskInfo.mediaType() ) ) {
                    if ( k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "blu-ray" ) )
                        d->usedWritingApp = K3b::WritingAppCdrecord;
                    else
                        d->usedWritingApp = K3b::WritingAppGrowisofs;
                } else
                    d->usedWritingApp = K3b::WritingAppCdrecord;
            }
        }

        if( m_readerDevice->copyrightProtectionSystemType() == K3b::Device::COPYRIGHT_PROTECTION_CSS ) { // CSS is the the only one we support ATM
            emit infoMessage( i18n("Found encrypted DVD."), MessageWarning );
            // check for libdvdcss
            bool haveLibdvdcss = false;
            kDebug() << "(K3b::DvdCopyJob) trying to open libdvdcss.";
            if( K3b::LibDvdCss* libcss = K3b::LibDvdCss::create() ) {
                kDebug() << "(K3b::DvdCopyJob) succeeded.";
                kDebug() << "(K3b::DvdCopyJob) dvdcss_open(" << m_readerDevice->blockDeviceName() << ") = "
                          << libcss->open(m_readerDevice) << endl;
                haveLibdvdcss = true;

                delete libcss;
            }
            else
                kDebug() << "(K3b::DvdCopyJob) failed.";

            if( !haveLibdvdcss ) {
                emit infoMessage( i18n("Cannot copy encrypted DVDs."), MessageError );
                d->running = false;
                jobFinished( false );
                return;
            }
        }


        //
        // We cannot rely on the kernel to determine the size of the DVD for some reason
        // On the other hand it is not always a good idea to rely on the size from the ISO9660
        // header since that may be wrong due to some buggy encoder or some boot code appended
        // after creating the image.
        // That is why we try our best to determine the size of the DVD. For DVD-ROM this is very
        // easy since it has only one track. The same goes for single session DVD-R(W) and DVD+R.
        // Multisession DVDs we will simply not copy. ;)
        // For DVD+RW and DVD-RW in restricted overwrite mode we are left with no other choice but
        // to use the ISO9660 header.
        //
        // On the other hand: in on-the-fly mode growisofs determines the size of the data to be written
        //                    by looking at the ISO9660 header when writing in DAO mode. So in this case
        //                    it would be best for us to do the same....
        //
        // With growisofs 5.15 we have the option to specify the size of the image to be written in DAO mode.
        //

        switch( dh->diskInfo().mediaType() ) {
        case K3b::Device::MEDIA_DVD_ROM:
        case K3b::Device::MEDIA_DVD_PLUS_R_DL:
        case K3b::Device::MEDIA_DVD_R_DL:
        case K3b::Device::MEDIA_DVD_R_DL_SEQ:
        case K3b::Device::MEDIA_DVD_R_DL_JUMP:
            if( !m_onlyCreateImage ) {
                if( dh->diskInfo().numLayers() > 1 &&
                    dh->diskInfo().size() > MediaSizeDvd4Gb ) {
                    if( !(m_writerDevice->type() & (K3b::Device::DEVICE_DVD_R_DL|K3b::Device::DEVICE_DVD_PLUS_R_DL)) ) {
                        emit infoMessage( i18n("The writer does not support writing Double Layer DVDs."), MessageError );
                        d->running = false;
                        jobFinished(false);
                        return;
                    }
                    else if( k3bcore->externalBinManager()->binObject( "growisofs" ) &&
                             !k3bcore->externalBinManager()->binObject( "growisofs" )->hasFeature( "dual-layer" ) ) {
                        emit infoMessage( i18n("This growisofs version does not support writing Double Layer DVDs."), MessageError );
                        d->running = false;
                        jobFinished(false);
                        return;
                    }
                }
            }
        case K3b::Device::MEDIA_DVD_R:
        case K3b::Device::MEDIA_DVD_R_SEQ:
        case K3b::Device::MEDIA_DVD_RW:
        case K3b::Device::MEDIA_DVD_RW_SEQ:
        case K3b::Device::MEDIA_DVD_PLUS_R:
        case K3b::Device::MEDIA_BD_ROM:
        case K3b::Device::MEDIA_BD_R:
        case K3b::Device::MEDIA_BD_R_SRM:

            if( dh->diskInfo().numSessions() > 1 ) {
                emit infoMessage( i18n("K3b does not support copying multi-session DVD or Blu-ray disks."), MessageError );
                d->running = false;
                jobFinished(false);
                return;
            }

            // growisofs only uses the size from the PVD for reserving
            // writable space in DAO mode
            // with version >= 5.15 growisofs supports specifying the size of the track
            if( m_writingMode != K3b::WritingModeSao || !m_onTheFly || m_onlyCreateImage ||
                ( k3bcore->externalBinManager()->binObject( "growisofs" ) &&
                  k3bcore->externalBinManager()->binObject( "growisofs" )->hasFeature( "daosize" ) ) ||
                d->usedWritingApp == K3b::WritingAppCdrecord ) {
                d->lastSector = dh->toc().lastSector();
                break;
            }

            // fallthrough

        case K3b::Device::MEDIA_DVD_PLUS_RW:
        case K3b::Device::MEDIA_DVD_RW_OVWR:
        case K3b::Device::MEDIA_BD_RE:
        {
            emit infoMessage( i18n("K3b relies on the size saved in the ISO 9660 header."), MessageWarning );
            emit infoMessage( i18n("This might result in a corrupt copy if the source was mastered with buggy software."), MessageWarning );

            K3b::Iso9660 isoF( m_readerDevice, 0 );
            if( isoF.open() ) {
                d->lastSector = ((long long)isoF.primaryDescriptor().logicalBlockSize*isoF.primaryDescriptor().volumeSpaceSize)/2048LL - 1;
            }
            else {
                emit infoMessage( i18n("Unable to determine the ISO 9660 filesystem size."), MessageError );
                jobFinished(false);
                d->running = false;
                return;
            }
        }
        break;

        case K3b::Device::MEDIA_DVD_RAM:
            emit infoMessage( i18n("K3b does not support copying DVD-RAM."), MessageError );
            jobFinished(false);
            d->running = false;
            return;

        default:
            emit infoMessage( i18n("Unsupported media type."), MessageError );
            jobFinished(false);
            d->running = false;
            return;
        }


        if( !m_onTheFly ) {
            //
            // Check the image path
            //
            QFileInfo fi( m_imagePath );
            if( !fi.isFile() ||
                questionYesNo( i18n("Do you want to overwrite %1?",m_imagePath),
                               i18n("File Exists") ) ) {
                if( fi.isDir() )
                    m_imagePath = K3b::findTempFile( "iso", m_imagePath );
                else if( !QFileInfo( m_imagePath.section( '/', 0, -2 ) ).isDir() ) {
                    emit infoMessage( i18n("Specified an unusable temporary path. Using default."), MessageWarning );
                    m_imagePath = K3b::findTempFile( "iso" );
                }
                // else the user specified a file in an existing dir

                emit infoMessage( i18n("Writing image file to %1.",m_imagePath), MessageInfo );
                emit newSubTask( i18n("Reading source medium.") );
            }
            else {
                jobFinished(false);
                d->running = false;
                return;
            }

            //
            // check free temp space
            //
            KIO::filesize_t imageSpaceNeeded = (KIO::filesize_t)(d->lastSector.lba()+1)*2048;
            unsigned long avail, size;
            QString pathToTest = m_imagePath.left( m_imagePath.lastIndexOf( '/' ) );
            if( !K3b::kbFreeOnFs( pathToTest, size, avail ) ) {
                emit infoMessage( i18n("Unable to determine free space in temporary folder '%1'.",pathToTest), MessageError );
                jobFinished(false);
                d->running = false;
                return;
            }
            else {
                if( avail < imageSpaceNeeded/1024 ) {
                    emit infoMessage( i18n("Not enough space left in temporary folder."), MessageError );
                    jobFinished(false);
                    d->running = false;
                    return;
                }
            }

            d->imageFile.setName( m_imagePath );
            if( !d->imageFile.open( QIODevice::WriteOnly ) ) {
                emit infoMessage( i18n("Unable to open '%1' for writing.",m_imagePath), MessageError );
                jobFinished( false );
                d->running = false;
                return;
            }
        }

        if( K3b::isMounted( m_readerDevice ) ) {
            emit infoMessage( i18n("Unmounting source medium"), MessageInfo );
            K3b::unmount( m_readerDevice );
        }

        if( m_onlyCreateImage || !m_onTheFly ) {
            emit newTask( i18n("Creating image") );
        }
        else if( m_onTheFly && !m_onlyCreateImage ) {
            if( waitForDvd() ) {
                prepareWriter();
                if( m_simulate )
                    emit newTask( i18n("Simulating copy") );
                else if( m_copies > 1 )
                    emit newTask( i18n("Writing copy %1",d->doneCopies+1) );
                else
                    emit newTask( i18n("Writing copy") );

                emit burning(true);
                d->writerRunning = true;
                d->writerJob->start();
            }
            else {
                if( d->canceled )
                    emit canceled();
                jobFinished(false);
                d->running = false;
                return;
            }
        }

        prepareReader();
        d->readerRunning = true;
        d->dataTrackReader->start();
    }
}


void K3b::DvdCopyJob::cancel()
{
    if( d->running ) {
        d->canceled = true;
        if( d->readerRunning  )
            d->dataTrackReader->cancel();
        if( d->writerRunning )
            d->writerJob->cancel();
        if ( d->verificationJob && d->verificationJob->active() )
            d->verificationJob->cancel();
        d->inPipe.close();
        d->outPipe.close();
        d->imageFile.close();
    }
    else {
        kDebug() << "(K3b::DvdCopyJob) not running.";
    }
}


void K3b::DvdCopyJob::prepareReader()
{
    if( !d->dataTrackReader ) {
        d->dataTrackReader = new K3b::DataTrackReader( this );
        connect( d->dataTrackReader, SIGNAL(percent(int)), this, SLOT(slotReaderProgress(int)) );
        connect( d->dataTrackReader, SIGNAL(processedSize(int,int)), this, SLOT(slotReaderProcessedSize(int,int)) );
        connect( d->dataTrackReader, SIGNAL(finished(bool)), this, SLOT(slotReaderFinished(bool)) );
        connect( d->dataTrackReader, SIGNAL(infoMessage(QString,int)), this, SIGNAL(infoMessage(QString,int)) );
        connect( d->dataTrackReader, SIGNAL(newTask(QString)), this, SIGNAL(newSubTask(QString)) );
        connect( d->dataTrackReader, SIGNAL(debuggingOutput(QString,QString)),
                 this, SIGNAL(debuggingOutput(QString,QString)) );
    }

    d->dataTrackReader->setDevice( m_readerDevice );
    d->dataTrackReader->setIgnoreErrors( m_ignoreReadErrors );
    d->dataTrackReader->setRetries( m_readRetries );
    d->dataTrackReader->setSectorRange( 0, d->lastSector );

    if( m_onTheFly && !m_onlyCreateImage )
        // there are several uses of pipe->writeTo( d->writerJob->ioDevice(), ... ) in this file!
#ifdef __GNUC__
#warning Growisofs needs stdin to be closed in order to exit gracefully. Cdrecord does not. However,  if closed with cdrecord we loose parts of stderr. Why?
#endif
        d->inPipe.writeTo( d->writerJob->ioDevice(), d->usedWritingApp == K3b::WritingAppGrowisofs );
    else
        d->inPipe.writeTo( &d->imageFile, true );

    d->inPipe.open( true );
    d->dataTrackReader->writeTo( &d->inPipe );
}


// ALWAYS CALL WAITFORDVD BEFORE PREPAREWRITER!
void K3b::DvdCopyJob::prepareWriter()
{
    delete d->writerJob;

    if ( d->usedWritingApp == K3b::WritingAppGrowisofs ) {
        K3b::GrowisofsWriter* job = new K3b::GrowisofsWriter( m_writerDevice, this, this );

        // these do only make sense with DVD-R(W)
        job->setSimulate( m_simulate );
        job->setBurnSpeed( m_speed );
        job->setWritingMode( d->usedWritingMode );
        job->setCloseDvd( true );

        //
        // In case the first layer size is not known let the
        // split be determined by growisofs
        //
        if( d->sourceDiskInfo.numLayers() > 1 &&
            d->sourceDiskInfo.firstLayerSize() > 0 ) {
            job->setLayerBreak( d->sourceDiskInfo.firstLayerSize().lba() );
        }
        else {
            // this is only used in DAO mode with growisofs >= 5.15
            job->setTrackSize( d->lastSector.lba()+1 );
        }

        job->setImageToWrite( QString() ); // write to stdin

        d->writerJob = job;
    }

    else {
        K3b::CdrecordWriter* writer = new K3b::CdrecordWriter( m_writerDevice, this, this );

        writer->setWritingMode( d->usedWritingMode );
        writer->setSimulate( m_simulate );
        writer->setBurnSpeed( m_speed );

        writer->addArgument( "-data" );
        writer->addArgument( QString("-tsize=%1s").arg( d->lastSector.lba()+1 ) )->addArgument("-");

        d->writerJob = writer;
    }


    connect( d->writerJob, SIGNAL(infoMessage(QString,int)), this, SIGNAL(infoMessage(QString,int)) );
    connect( d->writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterProgress(int)) );
    connect( d->writerJob, SIGNAL(processedSize(int,int)), this, SIGNAL(processedSize(int,int)) );
    connect( d->writerJob, SIGNAL(processedSubSize(int,int)), this, SIGNAL(processedSubSize(int,int)) );
    connect( d->writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
    connect( d->writerJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( d->writerJob, SIGNAL(writeSpeed(int,K3b::Device::SpeedMultiplicator)), this, SIGNAL(writeSpeed(int,K3b::Device::SpeedMultiplicator)) );
    connect( d->writerJob, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
    //  connect( d->writerJob, SIGNAL(newTask(QString)), this, SIGNAL(newTask(QString)) );
    connect( d->writerJob, SIGNAL(newSubTask(QString)), this, SIGNAL(newSubTask(QString)) );
    connect( d->writerJob, SIGNAL(debuggingOutput(QString,QString)),
             this, SIGNAL(debuggingOutput(QString,QString)) );
}


void K3b::DvdCopyJob::slotReaderProgress( int p )
{
    if( !m_onTheFly || m_onlyCreateImage ) {
        emit subPercent( p );

        int bigParts = ( m_onlyCreateImage ? 1 : (m_simulate ? 2 : ( d->verifyData ? m_copies*2 : m_copies ) + 1 ) );
        emit percent( p/bigParts );
    }
}


void K3b::DvdCopyJob::slotReaderProcessedSize( int p, int c )
{
    if( !m_onTheFly || m_onlyCreateImage )
        emit processedSubSize( p, c );

    if( m_onlyCreateImage )
        emit processedSize( p, c );
}


void K3b::DvdCopyJob::slotWriterProgress( int p )
{
    int bigParts = ( m_simulate ? 1 : ( d->verifyData ? m_copies*2 : m_copies ) ) + ( m_onTheFly ? 0 : 1 );
    int doneParts = ( m_simulate ? 0 : ( d->verifyData ? d->doneCopies*2 : d->doneCopies ) ) + ( m_onTheFly ? 0 : 1 );
    emit percent( 100*doneParts/bigParts + p/bigParts );

    emit subPercent( p );
}


void K3b::DvdCopyJob::slotVerificationProgress( int p )
{
    int bigParts = ( m_simulate ? 1 : ( d->verifyData ? m_copies*2 : m_copies ) ) + ( m_onTheFly ? 0 : 1 );
    int doneParts = ( m_simulate ? 0 : ( d->verifyData ? d->doneCopies*2 : d->doneCopies ) ) + ( m_onTheFly ? 0 : 1 ) + 1;
    emit percent( 100*doneParts/bigParts + p/bigParts );
}


void K3b::DvdCopyJob::slotReaderFinished( bool success )
{
    d->readerRunning = false;

    // already finished?
    if( !d->running )
        return;

    if( d->canceled ) {
        removeImageFiles();
        emit canceled();
        jobFinished(false);
        d->running = false;
    }

    if( success ) {
        emit infoMessage( i18n("Successfully read source medium."), MessageSuccess );
        if( m_onlyCreateImage ) {
            jobFinished(true);
            d->running = false;
        }
        else {
            if( m_writerDevice == m_readerDevice ) {
                // eject the media (we do this blocking to know if it worked
                // because if it did not it might happen that k3b overwrites a CD-RW
                // source)
                kDebug() << "Ejecting read medium" << m_readerDevice->blockDeviceName();
                if( !K3b::eject( m_readerDevice ) ) {
                    blockingInformation( i18n("K3b was unable to eject the source medium. Please do so manually.") );
                }
            }

            if( !m_onTheFly ) {

                d->imageFile.close();

                if( waitForDvd() ) {
                    prepareWriter();
                    if( m_copies > 1 )
                        emit newTask( i18n("Writing copy %1",d->doneCopies+1) );
                    else
                        emit newTask( i18n("Writing copy") );

                    emit burning(true);

                    d->writerRunning = true;
                    d->writerJob->start();
#ifdef __GNUC__
#warning Growisofs needs stdin to be closed in order to exit gracefully. Cdrecord does not. However,  if closed with cdrecord we loose parts of stderr. Why?
#endif
                    d->outPipe.writeTo( d->writerJob->ioDevice(), d->usedWritingApp == K3b::WritingAppGrowisofs );
                    d->outPipe.open( true );
                }
                else {
                    if( m_removeImageFiles )
                        removeImageFiles();
                    if( d->canceled )
                        emit canceled();
                    jobFinished(false);
                    d->running = false;
                }
            }
        }
    }
    else {
        removeImageFiles();
        jobFinished(false);
        d->running = false;
    }
}


void K3b::DvdCopyJob::slotWriterFinished( bool success )
{
    d->writerRunning = false;

    // already finished?
    if( !d->running )
        return;

    if( d->canceled ) {
        if( m_removeImageFiles )
            removeImageFiles();
        emit canceled();
        jobFinished(false);
        d->running = false;
    }

    if( success ) {
        emit infoMessage( i18n("Successfully written copy %1.",d->doneCopies+1), MessageInfo );

        if( d->verifyData && !m_simulate ) {
            if( !d->verificationJob ) {
                d->verificationJob = new K3b::VerificationJob( this, this );
                connect( d->verificationJob, SIGNAL(infoMessage(QString,int)),
                         this, SIGNAL(infoMessage(QString,int)) );
                connect( d->verificationJob, SIGNAL(newTask(QString)),
                         this, SIGNAL(newSubTask(QString)) );
                connect( d->verificationJob, SIGNAL(percent(int)),
                         this, SLOT(slotVerificationProgress(int)) );
                connect( d->verificationJob, SIGNAL(percent(int)),
                         this, SIGNAL(subPercent(int)) );
                connect( d->verificationJob, SIGNAL(finished(bool)),
                         this, SLOT(slotVerificationFinished(bool)) );
                connect( d->verificationJob, SIGNAL(debuggingOutput(QString,QString)),
                         this, SIGNAL(debuggingOutput(QString,QString)) );

            }
            d->verificationJob->setDevice( m_writerDevice );
            d->verificationJob->addTrack( 1, d->inPipe.checksum(), d->lastSector+1 );

            if( m_copies > 1 )
                emit newTask( i18n("Verifying copy %1",d->doneCopies+1) );
            else
                emit newTask( i18n("Verifying copy") );

            emit burning( false );

            d->verificationJob->start();
        }

        else if( ++d->doneCopies < m_copies ) {

            if( !K3b::eject( m_writerDevice ) ) {
                blockingInformation( i18n("K3b was unable to eject the written medium. Please do so manually.") );
            }

            if( waitForDvd() ) {
                prepareWriter();
                emit newTask( i18n("Writing copy %1",d->doneCopies+1) );

                emit burning(true);

                d->writerRunning = true;
                d->writerJob->start();
            }
            else {
                if( d->canceled )
                    emit canceled();
                jobFinished(false);
                d->running = false;
                return;
            }

            if( m_onTheFly ) {
                prepareReader();
                d->readerRunning = true;
                d->dataTrackReader->start();
            }
            else {
#ifdef __GNUC__
#warning Growisofs needs stdin to be closed in order to exit gracefully. Cdrecord does not. However,  if closed with cdrecord we loose parts of stderr. Why?
#endif
                d->outPipe.writeTo( d->writerJob->ioDevice(), true/*d->usedWritingApp == K3b::WritingAppGrowisofs*/ );
                d->outPipe.open( true );
            }
        }
        else {
            if ( k3bcore->globalSettings()->ejectMedia() ) {
                K3b::Device::eject( m_writerDevice );
            }
            if( m_removeImageFiles )
                removeImageFiles();
            d->running = false;
            jobFinished(true);
        }
    }
    else {
        if( m_removeImageFiles )
            removeImageFiles();
        d->running = false;
        jobFinished(false);
    }
}


void K3b::DvdCopyJob::slotVerificationFinished( bool success )
{
    if ( d->canceled ) {
        emit canceled();
        jobFinished( false );
    }

    // we simply ignore the results from the verification, the verification
    // job already emits a message
    else if( ++d->doneCopies < m_copies ) {

        if( waitForDvd() ) {
            prepareWriter();
            emit newTask( i18n("Writing copy %1",d->doneCopies+1) );

            emit burning(true);

            d->writerRunning = true;
            d->writerJob->start();
        }
        else {
            if( d->canceled )
                emit canceled();
            jobFinished(false);
            d->running = false;
            return;
        }

        if( m_onTheFly ) {
            prepareReader();
            d->readerRunning = true;
            d->dataTrackReader->start();
        }
        else {
#ifdef __GNUC__
#warning Growisofs needs stdin to be closed in order to exit gracefully. Cdrecord does not. However,  if closed with cdrecord we loose parts of stderr. Why?
#endif
            d->outPipe.writeTo( d->writerJob->ioDevice(), d->usedWritingApp == K3b::WritingAppGrowisofs );
            d->outPipe.open( true );
        }
    }
    else {
        if( m_removeImageFiles )
            removeImageFiles();
        d->running = false;
        jobFinished( success );
    }
}


// this is basically the same code as in K3b::DvdJob... :(
// perhaps this should be moved to some K3b::GrowisofsHandler which also parses the growisofs output?
bool K3b::DvdCopyJob::waitForDvd()
{
    if ( !K3b::Device::isDvdMedia( d->sourceDiskInfo.mediaType() ) &&
         !K3b::Device::isBdMedia( d->sourceDiskInfo.mediaType() ) ) {
        // this should NEVER happen
        emit infoMessage( i18n( "Unsupported media type: %1" , K3b::Device::mediaTypeString( d->sourceDiskInfo.mediaType() ) ), MessageError );
        return false;
    }

    Device::MediaType m = waitForMedium( m_writerDevice,
                                         K3b::Device::STATE_EMPTY,
                                         Device::MEDIA_WRITABLE_DVD|Device::MEDIA_WRITABLE_BD,
                                         d->sourceDiskInfo.size() );

    if( m == Device::MEDIA_UNKNOWN ) {
        cancel();
        return false;
    }

    else {
        // -------------------------------
        // DVD Plus
        // -------------------------------
        if( m & K3b::Device::MEDIA_DVD_PLUS_ALL ) {

            if ( m & ( Device::MEDIA_DVD_PLUS_R|Device::MEDIA_DVD_PLUS_R_DL ) )
                d->usedWritingMode = K3b::WritingModeSao;
            else
                d->usedWritingMode = K3b::WritingModeRestrictedOverwrite;

            if( m_simulate ) {
                if( !questionYesNo( i18n("%1 media do not support write simulation. "
                                         "Do you really want to continue? The disc will actually be "
                                         "written to.", Device::mediaTypeString(m, true)),
                                    i18n("No Simulation with %1", Device::mediaTypeString(m, true)) ) ) {
                    cancel();
                    return false;
                }

//	m_simulate = false;
                emit newTask( i18n("Writing DVD copy") );
            }

            if( m_writingMode != K3b::WritingModeAuto && m_writingMode != K3b::WritingModeRestrictedOverwrite )
                emit infoMessage( i18n("Writing mode ignored when writing DVD+R(W) media."), MessageInfo );

            emit infoMessage( i18n("Writing %1.", Device::mediaTypeString( m, true ) ), MessageInfo );
        }

        // -------------------------------
        // DVD Minus
        // -------------------------------
        else if ( m & K3b::Device::MEDIA_DVD_MINUS_ALL ) {
            if( m_simulate && !m_writerDevice->dvdMinusTestwrite() ) {
                if( !questionYesNo( i18n("Your writer (%1 %2) does not support simulation with DVD-R(W) media. "
                                         "Do you really want to continue? The media will actually be "
                                         "written to.",
                                         m_writerDevice->vendor(),
                                         m_writerDevice->description()),
                                    i18n("No Simulation with DVD-R(W)") ) ) {
                    cancel();
                    return false;
                }

//	m_simulate = false;
            }

            //
            // We do not default to DAO in onthefly mode since otherwise growisofs would
            // use the size from the PVD to reserve space on the DVD and that can be bad
            // if this size is wrong
            // With growisofs 5.15 we have the option to specify the size of the image to be written in DAO mode.
            //
//       bool sizeWithDao = ( k3bcore->externalBinManager()->binObject( "growisofs" ) &&
// 			   k3bcore->externalBinManager()->binObject( "growisofs" )->version >= K3b::Version( 5, 15, -1 ) );


            // TODO: check for feature 0x21

            if( m & K3b::Device::MEDIA_DVD_RW_OVWR ) {
                emit infoMessage( i18n("Writing DVD-RW in restricted overwrite mode."), MessageInfo );
                d->usedWritingMode = K3b::WritingModeRestrictedOverwrite;
            }
            else if( m & (K3b::Device::MEDIA_DVD_RW_SEQ|
                          K3b::Device::MEDIA_DVD_RW) ) {
                if( m_writingMode == K3b::WritingModeSao ) {
// 	    ( m_writingMode ==  K3b::WritingModeAuto &&
// 	      ( sizeWithDao || !m_onTheFly ) ) ) {
                    emit infoMessage( i18n("Writing DVD-RW in DAO mode."), MessageInfo );
                    d->usedWritingMode = K3b::WritingModeSao;
                }
                else {
                    emit infoMessage( i18n("Writing DVD-RW in incremental mode."), MessageInfo );
                    d->usedWritingMode = K3b::WritingModeIncrementalSequential;
                }
            }
            else {

                // FIXME: DVD-R DL jump and stuff

                if( m_writingMode == K3b::WritingModeRestrictedOverwrite )
                    emit infoMessage( i18n("Restricted Overwrite is not possible with DVD-R media."), MessageInfo );

                if( m_writingMode == K3b::WritingModeSao ) {
// 	    ( m_writingMode ==  K3b::WritingModeAuto &&
// 	      ( sizeWithDao || !m_onTheFly ) ) ) {
                    emit infoMessage( i18n("Writing %1 in DAO mode.",K3b::Device::mediaTypeString(m, true) ), MessageInfo );
                    d->usedWritingMode = K3b::WritingModeSao;
                }
                else {
                    emit infoMessage( i18n("Writing %1 in incremental mode.",K3b::Device::mediaTypeString(m, true) ), MessageInfo );
                    d->usedWritingMode = K3b::WritingModeIncrementalSequential;
                }
            }
        }


        // -------------------------------
        // Blu-ray
        // -------------------------------
        else {
            d->usedWritingMode = K3b::WritingModeSao;

            if( m_simulate ) {
                if( !questionYesNo( i18n("%1 media do not support write simulation. "
                                         "Do you really want to continue? The disc will actually be "
                                         "written to.", Device::mediaTypeString(m, true)),
                                    i18n("No Simulation with %1", Device::mediaTypeString(m, true)) ) ) {
                    cancel();
                    return false;
                }

                m_simulate = false;
                emit newTask( i18n("Writing BD copy") );
            }

            emit infoMessage( i18n("Writing %1.", Device::mediaTypeString(m, true) ), MessageInfo );
        }
    }

    return true;
}



void K3b::DvdCopyJob::removeImageFiles()
{
    if( QFile::exists( m_imagePath ) ) {
        d->imageFile.remove();
        emit infoMessage( i18n("Removed image file %1",m_imagePath), K3b::Job::MessageSuccess );
    }
}


QString K3b::DvdCopyJob::jobDescription() const
{
    if( m_onlyCreateImage ) {
        return i18n("Creating Image");
    }
    else {
        if( m_onTheFly )
            return i18n("Copying DVD or BD On-The-Fly");
        else
            return i18n("Copying DVD or BD");
    }
}


QString K3b::DvdCopyJob::jobDetails() const
{
    return i18np("Creating 1 copy",
                 "Creating %1 copies",
                 (m_simulate||m_onlyCreateImage) ? 1 : m_copies );
}


QString K3b::DvdCopyJob::jobSource() const
{
    if( Device::Device* device = readingDevice() )
        return device->vendor() + ' ' + device->description();
    else
        return QString();
}


QString K3b::DvdCopyJob::jobTarget() const
{
    if( Device::Device* device = writer() )
        return device->vendor() + ' ' + device->description();
    else
        return m_imagePath;
}


void K3b::DvdCopyJob::setVerifyData( bool b )
{
    d->verifyData = b;
}

#include "k3bdvdcopyjob.moc"
