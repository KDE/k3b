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

#include "k3bgrowisofswriter.h"

#include <k3bcore.h>
#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bprocess.h>
#include <k3bexternalbinmanager.h>
#include <k3bversion.h>
#include <k3bdiskinfo.h>
#include <k3bglobals.h>
#include <k3bthroughputestimator.h>
#include "k3bgrowisofshandler.h"
#include <k3bpipebuffer.h>
#include <k3bglobalsettings.h>
#include <k3bdeviceglobals.h>

#include <klocale.h>
#include <kdebug.h>
#include <kglobal.h>

#include <qfile.h>

#include <unistd.h>


class K3b::GrowisofsWriter::Private
{
public:
    Private()
        : writingMode( K3b::WRITING_MODE_AUTO ),
          closeDvd(false),
          multiSession(false),
          process( 0 ),
          growisofsBin( 0 ),
          trackSize(-1),
          layerBreak(0),
          usingRingBuffer(false),
          ringBuffer(0) {
    }

    K3b::WritingMode writingMode;
    bool closeDvd;
    bool multiSession;
    K3b::Process* process;
    const K3b::ExternalBin* growisofsBin;
    QString image;

    bool success;
    bool canceled;
    bool finished;

    QTime lastSpeedCalculationTime;
    int lastSpeedCalculationBytes;
    int lastProgress;
    unsigned int lastProgressed;
    double lastWritingSpeed;

    bool writingStarted;

    K3b::ThroughputEstimator* speedEst;
    K3b::GrowisofsHandler* gh;

    // used in DAO with growisofs >= 5.15
    long trackSize;

    long layerBreak;

    unsigned long long overallSizeFromOutput;
    long long firstSizeFromOutput;

    QFile inputFile;

    bool usingRingBuffer;
    K3b::PipeBuffer* ringBuffer;

    QString multiSessionInfo;

    int burnedMediumType;

    int speedMultiplicator() const {
        return ( burnedMediumType & K3b::Device::MEDIA_BD_ALL ? K3b::Device::SPEED_FACTOR_BD : K3b::Device::SPEED_FACTOR_DVD );
    }
};


K3b::GrowisofsWriter::GrowisofsWriter( K3b::Device::Device* dev, K3b::JobHandler* hdl,
                                        QObject* parent )
    : K3b::AbstractWriter( dev, hdl, parent )
{
    d = new Private;
    d->speedEst = new K3b::ThroughputEstimator( this );
    connect( d->speedEst, SIGNAL(throughput(int)),
             this, SLOT(slotThroughput(int)) );

    d->gh = new K3b::GrowisofsHandler( this );
    connect( d->gh, SIGNAL(infoMessage(const QString&, int)),
             this,SIGNAL(infoMessage(const QString&, int)) );
    connect( d->gh, SIGNAL(newSubTask(const QString&)),
             this, SIGNAL(newSubTask(const QString&)) );
    connect( d->gh, SIGNAL(buffer(int)),
             this, SIGNAL(buffer(int)) );
    connect( d->gh, SIGNAL(deviceBuffer(int)),
             this, SIGNAL(deviceBuffer(int)) );
    connect( d->gh, SIGNAL(flushingCache()),
             this, SLOT(slotFlushingCache()) );
}


K3b::GrowisofsWriter::~GrowisofsWriter()
{
    delete d->process;
    delete d;
}


bool K3b::GrowisofsWriter::active() const
{
    return (d->process ? d->process->isRunning() : false);
}


bool K3b::GrowisofsWriter::closeFd()
{
    if( d->process ) {
        if( d->usingRingBuffer )
            return !::close( d->ringBuffer->inFd() );
        else {
            d->process->closeWriteChannel();
            return true;
        }
    }
    else
        return false;
}


