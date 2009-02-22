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

#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <k3bprocess.h>
#include <k3bdevice.h>
#include <k3bdeviceglobals.h>
#include <k3bdevicemanager.h>
#include <k3bdevicehandler.h>
#include <k3bglobals.h>
#include <k3bthroughputestimator.h>
#include <k3bglobalsettings.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qfile.h>

#include <klocale.h>
#include <kdebug.h>
#include <kglobal.h>
#include <ktemporaryfile.h>


class K3b::CdrecordWriter::Private
{
public:
    Private()
        : cdTextFile(0) {
    }

    K3b::ThroughputEstimator* speedEst;
    bool canceled;
    bool usingBurnfree;
    int usedSpeed;

    struct Track {
        int size;
        bool audio;
    };

    QList<Track> tracks;

    KTemporaryFile* cdTextFile;

    Device::MediaType burnedMediaType;
    int usedSpeedFactor;
};


K3b::CdrecordWriter::CdrecordWriter( K3b::Device::Device* dev, K3b::JobHandler* hdl,
                                      QObject* parent )
    : K3b::AbstractWriter( dev, hdl, parent ),
      m_clone(false),
      m_cue(false)
{
    d = new Private();
    d->speedEst = new K3b::ThroughputEstimator( this );
    connect( d->speedEst, SIGNAL(throughput(int)),
             this, SLOT(slotThroughput(int)) );

    m_process = 0;
    m_writingMode = K3b::WRITING_MODE_TAO;
}


K3b::CdrecordWriter::~CdrecordWriter()
{
    delete d->cdTextFile;
    delete d;
    delete m_process;
}


bool K3b::CdrecordWriter::active() const
{
    return ( m_process && m_process->isRunning() );
}


int K3b::CdrecordWriter::fd() const
{
    if( m_process )
        return m_process->stdinFd();
    else
        return -1;
}


void K3b::CdrecordWriter::setDao( bool b )
{
    m_writingMode = ( b ? K3b::WRITING_MODE_DAO : K3b::WRITING_MODE_TAO );
}

void K3b::CdrecordWriter::setCueFile( const QString& s)
{
    m_cue = true;
    m_cueFile = s;

    // cuefile only works in DAO mode
    setWritingMode( K3b::WRITING_MODE_DAO );
}

void K3b::CdrecordWriter::setClone( bool b )
{
    m_clone = b;
}


void K3b::CdrecordWriter::setWritingMode( K3b::WritingMode mode )
{
    m_writingMode = mode;
}


