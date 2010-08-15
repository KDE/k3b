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

#include "k3bcdrecordwriter.h"

#include "k3bcore.h"
#include "k3bexternalbinmanager.h"
#include "k3bprocess.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3bdevicemanager.h"
#include "k3bdevicehandler.h"
#include "k3bglobals.h"
#include "k3bthroughputestimator.h"
#include "k3bglobalsettings.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qfile.h>

#include <klocale.h>
#include <kdebug.h>
#include <kglobal.h>
#include <ktemporaryfile.h>


Q_DECLARE_METATYPE( QProcess::ExitStatus )


class K3b::CdrecordWriter::Private
{
public:
    Private()
        : cdTextFile(0) {
    }

    const ExternalBin* cdrecordBinObject;
    Process process;

    WritingMode writingMode;
    FormattingMode formattingMode;
    bool totalTracksParsed;
    bool clone;
    bool cue;
    bool multi;
    bool force;
    bool formatting;

    QString cueFile;
    QStringList arguments;

    int currentTrack;
    int totalTracks;
    int totalSize;
    int alreadyWritten;

    int lastFifoValue;

    int cdrecordError;
    bool writingStarted;

    QByteArray rawCdText;

    K3b::ThroughputEstimator* speedEst;
    bool canceled;
    bool usingBurnfree;
    int usedSpeed;

    struct Track {
        int size;
        bool audio;
    };

    QList<Track> tracks;

    QTemporaryFile* cdTextFile;

    Device::MediaType burnedMediaType;
    K3b::Device::SpeedMultiplicator usedSpeedFactor;
};


K3b::CdrecordWriter::CdrecordWriter( K3b::Device::Device* dev, K3b::JobHandler* hdl,
                                      QObject* parent )
    : K3b::AbstractWriter( dev, hdl, parent )
{
    d = new Private();
    d->speedEst = new K3b::ThroughputEstimator( this );
    connect( d->speedEst, SIGNAL(throughput(int)),
             this, SLOT(slotThroughput(int)) );

    d->writingMode = K3b::WritingModeTao;
    d->formattingMode = K3b::FormattingQuick;
    d->clone = false;
    d->cue = false;
    d->multi = false;
    d->force = false;
    d->formatting = false;

    d->process.setSplitStdout(true);
    d->process.setSuppressEmptyLines(true);
    d->process.setFlags( K3bQProcess::RawStdin );
    connect( &d->process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );

    // we use a queued connection to give the process object time to wrap up and return to a correct state
    qRegisterMetaType<QProcess::ExitStatus>();
    connect( &d->process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotProcessExited(int, QProcess::ExitStatus)), Qt::QueuedConnection );
}


K3b::CdrecordWriter::~CdrecordWriter()
{
    delete d->cdTextFile;
    delete d;
}


bool K3b::CdrecordWriter::active() const
{
    return d->process.isRunning();
}


QIODevice* K3b::CdrecordWriter::ioDevice() const
{
    return &d->process;
}


bool K3b::CdrecordWriter::closeFd()
{
    if ( d->process.isRunning() ) {
        d->process.closeWriteChannel();
        return true;
    }
    else {
        return false;
    }
}


void K3b::CdrecordWriter::setDao( bool b )
{
    d->writingMode = ( b ? K3b::WritingModeSao : K3b::WritingModeTao );
}


void K3b::CdrecordWriter::setCueFile( const QString& s)
{
    d->cue = true;
    d->cueFile = s;

    // cuefile only works in DAO mode
    setWritingMode( K3b::WritingModeSao );
}


void K3b::CdrecordWriter::setClone( bool b )
{
    d->clone = b;
}


void K3b::CdrecordWriter::setRawCdText( const QByteArray& a )
{
    d->rawCdText = a;
}


void K3b::CdrecordWriter::setWritingMode( K3b::WritingMode mode )
{
    d->writingMode = mode;
}


void K3b::CdrecordWriter::setFormattingMode( FormattingMode mode )
{
    d->formattingMode = mode;
    d->formatting = true;
}


void K3b::CdrecordWriter::setMulti( bool b )
{
    d->multi = b;
}


void K3b::CdrecordWriter::setForce( bool b )
{
    d->force = b;
}


