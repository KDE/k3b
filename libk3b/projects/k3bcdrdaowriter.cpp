/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2003 Klaus-Dieter Krannich <kd@k3b.org>
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


#include "k3bcdrdaowriter.h"

#include "k3bcore.h"
#include "k3bexternalbinmanager.h"
#include "k3bdevicemanager.h"
#include "k3bprocess.h"
#include "k3bdevice.h"
#include "k3bdevicehandler.h"
#include "k3bthroughputestimator.h"
#include "k3bglobals.h"
#include "k3bglobalsettings.h"

#include <KDebug>
#include <KIO/NetAccess>
#include <KLocale>
#include <KStandardDirs>
#include <KTemporaryFile>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTcpSocket>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>



#define PGSMSG_MIN PGSMSG_RCD_ANALYZING
#define PGSMSG_RCD_ANALYZING   1
#define PGSMSG_RCD_EXTRACTING  2
#define PGSMSG_WCD_LEADIN      3
#define PGSMSG_WCD_DATA        4
#define PGSMSG_WCD_LEADOUT     5
#define PGSMSG_BLK             6
#define PGSMSG_MAX PGSMSG_BLK

struct ProgressMsg {
    int status;         // see PGSMSG_* constants
    int totalTracks;    // total number of tracks
    int track;          // actually written track
    int trackProgress;  // progress for current track 0..1000
    int totalProgress;  // total writing progress 0..1000
    int bufferFillRate; // buffer fill rate 0..100
};

#define PSGMSG_MINSIZE 24

struct ProgressMsg2 {
    int status;         // see PGSMSG_* constants
    int totalTracks;    // total number of tracks
    int track;          // actually written track
    int trackProgress;  // progress for current track 0..1000
    int totalProgress;  // total writing progress 0..1000
    int bufferFillRate; // buffer fill rate 0..100
    int writerFillRate; // device write buffer fill rate 0..100
};


inline bool operator<( const ProgressMsg2& m1, const ProgressMsg2& m2 )
{
    return m1.track < m2.track
        || ( m1.track == m2.track
             && m1.trackProgress < m2.trackProgress )
        || m1.totalProgress < m2.totalProgress;
}


inline bool operator==( const ProgressMsg2& m1, const ProgressMsg2& m2 )
{
    return m1.status == m2.status
        && m1.track == m2.track
        && m1.totalTracks == m2.totalTracks
        && m1.trackProgress == m2.trackProgress
        && m1.totalProgress == m2.totalProgress
        && m1.bufferFillRate == m2.bufferFillRate;
}

inline bool operator!=( const ProgressMsg2& m1, const ProgressMsg2& m2 )
{
    return !( m1 == m2 );
}



class K3b::CdrdaoWriter::Private
{
public:
    Private() {
    }

    K3b::ThroughputEstimator* speedEst;

    int usedSpeed;

    ProgressMsg2 oldMsg;
    ProgressMsg2 newMsg;

    unsigned int progressMsgSize;
};


K3b::CdrdaoWriter::CdrdaoWriter( K3b::Device::Device* dev, K3b::JobHandler* hdl,
                                 QObject* parent )
    : K3b::AbstractWriter( dev, hdl, parent ),
      m_command(WRITE),
      m_blankMode(FormattingQuick),
      m_sourceDevice(0),
      m_readRaw(false),
      m_multi(false),
      m_force(false),
      m_onTheFly(false),
      m_fastToc(false),
      m_readSubchan(None),
      m_taoSource(false),
      m_taoSourceAdjust(-1),
      m_paranoiaMode(-1),
      m_session(-1),
      m_eject( false ),
      m_process(0),
      m_comSock(0),
      m_currentTrack(0)
{
    d = new Private();
    d->speedEst = new K3b::ThroughputEstimator( this );
    connect( d->speedEst, SIGNAL(throughput(int)),
             this, SLOT(slotThroughput(int)) );

    ::memset( &d->oldMsg, 0, sizeof(ProgressMsg2) );
    ::memset( &d->newMsg, 0, sizeof(ProgressMsg2) );
#ifndef Q_OS_WIN32
    if( socketpair(AF_UNIX,SOCK_STREAM,0,m_cdrdaoComm) )
    {
#endif
		kDebug() << "(K3b::CdrdaoWriter) could not open socketpair for cdrdao remote messages";
#ifndef Q_OS_WIN32
    }
    else
    {
        delete m_comSock;
        m_comSock = new QTcpSocket();
        m_comSock->setSocketDescriptor( m_cdrdaoComm[1] );
        m_comSock->setReadBufferSize(49152);
        // magic number from Qt documentation
        connect( m_comSock, SIGNAL(readyRead()),
                 this, SLOT(parseCdrdaoMessage()));
    }
#endif
}