void K3b::CdrecordWriter::prepareProcess()
{
    if( m_process ) delete m_process;  // kdelibs want this!
    m_process = new K3b::Process();
    m_process->setRunPrivileged(true);
    //  m_process->setPriority( K3Process::PrioHighest );
    m_process->setSplitStdout(true);
    m_process->setSuppressEmptyLines(true);
    m_process->setRawStdin(true);  // we only use stdin when writing on-the-fly
    connect( m_process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
    connect( m_process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
    connect( m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotProcessExited(int, QProcess::ExitStatus)) );

    m_cdrecordBinObject = k3bcore->externalBinManager()->binObject("cdrecord");

    if( !m_cdrecordBinObject )
        return;

    d->burnedMediaType = burnDevice()->mediaType();

    *m_process << m_cdrecordBinObject;

    // display progress
    *m_process << "-v";

    if( m_cdrecordBinObject->hasFeature( "gracetime") )
        *m_process << "gracetime=2";  // 2 is the lowest allowed value (Joerg, why do you do this to us?)

    // Again we assume the device to be set!
    *m_process << QString("dev=%1").arg(K3b::externalBinDeviceParameter(burnDevice(), m_cdrecordBinObject));

    d->usedSpeed = burnSpeed();
    if( d->usedSpeed == 0 ) {
        // try to determine the writeSpeed
        // if it fails determineMaximalWriteSpeed() will return 0 and
        // the choice is left to cdrecord
        d->usedSpeed = burnDevice()->determineMaximalWriteSpeed();
    }

    if ( d->burnedMediaType & K3b::Device::MEDIA_DVD_ALL ) {
        d->usedSpeed /= K3b::Device::SPEED_FACTOR_DVD;
        d->usedSpeedFactor = K3b::Device::SPEED_FACTOR_DVD;
    }
    else if ( d->burnedMediaType & K3b::Device::MEDIA_BD_ALL ) {
        d->usedSpeed /= K3b::Device::SPEED_FACTOR_BD;
        d->usedSpeedFactor = K3b::Device::SPEED_FACTOR_BD;
    }
    else {
        // cdrecord provides progress and speed as multiple of 150 KB (except for audio tracks)
        d->usedSpeed /= K3b::Device::SPEED_FACTOR_CD;
        d->usedSpeedFactor = K3b::Device::SPEED_FACTOR_CD;
    }

    if( d->usedSpeed != 0 )
        *m_process << QString("speed=%1").arg(d->usedSpeed);


    if ( K3b::Device::isBdMedia( d->burnedMediaType ) ) {
        if ( !m_cdrecordBinObject->hasFeature( "blu-ray" ) ) {
            emit infoMessage( i18n( "Cdrecord version %1 does not support Blu-ray writing." ,m_cdrecordBinObject->version ), ERROR );
            // FIXME: add a way to fail the whole thing here
        }
        *m_process << "-sao";
    }
    else if ( K3b::Device::isDvdMedia( d->burnedMediaType ) ) {
        // cdrecord only supports SAo for DVD
        *m_process << "-sao";
    }
    else if( K3b::Device::isCdMedia( d->burnedMediaType ) ) {
        if( m_writingMode == K3b::WRITING_MODE_DAO || m_cue ) {
            if( burnDevice()->dao() )
                *m_process << "-sao";
            else {
                if( m_cdrecordBinObject->hasFeature( "tao" ) )
                    *m_process << "-tao";
                emit infoMessage( i18n("Writer does not support disk at once (DAO) recording"), WARNING );
            }
        }
        else if( m_writingMode == K3b::WRITING_MODE_RAW ) {
            if( burnDevice()->supportsWritingMode( K3b::Device::RAW_R96R ) )
                *m_process << "-raw96r";
            else if( burnDevice()->supportsWritingMode( K3b::Device::RAW_R16 ) )
                *m_process << "-raw16";
            else if( burnDevice()->supportsWritingMode( K3b::Device::RAW_R96P ) )
                *m_process << "-raw96p";
            else {
                emit infoMessage( i18n("Writer does not support raw writing."), WARNING );
                if( m_cdrecordBinObject->hasFeature( "tao" ) )
                    *m_process << "-tao";
            }
        }
        else if( m_cdrecordBinObject->hasFeature( "tao" ) )
            *m_process << "-tao";
    }
    else {
        emit infoMessage( i18n( "Cdrecord does not support writing %1 media." , K3b::Device::mediaTypeString( d->burnedMediaType ) ), ERROR );
        // FIXME: add a way to fail the whole thing here
    }

    if( simulate() )
        *m_process << "-dummy";

    d->usingBurnfree = false;
    if( k3bcore->globalSettings()->burnfree() ) {
        if( burnDevice()->burnproof() ) {

            d->usingBurnfree = true;

            // with cdrecord 1.11a02 burnproof was renamed to burnfree
            if( m_cdrecordBinObject->hasFeature( "burnproof" ) )
                *m_process << "driveropts=burnproof";
            else
                *m_process << "driveropts=burnfree";
        }
        else
            emit infoMessage( i18n("Writer does not support buffer underrun free recording (Burnfree)"), WARNING );
    }

    if( k3bcore->globalSettings()->force() ) {
        *m_process << "-force";
        emit infoMessage( i18n("'Force unsafe operations' enabled."), WARNING );
    }

    if( m_cue ) {
        m_process->setWorkingDirectory( m_cueFile );
        *m_process << QString("cuefile=%1").arg( m_cueFile );
    }

    if( m_clone )
        *m_process << "-clone";

    if( m_rawCdText.size() > 0 ) {
        delete d->cdTextFile;
        d->cdTextFile = new KTemporaryFile();
        d->cdTextFile->setPrefix( "/tmp/" ); // needs to be world readable in case cdrecord runs suid root
        d->cdTextFile->setSuffix( ".dat" );
        d->cdTextFile->write( m_rawCdText );
        d->cdTextFile->close();

        *m_process << "textfile=" + d->cdTextFile->fileName();
    }

    bool manualBufferSize = k3bcore->globalSettings()->useManualBufferSize();
    if( manualBufferSize ) {
        *m_process << QString("fs=%1m").arg( k3bcore->globalSettings()->bufferSize() );
    }

    bool overburn = k3bcore->globalSettings()->overburn();
    if( overburn ) {
        if( m_cdrecordBinObject->hasFeature("overburn") ) {
            if ( k3bcore->globalSettings()->force() )
                *m_process << "-ignsize";
            else
                *m_process << "-overburn";
        }
        else {
            emit infoMessage( i18n("Cdrecord %1 does not support overburning.",m_cdrecordBinObject->version), WARNING );
        }
    }

    // additional user parameters from config
    const QStringList& params = m_cdrecordBinObject->userParameters();
    for( QStringList::const_iterator it = params.constBegin(); it != params.constEnd(); ++it )
        *m_process << *it;

    // add the user parameters
    for( QStringList::const_iterator it = m_arguments.constBegin(); it != m_arguments.constEnd(); ++it )
        *m_process << *it;
}


K3b::CdrecordWriter* K3b::CdrecordWriter::addArgument( const QString& arg )
{
    m_arguments.append( arg );
    return this;
}


void K3b::CdrecordWriter::clearArguments()
{
    m_arguments.clear();
}


void K3b::CdrecordWriter::start()
{
    jobStarted();

    d->canceled = false;
    d->speedEst->reset();

    prepareProcess();

    if( !m_cdrecordBinObject ) {
        emit infoMessage( i18n("Could not find %1 executable.",QString("cdrecord")), ERROR );
        jobFinished(false);
        return;
    }

    emit debuggingOutput( QLatin1String( "Used versions" ), QLatin1String( "cdrecord: " ) + m_cdrecordBinObject->version );

    if( !m_cdrecordBinObject->copyright.isEmpty() )
        emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3"
                               ,(m_cdrecordBinObject->hasFeature( "wodim" ) ? "Wodim" : "Cdrecord" )
                               ,m_cdrecordBinObject->version
                               ,m_cdrecordBinObject->copyright), INFO );


    kDebug() << "***** " << m_cdrecordBinObject->name() << " parameters:\n";
    QString s = m_process->joinedArgs();
    kDebug() << s << flush;
    emit debuggingOutput( m_cdrecordBinObject->name() + " command:", s);

    m_currentTrack = 0;
    m_cdrecordError = UNKNOWN;
    m_totalTracksParsed = false;
    m_alreadyWritten = 0;
    d->tracks.clear();
    m_totalSize = 0;

    emit newSubTask( i18n("Preparing write process...") );

    // FIXME: check the return value
    if( K3b::isMounted( burnDevice() ) ) {
        emit infoMessage( i18n("Unmounting medium"), INFO );
        K3b::unmount( burnDevice() );
    }

    // block the device (including certain checks)
    k3bcore->blockDevice( burnDevice() );

    // lock the device for good in this process since it will
    // be opened in the cdrecord process
    burnDevice()->close();
    burnDevice()->usageLock();

    if( !m_process->start( K3Process::All ) ) {
        // something went wrong when starting the program
        // it "should" be the executable
        kDebug() << "(K3b::CdrecordWriter) could not start " << m_cdrecordBinObject->name();
        emit infoMessage( i18n("Could not start %1.",m_cdrecordBinObject->name()), K3b::Job::ERROR );
        jobFinished(false);
    }
    else {
        // FIXME: these messages should also take DVD into account.
        if( simulate() ) {
            emit newTask( i18n("Simulating") );
            emit infoMessage( i18n("Starting %1 simulation at %2x speed..."
                                   ,K3b::writingModeString(m_writingMode)
                                   ,d->usedSpeed),
                              K3b::Job::INFO );
        }
        else {
            emit newTask( i18n("Writing") );
            emit infoMessage( i18n("Starting %1 writing at %2x speed..."
                                   ,K3b::writingModeString(m_writingMode)
                                   ,d->usedSpeed),
                              K3b::Job::INFO );
        }
    }
}