bool K3b::CdrecordWriter::prepareProcess()
{
    d->cdrecordBinObject = k3bcore->externalBinManager()->binObject("cdrecord");

    if( !d->cdrecordBinObject ) {
        emit infoMessage( i18n("Could not find %1 executable.", QLatin1String("cdrecord")), MessageError );
        return false;
    }

    d->process.clearProgram();

    d->burnedMediaType = burnDevice()->mediaType();

    d->process << d->cdrecordBinObject;

    // display progress
    d->process << "-v";

    if( d->cdrecordBinObject->hasFeature( "gracetime") )
        d->process << "gracetime=2";  // 2 is the lowest allowed value (Joerg, why do you do this to us?)

    // Again we assume the device to be set!
    d->process << QString("dev=%1").arg(K3b::externalBinDeviceParameter(burnDevice(), d->cdrecordBinObject));

    d->usedSpeedFactor = K3b::speedMultiplicatorForMediaType( d->burnedMediaType );
    d->usedSpeed = burnSpeed();
    if( d->usedSpeed == 0 ) {
        // try to determine the writeSpeed
        // if it fails determineMaximalWriteSpeed() will return 0 and
        // the choice is left to cdrecord
        d->usedSpeed = burnDevice()->determineMaximalWriteSpeed();
    }

    if( d->usedSpeed != 0 )
        d->process << QString("speed=%1").arg( formatWritingSpeedFactor( d->usedSpeed, d->burnedMediaType, SpeedFormatInteger ) );


    if ( K3b::Device::isBdMedia( d->burnedMediaType ) ) {
        if ( !d->cdrecordBinObject->hasFeature( "blu-ray" ) ) {
            emit infoMessage( i18n( "Cdrecord version %1 does not support Blu-ray writing." ,d->cdrecordBinObject->version() ), MessageError );
            // FIXME: add a way to fail the whole thing here
        }
        d->process << "-sao";
    }
    else if ( K3b::Device::isDvdMedia( d->burnedMediaType ) ) {
        // cdrecord only supports SAO for DVD
        d->process << "-sao";


#ifdef __GNUC__
#warning Enable layer jump mode: add it to K3b::WritingMode and to the GUI
#endif
        // if( d->writingMode == Device::WRITINGMODE_LAYER_JUMP ) {
//             d->process << "-driveropts=layerbreak";
//         }
    }
    else if( K3b::Device::isCdMedia( d->burnedMediaType ) ) {
        if( d->writingMode == K3b::WritingModeSao || d->cue ) {
            if( burnDevice()->dao() )
                d->process << "-sao";
            else {
                if( d->cdrecordBinObject->hasFeature( "tao" ) )
                    d->process << "-tao";
                emit infoMessage( i18n("Writer does not support disk at once (DAO) recording"), MessageWarning );
            }
        }
        else if( d->writingMode == K3b::WritingModeRaw ) {
            if( burnDevice()->supportsWritingMode( K3b::Device::WRITINGMODE_RAW_R96R ) )
                d->process << "-raw96r";
            else if( burnDevice()->supportsWritingMode( K3b::Device::WRITINGMODE_RAW_R16 ) )
                d->process << "-raw16";
            else if( burnDevice()->supportsWritingMode( K3b::Device::WRITINGMODE_RAW_R96P ) )
                d->process << "-raw96p";
            else {
                emit infoMessage( i18n("Writer does not support raw writing."), MessageWarning );
                if( d->cdrecordBinObject->hasFeature( "tao" ) )
                    d->process << "-tao";
            }
        }
        else if( d->cdrecordBinObject->hasFeature( "tao" ) )
            d->process << "-tao";
    }
    else {
        emit infoMessage( i18n( "Cdrecord does not support writing %1 media." , K3b::Device::mediaTypeString( d->burnedMediaType ) ), MessageError );
        // FIXME: add a way to fail the whole thing here
    }

    if( simulate() )
        d->process << "-dummy";

    d->usingBurnfree = false;
    if( k3bcore->globalSettings()->burnfree() ) {
        if( burnDevice()->burnproof() ) {

            d->usingBurnfree = true;

            // with cdrecord 1.11a02 burnproof was renamed to burnfree
            if( d->cdrecordBinObject->hasFeature( "burnproof" ) )
                d->process << "driveropts=burnproof";
            else
                d->process << "driveropts=burnfree";
        }
        else
            emit infoMessage( i18n("Writer does not support buffer underrun free recording (Burnfree)"), MessageWarning );
    }

    if( k3bcore->globalSettings()->force() || d->force ) {
        d->process << "-force";
        emit infoMessage( i18n("'Force unsafe operations' enabled."), MessageWarning );
    }

    if( d->cue ) {
        d->process.setWorkingDirectory( d->cueFile );
        d->process << QString("cuefile=%1").arg( d->cueFile );
    }

    if( d->clone )
        d->process << "-clone";

    if( d->multi )
        d->process << "-multi";

    if( d->formatting ) {
        switch( d->formattingMode ) {
            case FormattingComplete:
                d->process << "blank=all";
                break;
            case FormattingQuick:
                d->process << "blank=fast";
                break;
        }
    }

    if( d->rawCdText.size() > 0 ) {
        delete d->cdTextFile;
        // yes, we do want to use QTemporaryFile and not KTemporaryFile because cdrecord
        // might be started suid root and the KDE tmp might be on an nfs mounted partition
        // (Mandriva for example uses ~/tmp)
        d->cdTextFile = new QTemporaryFile();
        if ( !d->cdTextFile->open() ||
             d->cdTextFile->write( d->rawCdText ) != d->rawCdText.size() ||
            !d->cdTextFile->flush() ) {
            emit infoMessage( i18n( "Failed to write temporary file '%1'", d->cdTextFile->fileName() ), MessageError );
            return false;
        }
        d->process << "textfile=" + d->cdTextFile->fileName();
    }

    bool manualBufferSize = k3bcore->globalSettings()->useManualBufferSize();
    if( manualBufferSize ) {
        d->process << QString("fs=%1m").arg( k3bcore->globalSettings()->bufferSize() );
    }

    bool overburn = k3bcore->globalSettings()->overburn();
    if( overburn ) {
        if( d->cdrecordBinObject->hasFeature("overburn") ) {
            if ( k3bcore->globalSettings()->force() )
                d->process << "-ignsize";
            else
                d->process << "-overburn";
        }
        else {
            emit infoMessage( i18n("Cdrecord %1 does not support overburning.",d->cdrecordBinObject->version()), MessageWarning );
        }
    }

    // additional user parameters from config
    const QStringList& params = d->cdrecordBinObject->userParameters();
    for( QStringList::const_iterator it = params.constBegin(); it != params.constEnd(); ++it )
        d->process << *it;

    // add the user parameters
    for( QStringList::const_iterator it = d->arguments.constBegin(); it != d->arguments.constEnd(); ++it )
        d->process << *it;

    return true;
}