K3b::CdrdaoWriter::~CdrdaoWriter()
{
    delete d->speedEst;
    delete d;

#ifndef Q_OS_WIN32
    // close the socket
    if( m_comSock ) {
        m_comSock->close();
        ::close( m_cdrdaoComm[0] );
    }
#endif
    delete m_process;
    delete m_comSock;
}


bool K3b::CdrdaoWriter::active() const
{
    return (m_process ? m_process->isRunning() : false);
}


void K3b::CdrdaoWriter::prepareArgumentList()
{

    // binary
    *m_process << m_cdrdaoBinObject;

    // command
    switch ( m_command )
    {
    case COPY:
        *m_process << "copy";
        setWriteArguments();
        setReadArguments();
        setCopyArguments();
        break;
    case WRITE:
        *m_process << "write";
        setWriteArguments();
        break;
    case READ:
        *m_process << "read-cd";
        // source device and source driver
        if ( m_sourceDevice )
            *m_process << "--device"
                       << K3b::externalBinDeviceParameter(m_sourceDevice, m_cdrdaoBinObject);
        if( defaultToGenericMMC( m_sourceDevice, false ) ) {
            kDebug() << "(K3b::CdrdaoWriter) defaulting to generic-mmc driver for " << m_sourceDevice->blockDeviceName();
            *m_process << "--driver" << "generic-mmc";
        }
        setReadArguments();
        break;
    case BLANK:
        *m_process << "blank";
        setBlankArguments();
        break;
    }

    setCommonArguments();
}

void K3b::CdrdaoWriter::setWriteArguments()
{
    // device and driver
    *m_process << "--device"
               << K3b::externalBinDeviceParameter(burnDevice(), m_cdrdaoBinObject);

    if( defaultToGenericMMC( burnDevice(), true ) ) {
        kDebug() << "(K3b::CdrdaoWriter) defaulting to generic-mmc driver for " << burnDevice()->blockDeviceName();
        *m_process << "--driver" << "generic-mmc:0x00000010";
    }

    // burn speed
    if( d->usedSpeed != 0 )
        *m_process << "--speed" << QString("%1").arg(d->usedSpeed);

    //simulate
    if( simulate() )
        *m_process << "--simulate";

    // multi
    if( m_multi )
        *m_process << "--multi";

    // force
    if( m_force )
        *m_process << "--force";

    // burnproof
    if ( !k3bcore->globalSettings()->burnfree() ) {
        if( m_cdrdaoBinObject->hasFeature( "disable-burnproof" ) )
            *m_process << "--buffer-under-run-protection" << "0";
        else
            emit infoMessage( i18n("Cdrdao %1 does not support disabling burnfree.",m_cdrdaoBinObject->version()), MessageWarning );
    }

    if( k3bcore->globalSettings()->force() ) {
        *m_process << "--force";
        emit infoMessage( i18n("'Force unsafe operations' enabled."), MessageWarning );
    }

    bool manualBufferSize =
        k3bcore->globalSettings()->useManualBufferSize();
    if( manualBufferSize ) {
        //
        // one buffer in cdrdao holds 1 second of audio data = 75 frames = 75 * 2352 bytes
        //
        int bufSizeInMb = k3bcore->globalSettings()->bufferSize();
        *m_process << "--buffers" << QString::number( bufSizeInMb*1024*1024/(75*2352) );
    }

    bool overburn =
        k3bcore->globalSettings()->overburn();
    if( overburn ) {
        if( m_cdrdaoBinObject->hasFeature("overburn") )
            *m_process << "--overburn";
        else
            emit infoMessage( i18n("Cdrdao %1 does not support overburning.",m_cdrdaoBinObject->version()), MessageWarning );
    }

}