void K3b::CdrecordWriter::cancel()
{
    if( active() ) {
        d->canceled = true;
        if( m_process && m_process->isRunning() )
            m_process->kill();
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

    emit debuggingOutput( m_cdrecordBinObject->name(), line );

    //
    // Progress and toc parsing
    //

    if( line.startsWith( "Track " ) ) {
        if( !m_totalTracksParsed ) {
            // this is not the progress display but the list of tracks that will get written
            // we always extract the tracknumber to get the highest at last
            bool ok;
            int tt = line.mid( 6, 2 ).toInt(&ok);

            if( ok ) {
                struct Private::Track track;
                track.audio  = ( line.mid( 10, 5 ) == "audio" );

                m_totalTracks = tt;

                int sizeStart = line.indexOf( QRegExp("\\d"), 10 );
                int sizeEnd = line.indexOf( "MB", sizeStart );
                track.size = line.mid( sizeStart, sizeEnd-sizeStart ).toInt(&ok);

                if( ok ) {
                    d->tracks.append(track);
                    m_totalSize += track.size;
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
            m_lastFifoValue = fifo;

            if( s_progressRx.numCaptures() > 4 )
                emit deviceBuffer( s_progressRx.cap(5).toInt() );

            //
            // cdrecord's output sucks a bit.
            // we get track sizes that differ from the sizes in the progress
            // info since these are dependant on the writing mode.
            // so we just use the track sizes and do a bit of math...
            //

            if( d->tracks.count() > m_currentTrack-1 && size > 0 ) {
                double convV = (double)d->tracks[m_currentTrack-1].size/(double)size;
                made = (int)((double)made * convV);
                size = d->tracks[m_currentTrack-1].size;
            }
            else {
                kError() << "(K3b::CdrecordWriter) Did not parse all tracks sizes!" << endl;
            }

            if( size > 0 ) {
                emit processedSubSize( made, size );
                emit subPercent( 100*made/size );
            }

            if( m_totalSize > 0 ) {
                emit processedSize( m_alreadyWritten+made, m_totalSize );
                emit percent( 100*(m_alreadyWritten+made)/m_totalSize );
            }

            d->speedEst->dataWritten( (m_alreadyWritten+made)*1024 );
        }
    }

    //
    // Cdrecord starts all error and warning messages with it's path
    // With Debian's script it starts with cdrecord (or /usr/bin/cdrecord or whatever! I hate this script!)
    //

    else if( line.startsWith( "cdrecord" ) ||
             line.startsWith( m_cdrecordBinObject->path ) ||
             line.startsWith( m_cdrecordBinObject->path.left(m_cdrecordBinObject->path.length()-5) ) ) {
        // get rid of the path and the following colon and space
        QString errStr = line.mid( line.indexOf(':') + 2 );

        if( errStr.startsWith( "Drive does not support SAO" ) ) {
            emit infoMessage( i18n("DAO (Disk At Once) recording not supported with this writer"), K3b::Job::ERROR );
            emit infoMessage( i18n("Please choose TAO (Track At Once) and try again"), K3b::Job::ERROR );
        }
        else if( errStr.startsWith( "Drive does not support RAW" ) ) {
            emit infoMessage( i18n("RAW recording not supported with this writer"), K3b::Job::ERROR );
        }
        else if( errStr.startsWith("Input/output error.") ) {
            emit infoMessage( i18n("Input/output error. Not necessarily serious."), WARNING );
        }
        else if( errStr.startsWith("shmget failed") ) {
            m_cdrecordError = SHMGET_FAILED;
        }
        else if( errStr.startsWith("OPC failed") ) {
            m_cdrecordError = OPC_FAILED;
        }
        else if( errStr.startsWith( "Drive needs to reload the media" ) ) {
            emit infoMessage( i18n("Reloading of medium required"), K3b::Job::INFO );
        }
        else if( errStr.startsWith( "The current problem looks like a buffer underrun" ) ) {
            if( m_cdrecordError == UNKNOWN ) // it is almost never a buffer underrun these days.
                m_cdrecordError = BUFFER_UNDERRUN;
        }
        else if( errStr.startsWith("WARNING: Data may not fit") ) {
            bool overburn = k3bcore->globalSettings()->overburn();
            if( overburn && m_cdrecordBinObject->hasFeature("overburn") )
                emit infoMessage( i18n("Trying to write more than the official disk capacity"), K3b::Job::WARNING );
            m_cdrecordError = OVERSIZE;
        }
        else if( errStr.startsWith("Bad Option") ) {
            m_cdrecordError = BAD_OPTION;
            // parse option
            int pos = line.indexOf( "Bad Option" ) + 13;
            int len = line.length() - pos - 1;
            emit infoMessage( i18n("No valid %1 option: %2",m_cdrecordBinObject->name(),line.mid(pos, len)),
                              ERROR );
        }
        else if( errStr.startsWith("Cannot set speed/dummy") ) {
            m_cdrecordError = CANNOT_SET_SPEED;
        }
        else if( errStr.startsWith("Cannot open new session") ) {
            m_cdrecordError = CANNOT_OPEN_NEW_SESSION;
        }
        else if( errStr.startsWith("Cannot send CUE sheet") ) {
            m_cdrecordError = CANNOT_SEND_CUE_SHEET;
        }
        else if( errStr.startsWith( "Trying to use ultra high speed" ) ||
                 errStr.startsWith( "Trying to use high speed" ) ||
                 errStr.startsWith( "Probably trying to use ultra high speed" ) ||
                 errStr.startsWith( "You did use a high speed medium on an improper writer" ) ||
                 errStr.startsWith( "You did use a ultra high speed medium on an improper writer" ) ) {
            m_cdrecordError = HIGH_SPEED_MEDIUM;
        }
        else if( errStr.startsWith( "You may have used an ultra low speed medium" ) ) {
            m_cdrecordError = LOW_SPEED_MEDIUM;
        }
        else if( errStr.startsWith( "Permission denied. Cannot open" ) ||
                 errStr.startsWith( "Operation not permitted." ) ) {
            m_cdrecordError = PERMISSION_DENIED;
        }
        else if( errStr.startsWith( "Can only copy session # 1") ) {
            emit infoMessage( i18n("Only session 1 will be cloned."), WARNING );
        }
        else if( errStr == "Cannot fixate disk." ) {
            emit infoMessage( i18n("Unable to fixate the disk."), ERROR );
            if( m_cdrecordError == UNKNOWN )
                m_cdrecordError = CANNOT_FIXATE_DISK;
        }
        else if( errStr == "A write error occurred." ) {
            m_cdrecordError = WRITE_ERROR;
        }
        else if( errStr.startsWith( "Try again with cdrecord blank=all." ) ) {
            m_cdrecordError = BLANK_FAILED;
        }
    }

    //
    // All other messages
    //

    else if( line.contains( "at speed" ) ) {
        // parse the speed and inform the user if cdrdao switched it down
        int pos = line.indexOf( "at speed" );
        int pos2 = line.indexOf( "in", pos+9 );
        int speed = static_cast<int>( line.mid( pos+9, pos2-pos-10 ).toDouble() );  // cdrecord-dvd >= 2.01a25 uses 8.0 and stuff
        if( speed != d->usedSpeed ) {
            emit infoMessage( i18n("Medium or burner do not support writing at %1x speed",d->usedSpeed), K3b::Job::WARNING );
            if( speed > d->usedSpeed )
                emit infoMessage( i18n("Switching burn speed up to %1x",speed), K3b::Job::WARNING );
            else
                emit infoMessage( i18n("Switching burn speed down to %1x",speed), K3b::Job::WARNING );
        }
    }
    else if( line.startsWith( "Starting new" ) ) {
        m_totalTracksParsed = true;
        if( m_currentTrack > 0 ) {// nothing has been written at the start of track 1
            if( d->tracks.count() > m_currentTrack-1 )
                m_alreadyWritten += d->tracks[m_currentTrack-1].size;
            else
                kError() << "(K3b::CdrecordWriter) Did not parse all tracks sizes!";
        }
        else
            emit infoMessage( i18n("Starting disc write"), INFO );

        m_currentTrack++;

        if( m_currentTrack > d->tracks.count() ) {
            kDebug() << "(K3b::CdrecordWriter) need to add dummy track struct.";
            struct Private::Track t;
            t.size = 1;
            t.audio = false;
            d->tracks.append(t);
        }

        kDebug() << "(K3b::CdrecordWriter) writing track " << m_currentTrack << " of " << m_totalTracks << " tracks.";
        emit nextTrack( m_currentTrack, m_totalTracks );
    }
    else if( line.startsWith( "Fixating" ) ) {
        emit newSubTask( i18n("Closing Session") );
    }
    else if( line.startsWith( "Writing lead-in" ) ) {
        m_totalTracksParsed = true;
        emit newSubTask( i18n("Writing Leadin") );
    }
    else if( line.startsWith( "Writing Leadout") ) {
        emit newSubTask( i18n("Writing Leadout") );
    }
    else if( line.startsWith( "Writing pregap" ) ) {
        emit newSubTask( i18n("Writing pregap") );
    }
    else if( line.startsWith( "Performing OPC" ) ) {
        emit infoMessage( i18n("Performing Optimum Power Calibration"), K3b::Job::INFO );
    }
    else if( line.startsWith( "Sending" ) ) {
        emit infoMessage( i18n("Sending CUE sheet"), K3b::Job::INFO );
    }
    else if( line.startsWith( "Turning BURN-Free on" ) || line.startsWith( "BURN-Free is ON") ) {
        emit infoMessage( i18n("Enabled Burnfree"), K3b::Job::INFO );
    }
    else if( line.startsWith( "Turning BURN-Free off" ) ) {
        emit infoMessage( i18n("Disabled Burnfree"), K3b::Job::WARNING );
    }
    else if( line.startsWith( "Re-load disk and hit" ) ) {
        // this happens on some notebooks where cdrecord is not able to close the
        // tray itself, so we need to ask the user to do so
        blockingInformation( i18n("Please reload the medium and press 'ok'"),
                             i18n("Unable to close the tray") );

        // now send a <CR> to cdrecord
        // hopefully this will do it since I have no possibility to test it!
        m_process->write( "\n", 1 );
    }
    else if( s_burnfreeCounterRx.indexIn( line ) ) {
        bool ok;
        int num = s_burnfreeCounterRx.cap(1).toInt(&ok);
        if( ok )
            emit infoMessage( i18np("Burnfree was used 1 time.", "Burnfree was used %1 times.", num), INFO );
    }
    else if( s_burnfreeCounterRxPredict.indexIn( line ) ) {
        bool ok;
        int num = s_burnfreeCounterRxPredict.cap(1).toInt(&ok);
        if( ok )
            emit infoMessage( i18np("Buffer was low 1 time.", "Buffer was low %1 times.", num), INFO );
    }
    else if( line.contains("Medium Error") ) {
        m_cdrecordError = MEDIUM_ERROR;
    }
    else if( line.startsWith( "Error trying to open" ) && line.contains( "(Device or resource busy)" ) ) {
        m_cdrecordError = DEVICE_BUSY;
    }
    else {
        // debugging
        kDebug() << "(" << m_cdrecordBinObject->name() << ") " << line;
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
            if( simulate() )
                emit infoMessage( i18n("Simulation successfully completed"), K3b::Job::SUCCESS );
            else
                emit infoMessage( i18n("Writing successfully completed"), K3b::Job::SUCCESS );

            int s = d->speedEst->average();
            emit infoMessage( ki18n("Average overall write speed: %1 KB/s (%2x)").subs(s).subs((double)s/( double )d->usedSpeedFactor, 0, 'g', 2).toString(), INFO );

            jobFinished( true );
        }
        break;

        default:
            kDebug() << "(K3b::CdrecordWriter) error: " << exitCode;

            if( m_cdrecordError == UNKNOWN && m_lastFifoValue <= 3 )
                m_cdrecordError = BUFFER_UNDERRUN;

            switch( m_cdrecordError ) {
            case OVERSIZE:
                if( k3bcore->globalSettings()->overburn() &&
                    m_cdrecordBinObject->hasFeature("overburn") )
                    emit infoMessage( i18n("Data did not fit on disk."), ERROR );
                else {
                    emit infoMessage( i18n("Data does not fit on disk."), ERROR );
                    if( m_cdrecordBinObject->hasFeature("overburn") )
                        emit infoMessage( i18n("Enable overburning in the advanced K3b settings to burn anyway."), INFO );
                }
                break;
            case BAD_OPTION:
                // error message has already been emitted earlier since we needed the actual line
                break;
            case SHMGET_FAILED:
                emit infoMessage( i18n("%1 could not reserve shared memory segment of requested size.",m_cdrecordBinObject->name()), ERROR );
                emit infoMessage( i18n("Probably you chose a too large buffer size."), ERROR );
                break;
            case OPC_FAILED:
                emit infoMessage( i18n("OPC failed. Probably the writer does not like the medium."), ERROR );
                break;
            case CANNOT_SET_SPEED:
                emit infoMessage( i18n("Unable to set write speed to %1.",d->usedSpeed), ERROR );
                emit infoMessage( i18n("Probably this is lower than your writer's lowest writing speed."), ERROR );
                break;
            case CANNOT_SEND_CUE_SHEET:
                emit infoMessage( i18n("Unable to send CUE sheet."), ERROR );
                if( m_writingMode == K3b::WRITING_MODE_DAO )
                    emit infoMessage( i18n("Sometimes using TAO writing mode solves this issue."), ERROR );
                break;
            case CANNOT_OPEN_NEW_SESSION:
                emit infoMessage( i18n("Unable to open new session."), ERROR );
                emit infoMessage( i18n("Probably a problem with the medium."), ERROR );
                break;
            case CANNOT_FIXATE_DISK:
                emit infoMessage( i18n("The disk might still be readable."), ERROR );
                if( m_writingMode == K3b::WRITING_MODE_TAO && burnDevice()->dao() )
                    emit infoMessage( i18n("Try DAO writing mode."), ERROR );
                break;
            case PERMISSION_DENIED:
                emit infoMessage( i18n("%1 has no permission to open the device.",QString("cdrecord")), ERROR );
#ifdef HAVE_K3BSETUP
                emit infoMessage( i18n("You may use K3bsetup2 to solve this problem."), ERROR );
#endif
                break;
            case BUFFER_UNDERRUN:
                emit infoMessage( i18n("Probably a buffer underrun occurred."), ERROR );
                if( !d->usingBurnfree && burnDevice()->burnproof() )
                    emit infoMessage( i18n("Please enable Burnfree or choose a lower burning speed."), ERROR );
                else
                    emit infoMessage( i18n("Please choose a lower burning speed."), ERROR );
                break;
            case HIGH_SPEED_MEDIUM:
                emit infoMessage( i18n("Found a high-speed medium not suitable for the writer being used."), ERROR );
                emit infoMessage( i18n("Use the 'force unsafe operations' option to ignore this."), ERROR );
                break;
            case LOW_SPEED_MEDIUM:
                emit infoMessage( i18n("Found a low-speed medium not suitable for the writer being used."), ERROR );
                emit infoMessage( i18n("Use the 'force unsafe operations' option to ignore this."), ERROR );
                break;
            case MEDIUM_ERROR:
                emit infoMessage( i18n("Most likely the burning failed due to low-quality media."), ERROR );
                break;
            case DEVICE_BUSY:
                emit infoMessage( i18n("Another application is blocking the device (most likely automounting)."), ERROR );
                break;
            case WRITE_ERROR:
                emit infoMessage( i18n("A write error occurred."), ERROR );
                if( m_writingMode == K3b::WRITING_MODE_DAO )
                    emit infoMessage( i18n("Sometimes using TAO writing mode solves this issue."), ERROR );
                break;
            case BLANK_FAILED:
                emit infoMessage( i18n("Some drives do not support all erase types."), ERROR );
                emit infoMessage( i18n("Try again using 'Complete' erasing."), ERROR );
                break;
            case UNKNOWN:
                if( (exitCode == 12) && K3b::kernelVersion() >= K3b::Version( 2, 6, 8 ) && m_cdrecordBinObject->hasFeature( "suidroot" ) ) {
                    emit infoMessage( i18n("Since kernel version 2.6.8 cdrecord cannot use SCSI transport when running suid root anymore."), ERROR );
                    emit infoMessage( i18n("You may use K3b::Setup to solve this problem or remove the suid bit manually."), ERROR );
                }
                else if( !wasSourceUnreadable() ) {
                    emit infoMessage( i18n("%1 returned an unknown error (code %2).",
                                           m_cdrecordBinObject->name(), exitCode),
                                      K3b::Job::ERROR );

                    if( (exitCode >= 254) && m_writingMode == K3b::WRITING_MODE_DAO ) {
                        emit infoMessage( i18n("Sometimes using TAO writing mode solves this issue."), ERROR );
                    }
                    else {
                        emit infoMessage( i18n("If you are running an unpatched cdrecord version..."), ERROR );
                        emit infoMessage( i18n("...and this error also occurs with high quality media..."), ERROR );
                        emit infoMessage( i18n("...and the K3b FAQ does not help you..."), ERROR );
                        emit infoMessage( i18n("...please include the debugging output in your problem report."), ERROR );
                    }
                }
                break;
            }
            jobFinished( false );
        }
    }
    else {
        emit infoMessage( i18n("%1 did not exit cleanly.",m_cdrecordBinObject->name()),
                          ERROR );
        jobFinished( false );
    }
}


void K3b::CdrecordWriter::slotThroughput( int t )
{
    emit writeSpeed( t, d->tracks.count() > m_currentTrack && !d->tracks[m_currentTrack-1].audio
                     ? K3b::Device::SPEED_FACTOR_CD_MODE1
                     : d->usedSpeedFactor );
}

#include "k3bcdrecordwriter.moc"