bool K3b::GrowisofsWriter::prepareProcess()
{
    d->growisofsBin = k3bcore->externalBinManager()->binObject( "growisofs" );
    if( !d->growisofsBin ) {
        emit infoMessage( i18n("Could not find %1 executable.",QString("growisofs")), ERROR );
        return false;
    }

    if( d->growisofsBin->version < K3b::Version( 5, 10 ) ) {
        emit infoMessage( i18n("Growisofs version %1 is too old. "
                               "K3b needs at least version 5.10.",d->growisofsBin->version),
                          ERROR );
        return false;
    }

    emit debuggingOutput( QLatin1String( "Used versions" ), QLatin1String( "growisofs: " ) + d->growisofsBin->version );

    if( !d->growisofsBin->copyright.isEmpty() )
        emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3",QString("growisofs")
                               ,d->growisofsBin->version,d->growisofsBin->copyright), INFO );


    //
    // The growisofs bin is ready. Now we add the parameters
    //
    delete d->process;
    d->process = new K3b::Process();
    d->process->setRunPrivileged(true);
    //  d->process->setPriority( K3Process::PrioHighest );
    d->process->setSplitStdout(true);
    d->process->setRawStdin(true);
    connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotReceivedStderr(const QString&)) );
    connect( d->process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotReceivedStderr(const QString&)) );
    connect( d->process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotProcessExited(int, QProcess::ExitStatus)) );


    //
    // growisofs < 5.20 wants the tracksize to be a multiple of 16 (1 ECC block: 16*2048 bytes)
    // we simply pad ourselves.
    //
    // But since the writer itself properly pads or writes a longer lead-out we don't really need
    // to write zeros. We just tell growisofs to reserve a multiple of 16 blocks.
    // This is only releveant in DAO mode anyway.
    //
    // FIXME: seems as we also need this for double layer writing. Better make it the default and
    //        actually write the pad bytes. The only possibility I see right now is to add a padding option
    //        to the pipebuffer.
    int trackSizePadding = 0;
    if( d->trackSize > 0 && d->growisofsBin->version < K3b::Version( 5, 20 ) ) {
        if( d->trackSize % 16 ) {
            trackSizePadding = (16 - d->trackSize%16);
            kDebug() << "(K3b::GrowisofsWriter) need to pad " << trackSizePadding << " blocks.";
        }
    }


    *d->process << d->growisofsBin;

    // set this var to true to enable the ringbuffer
    d->usingRingBuffer = !d->growisofsBin->hasFeature( "buffer" );

    QString s = burnDevice()->blockDeviceName() + "=";
    if( d->usingRingBuffer || d->image.isEmpty() ) {
        // we always read from stdin since the ringbuffer does the actual reading from the source
        s += "/dev/fd/0";
    }
    else
        s += d->image;

    if( d->multiSession && !d->multiSessionInfo.isEmpty() )
        *d->process << "-C" << d->multiSessionInfo;

    if( d->multiSession )
        *d->process << "-M";
    else
        *d->process << "-Z";
    *d->process << s;


    if( !d->image.isEmpty() && d->usingRingBuffer ) {
        d->inputFile.setFileName( d->image );
        d->trackSize = (K3b::filesize( d->image ) + 1024) / 2048;
        if( !d->inputFile.open( QIODevice::ReadOnly ) ) {
            emit infoMessage( i18n("Could not open file %1.",d->image), ERROR );
            return false;
        }
    }

    // now we use the force (luke ;) do not reload the dvd, K3b does that.
    *d->process << "-use-the-force-luke=notray";

    // we check for existing filesystems ourselves, so we always force the overwrite...
    *d->process << "-use-the-force-luke=tty";

    // we do the 4GB boundary check ourselves
    *d->process << "-use-the-force-luke=4gms";

    bool dvdCompat = d->closeDvd;

    // DL writing with forced layer break
    if( d->layerBreak > 0 ) {
        *d->process << "-use-the-force-luke=break:" + QString::number(d->layerBreak);
        dvdCompat = true;
    }

    // the tracksize parameter takes priority over the dao:tracksize parameter since growisofs 5.18
    else if( d->growisofsBin->hasFeature( "tracksize" ) && d->trackSize > 0 )
        *d->process << "-use-the-force-luke=tracksize:" + QString::number(d->trackSize + trackSizePadding);

    if( simulate() )
        *d->process << "-use-the-force-luke=dummy";

    if( d->writingMode == K3b::WRITING_MODE_DAO ) {
        dvdCompat = true;
        if( d->growisofsBin->hasFeature( "daosize" ) && d->trackSize > 0 )
            *d->process << "-use-the-force-luke=dao:" + QString::number(d->trackSize + trackSizePadding);
        else
            *d->process << "-use-the-force-luke=dao";
        d->gh->reset( burnDevice(), true );
    }
    else
        d->gh->reset( burnDevice(), false );

    d->burnedMediumType = burnDevice()->mediaType();

    //
    // Never use the -dvd-compat parameter with DVD+RW media
    // because the only thing it does is creating problems.
    // Normally this should be done in growisofs
    //
    int mediaType = burnDevice()->mediaType();
    if( dvdCompat &&
        mediaType != K3b::Device::MEDIA_DVD_PLUS_RW &&
        mediaType != K3b::Device::MEDIA_DVD_RW_OVWR )
        *d->process << "-dvd-compat";

    //
    // Some DVD writers do not allow changing the writing speed so we allow
    // the user to ignore the speed setting
    //
    int speed = burnSpeed();
    if( speed >= 0 ) {
        if( speed == 0 ) {
            // try to determine the writeSpeed
            // if it fails determineOptimalWriteSpeed() will return 0 and
            // the choice is left to growisofs which means that the choice is
            // really left to the drive since growisofs does not change the speed
            // if no option is given
            speed = burnDevice()->determineMaximalWriteSpeed();
        }

        if( speed != 0 ) {
            if ( d->burnedMediumType & K3b::Device::MEDIA_DVD_ALL ) {
                // speed may be a float number. example: DVD+R(W): 2.4x
                *d->process << QString("-speed=%1").arg( speed%1385 > 0
                                                         ? QString::number( (float)speed/1385.0, 'f', 1 )
                                                         : QString::number( speed/1385 ) );
            }
            else if ( d->burnedMediumType & K3b::Device::MEDIA_BD_ALL ) {
                *d->process << QString("-speed=%1").arg( QString::number( speed/4496 ) );
            }
        }
    }

    if( k3bcore->globalSettings()->overburn() )
        *d->process << "-overburn";

    if( !d->usingRingBuffer && d->growisofsBin->hasFeature( "buffer" ) ) {
        bool manualBufferSize = k3bcore->globalSettings()->useManualBufferSize();
        int bufSize = ( manualBufferSize ? k3bcore->globalSettings()->bufferSize() : 32 );
        *d->process << QString("-use-the-force-luke=bufsize:%1m").arg(bufSize);
    }

    // additional user parameters from config
    const QStringList& params = d->growisofsBin->userParameters();
    for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
        *d->process << *it;

    emit debuggingOutput( "Burned media", K3b::Device::mediaTypeString(mediaType) );

    return true;
}