void K3b::CdrdaoWriter::setReadArguments()
{
    // readRaw
    if ( m_readRaw )
        *m_process << "--read-raw";

    // subchan
    if ( m_readSubchan != None )
    {
        *m_process << "--read-subchan";
        switch ( m_readSubchan )
        {
        case RW:
            *m_process << "rw";
            break;
        case RW_RAW:
            *m_process << "rw_raw";
            break;
        case None:
            break;
        }
    }

    // TAO Source
    if ( m_taoSource )
        *m_process << "--tao-source";

    // TAO Source Adjust
    if ( m_taoSourceAdjust != -1 )
        *m_process << "--tao-source-adjust"
                   << QString("%1").arg(m_taoSourceAdjust);

    // paranoia Mode
    if ( m_paranoiaMode != -1 )
        *m_process << "--paranoia-mode"
                   << QString("%1").arg(m_paranoiaMode);

    // session
    if ( m_session != -1 )
        *m_process << "--session"
                   << QString("%1").arg(m_session);

    // fast TOC
    if ( m_fastToc )
        *m_process << "--fast-toc";

}

void K3b::CdrdaoWriter::setCopyArguments()
{
    // source device and source driver
    *m_process << "--source-device" << K3b::externalBinDeviceParameter(m_sourceDevice, m_cdrdaoBinObject);
    if( defaultToGenericMMC( m_sourceDevice, false ) ) {
        kDebug() << "(K3b::CdrdaoWriter) defaulting to generic-mmc driver for " << m_sourceDevice->blockDeviceName();
        *m_process << "--source-driver" << "generic-mmc";
    }

    // on-the-fly
    if ( m_onTheFly )
        *m_process << "--on-the-fly";
}

void K3b::CdrdaoWriter::setBlankArguments()
{
    // device and driver
    *m_process << "--device"
               << K3b::externalBinDeviceParameter(burnDevice(), m_cdrdaoBinObject);

    if( defaultToGenericMMC( burnDevice(), true ) ) {
        kDebug() << "(K3b::CdrdaoWriter) defaulting to generic-mmc driver for " << burnDevice()->blockDeviceName();
        *m_process << "--driver" << "generic-mmc";
    }

    // burn speed
    if( d->usedSpeed != 0 )
        *m_process << "--speed" << QString("%1").arg(d->usedSpeed);

    // blank-mode
    switch (m_blankMode)
    {
    case FormattingComplete:
        *m_process << "--blank-mode" << "full";
        break;
    case FormattingQuick:
        *m_process << "--blank-mode" << "minimal";
        break;
    }
}

void K3b::CdrdaoWriter::setCommonArguments()
{

    // additional user parameters from config
    const QStringList& params = m_cdrdaoBinObject->userParameters();
    for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
        *m_process << *it;


    // display debug info
    *m_process << "-n" << "-v" << "2";

    // we have the power to do what ever we want. ;)
    *m_process << "--force";

    // eject
    if( m_eject )
        *m_process << "--eject";

    // remote
    *m_process << "--remote" <<  QString("%1").arg(m_cdrdaoComm[0]);

    // data File
    if ( ! m_dataFile.isEmpty() )
        *m_process << "--datafile" << m_dataFile;

    // BIN/CUE
    if ( ! m_cueFileLnk.isEmpty() )
        *m_process << m_cueFileLnk;
    // TOC File
    else if ( ! m_tocFile.isEmpty() )
        *m_process << m_tocFile;
}

K3b::CdrdaoWriter* K3b::CdrdaoWriter::addArgument( const QString& arg )
{
    *m_process << arg;
    return this;
}