K3b::CdrecordWriter* K3b::CdrecordWriter::addArgument( const QString& arg )
{
    d->arguments.append( arg );
    return this;
}


void K3b::CdrecordWriter::clearArguments()
{
    d->arguments.clear();
}


void K3b::CdrecordWriter::start()
{
    jobStarted();

    d->canceled = false;
    d->speedEst->reset();
    d->writingStarted = false;

    if ( !prepareProcess() ) {
        jobFinished(false);
        return;
    }

    emit debuggingOutput( QLatin1String( "Used versions" ), QLatin1String( "cdrecord: " ) + d->cdrecordBinObject->version() );

    if( !d->cdrecordBinObject->copyright().isEmpty() )
        emit infoMessage( i18n("Using %1 %2 – Copyright © %3"
                               ,(d->cdrecordBinObject->hasFeature( "wodim" ) ? "Wodim" : "Cdrecord" )
                               ,d->cdrecordBinObject->version()
                               ,d->cdrecordBinObject->copyright()), MessageInfo );


    kDebug() << "***** " << d->cdrecordBinObject->name() << " parameters:\n";
    QString s = d->process.joinedArgs();
    kDebug() << s << flush;
    emit debuggingOutput( d->cdrecordBinObject->name() + " command:", s);

    d->currentTrack = 0;
    d->cdrecordError = UNKNOWN;
    d->totalTracksParsed = false;
    d->alreadyWritten = 0;
    d->tracks.clear();
    d->totalSize = 0;

    emit newSubTask( i18n("Preparing write process...") );

    // FIXME: check the return value
    if( K3b::isMounted( burnDevice() ) ) {
        emit infoMessage( i18n("Unmounting medium"), MessageInfo );
        K3b::unmount( burnDevice() );
    }

    // block the device (including certain checks)
    k3bcore->blockDevice( burnDevice() );

    // lock the device for good in this process since it will
    // be opened in the cdrecord process
    burnDevice()->close();
    burnDevice()->usageLock();

    if( !d->process.start( KProcess::MergedChannels ) ) {
        // something went wrong when starting the program
        // it "should" be the executable
        kDebug() << "(K3b::CdrecordWriter) could not start " << d->cdrecordBinObject->name();
        emit infoMessage( i18n("Could not start %1.",d->cdrecordBinObject->name()), K3b::Job::MessageError );
        jobFinished(false);
    }
    else {
        const QString formattedSpeed = formatWritingSpeedFactor( d->usedSpeed, d->burnedMediaType, SpeedFormatInteger );
        const QString formattedMode = writingModeString( d->writingMode );
        // FIXME: these messages should also take DVD into account.
        if( simulate() ) {
            emit newTask( i18n("Simulating") );
            if ( d->burnedMediaType & Device::MEDIA_DVD_PLUS_ALL )
                // xgettext: no-c-format
                emit infoMessage( i18n("Starting simulation at %1x speed...", formattedSpeed ), Job::MessageInfo );
            else
                emit infoMessage( i18n("Starting %1 simulation at %2x speed...", formattedMode, formattedSpeed ), Job::MessageInfo );
        }
        else {
            emit newTask( i18n("Writing") );
            if ( d->burnedMediaType & Device::MEDIA_DVD_PLUS_ALL )
                // xgettext: no-c-format
                emit infoMessage( i18n("Starting writing at %1x speed...", formattedSpeed ), Job::MessageInfo );
            else
                emit infoMessage( i18n("Starting %1 writing at %2x speed...", formattedMode, formattedSpeed ), Job::MessageInfo );
        }
    }
}