void K3b::GrowisofsWriter::start()
{
    jobStarted();

    d->lastWritingSpeed = 0;
    d->lastProgressed = 0;
    d->lastProgress = 0;
    d->firstSizeFromOutput = -1;
    d->lastSpeedCalculationTime = QTime::currentTime();
    d->lastSpeedCalculationBytes = 0;
    d->writingStarted = false;
    d->canceled = false;
    d->speedEst->reset();
    d->finished = false;

    if( !prepareProcess() ) {
        jobFinished( false );
    }
    else {

        kDebug() << "***** " << d->growisofsBin->name() << " parameters:\n";
        QString s = d->process->joinedArgs();
        kDebug() << s << flush;
        emit debuggingOutput( d->growisofsBin->name() + " command:", s);


        emit newSubTask( i18n("Preparing write process...") );

        // FIXME: check the return value
        if( K3b::isMounted( burnDevice() ) ) {
            emit infoMessage( i18n("Unmounting medium"), INFO );
            K3b::unmount( burnDevice() );
        }

        // block the device (including certain checks)
        k3bcore->blockDevice( burnDevice() );

        // lock the device for good in this process since it will
        // be opened in the growisofs process
        burnDevice()->close();
        burnDevice()->usageLock();

        if( !d->process->start( K3Process::All ) ) {
            // something went wrong when starting the program
            // it "should" be the executable
            kDebug() << "(K3b::GrowisofsWriter) could not start " << d->growisofsBin->path;
            emit infoMessage( i18n("Could not start %1.",d->growisofsBin->name()), K3b::Job::ERROR );
            jobFinished(false);
        }
        else {
            if( simulate() ) {
                emit newTask( i18n("Simulating") );
                emit infoMessage( i18n("Starting simulation..."),
                                  K3b::Job::INFO );
            }
            else {
                emit newTask( i18n("Writing") );
                emit infoMessage( i18n("Starting disc write..."), K3b::Job::INFO );
            }

            d->gh->handleStart();

            // create the ring buffer
            if( d->usingRingBuffer ) {
                if( !d->ringBuffer ) {
                    d->ringBuffer = new K3b::PipeBuffer( this, this );
                    connect( d->ringBuffer, SIGNAL(percent(int)), this, SIGNAL(buffer(int)) );
                    connect( d->ringBuffer, SIGNAL(finished(bool)), this, SLOT(slotRingBufferFinished(bool)) );
                }

                d->ringBuffer->writeToFd( d->process->stdinFd() );
                bool manualBufferSize = k3bcore->globalSettings()->useManualBufferSize();
                int bufSize = ( manualBufferSize ? k3bcore->globalSettings()->bufferSize() : 20 );
                d->ringBuffer->setBufferSize( bufSize );

                if( !d->image.isEmpty() )
                    d->ringBuffer->readFromFd( d->inputFile.handle() );

                d->ringBuffer->start();
            }
        }
    }
}