void K3b::CdrdaoWriter::start()
{
    jobStarted();

    d->speedEst->reset();

    delete m_process;  // kdelibs want this!
    m_process = new K3b::Process();
    m_process->setSplitStdout(false);
    m_process->setOutputChannelMode( KProcess::MergedChannels );
    m_process->setFlags( K3bQProcess::RawStdin );
    connect( m_process, SIGNAL(stdoutLine(QString)),
             this, SLOT(slotStdLine(QString)) );
    connect( m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
             this, SLOT(slotProcessExited(int,QProcess::ExitStatus)) );

    m_canceled = false;
    m_knownError = false;

    m_cdrdaoBinObject = k3bcore->externalBinManager()->binObject("cdrdao");

    if( !m_cdrdaoBinObject ) {
        emit infoMessage( i18n("Could not find %1 executable.",QString("cdrdao")), MessageError );
        jobFinished(false);
        return;
    }

    emit debuggingOutput( QLatin1String("Used versions"), QString::fromLatin1( "cdrdao: %1" ).arg( m_cdrdaoBinObject->version()) );

    if( !m_cdrdaoBinObject->copyright().isEmpty() )
        emit infoMessage( i18n("Using %1 %2 – Copyright © %3",m_cdrdaoBinObject->name(),m_cdrdaoBinObject->version(),m_cdrdaoBinObject->copyright()), MessageInfo );


    // the message size changed in cdrdao 1.1.8)
    if( m_cdrdaoBinObject->version() >= K3b::Version( 1, 1, 8 ) )
        d->progressMsgSize = sizeof(ProgressMsg2);
    else
        d->progressMsgSize = sizeof(ProgressMsg);

    // since the --speed parameter is used several times in this code we
    // determine the speed in auto once at the beginning
    d->usedSpeed = burnSpeed();
    if( d->usedSpeed == 0 ) {
        // try to determine the writeSpeed
        // if it fails determineMaximalWriteSpeed() will return 0 and
        // the choice is left to cdrdao
        d->usedSpeed = burnDevice()->determineMaximalWriteSpeed();
    }
    d->usedSpeed /= 175;

    switch ( m_command )
    {
    case WRITE:
    case COPY:
        if (!m_tocFile.isEmpty())
        {

            // if tocfile is a cuesheet than create symlinks to *.cue and the binary listed inside the cuesheet.
            // now works without the .bin extension too.
            if ( !cueSheet() ) {
                m_backupTocFile = m_tocFile + ".k3bbak";

                // workaround, cdrdao deletes the tocfile when --remote parameter is set
                if ( !KIO::NetAccess::file_copy(KUrl(m_tocFile),KUrl(m_backupTocFile), (QWidget*) 0) )
                {
                    kDebug() << "(K3b::CdrdaoWriter) could not backup " << m_tocFile << " to " << m_backupTocFile;
                    emit infoMessage( i18n("Could not backup tocfile."), MessageError );
                    jobFinished(false);
                    return;
                }
            }
        }
        break;
    case BLANK:
    case READ:
        break;
    }
    prepareArgumentList();
    // set working dir to dir part of toc file (to allow rel names in toc-file)
    m_process->setWorkingDirectory( QFileInfo( m_tocFile ).absolutePath() );

    kDebug() << "***** cdrdao parameters:\n";
    QString s = m_process->joinedArgs();
    kDebug() << s << flush;
    emit debuggingOutput("cdrdao command:", s);

    m_currentTrack = 0;
    reinitParser();

    switch ( m_command )
    {
    case READ:
        emit newSubTask( i18n("Preparing read process...") );
        break;
    case WRITE:
        emit newSubTask( i18n("Preparing write process...") );
        break;
    case COPY:
        emit newSubTask( i18n("Preparing copy process...") );
        break;
    case BLANK:
        emit newSubTask( i18n("Preparing blanking process...") );
        break;
    }

    // FIXME: check the return value
    if( K3b::isMounted( burnDevice() ) ) {
        emit infoMessage( i18n("Unmounting medium"), MessageInfo );
        K3b::unmount( burnDevice() );
    }

    // block the device (including certain checks)
    k3bcore->blockDevice( burnDevice() );

    // lock the device for good in this process since it will
    // be opened in the growisofs process
    burnDevice()->close();
    burnDevice()->usageLock();

    if( !m_process->start( KProcess::MergedChannels ) )
    {
        // something went wrong when starting the program
        // it "should" be the executable
        kDebug() << "(K3b::CdrdaoWriter) could not start cdrdao";
        emit infoMessage( i18n("Could not start %1.",QString("cdrdao")), K3b::Job::MessageError );
        jobFinished(false);
    }
    else
    {
        switch ( m_command )
        {
        case WRITE:
            if( simulate() )
            {
                // xgettext: no-c-format
                emit infoMessage(i18n("Starting DAO simulation at %1x speed...",d->usedSpeed),
                                 K3b::Job::MessageInfo );
                emit newTask( i18n("Simulating") );
            }
            else
            {
                // xgettext: no-c-format
                emit infoMessage( i18n("Starting DAO writing at %1x speed...",d->usedSpeed), K3b::Job::MessageInfo );
                emit newTask( i18n("Writing") );
            }
            break;
        case READ:
            emit infoMessage(i18n("Starting reading..."), K3b::Job::MessageInfo );
            emit newTask( i18n("Reading") );
            break;
        case COPY:
            if( simulate() )
            {
                // xgettext: no-c-format
                emit infoMessage(i18n("Starting simulation copy at %1x speed...",d->usedSpeed), K3b::Job::MessageInfo );
                emit newTask( i18n("Simulating") );
            }
            else
            {
                // xgettext: no-c-format
                emit infoMessage( i18n("Starting copy at %1x speed...",d->usedSpeed), K3b::Job::MessageInfo );
                emit newTask( i18n("Copying") );
            }
            break;
        case BLANK:
            emit infoMessage(i18n("Starting blanking..."), K3b::Job::MessageInfo );
            emit newTask( i18n("Blanking") );
        }
    }
}