void K3b::CdrecordWriter::cancel()
{
    if( active() ) {
        d->canceled = true;
        if( d->process.isRunning() )
            d->process.terminate();
    }
}


void K3b::CdrecordWriter::slotStdLine( const QString& line )
{
    static QRegExp s_burnfreeCounterRx( "^BURN\\-Free\\swas\\s(\\d+)\\stimes\\sused" );
    static QRegExp s_burnfreeCounterRxPredict( "^Total\\sof\\s(\\d+)\\s\\spossible\\sbuffer\\sunderruns\\spredicted" );

    // tracknumber: cap(1)
    // done: cap(2)
    // complete: cap(3)
    // fifo: cap(4)  (it seems as if some patched cdrecord versions do not emit the fifo info but only the buf... :(
    // buffer: cap(5)
    static QRegExp s_progressRx( "Track\\s(\\d\\d)\\:\\s*(\\d*)\\sof\\s*(\\d*)\\sMB\\swritten\\s(?:\\(fifo\\s*(\\d*)\\%\\)\\s*)?(?:\\[buf\\s*(\\d*)\\%\\])?.*" );

    emit debuggingOutput( d->cdrecordBinObject->name(), line );

    //
    // Progress and toc parsing
    //

    if( line.startsWith( "Track " ) ) {
        if( !d->totalTracksParsed ) {
            // this is not the progress display but the list of tracks that will get written
            // we always extract the tracknumber to get the highest at last
            bool ok;
            int tt = line.mid( 6, 2 ).toInt(&ok);

            if( ok ) {
                struct Private::Track track;
                track.audio  = ( line.mid( 10, 5 ) == "audio" );

                d->totalTracks = tt;

                int sizeStart = line.indexOf( QRegExp("\\d"), 10 );
                int sizeEnd = line.indexOf( "MB", sizeStart );
                track.size = line.mid( sizeStart, sizeEnd-sizeStart ).toInt(&ok);

                if( ok ) {
                    d->tracks.append(track);
                    d->totalSize += track.size;
                }
                else
                    kDebug() << "(K3b::CdrecordWriter) track number parse error: "
                             << line.mid( sizeStart, sizeEnd-sizeStart );
            }
            else
                kDebug() << "(K3b::CdrecordWriter) track number parse error: "
                         << line.mid( 6, 2 );
        }

        else if( s_progressRx.exactMatch( line ) ) {
            //      int num = s_progressRx.cap(1).toInt();
            int made = s_progressRx.cap(2).toInt();
            int size = s_progressRx.cap(3).toInt();
            int fifo = s_progressRx.cap(4).toInt();

            emit buffer( fifo );
            d->lastFifoValue = fifo;

            if( s_progressRx.numCaptures() > 4 )
                emit deviceBuffer( s_progressRx.cap(5).toInt() );

            //
            // cdrecord's output sucks a bit.
            // we get track sizes that differ from the sizes in the progress
            // info since these are dependant on the writing mode.
            // so we just use the track sizes and do a bit of math...
            //

            if( d->tracks.count() > d->currentTrack-1 && size > 0 ) {
                double convV = (double)d->tracks[d->currentTrack-1].size/(double)size;
                made = (int)((double)made * convV);
                size = d->tracks[d->currentTrack-1].size;
            }
            else {
                kError() << "(K3b::CdrecordWriter) Did not parse all tracks sizes!" << endl;
            }

            if( !d->writingStarted ) {
                d->writingStarted = true;
                emit newSubTask( i18n("Writing data") );
            }

            if( size > 0 ) {
                emit processedSubSize( made, size );
                emit subPercent( 100*made/size );
            }

            if( d->totalSize > 0 ) {
                emit processedSize( d->alreadyWritten+made, d->totalSize );
                emit percent( 100*(d->alreadyWritten+made)/d->totalSize );
            }

            d->speedEst->dataWritten( (d->alreadyWritten+made)*1024 );
        }
    }

    //
    // Cdrecord starts all error and warning messages with it's path
    // With Debian's script it starts with cdrecord (or /usr/bin/cdrecord or whatever! I hate this script!)
    //

    else if( line.startsWith( "cdrecord" ) ||
             line.startsWith( d->cdrecordBinObject->path() ) ||
             line.startsWith( d->cdrecordBinObject->path().left(d->cdrecordBinObject->path().length()-5) ) ) {
        // get rid of the path and the following colon and space
        QString errStr = line.mid( line.indexOf(':') + 2 );

        if( errStr.startsWith( "Drive does not support SAO" ) ) {
            emit infoMessage( i18n("DAO (Disk At Once) recording not supported with this writer"), K3b::Job::MessageError );
            emit infoMessage( i18n("Please choose TAO (Track At Once) and try again"), K3b::Job::MessageError );
        }
        else if( errStr.startsWith( "Drive does not support RAW" ) ) {
            emit infoMessage( i18n("RAW recording not supported with this writer"), K3b::Job::MessageError );
        }
        else if( errStr.startsWith("Input/output error.") ) {
            emit infoMessage( i18n("Input/output error. Not necessarily serious."), MessageWarning );
        }
        else if( errStr.startsWith("shmget failed") ) {
            d->cdrecordError = SHMGET_FAILED;
        }
        else if( errStr.startsWith("OPC failed") ) {
            d->cdrecordError = OPC_FAILED;
        }
        else if( errStr.startsWith( "Drive needs to reload the media" ) ) {
            emit infoMessage( i18n("Reloading of medium required"), K3b::Job::MessageInfo );
        }
        else if( errStr.startsWith( "The current problem looks like a buffer underrun" ) ) {
            if( d->cdrecordError == UNKNOWN ) // it is almost never a buffer underrun these days.
                d->cdrecordError = BUFFER_UNDERRUN;
        }
        else if( errStr.startsWith("MessageWarning: Data may not fit") ) {
            bool overburn = k3bcore->globalSettings()->overburn();
            if( overburn && d->cdrecordBinObject->hasFeature("overburn") )
                emit infoMessage( i18n("Trying to write more than the official disk capacity"), K3b::Job::MessageWarning );
            d->cdrecordError = OVERSIZE;
        }
        else if( errStr.startsWith("Bad Option") ) {
            d->cdrecordError = BAD_OPTION;
            // parse option
            int pos = line.indexOf( "Bad Option" ) + 12;
            int len = line.length() - pos - 1;
            emit infoMessage( i18n("No valid %1 option: %2",d->cdrecordBinObject->name(),line.mid(pos, len)),
                              MessageError );
        }
        else if( errStr.startsWith("Cannot set speed/dummy") ) {
            d->cdrecordError = CANNOT_SET_SPEED;
        }
        else if( errStr.startsWith("Cannot open new session") ) {
            d->cdrecordError = CANNOT_OPEN_NEW_SESSION;
        }
        else if( errStr.startsWith("Cannot send CUE sheet") ) {
            d->cdrecordError = CANNOT_SEND_CUE_SHEET;
        }
        else if( errStr.startsWith( "Trying to use ultra high speed" ) ||
                 errStr.startsWith( "Trying to use high speed" ) ||
                 errStr.startsWith( "Probably trying to use ultra high speed" ) ||
                 errStr.startsWith( "You did use a high speed medium on an improper writer" ) ||
                 errStr.startsWith( "You did use a ultra high speed medium on an improper writer" ) ) {
            d->cdrecordError = HIGH_SPEED_MEDIUM;
        }
        else if( errStr.startsWith( "You may have used an ultra low speed medium" ) ) {
            d->cdrecordError = LOW_SPEED_MEDIUM;
        }
        else if( errStr.startsWith( "Permission denied. Cannot open" ) ||
                 errStr.startsWith( "Operation not permitted." ) ) {
            d->cdrecordError = PERMISSION_DENIED;
        }
        else if( errStr.startsWith( "Can only copy session # 1") ) {
            emit infoMessage( i18n("Only session 1 will be cloned."), MessageWarning );
        }
        else if( errStr == "Cannot fixate disk." ) {
            emit infoMessage( i18n("Unable to fixate the disk."), MessageError );
            if( d->cdrecordError == UNKNOWN )
                d->cdrecordError = CANNOT_FIXATE_DISK;
        }
        else if( errStr == "A write error occurred." ) {
            d->cdrecordError = WRITE_ERROR;
        }
        else if( errStr.startsWith( "Try again with cdrecord blank=all." ) ) {
            d->cdrecordError = BLANK_FAILED;
        }
        else if( errStr.startsWith( "faio_wait_on_buffer for reader timed out" ) ) {
            d->cdrecordError = SHORT_READ;
        }
    }

    //
    // All other messages
    //

    else if( line.contains( "at speed" ) ) {
        // parse the speed and inform the user if cdrdao switched it down
        const int pos = line.indexOf( "at speed" );
        const int pos2 = line.indexOf( "in", pos+9 );
        const int speed( double( K3b::speedMultiplicatorForMediaType( d->burnedMediaType ) ) * line.mid( pos+9, pos2-pos-10 ).toDouble() );  // cdrecord-dvd >= 2.01a25 uses 8.0 and stuff
        if( speed > 0 && double( qAbs( speed - d->usedSpeed ) ) > 0.5*double( K3b::speedMultiplicatorForMediaType( d->burnedMediaType ) ) ) {
            // xgettext: no-c-format
            emit infoMessage( i18n("Medium or burner does not support writing at %1x speed", formatWritingSpeedFactor( d->usedSpeed, d->burnedMediaType ) ),
                              K3b::Job::MessageWarning );
            if( speed > d->usedSpeed )
                // xgettext: no-c-format
                emit infoMessage( i18n("Switching burn speed up to %1x", formatWritingSpeedFactor( speed, d->burnedMediaType ) ), K3b::Job::MessageWarning );
            else
                // xgettext: no-c-format
                emit infoMessage( i18n("Switching burn speed down to %1x", formatWritingSpeedFactor( speed, d->burnedMediaType ) ), K3b::Job::MessageWarning );
        }
    }
    else if( line.startsWith( "Starting new" ) ) {
        d->totalTracksParsed = true;
        if( d->currentTrack > 0 ) {// nothing has been written at the start of track 1
            if( d->tracks.count() > d->currentTrack-1 )
                d->alreadyWritten += d->tracks[d->currentTrack-1].size;
            else
                kError() << "(K3b::CdrecordWriter) Did not parse all tracks sizes!";
        }
        else
            emit infoMessage( i18n("Starting disc write"), MessageInfo );

        d->currentTrack++;

        if( d->currentTrack > d->tracks.count() ) {
            kDebug() << "(K3b::CdrecordWriter) need to add dummy track struct.";
            struct Private::Track t;
            t.size = 1;
            t.audio = false;
            d->tracks.append(t);
        }

        kDebug() << "(K3b::CdrecordWriter) writing track " << d->currentTrack << " of " << d->totalTracks << " tracks.";
        emit nextTrack( d->currentTrack, d->totalTracks );
    }
    else if( line.startsWith( "Fixating" ) ) {
        emit newSubTask( i18n("Closing Session") );
    }
    else if( line.startsWith( "Writing lead-in" ) ) {
        d->totalTracksParsed = true;
        emit newSubTask( i18n("Writing Leadin") );
    }
    else if( line.startsWith( "Writing Leadout") ) {
        emit newSubTask( i18n("Writing Leadout") );
    }
    else if( line.startsWith( "Writing pregap" ) ) {
        emit newSubTask( i18n("Writing pregap") );
    }
    else if( line.startsWith( "Performing OPC" ) ) {
        emit infoMessage( i18n("Performing Optimum Power Calibration"), K3b::Job::MessageInfo );
    }
    else if( line.startsWith( "Sending" ) ) {
        emit infoMessage( i18n("Sending CUE sheet"), K3b::Job::MessageInfo );
    }
    else if( line.startsWith( "Turning BURN-Free on" ) || line.startsWith( "BURN-Free is ON") ) {
        emit infoMessage( i18n("Enabled Burnfree"), K3b::Job::MessageInfo );
    }
    else if( line.startsWith( "Turning BURN-Free off" ) ) {
        emit infoMessage( i18n("Disabled Burnfree"), K3b::Job::MessageWarning );
    }
    else if( line.startsWith( "Re-load disk and hit" ) ) {
        // this happens on some notebooks where cdrecord is not able to close the
        // tray itself, so we need to ask the user to do so
        blockingInformation( i18n("Please reload the medium and press 'ok'"),
                             i18n("Unable to close the tray") );

        // now send a <CR> to cdrecord
        // hopefully this will do it since I have no possibility to test it!
        d->process.write( "\n", 1 );
    }
    else if( s_burnfreeCounterRx.indexIn( line ) ) {
        bool ok;
        int num = s_burnfreeCounterRx.cap(1).toInt(&ok);
        if( ok )
            emit infoMessage( i18np("Burnfree was used once.", "Burnfree was used %1 times.", num), MessageInfo );
    }
    else if( s_burnfreeCounterRxPredict.indexIn( line ) ) {
        bool ok;
        int num = s_burnfreeCounterRxPredict.cap(1).toInt(&ok);
        if( ok )
            emit infoMessage( i18np("Buffer was low once.", "Buffer was low %1 times.", num), MessageInfo );
    }
    else if( line.contains("Medium Error") ) {
        d->cdrecordError = MEDIUM_ERROR;
    }
    else if( line.startsWith( "Error trying to open" ) && line.contains( "(Device or resource busy)" ) ) {
        d->cdrecordError = DEVICE_BUSY;
    }
    else {
        // debugging
        kDebug() << "(" << d->cdrecordBinObject->name() << ") " << line;
    }
}