void K3b::GrowisofsWriter::cancel()
{
    if( active() ) {
        d->canceled = true;
        closeFd();
        if( d->usingRingBuffer && d->ringBuffer )
            d->ringBuffer->cancel();
        d->process->kill();
    }
}


void K3b::GrowisofsWriter::setWritingMode( K3b::WritingMode m )
{
    d->writingMode = m;
}


void K3b::GrowisofsWriter::setTrackSize( long size )
{
    d->trackSize = size;
}


void K3b::GrowisofsWriter::setLayerBreak( long lb )
{
    d->layerBreak = lb;
}


void K3b::GrowisofsWriter::setCloseDvd( bool b )
{
    d->closeDvd = b;
}


void K3b::GrowisofsWriter::setMultiSession( bool b )
{
    d->multiSession = b;
}


void K3b::GrowisofsWriter::setImageToWrite( const QString& filename )
{
    d->image = filename;
}


void K3b::GrowisofsWriter::slotReceivedStderr( const QString& line )
{
    emit debuggingOutput( d->growisofsBin->name(), line );

    if( line.contains( "remaining" ) ) {

        if( !d->writingStarted ) {
            d->writingStarted = true;
            emit newSubTask( i18n("Writing data") );
        }

        // parse progress
        int pos = line.indexOf( '/' );
        unsigned long long done = line.left( pos ).toULongLong();
        bool ok = true;
        d->overallSizeFromOutput = line.mid( pos+1, line.indexOf( '(', pos ) - pos - 1 ).toULongLong( &ok );
        if( d->firstSizeFromOutput == -1 )
            d->firstSizeFromOutput = done;
        done -= d->firstSizeFromOutput;
        d->overallSizeFromOutput -= d->firstSizeFromOutput;
        if( ok ) {
            int p = (int)(100 * done / d->overallSizeFromOutput);
            if( p > d->lastProgress ) {
                emit percent( p );
                emit subPercent( p );
                d->lastProgress = p;
            }
            if( (unsigned int)(done/1024/1024) > d->lastProgressed ) {
                d->lastProgressed = (unsigned int)(done/1024/1024);
                emit processedSize( d->lastProgressed, (int)(d->overallSizeFromOutput/1024/1024)  );
                emit processedSubSize( d->lastProgressed, (int)(d->overallSizeFromOutput/1024/1024)  );
            }

            // try parsing write speed (since growisofs 5.11)
            pos = line.indexOf( '@' );
            if( pos != -1 ) {
                pos += 1;
                double speed = line.mid( pos, line.indexOf( 'x', pos ) - pos ).toDouble(&ok);
                if( ok ) {
                    if( d->lastWritingSpeed != speed )
                        emit writeSpeed( (int)(speed*d->speedMultiplicator()), d->speedMultiplicator() );
                    d->lastWritingSpeed = speed;
                }
                else
                    kDebug() << "(K3b::GrowisofsWriter) speed parsing failed: '"
                             << line.mid( pos, line.indexOf( 'x', pos ) - pos ) << "'" << endl;
            }
            else {
                d->speedEst->dataWritten( done/1024 );
            }
        }
        else
            kDebug() << "(K3b::GrowisofsWriter) progress parsing failed: '"
                     << line.mid( pos+1, line.indexOf( '(', pos ) - pos - 1 ).trimmed() << "'" << endl;
    }

    //  else
    // to be able to parse the ring buffer fill in growisofs 6.0 we need to do this all the time
    // FIXME: get rid of the K3b::GrowisofsHandler once it is sure that we do not need the K3b::GrowisofsImager anymore
    d->gh->handleLine( line );
}


