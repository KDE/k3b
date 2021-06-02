/*

    SPDX-FileCopyrightText: 2003-2010 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#include "k3bcdcopyjob.h"
#include "k3baudiosessionreadingjob.h"

#include "k3bexternalbinmanager.h"
#include "k3bdevice.h"
#include "k3bdiskinfo.h"
#include "k3btoc.h"
#include "k3bglobals.h"
#include "k3bdevicehandler.h"
#include "k3breadcdreader.h"
#include "k3bdatatrackreader.h"
#include "k3bcdrecordwriter.h"
#include "k3bcdtext.h"
#include "k3bcore.h"
#include "k3binffilewriter.h"
#include "k3bglobalsettings.h"
#include "k3bcddb.h"
#include "k3b_i18n.h"

#include <KConfig>
#include <KIO/Global>
#include <KIO/DeleteJob>
#include <KIO/Job>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>
#include <QVector>
#include <QApplication>

#include <KCddb/Client>
#include <KCddb/Cdinfo>



class K3b::CdCopyJob::Private
{
public:
    Private()
        : canceled(false),
          running(false),
          readcdReader(0),
          dataTrackReader(0),
          audioSessionReader(0),
          cdrecordWriter(0),
          infFileWriter(0),
          cddb(0) {
    }

    bool canceled;
    bool error;
    bool readingSuccessful;
    bool running;

    int numSessions;
    bool doNotCloseLastSession;

    int doneCopies;
    int currentReadSession;
    int currentWrittenSession;

    K3b::Device::Toc toc;
    QByteArray cdTextRaw;

    K3b::ReadcdReader* readcdReader;
    K3b::DataTrackReader* dataTrackReader;
    K3b::AudioSessionReadingJob* audioSessionReader;
    K3b::CdrecordWriter* cdrecordWriter;
    K3b::InfFileWriter* infFileWriter;

    bool audioReaderRunning;
    bool dataReaderRunning;
    bool writerRunning;

    // image filenames, one for every track
    QStringList imageNames;

    // inf-filenames for writing audio tracks
    QStringList infNames;

    // indicates if we created a dir or not
    bool deleteTempDir;

    KCDDB::Client* cddb;
    KCDDB::CDInfo cddbInfo;

    bool haveCddb;
    bool haveCdText;

    QVector<bool> dataSessionProbablyTAORecorded;

    // used to determine progress
    QVector<long> sessionSizes;
    long overallSize;
};


K3b::CdCopyJob::CdCopyJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::BurnJob( hdl, parent ),
      m_simulate(false),
      m_copies(1),
      m_onlyCreateImages(false),
      m_onTheFly(true),
      m_ignoreDataReadErrors(false),
      m_ignoreAudioReadErrors(true),
      m_noCorrection(false),
      m_dataReadRetries(128),
      m_audioReadRetries(5),
      m_copyCdText(true),
      m_writingMode( K3b::WritingModeAuto )
{
    d = new Private();
}


K3b::CdCopyJob::~CdCopyJob()
{
    delete d->infFileWriter;
    delete d;
}


void K3b::CdCopyJob::start()
{
    d->running = true;
    d->canceled = false;
    d->error = false;
    d->readingSuccessful = false;
    d->audioReaderRunning = d->dataReaderRunning = d->writerRunning = false;
    d->sessionSizes.clear();
    d->dataSessionProbablyTAORecorded.clear();
    d->deleteTempDir = false;
    d->haveCdText = false;
    d->haveCddb = false;

    if ( m_onlyCreateImages )
        m_onTheFly = false;

    jobStarted();

    emit newTask( i18n("Checking Source Medium") );

    emit burning(false);
    emit newSubTask( i18n("Waiting for source medium") );

    // wait for a source disk
    if( waitForMedium( m_readerDevice,
                       K3b::Device::STATE_COMPLETE|K3b::Device::STATE_INCOMPLETE,
                       K3b::Device::MEDIA_WRITABLE_CD|K3b::Device::MEDIA_CD_ROM ) == Device::MEDIA_UNKNOWN ) {
        finishJob( true, false );
        return;
    }

    emit newSubTask( i18n("Checking source medium") );

    // FIXME: read ISRCs and MCN

    connect( K3b::Device::mediaInfo( m_readerDevice ), SIGNAL(finished(K3b::Device::DeviceHandler*)),
             this, SLOT(slotDiskInfoReady(K3b::Device::DeviceHandler*)) );
}


void K3b::CdCopyJob::slotDiskInfoReady( K3b::Device::DeviceHandler* dh )
{
    if( dh->success() ) {
        d->toc = dh->toc();

        //
        // for now we copy audio, pure data (aka 1 data track), cd-extra (2 session, audio and data),
        // and data multisession which one track per session.
        // Everything else will be rejected
        //
        bool canCopy = true;
        bool audio = false;
        d->numSessions = dh->diskInfo().numSessions();
        d->doNotCloseLastSession = (dh->diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE);
        switch( dh->toc().contentType() ) {
        case K3b::Device::DATA:
            // check if every track is in it's own session
            // only then we copy the cd
            if( (int)dh->toc().count() != dh->diskInfo().numSessions() ) {
                emit infoMessage( i18n("K3b does not copy CDs containing multiple data tracks."), MessageError );
                canCopy = false;
            }
            else if( dh->diskInfo().numSessions() > 1 )
                emit infoMessage( i18n("Copying Multisession Data CD."), MessageInfo );
            else
                emit infoMessage( i18n("Copying Data CD."), MessageInfo );
            break;

        case K3b::Device::MIXED:
            audio = true;
            if( dh->diskInfo().numSessions() != 2 || d->toc[0].type() != K3b::Device::Track::TYPE_AUDIO ) {
                emit infoMessage( i18n("K3b can only copy CD-Extra mixed mode CDs."), MessageError );
                canCopy = false;
            }
            else
                emit infoMessage( i18n("Copying Enhanced Audio CD (CD-Extra)."), MessageInfo );
            break;

        case K3b::Device::AUDIO:
            audio = true;
            emit infoMessage( i18n("Copying Audio CD."), MessageInfo );
            break;

        case K3b::Device::NONE:
        default:
            emit infoMessage( i18n("The source disk is empty."), MessageError );
            canCopy = false;
            break;
        }

        //
        // A data track recorded in TAO mode has two run-out blocks which cannot be read and contain
        // zero data anyway. The problem is that I do not know of a valid method to determine if a track
        // was written in TAO (the control nibble does definitely not work, I never saw one which did not
        // equal 4).
        // So the solution for now is to simply try to read the last sector of a data track. If this is not
        // possible we assume it was written in TAO mode and reduce the length by 2 sectors
        //
        unsigned char buffer[2048];
        int i = 1;
        for( K3b::Device::Toc::iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
            if( (*it).type() == K3b::Device::Track::TYPE_DATA ) {
                // we try twice just to be sure
                if( m_readerDevice->read10( buffer, 2048, (*it).lastSector().lba(), 1 ) ||
                    m_readerDevice->read10( buffer, 2048, (*it).lastSector().lba(), 1 ) ) {
                    d->dataSessionProbablyTAORecorded.append(false);
                    qDebug() << "(K3b::CdCopyJob) track " << i << " probably DAO recorded.";
                }
                else {
                    d->dataSessionProbablyTAORecorded.append(true);
                    qDebug() << "(K3b::CdCopyJob) track " << i << " probably TAO recorded.";
                }
            }

            ++i;
        }


        //
        // To copy mode2 data tracks we need cdrecord >= 2.01a12 which introduced the -xa1 and -xamix options
        //
        if( k3bcore->externalBinManager()->binObject("cdrecord") &&
            !k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "xamix" ) ) {
            for( K3b::Device::Toc::const_iterator it = d->toc.constBegin(); it != d->toc.constEnd(); ++it ) {
                if( (*it).type() == K3b::Device::Track::TYPE_DATA &&
                    ( (*it).mode() == K3b::Device::Track::XA_FORM1 ||
                      (*it).mode() == K3b::Device::Track::XA_FORM2 ) ) {
                    emit infoMessage( i18n("K3b needs cdrecord 2.01a12 or newer to copy Mode2 data tracks."), MessageError );
                    finishJob( true, false );
                    return;
                }
            }
        }


        //
        // It is not possible to create multisession cds in raw writing mode
        //
        if( d->numSessions > 1 && m_writingMode == K3b::WritingModeRaw ) {
            if( !questionYesNo( i18n("You will only be able to copy the first session in raw writing mode. "
                                     "Continue anyway?"),
                                i18n("Multisession CD") ) ) {
                finishJob( true, false );
                return;
            }
            else {
                emit infoMessage( i18n("Only copying first session."), MessageWarning );
                // TODO: remove the second session from the progress stuff
            }
        }


        //
        // We already create the temp filenames here since we need them to check the free space
        //
        if( !m_onTheFly || m_onlyCreateImages ) {
            if( !prepareImageFiles() ) {
                finishJob( false, true );
                return;
            }

            //
            // check free temp space
            //
            KIO::filesize_t imageSpaceNeeded = 0;
            for( K3b::Device::Toc::const_iterator it = d->toc.constBegin(); it != d->toc.constEnd(); ++it ) {
                if( (*it).type() == K3b::Device::Track::TYPE_AUDIO )
                    imageSpaceNeeded += (*it).length().audioBytes() + 44;
                else
                    imageSpaceNeeded += (*it).length().mode1Bytes();
            }

            unsigned long avail, size;
            QString pathToTest = m_tempPath.left( m_tempPath.lastIndexOf( '/' ) );
            if( !K3b::kbFreeOnFs( pathToTest, size, avail ) ) {
                emit infoMessage( i18n("Unable to determine free space in temporary folder '%1'.",pathToTest), MessageError );
                d->error = true;
                canCopy = false;
            }
            else {
                if( avail < imageSpaceNeeded/1024 ) {
                    emit infoMessage( i18n("Not enough space left in temporary folder."), MessageError );
                    d->error = true;
                    canCopy = false;
                }
            }
        }

        if( canCopy ) {
            if( K3b::isMounted( m_readerDevice ) ) {
                emit infoMessage( i18n("Unmounting source medium"), MessageInfo );
                K3b::unmount( m_readerDevice );
            }

            d->overallSize = 0;

            // now create some progress helper values
            for( K3b::Device::Toc::const_iterator it = d->toc.constBegin(); it != d->toc.constEnd(); ++it ) {
                d->overallSize += (*it).length().lba();
                if( d->sessionSizes.isEmpty() || (*it).type() == K3b::Device::Track::TYPE_DATA )
                    d->sessionSizes.append( (*it).length().lba() );
                else
                    d->sessionSizes[0] += (*it).length().lba();
            }

            if( audio && !m_onlyCreateImages ) {
                if( m_copyCdText )
                    searchCdText();
                else
                    queryCddb();
            }
            else
                startCopy();
        }
        else {
            finishJob( false, true );
        }
    }
    else {
        emit infoMessage( i18n("Unable to read Table of contents"), MessageError );
        finishJob( false, true );
    }
}


void K3b::CdCopyJob::searchCdText()
{
    emit newSubTask( i18n("Searching CD-Text") );

    connect( K3b::Device::sendCommand( K3b::Device::DeviceHandler::CommandCdTextRaw, m_readerDevice ),
             SIGNAL(finished(K3b::Device::DeviceHandler*)),
             this,
             SLOT(slotCdTextReady(K3b::Device::DeviceHandler*)) );
}


void K3b::CdCopyJob::slotCdTextReady( K3b::Device::DeviceHandler* dh )
{
    if( dh->success() ) {
        if( K3b::Device::CdText::checkCrc( dh->cdTextRaw() ) ) {
            K3b::Device::CdText cdt( dh->cdTextRaw() );
            emit infoMessage( i18n("Found CD-Text (%1 - %2).",cdt.performer(),cdt.title()), MessageSuccess );
            d->haveCdText = true;
            d->cdTextRaw = dh->cdTextRaw();
        }
        else {
            emit infoMessage( i18n("Found corrupted CD-Text. Ignoring it."), MessageWarning );
            d->haveCdText = false;
        }
    }
    else {
        emit infoMessage( i18n("No CD-Text found."), MessageInfo );

        d->haveCdText = false;
    }

    queryCddb();
}


void K3b::CdCopyJob::queryCddb()
{
    emit newSubTask( i18n("Querying CDDB") );

    d->haveCddb = false;

    if( !d->cddb ) {
        d->cddb = new KCDDB::Client();
        d->cddb->setBlockingMode( false );
        connect( d->cddb, SIGNAL(finished(KCDDB::Result)),
                 this, SLOT(slotCddbQueryFinished(KCDDB::Result)) );
    }

    d->cddb->config().load();
    d->cddb->lookup( K3b::CDDB::createTrackOffsetList( d->toc ) );
}


void K3b::CdCopyJob::slotCddbQueryFinished( KCDDB::Result result )
{
    if( result == KCDDB::Success ) {
        d->cddbInfo = d->cddb->lookupResponse().first();
        d->haveCddb = true;
        emit infoMessage( i18n("Found CDDB entry (%1 - %2).",
                               d->cddbInfo.get( KCDDB::Artist ).toString(),
                               d->cddbInfo.get( KCDDB::Title ).toString() ),
                          MessageSuccess );

        // save the entry locally
        d->cddb->store( d->cddbInfo, K3b::CDDB::createTrackOffsetList( d->toc ) );
    }
    else if ( result == KCDDB::MultipleRecordFound ) {
        KCDDB::CDInfoList results = d->cddb->lookupResponse();
        int i = K3b::CDDB::MultiEntriesDialog::selectCddbEntry( results, qApp->activeWindow() );
        if ( i >= 0 ) {
            d->haveCddb = true;
            d->cddbInfo = results[i];

            // save the entry locally
            d->cddb->store( d->cddbInfo, K3b::CDDB::createTrackOffsetList( d->toc ) );
        }
        else {
            d->haveCddb = false;
        }
    }
    else if( result == KCDDB::NoRecordFound ) {
        emit infoMessage( i18n("No CDDB entry found."), MessageWarning );
    }
    else {
        emit infoMessage( i18n("CDDB error (%1).",
                               KCDDB::resultToString( result ) ),
                          MessageError );
    }

    startCopy();
}


void K3b::CdCopyJob::startCopy()
{
    d->currentWrittenSession = d->currentReadSession = 1;
    d->doneCopies = 0;

    if ( d->haveCdText && d->haveCddb ) {
        K3b::Device::CdText cdt( d->cdTextRaw );
        if ( !questionYesNo( i18n( "Found CD-Text (%1 - %2) and CDDB (%3 - %4) entries. "
                                   "Which one should be used to generate the CD-Text on the new CD?",
                                   cdt.performer(),
                                   cdt.title(),
                                   d->cddbInfo.get( KCDDB::Artist ).toString(),
                                   d->cddbInfo.get( KCDDB::Title ).toString() ),
                             i18n( "CD-Text" ),
                             KGuiItem( i18n( "Use CD-Text data" ) ),
                             KGuiItem( i18n( "Use CDDB entry" ) ) ) ) {
            d->haveCdText = false;
        }
    }

    if( m_onTheFly && !m_onlyCreateImages ) {
        emit newSubTask( i18n("Preparing write process...") );

        if( writeNextSession() )
            readNextSession();
        else {
            finishJob( d->canceled, d->error );
        }
    }
    else
        readNextSession();
}


void K3b::CdCopyJob::cancel()
{
    d->canceled = true;

    if( d->writerRunning ) {
        //
        // we will handle cleanup in slotWriterFinished()
        // if we are writing onthefly the reader won't be able to write
        // anymore and will finish unsuccessfully, too
        //
        d->cdrecordWriter->cancel();
    }
    else if( d->audioReaderRunning )
        d->audioSessionReader->cancel();
    else if( d->dataReaderRunning )
        //    d->readcdReader->cancel();
        d->dataTrackReader->cancel();
}


bool K3b::CdCopyJob::prepareImageFiles()
{
    qDebug() << "(K3b::CdCopyJob) prepareImageFiles()";

    d->imageNames.clear();
    d->infNames.clear();
    d->deleteTempDir = false;

    QFileInfo fi( m_tempPath );

    if( d->toc.count() > 1 || d->toc.contentType() == K3b::Device::AUDIO ) {
        // create a directory which contains all the images and inf and stuff
        // and save it in some cool structure

        bool tempDirReady = false;
        if( !fi.isDir() ) {
            if( QFileInfo( m_tempPath.section( '/', 0, -2 ) ).isDir() ) {
                if( !QFile::exists( m_tempPath ) ) {
                    QDir dir( m_tempPath.section( '/', 0, -2 ) );
                    dir.mkdir( m_tempPath.section( '/', -1 ) );
                    tempDirReady = true;
                }
                else
                    m_tempPath = m_tempPath.section( '/', 0, -2 );
            }
            else {
                emit infoMessage( i18n("Specified an unusable temporary path. Using default."), MessageWarning );
                m_tempPath = K3b::defaultTempPath();
            }
        }

        // create temp dir
        if( !tempDirReady ) {
            QDir dir( m_tempPath );
            m_tempPath = K3b::findUniqueFilePrefix( "k3bCdCopy", m_tempPath );
            qDebug() << "(K3b::CdCopyJob) creating temp dir: " << m_tempPath;
            if( !dir.mkdir( m_tempPath ) ) {
                emit infoMessage( i18n("Unable to create temporary folder '%1'.",m_tempPath), MessageError );
                return false;
            }
            d->deleteTempDir = true;
        }

        m_tempPath = K3b::prepareDir( m_tempPath );
        emit infoMessage( i18n("Using temporary folder %1.",m_tempPath), MessageInfo );

        // create temp filenames
        int i = 1;
        for( K3b::Device::Toc::const_iterator it = d->toc.constBegin(); it != d->toc.constEnd(); ++it ) {
            if( (*it).type() == K3b::Device::Track::TYPE_AUDIO ) {
                d->imageNames.append( m_tempPath + QString("Track%1.wav").arg(QString::number(i).rightJustified(2, '0')) );
                d->infNames.append( m_tempPath + QString("Track%1.inf").arg(QString::number(i).rightJustified(2, '0')) );
            }
            else
                d->imageNames.append( m_tempPath + QString("Track%1.iso").arg(QString::number(i).rightJustified(2, '0')) );
            ++i;
        }

        qDebug() << "(K3b::CdCopyJob) created image filenames:";
        for( int i = 0; i < d->imageNames.count(); ++i )
            qDebug() << "(K3b::CdCopyJob) " << d->imageNames[i];

        return true;
    }
    else {
        // we only need a single image file
        if( !fi.isFile() ||
            questionYesNo( i18n("Do you want to overwrite %1?",m_tempPath),
                           i18n("File Exists") ) ) {
            if( fi.isDir() )
                m_tempPath = K3b::findTempFile( "iso", m_tempPath );
            else if( !QFileInfo( m_tempPath.section( '/', 0, -2 ) ).isDir() ) {
                emit infoMessage( i18n("Specified an unusable temporary path. Using default."), MessageWarning );
                m_tempPath = K3b::findTempFile( "iso" );
            }
            // else the user specified a file in an existing dir

            emit infoMessage( i18n("Writing image file to %1.",m_tempPath), MessageInfo );
        }
        else
            return false;

        d->imageNames.append( m_tempPath );

        return true;
    }
}


void K3b::CdCopyJob::readNextSession()
{
    if( !m_onTheFly || m_onlyCreateImages ) {
        if( d->numSessions > 1 )
            emit newTask( i18n("Reading Session %1",d->currentReadSession) );
        else
            emit newTask( i18n("Reading Source Medium") );

        if( d->currentReadSession == 1 )
            emit newSubTask( i18n("Reading track %1 of %2",QString::number(1),d->toc.count()) );
    }

    // there is only one situation where we need the audiosessionreader:
    // if the first session is an audio session. That means the first track
    // is an audio track
    if( d->currentReadSession == 1 && d->toc[0].type() == K3b::Device::Track::TYPE_AUDIO ) {
        if( !d->audioSessionReader ) {
            d->audioSessionReader = new K3b::AudioSessionReadingJob( this, this );
            connect( d->audioSessionReader, SIGNAL(nextTrack(int,int)),
                     this, SLOT(slotReadingNextTrack(int,int)) );
            connectSubJob( d->audioSessionReader,
                           SLOT(slotSessionReaderFinished(bool)),
                           K3b::Job::DEFAULT_SIGNAL_CONNECTION,
                           K3b::Job::DEFAULT_SIGNAL_CONNECTION,
                           SLOT(slotReaderProgress(int)),
                           SLOT(slotReaderSubProgress(int)) );
        }

        d->audioSessionReader->setDevice( m_readerDevice );
        d->audioSessionReader->setToc( d->toc );
        d->audioSessionReader->setParanoiaMode( m_paranoiaMode );
        d->audioSessionReader->setReadRetries( m_audioReadRetries );
        d->audioSessionReader->setNeverSkip( !m_ignoreAudioReadErrors );
        if( m_onTheFly )
            d->audioSessionReader->writeTo( d->cdrecordWriter->ioDevice() );
        else
            d->audioSessionReader->setImageNames( d->imageNames );  // the audio tracks are always the first tracks

        d->audioReaderRunning = true;
        d->audioSessionReader->start();
    }
    else {
        if( !d->dataTrackReader ) {
            d->dataTrackReader = new K3b::DataTrackReader( this, this );
            connect( d->dataTrackReader, SIGNAL(percent(int)), this, SLOT(slotReaderProgress(int)) );
            connect( d->dataTrackReader, SIGNAL(processedSize(int,int)), this, SLOT(slotReaderProcessedSize(int,int)) );
            connect( d->dataTrackReader, SIGNAL(finished(bool)), this, SLOT(slotSessionReaderFinished(bool)) );
            connect( d->dataTrackReader, SIGNAL(infoMessage(QString,int)), this, SIGNAL(infoMessage(QString,int)) );
            connect( d->dataTrackReader, SIGNAL(debuggingOutput(QString,QString)),
                     this, SIGNAL(debuggingOutput(QString,QString)) );
        }

        d->dataTrackReader->setDevice( m_readerDevice );
        d->dataTrackReader->setIgnoreErrors( m_ignoreDataReadErrors );
        d->dataTrackReader->setNoCorrection( m_noCorrection );
        d->dataTrackReader->setRetries( m_dataReadRetries );
        if( m_onlyCreateImages )
            d->dataTrackReader->setSectorSize( K3b::DataTrackReader::MODE1 );
        else
            d->dataTrackReader->setSectorSize( K3b::DataTrackReader::AUTO );

        K3b::Device::Track* track = 0;
        int dataTrackIndex = 0;
        if( d->toc.contentType() == K3b::Device::MIXED ) {
            track = &d->toc[d->toc.count()-1];
            dataTrackIndex = 0;
        }
        else {
            track = &d->toc[d->currentReadSession-1]; // only one track per session
            dataTrackIndex = d->currentReadSession-1;
        }

        // HACK: if the track is TAO recorded cut the two run-out sectors
        if( d->dataSessionProbablyTAORecorded.count() > dataTrackIndex &&
            d->dataSessionProbablyTAORecorded[dataTrackIndex] )
            d->dataTrackReader->setSectorRange( track->firstSector(), track->lastSector() - 2 );
        else
            d->dataTrackReader->setSectorRange( track->firstSector(), track->lastSector() );

        int trackNum = d->currentReadSession;
        if( d->toc.contentType() == K3b::Device::MIXED )
            trackNum = d->toc.count();

        if( m_onTheFly )
            d->dataTrackReader->writeTo( d->cdrecordWriter->ioDevice() );
        else
            d->dataTrackReader->setImagePath( d->imageNames[trackNum-1] );

        d->dataReaderRunning = true;
        if( !m_onTheFly || m_onlyCreateImages )
            slotReadingNextTrack( 1, 1 );

        d->dataTrackReader->start();
    }
}


bool K3b::CdCopyJob::writeNextSession()
{
    // we emit our own task since the cdrecord task is way too simple
    if( d->numSessions > 1 ) {
        if( m_simulate )
            emit newTask( i18n("Simulating Session %1",d->currentWrittenSession) );
        else if( m_copies > 1 )
            emit newTask( i18n("Writing Copy %1 (Session %2)",d->doneCopies+1,d->currentWrittenSession) );
        else
            emit newTask( i18n("Writing Copy (Session %1)",d->currentWrittenSession) );
    }
    else {
        if( m_simulate )
            emit newTask( i18n("Simulating") );
        else if( m_copies > 1 )
            emit newTask( i18n("Writing Copy %1",d->doneCopies+1) );
        else
            emit newTask( i18n("Writing Copy") );
    }

    if ( d->currentWrittenSession == 1 ) {
        emit newSubTask( i18n("Waiting for media") );

        if( waitForMedium( m_writerDevice,
                          K3b::Device::STATE_EMPTY,
                          K3b::Device::MEDIA_WRITABLE_CD ) == Device::MEDIA_UNKNOWN ) {
            finishJob( true, false );
            return false;
        }
    }

    if( !d->cdrecordWriter ) {
        d->cdrecordWriter = new K3b::CdrecordWriter( m_writerDevice, this, this );
        connect( d->cdrecordWriter, SIGNAL(infoMessage(QString,int)), this, SIGNAL(infoMessage(QString,int)) );
        connect( d->cdrecordWriter, SIGNAL(percent(int)), this, SLOT(slotWriterProgress(int)) );
        connect( d->cdrecordWriter, SIGNAL(processedSize(int,int)), this, SIGNAL(processedSize(int,int)) );
        connect( d->cdrecordWriter, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
        connect( d->cdrecordWriter, SIGNAL(processedSubSize(int,int)), this, SIGNAL(processedSubSize(int,int)) );
        connect( d->cdrecordWriter, SIGNAL(nextTrack(int,int)), this, SLOT(slotWritingNextTrack(int,int)) );
        connect( d->cdrecordWriter, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
        connect( d->cdrecordWriter, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
        connect( d->cdrecordWriter, SIGNAL(writeSpeed(int,K3b::Device::SpeedMultiplicator)), this, SIGNAL(writeSpeed(int,K3b::Device::SpeedMultiplicator)) );
        connect( d->cdrecordWriter, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
        //    connect( d->cdrecordWriter, SIGNAL(newTask(QString)), this, SIGNAL(newTask(QString)) );
        connect( d->cdrecordWriter, SIGNAL(newSubTask(QString)), this, SIGNAL(newSubTask(QString)) );
        connect( d->cdrecordWriter, SIGNAL(debuggingOutput(QString,QString)),
                 this, SIGNAL(debuggingOutput(QString,QString)) );
    }

    d->cdrecordWriter->setBurnDevice( m_writerDevice );
    d->cdrecordWriter->clearArguments();
    d->cdrecordWriter->setSimulate( m_simulate );
    d->cdrecordWriter->setBurnSpeed( m_speed );


    // create the cdrecord arguments
    if( d->currentWrittenSession == 1 && d->toc[0].type() == K3b::Device::Track::TYPE_AUDIO ) {
        //
        // Audio session
        //


        if( !d->infFileWriter )
            d->infFileWriter = new K3b::InfFileWriter();

        //
        // create the inf files if not already done
        //
        if( d->infNames.isEmpty() || !QFile::exists( d->infNames[0] ) ) {

            int trackNumber = 1;

            for( K3b::Device::Toc::const_iterator it = d->toc.constBegin(); it != d->toc.constEnd(); ++it ) {
                const K3b::Device::Track& track = *it;

                if( track.type() == K3b::Device::Track::TYPE_DATA )
                    break;

                d->infFileWriter->setTrack( track );
                d->infFileWriter->setTrackNumber( trackNumber );

                if( d->haveCddb ) {
                    d->infFileWriter->setTrackTitle( d->cddbInfo.track( trackNumber-1 ).get( KCDDB::Title ).toString() );
                    d->infFileWriter->setTrackPerformer( d->cddbInfo.track( trackNumber-1 ).get( KCDDB::Artist ).toString() );
                    d->infFileWriter->setTrackMessage( d->cddbInfo.track( trackNumber-1 ).get( KCDDB::Comment ).toString() );

                    d->infFileWriter->setAlbumTitle( d->cddbInfo.get( KCDDB::Title ).toString() );
                    d->infFileWriter->setAlbumPerformer( d->cddbInfo.get( KCDDB::Artist ).toString() );
                }

                if( m_onTheFly ) {

                    d->infFileWriter->setBigEndian( true );

                    // we let KTempFile choose a temp file but delete it on our own
                    // the same way we delete them when writing with images
                    // It is important that the files have the ending inf because
                    // cdrecord only checks this
                    QTemporaryFile tmp( "XXXXXX.inf" );
                    tmp.setAutoRemove( false );
                    tmp.open();
                    d->infNames.append( tmp.fileName() );
                    QTextStream stream( &tmp );
                    bool success = d->infFileWriter->save( stream );
                    tmp.close();
                    if( !success )
                        return false;
                }
                else {
                    d->infFileWriter->setBigEndian( false );

                    if( !d->infFileWriter->save( d->infNames[trackNumber-1] ) )
                        return false;
                }

                ++trackNumber;
            }
        }

        //
        // the inf files are ready and named correctly when writing with images
        //
        K3b::WritingMode usedWritingMode = m_writingMode;
        if( usedWritingMode == K3b::WritingModeAuto ) {
            //
            // there are a lot of writers out there which produce coasters
            // in dao mode if the CD contains pregaps of length 0 (or maybe already != 2 secs?)
            //
            bool zeroPregap = false;
            if( d->numSessions == 1 ) {
                for( K3b::Device::Toc::const_iterator it = d->toc.constBegin(); it != d->toc.constEnd(); ++it ) {
                    const K3b::Device::Track& track = *it;
                    if( track.index0() == 0 ) {
                        ++it;
                        if( it != d->toc.constEnd() )
                            zeroPregap = true;
                        --it;
                    }
                }
            }

            if( zeroPregap && m_writerDevice->supportsRawWriting() ) {
                if( d->numSessions == 1 )
                    usedWritingMode = K3b::WritingModeRaw;
                else
                    usedWritingMode = K3b::WritingModeTao;
            }
            else if( m_writerDevice->dao() )
                usedWritingMode = K3b::WritingModeSao;
            else if( m_writerDevice->supportsRawWriting() )
                usedWritingMode = K3b::WritingModeRaw;
            else
                usedWritingMode = K3b::WritingModeTao;
        }
        d->cdrecordWriter->setWritingMode( usedWritingMode  );

        d->cdrecordWriter->setMulti( d->numSessions > 1 );

        if( d->haveCddb || d->haveCdText ) {
            if( usedWritingMode == K3b::WritingModeTao ) {
                emit infoMessage( i18n("It is not possible to write CD-Text in TAO mode."), MessageWarning );
            }
            else if( d->haveCdText ) {
                // use the raw CDTEXT data
                d->cdrecordWriter->setRawCdText( d->cdTextRaw );
            }
            else {
                // make sure the writer job does not create raw cdtext
                d->cdrecordWriter->setRawCdText( QByteArray() );
                // cdrecord will use the cdtext data in the inf files
                d->cdrecordWriter->addArgument( "-text" );
            }
        }

        d->cdrecordWriter->addArgument( "-useinfo" );

        //
        // add all the audio tracks
        //
        d->cdrecordWriter->addArgument( "-audio" )->addArgument( "-shorttrack" );

        for( int i = 0; i < d->infNames.count(); ++i ) {
            if( m_onTheFly )
                d->cdrecordWriter->addArgument( d->infNames[i] );
            else
                d->cdrecordWriter->addArgument( d->imageNames[i] );
        }
    }
    else {
        //
        // Data Session
        //
        K3b::Device::Track* track = 0;
        int dataTrackIndex = 0;
        if( d->toc.contentType() == K3b::Device::MIXED ) {
            track = &d->toc[d->toc.count()-1];
            dataTrackIndex = 0;
        }
        else {
            track = &d->toc[d->currentWrittenSession-1];
            dataTrackIndex = d->currentWrittenSession-1;
        }

        bool multi = d->doNotCloseLastSession || (d->numSessions > 1 && d->currentWrittenSession < d->toc.count());
        K3b::WritingMode usedWritingMode = m_writingMode;
        if( usedWritingMode == K3b::WritingModeAuto ) {
            // at least the NEC3540a does write 2056 byte sectors only in tao mode. Same for LG4040b
            // since writing data tracks in TAO mode is no loss let's default to TAO in the case of 2056 byte
            // sectors (which is when writing xa form1 sectors here)
            if( m_writerDevice->dao() &&
                d->toc.count() == 1 &&
                !multi &&
                track->mode() == K3b::Device::Track::MODE1 )
                usedWritingMode = K3b::WritingModeSao;
            else
                usedWritingMode = K3b::WritingModeTao;
        }
        d->cdrecordWriter->setWritingMode( usedWritingMode );

        //
        // all but the last session of a multisession disk are written in multi mode
        // and every data track has it's own session which we forced above
        //
        d->cdrecordWriter->setMulti( multi );

        // just to let the reader init
        if( m_onTheFly )
            d->cdrecordWriter->addArgument( "-waiti" );

        if( track->mode() == K3b::Device::Track::MODE1 )
            d->cdrecordWriter->addArgument( "-data" );
        else if( track->mode() == K3b::Device::Track::XA_FORM1 )
            d->cdrecordWriter->addArgument( "-xa1" );
        else
            d->cdrecordWriter->addArgument( "-xamix" );

        if( m_onTheFly ) {
            // HACK: if the track is TAO recorded cut the two run-out sectors
            unsigned long trackLen = track->length().lba();
            if( d->dataSessionProbablyTAORecorded.count() > dataTrackIndex &&
                d->dataSessionProbablyTAORecorded[dataTrackIndex] )
                trackLen -= 2;

            if( track->mode() == K3b::Device::Track::MODE1 )
                trackLen = trackLen * 2048;
            else if( track->mode() == K3b::Device::Track::XA_FORM1 )
                trackLen = trackLen * 2056; // see k3bdatatrackreader.h
            else
                trackLen = trackLen * 2332; // see k3bdatatrackreader.h
            d->cdrecordWriter->addArgument( QString("-tsize=%1").arg(trackLen) )->addArgument("-");
        }
        else if( d->toc.contentType() == K3b::Device::MIXED )
            d->cdrecordWriter->addArgument( d->imageNames[d->toc.count()-1] );
        else
            d->cdrecordWriter->addArgument( d->imageNames[d->currentWrittenSession-1] );

        // clear cd text from previous sessions
        d->cdrecordWriter->setRawCdText( QByteArray() );
    }


    //
    // Finally start the writer
    //
    emit burning(true);
    d->writerRunning = true;
    d->cdrecordWriter->start();

    return true;
}


// both the readcdreader and the audiosessionreader are connected to this slot
void K3b::CdCopyJob::slotSessionReaderFinished( bool success )
{
    d->audioReaderRunning = d->dataReaderRunning = false;

    if( success ) {
        if( d->numSessions > 1 )
            emit infoMessage( i18n("Successfully read session %1.",d->currentReadSession), MessageSuccess );
        else
            emit infoMessage( i18n("Successfully read source disk."), MessageSuccess );

        if( !m_onTheFly ) {
            if( d->numSessions > d->currentReadSession ) {
                d->currentReadSession++;
                readNextSession();
            }
            else {
                d->readingSuccessful = true;
                if( !m_onlyCreateImages ) {
                    if( m_readerDevice == m_writerDevice ) {
                        // eject the media (we do this blocking to know if it worked
                        // because if it did not it might happen that k3b overwrites a CD-RW
                        // source)
                        if( !K3b::eject( m_readerDevice ) ) {
                            blockingInformation( i18n("K3b was unable to eject the source disk. Please do so manually.") );
                        }
                    }

                    if( !writeNextSession() ) {
                        // nothing is running here...
                        finishJob( d->canceled, d->error );
                    }
                }
                else {
                    finishJob( false, false );
                }
            }
        }
    }
    else {
        if( !d->canceled ) {
            emit infoMessage( i18n("Error while reading session %1.",d->currentReadSession), MessageError );
            if( m_onTheFly )
                d->cdrecordWriter->setSourceUnreadable(true);
        }

        finishJob( d->canceled, !d->canceled );
    }
}


void K3b::CdCopyJob::slotWriterFinished( bool success )
{
    emit burning(false);

    d->writerRunning = false;

    if( success ) {
        //
        // if this was the last written session we need to reset d->currentWrittenSession
        // and start a new writing if more copies are wanted
        //

        if( d->currentWrittenSession < d->numSessions ) {
            d->currentWrittenSession++;
            d->currentReadSession++;

            // many drives need to reload the medium to return to a proper state
            if ( m_writerDevice->diskInfo().numSessions() < ( int )d->currentWrittenSession ) {
                emit infoMessage( i18n( "Need to reload medium to return to proper state." ), MessageInfo );
                emit newSubTask( i18n("Reloading the medium") );
                connect( K3b::Device::reload( m_writerDevice ), SIGNAL(finished(K3b::Device::DeviceHandler*)),
                         this, SLOT(slotMediaReloadedForNextSession(K3b::Device::DeviceHandler*)) );
            }

            if( !writeNextSession() ) {
                // nothing is running here...
                finishJob( d->canceled, d->error );
            }
            else if( m_onTheFly )
                readNextSession();
        }
        else {
            d->doneCopies++;

            if( !m_simulate && d->doneCopies < m_copies ) {
                // start next copy
                if( !K3b::eject( m_writerDevice ) ) {
                    blockingInformation( i18n("K3b was unable to eject the written disk. Please do so manually.") );
                }

                d->currentWrittenSession = 1;
                d->currentReadSession = 1;
                if( writeNextSession() ) {
                    if( m_onTheFly )
                        readNextSession();
                }
                else {
                    // nothing running here...
                    finishJob( d->canceled, d->error );
                }
            }
            else {
                if ( k3bcore->globalSettings()->ejectMedia() ) {
                    K3b::Device::eject( m_writerDevice );
                }
                finishJob( false, false );
            }
        }
    }
    else {
        //
        // If we are writing on the fly the reader will also stop when it is not able to write anymore
        // The error handling will be done only here in that case
        //

        // the K3b::CdrecordWriter emitted an error message

        finishJob( d->canceled, !d->canceled );
    }
}


void K3b::CdCopyJob::slotMediaReloadedForNextSession( K3b::Device::DeviceHandler* dh )
{
    if( !dh->success() )
        blockingInformation( i18n("Please reload the medium and press 'OK'"),
                             i18n("Failed to reload the medium") );

    if( !writeNextSession() ) {
        // nothing is running here...
        finishJob( d->canceled, d->error );
    }
    else if( m_onTheFly )
        readNextSession();
}


void K3b::CdCopyJob::cleanup()
{
    if( m_onTheFly || !m_keepImage || ((d->canceled || d->error) && !d->readingSuccessful) ) {
        emit infoMessage( i18n("Removing temporary files."), MessageInfo );
        for( QStringList::iterator it = d->infNames.begin(); it != d->infNames.end(); ++it )
            QFile::remove( *it );
    }

    if( !m_onTheFly && (!m_keepImage || ((d->canceled || d->error) && !d->readingSuccessful)) ) {
        emit infoMessage( i18n("Removing image files."), MessageInfo );
        for( QStringList::iterator it = d->imageNames.begin(); it != d->imageNames.end(); ++it )
            QFile::remove( *it );

        // remove the tempdir created in prepareImageFiles()
        if( d->deleteTempDir ) {
            KIO::del( QUrl::fromLocalFile( m_tempPath ), KIO::HideProgressInfo )->exec();
            d->deleteTempDir = false;
        }
    }
}


void K3b::CdCopyJob::slotReaderProgress( int p )
{
    if( !m_onTheFly || m_onlyCreateImages ) {
        int bigParts = ( m_onlyCreateImages ? 1 : (m_simulate ? 2 : m_copies + 1 ) );
        double done = (double)p * (double)d->sessionSizes[d->currentReadSession-1] / 100.0;
        for( int i = 0; i < d->currentReadSession-1; ++i )
            done += (double)d->sessionSizes[i];
        emit percent( (int)(100.0*done/(double)d->overallSize/(double)bigParts) );

        if( d->dataReaderRunning )
            emit subPercent(p);
    }
}


void K3b::CdCopyJob::slotReaderSubProgress( int p )
{
    // only if reading an audiosession
    if( !m_onTheFly || m_onlyCreateImages ) {
        emit subPercent( p );
    }
}


void K3b::CdCopyJob::slotReaderProcessedSize( int p, int pp )
{
    if( !m_onTheFly )
        emit processedSubSize( p, pp );
}


void K3b::CdCopyJob::slotWriterProgress( int p )
{
    int bigParts = ( m_simulate ? 1 : m_copies ) + ( m_onTheFly ? 0 : 1 );
    long done = ( m_onTheFly ? d->doneCopies : d->doneCopies+1 ) * d->overallSize
                + (p * d->sessionSizes[d->currentWrittenSession-1] / 100);
    for( int i = 0; i < d->currentWrittenSession-1; ++i )
        done += d->sessionSizes[i];
    emit percent( 100*done/d->overallSize/bigParts );
}


void K3b::CdCopyJob::slotWritingNextTrack( int t, int tt )
{
    if( d->toc.contentType() == K3b::Device::MIXED ) {
        if( d->currentWrittenSession == 1 )
            emit newSubTask( i18n("Writing track %1 of %2",t,d->toc.count()) );
        else
            emit newSubTask( i18n("Writing track %1 of %2",d->toc.count(),d->toc.count()) );
    }
    else if( d->numSessions > 1 )
        emit newSubTask( i18n("Writing track %1 of %2",d->currentWrittenSession,d->toc.count()) );
    else
        emit newSubTask( i18n("Writing track %1 of %2",t,tt) );
}


void K3b::CdCopyJob::slotReadingNextTrack( int t, int )
{
    if( !m_onTheFly || m_onlyCreateImages ) {
        int track = t;
        if( d->audioReaderRunning )
            track = t;
        else if( d->toc.contentType() == K3b::Device::MIXED )
            track = d->toc.count();
        else
            track = d->currentReadSession;

        emit newSubTask( i18n("Reading track %1 of %2",track,d->toc.count()) );
    }
}


QString K3b::CdCopyJob::jobDescription() const
{
    if( m_onlyCreateImages ) {
        return i18n("Creating CD Image");
    }
    else if( m_simulate ) {
        if( m_onTheFly )
            return i18n("Simulating CD Copy On-The-Fly");
        else
            return i18n("Simulating CD Copy");
    }
    else {
        if( m_onTheFly )
            return i18n("Copying CD On-The-Fly");
        else
            return i18n("Copying CD");
    }
}


QString K3b::CdCopyJob::jobDetails() const
{
    return i18np("Creating 1 copy",
                 "Creating %1 copies",
                 (m_simulate||m_onlyCreateImages) ? 1 : m_copies );
}


QString K3b::CdCopyJob::jobSource() const
{
    if( Device::Device* device = reader() )
        return device->vendor() + ' ' + device->description();
    else
        return QString();
}


QString K3b::CdCopyJob::jobTarget() const
{
    if( Device::Device* device = writer() )
        return device->vendor() + ' ' + device->description();
    else
        return m_tempPath;
}


void K3b::CdCopyJob::finishJob( bool c, bool e )
{
    if( d->running ) {
        if( c ) {
            d->canceled = true;
            emit canceled();
        }
        if( e )
            d->error = true;

        cleanup();

        d->running = false;

        jobFinished( !(c||e) );
    }
}