void K3b::CdrecordWriter::slotProcessExited( int exitCode, QProcess::ExitStatus exitStatus )
{
    // remove temporary cdtext file
    delete d->cdTextFile;
    d->cdTextFile = 0;

    // release the device within this process
    burnDevice()->usageUnlock();

    // unblock the device
    k3bcore->unblockDevice( burnDevice() );

    if( d->canceled ) {
        // this will unblock and eject the drive and emit the finished/canceled signals
        K3b::AbstractWriter::cancel();
        return;
    }


    if( exitStatus == QProcess::NormalExit ) {
        switch( exitCode ) {
        case 0:
        {
            if( d->formatting )
                emit infoMessage( i18n("Erasing successfully completed"), K3b::Job::MessageSuccess );
            else if( simulate() )
                emit infoMessage( i18n("Simulation successfully completed"), K3b::Job::MessageSuccess );
            else
                emit infoMessage( i18n("Writing successfully completed"), K3b::Job::MessageSuccess );

            if( !d->formatting ) {
                int s = d->speedEst->average();
                emit infoMessage( ki18n("Average overall write speed: %1 KB/s (%2x)" )
                                  .subs( s ).subs( ( double )s/( double )d->usedSpeedFactor, 0, 'g', 2 ).toString(),
                                  MessageInfo );
            }

            jobFinished( true );
        }
        break;

        default:
            kDebug() << "(K3b::CdrecordWriter) error: " << exitCode;

            if( d->cdrecordError == UNKNOWN && d->lastFifoValue <= 3 )
                d->cdrecordError = BUFFER_UNDERRUN;

            switch( d->cdrecordError ) {
            case OVERSIZE:
                if( k3bcore->globalSettings()->overburn() &&
                    d->cdrecordBinObject->hasFeature("overburn") )
                    emit infoMessage( i18n("Data did not fit on disk."), MessageError );
                else {
                    emit infoMessage( i18n("Data does not fit on disk."), MessageError );
                    if( d->cdrecordBinObject->hasFeature("overburn") )
                        emit infoMessage( i18n("Enable overburning in the advanced K3b settings to burn anyway."), MessageInfo );
                }
                break;
            case BAD_OPTION:
                // error message has already been emitted earlier since we needed the actual line
                break;
            case SHMGET_FAILED:
                emit infoMessage( i18n("%1 could not reserve shared memory segment of requested size.",d->cdrecordBinObject->name()), MessageError );
                emit infoMessage( i18n("Probably you chose a too large buffer size."), MessageError );
                break;
            case OPC_FAILED:
                emit infoMessage( i18n("OPC failed. Probably the writer does not like the medium."), MessageError );
                break;
            case CANNOT_SET_SPEED:
                emit infoMessage( i18n("Unable to set write speed to %1.",formatWritingSpeedFactor( d->usedSpeed, d->burnedMediaType, SpeedFormatInteger ) ), MessageError );
                emit infoMessage( i18n("Probably this is lower than your writer's lowest writing speed."), MessageError );
                break;
            case CANNOT_SEND_CUE_SHEET:
                emit infoMessage( i18n("Unable to send CUE sheet."), MessageError );
                if( d->writingMode == K3b::WritingModeSao )
                    emit infoMessage( i18n("Sometimes using TAO writing mode solves this issue."), MessageError );
                break;
            case CANNOT_OPEN_NEW_SESSION:
                emit infoMessage( i18n("Unable to open new session."), MessageError );
                emit infoMessage( i18n("Probably a problem with the medium."), MessageError );
                break;
            case CANNOT_FIXATE_DISK:
                emit infoMessage( i18n("The disk might still be readable."), MessageError );
                if( d->writingMode == K3b::WritingModeTao && burnDevice()->dao() )
                    emit infoMessage( i18n("Try DAO writing mode."), MessageError );
                break;
            case PERMISSION_DENIED:
                emit infoMessage( i18n("%1 has no permission to open the device.",QString("cdrecord")), MessageError );
#ifdef BUILD_K3BSETUP
                emit infoMessage( i18n("You may use K3bsetup to solve this problem."), MessageError );
#endif
                break;
            case BUFFER_UNDERRUN:
                emit infoMessage( i18n("Probably a buffer underrun occurred."), MessageError );
                if( !d->usingBurnfree && burnDevice()->burnproof() )
                    emit infoMessage( i18n("Please enable Burnfree or choose a lower burning speed."), MessageError );
                else
                    emit infoMessage( i18n("Please choose a lower burning speed."), MessageError );
                break;
            case HIGH_SPEED_MEDIUM:
                emit infoMessage( i18n("Found a high-speed medium not suitable for the writer being used."), MessageError );
                emit infoMessage( i18n("Use the 'force unsafe operations' option to ignore this."), MessageError );
                break;
            case LOW_SPEED_MEDIUM:
                emit infoMessage( i18n("Found a low-speed medium not suitable for the writer being used."), MessageError );
                emit infoMessage( i18n("Use the 'force unsafe operations' option to ignore this."), MessageError );
                break;
            case MEDIUM_ERROR:
                emit infoMessage( i18n("Most likely the burning failed due to low-quality media."), MessageError );
                break;
            case DEVICE_BUSY:
                emit infoMessage( i18n("Another application is blocking the device (most likely automounting)."), MessageError );
                break;
            case WRITE_ERROR:
                emit infoMessage( i18n("A write error occurred."), MessageError );
                if( d->writingMode == K3b::WritingModeSao )
                    emit infoMessage( i18n("Sometimes using TAO writing mode solves this issue."), MessageError );
                break;
            case BLANK_FAILED:
                emit infoMessage( i18n("Some drives do not support all erase types."), MessageError );
                emit infoMessage( i18n("Try again using 'Complete' erasing."), MessageError );
                break;
            case SHORT_READ:
                emit infoMessage( QLatin1String("Internal error: short read. Please report!"), MessageError );
                break;
            case UNKNOWN:
                if( (exitCode == 12) && K3b::kernelVersion() >= K3b::Version( 2, 6, 8 ) && d->cdrecordBinObject->hasFeature( "suidroot" ) ) {
                    emit infoMessage( i18n("Since kernel version 2.6.8 cdrecord cannot use SCSI transport when running suid root anymore."), MessageError );
                    emit infoMessage( i18n("You may use K3b::Setup to solve this problem or remove the suid bit manually."), MessageError );
                }
                else if( !wasSourceUnreadable() ) {
                    emit infoMessage( i18n("%1 returned an unknown error (code %2).",
                                           d->cdrecordBinObject->name(), exitCode),
                                      K3b::Job::MessageError );

                    if( (exitCode >= 254) && d->writingMode == K3b::WritingModeSao ) {
                        emit infoMessage( i18n("Sometimes using TAO writing mode solves this issue."), MessageError );
                    }
                    else {
                        emit infoMessage( i18n("If you are running an unpatched cdrecord version..."), MessageError );
                        emit infoMessage( i18n("...and this error also occurs with high quality media..."), MessageError );
                        emit infoMessage( i18n("...and the K3b FAQ does not help you..."), MessageError );
                        emit infoMessage( i18n("...please include the debugging output in your problem report."), MessageError );
                    }
                }
                break;
            }
            jobFinished( false );
        }
    }
    else {
        emit infoMessage( i18n("%1 crashed.", d->cdrecordBinObject->name()),
                          MessageError );
        jobFinished( false );
    }
}


void K3b::CdrecordWriter::slotThroughput( int t )
{
    emit writeSpeed( t, d->tracks.count() > d->currentTrack && !d->tracks[d->currentTrack-1].audio
                     ? K3b::Device::SPEED_FACTOR_CD_MODE1
                     : d->usedSpeedFactor );
}


qint64 K3b::CdrecordWriter::write( const char* data, qint64 maxSize )
{
    return d->process.write( data, maxSize );
}

#include "k3bcdrecordwriter.moc"