void K3b::CdrdaoWriter::cancel()
{
    m_canceled = true;

    if( m_process ) {
        if( m_process->isRunning() ) {
            m_process->disconnect();
            m_process->terminate();

            // we need to unlock the device because cdrdao locked it while writing
            //
            // FIXME: try to determine wheater we are writing or reading and choose
            // the device to unblock based on that result.
            //
            if( m_command == READ ) {
                // FIXME: this is a hack
                setBurnDevice( m_sourceDevice );
            }

            // this will unblock and eject the drive and emit the finished/canceled signals
            K3b::AbstractWriter::cancel();
        }
    }
}


bool K3b::CdrdaoWriter::cueSheet()
{

    // TODO: do this in the K3b::CueFileParser

    if ( m_tocFile.toLower().endsWith( ".cue" ) ) {
        QFile f( m_tocFile );
        if ( f.open( QIODevice::ReadOnly ) ) {
            QTextStream ts( &f );
            QString line = ts.readLine();
            f.close();
            int pos = line.indexOf( "FILE \"" );
            if( pos < 0 )
                return false;

            pos += 6;
            int endPos = line.indexOf( "\" BINARY", pos+1 );
            if( endPos < 0 )
                return false;

            line = line.mid( pos, endPos-pos );
            QFileInfo fi( QFileInfo( m_tocFile ).path() + '/' + QFileInfo( line ).fileName() );
            QString binpath = fi.filePath();
            kDebug() << QString("K3b::CdrdaoWriter::cueSheet() BinFilePath from CueFile: %1").arg( line );
            kDebug() << QString("K3b::CdrdaoWriter::cueSheet() absolute BinFilePath: %1").arg( binpath );

            if ( !fi.exists() )
                return false;

            KTemporaryFile tempF;
            tempF.open();
            QString tempFile = tempF.fileName();
            tempF.remove();

            if ( symlink(QFile::encodeName( binpath ), QFile::encodeName( tempFile + ".bin") ) == -1 )
                return false;
            if ( symlink(QFile::encodeName( m_tocFile ), QFile::encodeName( tempFile + ".cue") ) == -1 )
                return false;

            kDebug() << QString("K3b::CdrdaoWriter::cueSheet() symlink BinFileName: %1.bin").arg( tempFile );
            kDebug() << QString("K3b::CdrdaoWriter::cueSheet() symlink CueFileName: %1.cue").arg( tempFile );
            m_binFileLnk = tempFile + ".bin";
            m_cueFileLnk = tempFile + ".cue";
            return true;
        }
    }

    return false;
}

void K3b::CdrdaoWriter::slotStdLine( const QString& line )
{
    parseCdrdaoLine(line);
}