void K3b::GrowisofsWriter::slotProcessExited( int exitCode, QProcess::ExitStatus )
{
    d->inputFile.close();

    // release the device within this process
    burnDevice()->usageUnlock();

    // unblock the device
    k3bcore->unblockDevice( burnDevice() );

    if( d->canceled ) {
        if( !d->finished ) {
            d->finished = true;
            // this will unblock and eject the drive and emit the finished/canceled signals
            K3b::AbstractWriter::cancel();
        }
        return;
    }

    d->finished = true;

    // it seems that growisofs sometimes exits with a valid exit code while a write error occurred
    if( (exitCode == 0) && d->gh->error() != K3b::GrowisofsHandler::ERROR_WRITE_FAILED ) {

        int s = d->speedEst->average();
        if( s > 0 )
            emit infoMessage( ki18n("Average overall write speed: %1 KB/s (%2x)")
                              .subs( s )
                              .subs( ( double )s/( double )d->speedMultiplicator(), 0, 'g', 2 ).toString(), INFO );

        if( simulate() )
            emit infoMessage( i18n("Simulation successfully completed"), K3b::Job::SUCCESS );
        else
            emit infoMessage( i18n("Writing successfully completed"), K3b::Job::SUCCESS );

        d->success = true;
    }
    else {
        if( !wasSourceUnreadable() )
            d->gh->handleExit( exitCode );
        d->success = false;
    }

    jobFinished(d->success);
}


void K3b::GrowisofsWriter::slotRingBufferFinished( bool )
{
    if( !d->finished ) {
        d->finished = true;
        // this will unblock and eject the drive and emit the finished/canceled signals
        K3b::AbstractWriter::cancel();
    }
}


void K3b::GrowisofsWriter::slotThroughput( int t )
{
    emit writeSpeed( t, d->speedMultiplicator() );
}


void K3b::GrowisofsWriter::slotFlushingCache()
{
    if( !d->canceled ) {
        //
        // growisofs's progress output stops before 100%, so we do it manually
        //
        emit percent( 100 );
        emit processedSize( d->overallSizeFromOutput/1024/1024,
                            d->overallSizeFromOutput/1024/1024 );
    }
}


void K3b::GrowisofsWriter::setMultiSessionInfo( const QString& info )
{
    d->multiSessionInfo = info;
}

#include "k3bgrowisofswriter.moc"