void K3b::CdrdaoWriter::slotProcessExited( int exitCode, QProcess::ExitStatus exitStatus )
{
    // release the device within this process
    burnDevice()->usageUnlock();

    // unblock the device
    k3bcore->unblockDevice( burnDevice() );

    switch ( m_command )
    {
    case WRITE:
    case COPY:
        if ( !m_binFileLnk.isEmpty() ) {
            KIO::NetAccess::del(KUrl(m_cueFileLnk), (QWidget*) 0);
            KIO::NetAccess::del(KUrl(m_binFileLnk), (QWidget*) 0);
        }
        else if( (!QFile::exists( m_tocFile ) || K3b::filesize( KUrl(m_tocFile) ) == 0 ) && !m_onTheFly )
        {
            // cdrdao removed the tocfile :(
            // we need to recover it
            if ( !KIO::NetAccess::file_copy(KUrl(m_backupTocFile), KUrl(m_tocFile), (QWidget*) 0) )
            {
                kDebug() << "(K3b::CdrdaoWriter) restoring tocfile " << m_tocFile << " failed.";
                emit infoMessage( i18n("Due to a bug in cdrdao the toc/cue file %1 has been deleted. "
                                       "K3b was unable to restore it from the backup %2.",m_tocFile,m_backupTocFile), MessageError );
            }
            else if ( !KIO::NetAccess::del(KUrl(m_backupTocFile), (QWidget*) 0) )
            {
                kDebug() << "(K3b::CdrdaoWriter) delete tocfile backkup " << m_backupTocFile << " failed.";
            }
        }
        break;
    case BLANK:
    case READ:
        break;
    }

    if( m_canceled )
        return;

    if( exitStatus == QProcess::NormalExit )
    {
        switch( exitCode )
        {
        case 0:
            if( simulate() )
                emit infoMessage( i18n("Simulation successfully completed"), K3b::Job::MessageSuccess );
            else
                switch ( m_command )
                {
                case READ:
                    emit infoMessage( i18n("Reading successfully completed"), K3b::Job::MessageSuccess );
                    break;
                case WRITE:
                    emit infoMessage( i18n("Writing successfully completed"), K3b::Job::MessageSuccess );
                    break;
                case COPY:
                    emit infoMessage( i18n("Copying successfully completed"), K3b::Job::MessageSuccess );
                    break;
                case BLANK:
                    emit infoMessage( i18n("Blanking successfully completed"), K3b::Job::MessageSuccess );
                    break;
                }

            if( m_command == WRITE || m_command == COPY ) {
                int s = d->speedEst->average();
                emit infoMessage( ki18n("Average overall write speed: %1 KB/s (%2x)").subs(s).subs((double)s/150.0, 0, 'g', 2).toString(), MessageInfo );
            }

            jobFinished( true );
            break;

        default:
            if( !m_knownError && !wasSourceUnreadable() ) {
                emit infoMessage( i18n("%1 returned an unknown error (code %2).",m_cdrdaoBinObject->name(), exitCode),
                                  K3b::Job::MessageError );
                emit infoMessage( i18n("Please include the debugging output in your problem report."), K3b::Job::MessageError );
            }

            jobFinished( false );
            break;
        }
    }
    else
    {
        emit infoMessage( i18n("%1 crashed.", QString("cdrdao")), K3b::Job::MessageError );
        jobFinished( false );
    }
}


void K3b::CdrdaoWriter::unknownCdrdaoLine( const QString& line )
{
    if( line.contains( "at speed" ) )
    {
        // parse the speed and inform the user if cdrdao switched it down
        int pos = line.indexOf( "at speed" );
        int po2 = line.indexOf( QRegExp("\\D"), pos + 9 );
        int speed = line.mid( pos+9, po2-pos-9 ).toInt();
        if( speed < d->usedSpeed )
        {
            // xgettext: no-c-format
            emit infoMessage( i18n("Medium or burner does not support writing at %1x speed",d->usedSpeed), K3b::Job::MessageWarning );
            // xgettext: no-c-format
            emit infoMessage( i18n("Switching down burn speed to %1x",speed), K3b::Job::MessageWarning );
        }
    }
}


void K3b::CdrdaoWriter::reinitParser()
{
    ::memset( &d->oldMsg, 0, sizeof(ProgressMsg2) );
    ::memset( &d->newMsg, 0, sizeof(ProgressMsg2) );

    m_currentTrack=0;
}

void K3b::CdrdaoWriter::parseCdrdaoLine( const QString& str )
{
    emit debuggingOutput( "cdrdao", str );
    //  kDebug() << "(cdrdaoparse)" << str;
    // find some messages from cdrdao
    // -----------------------------------------------------------------------------------------
    if( (str).startsWith( "Warning" ) || (str).startsWith( "MessageWarning" ) || (str).startsWith( "MessageError" ) )
    {
        parseCdrdaoError( str );
    }
    else if( (str).startsWith( "Wrote" ) && !str.contains("blocks") )
    {
        parseCdrdaoWrote( str );
    }
    else if( (str).startsWith( "Executing power" ) )
    {
        emit newSubTask( i18n("Executing Power calibration") );
    }
    else if( (str).startsWith( "Power calibration successful" ) )
    {
        emit infoMessage( i18n("Power calibration successful"), K3b::Job::MessageInfo );
        emit newSubTask( i18n("Preparing burn process...") );
    }
    else if( (str).startsWith( "Flushing cache" ) )
    {
        emit newSubTask( i18n("Flushing cache") );
    }
    else if( (str).startsWith( "Writing CD-TEXT lead" ) )
    {
        emit newSubTask( i18n("Writing CD-Text lead-in...") );
    }
    else if( (str).startsWith( "Turning BURN-Proof on" ) )
    {
        emit infoMessage( i18n("Turning BURN-Proof on"), K3b::Job::MessageInfo );
    }
    else if( str.startsWith( "Copying" ) )
    {
        emit infoMessage( str, K3b::Job::MessageInfo );
    }
    else if( str.startsWith( "Found ISRC" ) )
    {
        emit infoMessage( i18n("Found ISRC code"), K3b::Job::MessageInfo );
    }
    else if( str.startsWith( "Found pre-gap" ) )
    {
        emit infoMessage( i18n("Found pregap: %1", str.mid(str.indexOf(":")+1) ), K3b::Job::MessageInfo );
    }
    else
        unknownCdrdaoLine(str);
}

void K3b::CdrdaoWriter::parseCdrdaoError( const QString& line )
{
    int pos = -1;

    if( line.contains( "No driver found" ) ||
        line.contains( "use option --driver" ) )
    {
        emit infoMessage( i18n("No cdrdao driver found."), K3b::Job::MessageError );
        emit infoMessage( i18n("Please select one manually in the device settings."), K3b::Job::MessageError );
        emit infoMessage( i18n("For most current drives this would be 'generic-mmc'."), K3b::Job::MessageError );
        m_knownError = true;
    }
    else if( line.contains( "Cannot setup device" ) )
    {
        // no nothing...
    }
    else if( line.contains( "not ready") )
    {
        emit infoMessage( i18n("Device not ready, waiting."),K3b::Job::MessageWarning );
    }
    else if( line.contains("Drive does not accept any cue sheet") )
    {
        emit infoMessage( i18n("Cue sheet not accepted."), K3b::Job::MessageError );
        m_knownError = true;
    }
    else if( (pos = line.indexOf( "Illegal option" )) > 0 ) {
        // MessageError: Illegal option: -wurst
        emit infoMessage( i18n("No valid %1 option: %2",m_cdrdaoBinObject->name(),line.mid(pos+16)),
                          MessageError );
        m_knownError = true;
    }
    else if( line.contains( "exceeds capacity" ) ) {
        emit infoMessage( i18n("Data does not fit on disk."), MessageError );
        if( m_cdrdaoBinObject->hasFeature("overburn") )
            emit infoMessage( i18n("Enable overburning in the advanced K3b settings to burn anyway."), MessageInfo );
        m_knownError = true;
    }
    //  else if( !line.contains( "remote progress message" ) )
//     emit infoMessage( line, K3b::Job::MessageError );
}

void K3b::CdrdaoWriter::parseCdrdaoWrote( const QString& line )
{
    int pos, po2;
    pos = line.indexOf( "Wrote" );
    po2 = line.indexOf( " ", pos + 6 );
    int processed = line.mid( pos+6, po2-pos-6 ).toInt();

    pos = line.indexOf( "of" );
    po2 = line.indexOf( " ", pos + 3 );
    m_size = line.mid( pos+3, po2-pos-3 ).toInt();

    d->speedEst->dataWritten( processed*1024 );

    emit processedSize( processed, m_size );
}


void K3b::CdrdaoWriter::parseCdrdaoMessage()
{
    static const char msgSync[] = { 0xff, 0x00, 0xff, 0x00 };
    unsigned int avail = m_comSock->bytesAvailable();
    unsigned int msgs = avail / ( sizeof(msgSync)+d->progressMsgSize );
    unsigned int count = 0;

    if ( msgs < 1 )
        return;
    else if ( msgs > 1) {
        // move the read-index forward to the beginnig of the most recent message
        count = ( msgs-1 ) * ( sizeof(msgSync)+d->progressMsgSize );
        m_comSock->seek( count );
        kDebug() << "(K3b::CdrdaoParser) " << msgs-1 << " message(s) skipped";
    }

    while( count < avail ) {

        // search for msg sync
        int state = 0;
        char buf;
        while( state < 4 ) {
            m_comSock->getChar( &buf );
            ++count;
            if( count == avail ) {
                //        kDebug() << "(K3b::CdrdaoParser) remote message sync not found (" << count << ")";
                return;
            }

            if( buf == msgSync[state] )
                ++state;
            else
                state = 0;
        }

        if( (avail - count) < d->progressMsgSize ) {
            kDebug() << "(K3b::CdrdaoParser) could not read complete remote message.";
            return;
        }

        // read one message (the message size changed in cdrdao 1.1.8)
        ::memset( &d->newMsg, 0, d->progressMsgSize );
        int size = m_comSock->read( (char*)&d->newMsg, d->progressMsgSize);
        if( size == -1 ) {
            kDebug() << "(K3b::CdrdaoParser) read error";
            return;
        }
        count += size;

        // sometimes the progress takes one step back (on my system when using paranoia-level 3)
        // so we just use messages that are greater than the previous or first messages
        if( d->oldMsg < d->newMsg
            || ( d->newMsg.track == 1 &&
                 d->newMsg.trackProgress <= 10 )) {

            if( d->newMsg.track != m_currentTrack ) {
                switch( d->newMsg.status ) {
                case PGSMSG_RCD_EXTRACTING:
                    emit nextTrack( d->newMsg.track, d->newMsg.totalTracks );
                    break;
                case PGSMSG_WCD_LEADIN:
                    emit newSubTask( i18n("Writing leadin") );
                    break;
                case PGSMSG_WCD_DATA:
                    emit nextTrack( d->newMsg.track, d->newMsg.totalTracks );
                    break;
                case PGSMSG_WCD_LEADOUT:
                    emit newSubTask( i18n("Writing leadout") );
                    break;
                }

                m_currentTrack = d->newMsg.track;
            }

            if( d->newMsg.status == PGSMSG_WCD_LEADIN || d->newMsg.status == PGSMSG_WCD_LEADOUT ) {
                // cdrdao >= 1.1.8 emits progress data when writing the lead-in and lead-out :)
                emit subPercent( d->newMsg.totalProgress/10 );
            }
            else {
                emit subPercent( d->newMsg.trackProgress/10 );
                emit percent( d->newMsg.totalProgress/10 );
            }

            emit buffer(d->newMsg.bufferFillRate);

            if( d->progressMsgSize == (unsigned int)sizeof(ProgressMsg2) )
                emit deviceBuffer( d->newMsg.writerFillRate );

            ::memcpy( &d->oldMsg, &d->newMsg, d->progressMsgSize );
        }
    }
}


void K3b::CdrdaoWriter::slotThroughput( int t )
{
    // FIXME: determine sector size
    emit writeSpeed( t, K3b::Device::SPEED_FACTOR_CD_MODE1 );
}


QString K3b::CdrdaoWriter::findDriverFile( const K3b::ExternalBin* bin )
{
    if( !bin )
        return QString();

    // cdrdao normally in (prefix)/bin and driver table in (prefix)/share/cdrdao
    QString path = bin->path();
    path.truncate( path.lastIndexOf("/") );
    path.truncate( path.lastIndexOf("/") );
    path += "/share/cdrdao/drivers";
    if( QFile::exists(path) )
        return path;
    else {
        kDebug() << "(K3b::CdrdaoWriter) could not find cdrdao driver table.";
        return QString();
    }
}


// returns true if the driver file could be opened and no driver could be found
// TODO: cache the drivers
bool K3b::CdrdaoWriter::defaultToGenericMMC( K3b::Device::Device* dev, bool writer )
{
    QString driverTable = findDriverFile( m_cdrdaoBinObject );
    if( !driverTable.isEmpty() ) {
        QFile f( driverTable );
        if( f.open( QIODevice::ReadOnly ) ) {
            // read all drivers
            QStringList drivers;
            QTextStream fStr( &f );
            while( !fStr.atEnd() ) {
                QString line = fStr.readLine();
                if( line.isEmpty() )
                    continue;
                if( line[0] == '#' )
                    continue;
                if( line[0] == 'R' && writer )
                    continue;
                if( line[0] == 'W' && !writer )
                    continue;
                drivers.append(line);
            }

            // search for the driver
            for( QStringList::const_iterator it = drivers.constBegin(); it != drivers.constEnd(); ++it ) {
                if( (*it).section( '|', 1, 1 ) == dev->vendor() &&
                    (*it).section( '|', 2, 2 ) == dev->description() )
                    return false;
            }

            // no driver found
            return true;
        }
        else {
            kDebug() << "(K3b::CdrdaoWriter) could not open driver table " << driverTable;
            return false;
        }
    }
    else
        return false;
}

#include "k3bcdrdaowriter.moc"
