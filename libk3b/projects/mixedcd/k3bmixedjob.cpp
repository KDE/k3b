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

#include "k3bmixedjob.h"
#include "k3bmixeddoc.h"
#include "k3bactivepipe.h"
#include "k3bfilesplitter.h"

#include "k3bdatadoc.h"
#include "k3bisoimager.h"
#include "k3bisooptions.h"
#include "k3bmsinfofetcher.h"
#include "k3baudioimager.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudionormalizejob.h"
#include "k3baudiojobtempdata.h"
#include "k3baudiomaxspeedjob.h"
#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bdevicehandler.h"
#include "k3bmsf.h"
#include "k3bglobals.h"
#include "k3bexternalbinmanager.h"
#include "k3bversion.h"
#include "k3bcore.h"
#include "k3bcdrecordwriter.h"
#include "k3bcdrdaowriter.h"
#include "k3btocfilewriter.h"
#include "k3binffilewriter.h"
#include "k3bglobalsettings.h"
#include "k3baudiofile.h"

#include <qfile.h>
#include <qdatastream.h>
#include <qapplication.h>

#include <kdebug.h>
#include <klocale.h>
#include <ktemporaryfile.h>
#include <kio/netaccess.h>
#include <kio/global.h>
#include <kstringhandler.h>


static QString createNonExistingFilesString( const QList<K3b::AudioFile*>& items, int max )
{
    QString s;
    int cnt = 0;
    for( QList<K3b::AudioFile*>::const_iterator it = items.begin();
         it != items.end(); ++it ) {

        s += KStringHandler::csqueeze( (*it)->filename(), 60 );

        ++cnt;
        if( cnt >= max || it == items.end() )
            break;

        s += "<br>";
    }

    if( items.count() > max )
        s += "...";

    return s;
}



class K3b::MixedJob::Private
{
public:
    Private()
        : maxSpeedJob(0) {
    }


    int copies;
    int copiesDone;

    K3b::AudioMaxSpeedJob* maxSpeedJob;
    bool maxSpeed;

    ActivePipe pipe;

    FileSplitter dataImageFile;
};


K3b::MixedJob::MixedJob( K3b::MixedDoc* doc, K3b::JobHandler* hdl, QObject* parent )
    : K3b::BurnJob( hdl, parent ),
      m_doc( doc ),
      m_normalizeJob(0)
{
    d = new Private;

    m_isoImager = new K3b::IsoImager( doc->dataDoc(), this, this );
    connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
    connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
    connect( m_isoImager, SIGNAL(debuggingOutput(const QString&, const QString&)),
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

    m_tempData = new K3b::AudioJobTempData( m_doc->audioDoc(), this );
    m_audioImager = new K3b::AudioImager( doc->audioDoc(), m_tempData, this, this );
    connect( m_audioImager, SIGNAL(infoMessage(const QString&, int)),
             this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_audioImager, SIGNAL(percent(int)), this, SLOT(slotAudioDecoderPercent(int)) );
    connect( m_audioImager, SIGNAL(subPercent(int)), this, SLOT(slotAudioDecoderSubPercent(int)) );
    connect( m_audioImager, SIGNAL(finished(bool)), this, SLOT(slotAudioDecoderFinished(bool)) );
    connect( m_audioImager, SIGNAL(nextTrack(int, int)), this, SLOT(slotAudioDecoderNextTrack(int, int)) );

    m_msInfoFetcher = new K3b::MsInfoFetcher( this, this );
    connect( m_msInfoFetcher, SIGNAL(finished(bool)), this, SLOT(slotMsInfoFetched(bool)) );
    connect( m_msInfoFetcher, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );

    m_writer = 0;
    m_tocFile = 0;
}


K3b::MixedJob::~MixedJob()
{
    delete m_tocFile;
    delete d;
}


K3b::Device::Device* K3b::MixedJob::writer() const
{
    if( m_doc->onlyCreateImages() )
        return 0;
    else
        return m_doc->burner();
}


K3b::Doc* K3b::MixedJob::doc() const
{
    return m_doc;
}


void K3b::MixedJob::start()
{
    jobStarted();

    m_canceled = false;
    m_errorOccuredAndAlreadyReported = false;
    d->copiesDone = 0;
    d->copies = m_doc->copies();
    m_currentAction = PREPARING_DATA;
    d->maxSpeed = false;

    if( m_doc->dummy() )
        d->copies = 1;

    prepareProgressInformation();

    //
    // Check if all files exist
    //
    QList<K3b::AudioFile*> nonExistingFiles;
    K3b::AudioTrack* track = m_doc->audioDoc()->firstTrack();
    while( track ) {
        K3b::AudioDataSource* source = track->firstSource();
        while( source ) {
            if( K3b::AudioFile* file = dynamic_cast<K3b::AudioFile*>( source ) ) {
                if( !QFile::exists( file->filename() ) )
                    nonExistingFiles.append( file );
            }
            source = source->next();
        }
        track = track->next();
    }
    if( !nonExistingFiles.isEmpty() ) {
        if( questionYesNo( "<p>" + i18n("The following files could not be found. Do you want to remove them from the "
                                        "project and continue without adding them to the image?") +
                           "<p>" + createNonExistingFilesString( nonExistingFiles, 10 ),
                           i18n("Warning"),
                           KGuiItem( i18n("Remove missing files and continue") ),
                           KGuiItem( i18n("Cancel and go back") ) ) ) {
            for( QList<K3b::AudioFile*>::const_iterator it = nonExistingFiles.constBegin();
                 it != nonExistingFiles.constEnd(); ++it ) {
                delete *it;
            }
        }
        else {
            m_canceled = true;
            emit canceled();
            jobFinished(false);
            return;
        }
    }

    //
    // Make sure the project is not empty
    //
    if( m_doc->audioDoc()->numOfTracks() == 0 ) {
        emit infoMessage( i18n("Please add files to your project first."), MessageError );
        jobFinished(false);
        return;
    }


    // set some flags that are needed
    m_doc->audioDoc()->setOnTheFly( m_doc->onTheFly() );  // for the toc writer
    m_doc->audioDoc()->setHideFirstTrack( false );   // unsupported
    m_doc->dataDoc()->setBurner( m_doc->burner() );  // so the isoImager can read ms data

    emit newTask( i18n("Preparing data") );

    determineWritingMode();

    //
    // First we make sure the data portion is valid
    //

    // we do not have msinfo yet
    m_currentAction = INITIALIZING_IMAGER;
    m_isoImager->setMultiSessionInfo( QString() );
    m_isoImager->init();
}


void K3b::MixedJob::startFirstCopy()
{
    //
    // if not onthefly create the iso image and then the wavs
    // and write then
    // if onthefly calculate the iso size
    //
    if( m_doc->onTheFly() ) {
        if( m_doc->speed() == 0 ) {
            emit newSubTask( i18n("Determining maximum writing speed") );

            //
            // try to determine the max possible speed
            // no need to check the data track's max speed. Most current systems are able
            // to handle the maximum possible
            //
            if( !d->maxSpeedJob ) {
                // the maxspeed job gets the device from the doc:
                m_doc->audioDoc()->setBurner( m_doc->burner() );
                d->maxSpeedJob = new K3b::AudioMaxSpeedJob( m_doc->audioDoc(), this, this );
                connect( d->maxSpeedJob, SIGNAL(percent(int)),
                         this, SIGNAL(subPercent(int)) );
                connect( d->maxSpeedJob, SIGNAL(finished(bool)),
                         this, SLOT(slotMaxSpeedJobFinished(bool)) );
            }
            d->maxSpeedJob->start();
        }
        else if( m_doc->mixedType() != K3b::MixedDoc::DATA_SECOND_SESSION ) {
            m_currentAction = PREPARING_DATA;
            m_isoImager->calculateSize();
        }
        else {
            // we cannot calculate the size since we don't have the msinfo yet
            // so first write the audio session
            writeNextCopy();
        }
    }
    else {
        emit burning(false);

        emit infoMessage( i18n("Creating audio image files in %1",m_doc->tempDir()), MessageInfo );

        m_tempFilePrefix = K3b::findUniqueFilePrefix( ( !m_doc->audioDoc()->title().isEmpty()
                                                        ? m_doc->audioDoc()->title()
                                                        : m_doc->dataDoc()->isoOptions().volumeID() ),
                                                      m_doc->tempDir() );

        m_tempData->prepareTempFileNames( m_doc->tempDir() );

        if( m_doc->mixedType() != K3b::MixedDoc::DATA_SECOND_SESSION ) {
            createIsoImage();
        }
        else {
            emit newTask( i18n("Creating audio image files") );
            m_currentAction = CREATING_AUDIO_IMAGE;
            m_audioImager->start();
        }
    }
}


void K3b::MixedJob::startSecondSession()
{
    // start the next session
    m_currentAction = WRITING_ISO_IMAGE;
    if( d->copiesDone > 0 ) {
        // we only create the image once. This should not be a problem???
        if( !prepareWriter() || !startWriting() ) {
            cleanupAfterError();
            jobFinished(false);
        }
    }
    else if( m_doc->dummy() ) {
        // do not try to get ms info in simulation mode since the cd is empty!
        if( m_doc->onTheFly() ) {
            m_currentAction = PREPARING_DATA;
            m_isoImager->calculateSize();
        }
        else
            createIsoImage();
    }
    else {
        m_currentAction = FETCHING_MSMessageInfo;
        m_msInfoFetcher->setDevice( m_doc->burner() );
        m_msInfoFetcher->start();
    }
}


void K3b::MixedJob::slotMaxSpeedJobFinished( bool success )
{
    d->maxSpeed = success;
    if( !success )
        emit infoMessage( i18n("Unable to determine maximum speed for some reason. Ignoring."), MessageWarning );

    if( m_doc->mixedType() != K3b::MixedDoc::DATA_SECOND_SESSION ) {
        m_currentAction = PREPARING_DATA;
        m_isoImager->calculateSize();
    }
    else {
        // we cannot calculate the size since we don't have the msinfo yet
        // so first write the audio session
        writeNextCopy();
    }
}


void K3b::MixedJob::writeNextCopy()
{
    // the prepareWriter method needs the action to be set
    if( m_doc->mixedType() == K3b::MixedDoc::DATA_FIRST_TRACK )
        m_currentAction = WRITING_ISO_IMAGE;
    else
        m_currentAction = WRITING_AUDIO_IMAGE;

    if( !prepareWriter() || !startWriting() ) {
        cleanupAfterError();
        jobFinished(false);
    }
}


void K3b::MixedJob::cancel()
{
    m_canceled = true;

    if( d->maxSpeedJob )
        d->maxSpeedJob->cancel();

    if( m_writer && m_writer->active() )
        m_writer->cancel();
    if ( m_isoImager->active() )
        m_isoImager->cancel();
    if ( m_audioImager->active() )
        m_audioImager->cancel();
    if ( m_msInfoFetcher->active() )
        m_msInfoFetcher->cancel();

#ifdef __GNUC__
#warning FIXME: wait for subjobs to finish after cancellation
#endif

    emit infoMessage( i18n("Writing canceled."), K3b::Job::MessageError );
    removeBufferFiles();
    emit canceled();
    jobFinished(false);
}


void K3b::MixedJob::slotMsInfoFetched( bool success )
{
    if( m_canceled || m_errorOccuredAndAlreadyReported )
        return;

    if( success ) {
        if( m_usedDataWritingApp == K3b::WritingAppCdrecord )
            m_isoImager->setMultiSessionInfo( m_msInfoFetcher->msInfo() );
        else  // cdrdao seems to write a 150 blocks pregap that is not used by cdrecord
            m_isoImager->setMultiSessionInfo( QString("%1,%2")
                                              .arg(m_msInfoFetcher->lastSessionStart())
                                              .arg(m_msInfoFetcher->nextSessionStart()+150) );

        if( m_doc->onTheFly() ) {
            m_currentAction = PREPARING_DATA;
            m_isoImager->calculateSize();
        }
        else {
            createIsoImage();
        }
    }
    else {
        // the MsInfoFetcher already emitted failure info
        cleanupAfterError();
        jobFinished(false);
    }
}


void K3b::MixedJob::slotIsoImagerFinished( bool success )
{
    if( m_canceled || m_errorOccuredAndAlreadyReported )
        return;

    //
    // Initializing imager before the first copy
    //
    if( m_currentAction == INITIALIZING_IMAGER ) {
        if( success ) {
            m_currentAction = PREPARING_DATA;

            // check the size
            m_projectSize  = m_isoImager->size() + m_doc->audioDoc()->length();
            if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION )
                m_projectSize += 11400; // the session gap

            startFirstCopy();
        }
        else {
            cleanupAfterError();
            jobFinished( false );
        }
    }

    //
    // Recalculated iso image size
    //
    else if( m_currentAction == PREPARING_DATA ) {
        if( success ) {
            // 1. data in first track:
            //    start isoimager and writer
            //    when isoimager finishes start audiodecoder

            // 2. data in last track
            //    start audiodecoder and writer
            //    when audiodecoder finishes start isoimager

            // 3. data in second session
            //    start audiodecoder and writer
            //    start isoimager and writer

            if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION ) {
                m_currentAction = WRITING_ISO_IMAGE;
                if( !prepareWriter() || !startWriting() ) {
                    cleanupAfterError();
                    jobFinished(false);
                }
            }
            else
                writeNextCopy();
        }
        else {
            cleanupAfterError();
            jobFinished( false );
        }
    }

    //
    // Image creation finished
    //
    else {
        if( !success ) {
            emit infoMessage( i18n("Error while creating ISO image."), MessageError );
            cleanupAfterError();

            jobFinished( false );
            return;
        }

        if( m_doc->onTheFly() ) {
            if( m_doc->mixedType() == K3b::MixedDoc::DATA_FIRST_TRACK ) {
                m_currentAction = WRITING_AUDIO_IMAGE;
                m_audioImager->writeTo( m_writer->ioDevice() );
                m_audioImager->start();
            }
        }
        else {
            emit infoMessage( i18n("ISO image successfully created."), MessageSuccess );

            if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION ) {
                m_currentAction = WRITING_ISO_IMAGE;

                if( !prepareWriter() || !startWriting() ) {
                    cleanupAfterError();
                    jobFinished(false);
                }
            }
            else {
                emit newTask( i18n("Creating audio image files") );
                m_currentAction = CREATING_AUDIO_IMAGE;
                m_audioImager->start();
            }
        }
    }
}


void K3b::MixedJob::slotWriterFinished( bool success )
{
    if( m_canceled || m_errorOccuredAndAlreadyReported )
        return;

    if( !success ) {
        cleanupAfterError();
        jobFinished(false);
        return;
    }

    emit burning(false);

    if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION && m_currentAction == WRITING_AUDIO_IMAGE ) {
        // many drives need to reload the medium to return to a proper state
        if ( ( int )m_doc->burner()->readToc().count() < m_doc->numOfTracks()-1 ) {
            emit infoMessage( i18n( "Need to reload medium to return to proper state." ), MessageInfo );
            connect( K3b::Device::reload( m_doc->burner() ),
                     SIGNAL(finished(K3b::Device::DeviceHandler*)),
                     this,
                     SLOT(slotMediaReloadedForSecondSession(K3b::Device::DeviceHandler*)) );
        }
        else {
            startSecondSession();
        }
    }
    else {
        d->copiesDone++;
        if( d->copiesDone < d->copies ) {
            if( !K3b::eject( m_doc->burner() ) ) {
                blockingInformation( i18n("K3b was unable to eject the written disk. Please do so manually.") );
            }
            writeNextCopy();
        }
        else {
            if( !m_doc->onTheFly() && m_doc->removeImages() )
                removeBufferFiles();

            if ( k3bcore->globalSettings()->ejectMedia() ) {
                K3b::Device::eject( m_doc->burner() );
            }

            jobFinished(true);
        }
    }
}


void K3b::MixedJob::slotMediaReloadedForSecondSession( K3b::Device::DeviceHandler* dh )
{
    if( !dh->success() ) {
        blockingInformation( i18n("Please reload the medium and press 'ok'"),
                             i18n("Unable to close the tray") );
    }

    startSecondSession();
}


void K3b::MixedJob::slotAudioDecoderFinished( bool success )
{
    if( m_canceled || m_errorOccuredAndAlreadyReported )
        return;

    if( !success ) {
        emit infoMessage( i18n("Error while decoding audio tracks."), MessageError );
        cleanupAfterError();
        jobFinished(false);
        return;
    }

    if( m_doc->onTheFly() ) {
        if( m_doc->mixedType() == K3b::MixedDoc::DATA_LAST_TRACK ) {
            m_currentAction = WRITING_ISO_IMAGE;
            m_isoImager->start();
            d->pipe.readFrom( m_isoImager->ioDevice() );
            d->pipe.writeTo( m_writer->ioDevice() );
            d->pipe.open();
        }
    }
    else {
        emit infoMessage( i18n("Audio images successfully created."), MessageSuccess );

        if( m_doc->audioDoc()->normalize() ) {
            normalizeFiles();
        }
        else {
            if( m_doc->mixedType() == K3b::MixedDoc::DATA_FIRST_TRACK )
                m_currentAction = WRITING_ISO_IMAGE;
            else
                m_currentAction = WRITING_AUDIO_IMAGE;

            if( !prepareWriter() || !startWriting() ) {
                cleanupAfterError();
                jobFinished(false);
            }
        }
    }
}


void K3b::MixedJob::slotAudioDecoderNextTrack( int t, int tt )
{
    if( m_doc->onlyCreateImages() || !m_doc->onTheFly() ) {
        K3b::AudioTrack* track = m_doc->audioDoc()->getTrack(t);
        emit newSubTask( i18n("Decoding audio track %1 of %2%3",
                              t,
                              tt,
                              ( track->title().isEmpty() || track->artist().isEmpty()
                                ? QString()
                                : " (" + track->artist() + " - " + track->title() + ")" ) ) );
    }
}


bool K3b::MixedJob::prepareWriter()
{
    delete m_writer;
    m_writer = 0;

    if( ( m_currentAction == WRITING_ISO_IMAGE && m_usedDataWritingApp == K3b::WritingAppCdrecord ) ||
        ( m_currentAction == WRITING_AUDIO_IMAGE && m_usedAudioWritingApp == K3b::WritingAppCdrecord ) )  {

        if( !writeInfFiles() ) {
            kDebug() << "(K3b::MixedJob) could not write inf-files.";
            emit infoMessage( i18n("IO Error"), MessageError );

            return false;
        }

        K3b::CdrecordWriter* writer = new K3b::CdrecordWriter( m_doc->burner(), this, this );

        // only write the audio tracks in DAO mode
        if( m_currentAction == WRITING_ISO_IMAGE )
            writer->setWritingMode( m_usedDataWritingMode );
        else
            writer->setWritingMode( m_usedAudioWritingMode );

        writer->setSimulate( m_doc->dummy() );
        writer->setBurnSpeed( m_doc->speed() );

        if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION ) {
            if( m_currentAction == WRITING_ISO_IMAGE ) {
                if( m_doc->onTheFly() )
                    writer->addArgument("-waiti");

                addDataTrack( writer );
            }
            else {
                writer->setMulti( true );
                addAudioTracks( writer );
            }
        }
        else {
            if( m_doc->mixedType() == K3b::MixedDoc::DATA_FIRST_TRACK )
                addDataTrack( writer );
            addAudioTracks( writer );
            if( m_doc->mixedType() == K3b::MixedDoc::DATA_LAST_TRACK )
                addDataTrack( writer );
        }

        m_writer = writer;
    }
    else {
        if( !writeTocFile() ) {
            kDebug() << "(K3b::DataJob) could not write tocfile.";
            emit infoMessage( i18n("IO Error"), MessageError );

            return false;
        }

        // create the writer
        // create cdrdao job
        K3b::CdrdaoWriter* writer = new K3b::CdrdaoWriter( m_doc->burner(), this, this );
        writer->setSimulate( m_doc->dummy() );
        writer->setBurnSpeed( m_doc->speed() );

        // multisession only for the first session
        writer->setMulti( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION
                          && m_currentAction == WRITING_AUDIO_IMAGE );

        writer->setTocFile( m_tocFile->fileName() );

        m_writer = writer;
    }

    connect( m_writer, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_writer, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
    connect( m_writer, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
    connect( m_writer, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
    connect( m_writer, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
    connect( m_writer, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
    connect( m_writer, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
    connect( m_writer, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( m_writer, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)), this, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)) );
    connect( m_writer, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
    //  connect( m_writer, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
    connect( m_writer, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_writer, SIGNAL(debuggingOutput(const QString&, const QString&)),
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

    return true;
}


bool K3b::MixedJob::writeInfFiles()
{
    K3b::InfFileWriter infFileWriter;
    K3b::AudioTrack* track = m_doc->audioDoc()->firstTrack();
    while( track ) {

        infFileWriter.setTrack( track->toCdTrack() );
        infFileWriter.setTrackNumber( track->trackNumber() );
        if( !m_doc->onTheFly() )
            infFileWriter.setBigEndian( false );

        if( !infFileWriter.save( m_tempData->infFileName(track) ) )
            return false;

        track = track->next();
    }
    return true;
}


bool K3b::MixedJob::writeTocFile()
{
    // FIXME: create the tocfile in the same directory like all the other files.

    if( m_tocFile ) delete m_tocFile;
    m_tocFile = new KTemporaryFile();
    m_tocFile->setSuffix( ".toc" );
    m_tocFile->open();

    // write the toc-file
    QTextStream s( m_tocFile );

    K3b::TocFileWriter tocFileWriter;

    //
    // TOC
    //
    tocFileWriter.setData( m_doc->toToc( m_usedDataMode == K3b::DataMode2
                                         ? K3b::Device::Track::XA_FORM1
                                         : K3b::Device::Track::MODE1,
                                         m_doc->onTheFly()
                                         ? m_isoImager->size()
                                         : m_doc->dataDoc()->length() ) );

    //
    // CD-Text
    //
    if( m_doc->audioDoc()->cdText() ) {
        K3b::Device::CdText text = m_doc->audioDoc()->cdTextData();
        // if data in first track we need to add a dummy cdtext
        if( m_doc->mixedType() == K3b::MixedDoc::DATA_FIRST_TRACK )
            text.insert( 0, K3b::Device::TrackCdText() );

        tocFileWriter.setCdText( text );
    }

    //
    // Session to write
    //
    tocFileWriter.setSession( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION &&
                              m_currentAction == WRITING_ISO_IMAGE ? 2 : 1 );

    //
    // image filenames
    //
    if( !m_doc->onTheFly() ) {
        QStringList files;
        K3b::AudioTrack* track = m_doc->audioDoc()->firstTrack();
        while( track ) {
            files += m_tempData->bufferFileName( track );
            track = track->next();
        }
        if( m_doc->mixedType() == K3b::MixedDoc::DATA_FIRST_TRACK )
            files.prepend( m_isoImageFilePath );
        else
            files.append( m_isoImageFilePath );

        tocFileWriter.setFilenames( files );
    }

    bool success = tocFileWriter.save( s );

    m_tocFile->close();

    // backup for debugging
//    KIO::NetAccess::del("/tmp/trueg/tocfile_debug_backup.toc",0L);
//    KIO::NetAccess::copy( m_tocFile->name(), "/tmp/trueg/tocfile_debug_backup.toc",0L );

    return success;
}


void K3b::MixedJob::addAudioTracks( K3b::CdrecordWriter* writer )
{
    writer->addArgument( "-useinfo" );

    // add raw cdtext data
    if( m_doc->audioDoc()->cdText() ) {
        writer->setRawCdText( m_doc->audioDoc()->cdTextData().rawPackData() );
    }

    writer->addArgument( "-audio" );

    // we always pad because although K3b makes sure all tracks' length are multiples of 2352
    // it seems that normalize sometimes corrupts these lengths
    // FIXME: see K3b::AudioJob for the whole less4secs and zeroPregap handling
    writer->addArgument( "-pad" );

    // Allow tracks shorter than 4 seconds
    writer->addArgument( "-shorttrack" );

    // add all the audio tracks
    K3b::AudioTrack* track = m_doc->audioDoc()->firstTrack();
    while( track ) {
        if( m_doc->onTheFly() ) {
            // this is only supported by cdrecord versions >= 2.01a13
            writer->addArgument( QFile::encodeName( m_tempData->infFileName( track ) ) );
        }
        else {
            writer->addArgument( QFile::encodeName( m_tempData->bufferFileName( track ) ) );
        }
        track = track->next();
    }
}

void K3b::MixedJob::addDataTrack( K3b::CdrecordWriter* writer )
{
    // add data track
    if(  m_usedDataMode == K3b::DataMode2 ) {
        if( k3bcore->externalBinManager()->binObject("cdrecord") &&
            k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "xamix" ) )
            writer->addArgument( "-xa" );
        else
            writer->addArgument( "-xa1" );
    }
    else
        writer->addArgument( "-data" );

    if( m_doc->onTheFly() )
        writer->addArgument( QString("-tsize=%1s").arg(m_isoImager->size()) )->addArgument("-");
    else
        writer->addArgument( m_isoImageFilePath );
}


void K3b::MixedJob::slotWriterNextTrack( int t, int )
{
    K3b::AudioTrack* track = 0;

    if( m_doc->mixedType() == K3b::MixedDoc::DATA_FIRST_TRACK ) {
        if( t > 1 )
            track = m_doc->audioDoc()->getTrack(t-1);
    }
    else if( m_doc->mixedType() == K3b::MixedDoc::DATA_LAST_TRACK ) {
        if( t < m_doc->audioDoc()->numOfTracks()+1 )
            track = m_doc->audioDoc()->getTrack(t);
    }
    else if( m_currentAction == WRITING_AUDIO_IMAGE )
        track = m_doc->audioDoc()->getTrack(t);
    else
        t = m_doc->numOfTracks();

    if( track )
        emit newSubTask( i18n("Writing track %1 of %2%3"
                              ,t
                              ,m_doc->numOfTracks()
                              , track->title().isEmpty() || track->artist().isEmpty()
                              ? QString()
                              : " (" + track->artist() + " - " + track->title() + ")" ) );
    else
        emit newSubTask( i18n("Writing track %1 of %2 (%3)",
                              t,
                              m_doc->numOfTracks(),
                              i18n("ISO9660 data")) );
}


void K3b::MixedJob::slotWriterJobPercent( int p )
{
    double totalTasks = d->copies;
    double tasksDone = d->copiesDone;
    if( m_doc->audioDoc()->normalize() ) {
        totalTasks+=1.0;
        tasksDone+=1.0;
    }
    if( !m_doc->onTheFly() ) {
        totalTasks+=1.0;
    }

    if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION ) {
        if( m_currentAction == WRITING_AUDIO_IMAGE ) {
            // the audio imager has finished in all cases
            // the iso imager only if this is not the first copy
            if( d->copiesDone > 0 )
                tasksDone += 1.0;
            else if( !m_doc->onTheFly() )
                tasksDone += m_audioDocPartOfProcess;

            p = (int)((double)p*m_audioDocPartOfProcess);
        }
        else {
            // all images have been created
            if( !m_doc->onTheFly() )
                tasksDone += 1.0;

            p = (int)(100.0*m_audioDocPartOfProcess + (double)p*(1.0-m_audioDocPartOfProcess));
        }
    }
    else if( !m_doc->onTheFly() )
        tasksDone += 1.0;

    emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3b::MixedJob::slotAudioDecoderPercent( int p )
{
    // the only thing finished here might be the isoimager which is part of this task
    if( !m_doc->onTheFly() ) {
        double totalTasks = d->copies+1;
        if( m_doc->audioDoc()->normalize() )
            totalTasks+=1.0;

        if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION )
            p = (int)((double)p*m_audioDocPartOfProcess);
        else
            p = (int)(100.0*(1.0-m_audioDocPartOfProcess) + (double)p*m_audioDocPartOfProcess);

        emit percent( (int)((double)p / totalTasks) );
    }
}


void K3b::MixedJob::slotAudioDecoderSubPercent( int p )
{
    if( !m_doc->onTheFly() ) {
        emit subPercent( p );
    }
}


void K3b::MixedJob::slotIsoImagerPercent( int p )
{
    if( !m_doc->onTheFly() ) {
        emit subPercent( p );
        if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION ) {

            double totalTasks = d->copies+1.0;
            double tasksDone = d->copiesDone;
            if( m_doc->audioDoc()->normalize() ) {
                totalTasks+=1.0;
                // the normalizer finished
                tasksDone+=1.0;
            }

            // the writing of the audio part finished
            tasksDone += m_audioDocPartOfProcess;

            // the audio decoder finished (which is part of this task in terms of progress)
            p = (int)(100.0*m_audioDocPartOfProcess + (double)p*(1.0-m_audioDocPartOfProcess));

            emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
        }
        else {
            double totalTasks = d->copies+1.0;
            if( m_doc->audioDoc()->normalize() )
                totalTasks+=1.0;

            emit percent( (int)((double)(p*(1.0-m_audioDocPartOfProcess)) / totalTasks) );
        }
    }
}


bool K3b::MixedJob::startWriting()
{
    if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION ) {
        if( m_currentAction == WRITING_ISO_IMAGE) {
            if( m_doc->dummy() )
                emit newTask( i18n("Simulating second session") );
            else if( d->copies > 1 )
                emit newTask( i18n("Writing second session of copy %1", d->copiesDone+1) );
            else
                emit newTask( i18n("Writing second session") );
        }
        else {
            if( m_doc->dummy() )
                emit newTask( i18n("Simulating first session") );
            else if( d->copies > 1 )
                emit newTask( i18n("Writing first session of copy %1", d->copiesDone+1) );
            else
                emit newTask( i18n("Writing first session") );
        }
    }
    else if( m_doc->dummy() )
        emit newTask( i18n("Simulating") );
    else
        emit newTask( i18n("Writing Copy %1", d->copiesDone+1) );


    // if we append the second session the cd is already in the drive
    if( !(m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION
          && m_currentAction == WRITING_ISO_IMAGE) ) {

        emit newSubTask( i18n("Waiting for media") );
        if( waitForMedium( m_doc->burner() ) == Device::MEDIA_UNKNOWN ) {
            cancel();
            return false;
        }

        // just to be sure we did not get canceled during the async discWaiting
        if( m_canceled )
            return false;

        // check if the project will fit on the CD
        if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION ) {
            // the media is in and has been checked so this should be fast (hopefully)
            K3b::Msf mediaSize = m_doc->burner()->diskInfo().capacity();
            if( mediaSize < m_projectSize ) {
                if( k3bcore->globalSettings()->overburn() ) {
                    emit infoMessage( i18n("Trying to write more than the official disk capacity"), K3b::Job::MessageWarning );
                }
                else {
                    emit infoMessage( i18n("Data does not fit on disk."), MessageError );
                    return false;
                }
            }
        }
    }

    // in case we determined the max possible writing speed we have to reset the speed on the writer job
    // here since an inserted media is necessary
    // the Max speed job will compare the max speed value with the supported values of the writer
    if( d->maxSpeed )
        m_writer->setBurnSpeed( d->maxSpeedJob->maxSpeed() );

    emit burning(true);
    m_writer->start();

    if( m_doc->onTheFly() ) {
        if ( m_currentAction == WRITING_AUDIO_IMAGE ) {
            // now the writer is running and we can get it's stdin
            // we only use this method when writing on-the-fly since
            // we cannot easily change the audioDecode fd while it's working
            // which we would need to do since we write into several
            // image files.
            m_audioImager->writeTo( m_writer->ioDevice() );
            m_audioImager->start();
        }
        else {
            m_isoImager->start();
            d->pipe.readFrom( m_isoImager->ioDevice() );
            d->pipe.writeTo( m_writer->ioDevice() );
            d->pipe.open();
        }
    }

    return true;
}


void K3b::MixedJob::createIsoImage()
{
    m_currentAction = CREATING_ISO_IMAGE;

    // prepare iso image file
    m_isoImageFilePath = m_tempFilePrefix + "_datatrack.iso";

    if( !m_doc->onTheFly() )
        emit newTask( i18n("Creating ISO image file") );
    emit newSubTask( i18n("Creating ISO image in %1", m_isoImageFilePath) );
    emit infoMessage( i18n("Creating ISO image in %1", m_isoImageFilePath), MessageInfo );

    d->dataImageFile.setName( m_isoImageFilePath );
    if ( d->dataImageFile.open( QIODevice::WriteOnly ) ) {
        m_isoImager->start();
        d->pipe.readFrom( m_isoImager->ioDevice() );
        d->pipe.writeTo( &d->dataImageFile, true );
        d->pipe.open( true );
    }
    else {
        emit infoMessage( i18n("Could not open %1 for writing", m_isoImageFilePath ), MessageError );
        cleanupAfterError();
        jobFinished(false);
    }
}


void K3b::MixedJob::cleanupAfterError()
{
    m_errorOccuredAndAlreadyReported = true;
    //  m_audioImager->cancel();
    m_isoImager->cancel();
    if( m_writer && m_writer->active() )
        m_writer->cancel();

    delete m_tocFile;
    m_tocFile = 0;

    // remove the temp files
    removeBufferFiles();

#ifdef __GNUC__
#warning FIXME: eject medium if necessary after cleanupAfterError
#endif
}


void K3b::MixedJob::removeBufferFiles()
{
    if ( !m_doc->onTheFly() ) {
        emit infoMessage( i18n("Removing buffer files."), MessageInfo );
    }

    if( QFile::exists( m_isoImageFilePath ) )
        if( !QFile::remove( m_isoImageFilePath ) )
            emit infoMessage( i18n("Could not delete file %1.",m_isoImageFilePath), MessageError );

    // removes buffer images and temp toc or inf files
    m_tempData->cleanup();
}


void K3b::MixedJob::determineWritingMode()
{
    // we don't need this when only creating image and it is possible
    // that the burn device is null
    if( m_doc->onlyCreateImages() )
        return;

    // at first we determine the data mode
    // --------------------------------------------------------------
    if( m_doc->dataDoc()->dataMode() == K3b::DataModeAuto ) {
        if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION )
            m_usedDataMode = K3b::DataMode2;
        else
            m_usedDataMode = K3b::DataMode1;
    }
    else
        m_usedDataMode = m_doc->dataDoc()->dataMode();


    // we try to use cdrecord if possible
    bool cdrecordOnTheFly = false;
    bool cdrecordCdText = false;
    bool cdrecordUsable = false;

    if( k3bcore->externalBinManager()->binObject("cdrecord") ) {
        cdrecordOnTheFly =
            k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "audio-stdin" );
        cdrecordCdText =
            k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "cdtext" );
        cdrecordUsable =
            !( !cdrecordOnTheFly && m_doc->onTheFly() ) &&
            !( m_doc->audioDoc()->cdText() && !cdrecordCdText );
    }

    // Writing Application
    // --------------------------------------------------------------
    // cdrecord seems to have problems writing xa 1 disks in dao mode? At least on my system!
    if( writingApp() == K3b::WritingAppAuto ) {
        if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION ) {
            if( m_doc->writingMode() == K3b::WritingModeSao ||
                ( m_doc->writingMode() == K3b::WritingModeAuto && !cdrecordUsable ) ) {
                m_usedAudioWritingApp = K3b::WritingAppCdrdao;
                m_usedDataWritingApp = K3b::WritingAppCdrdao;
            }
            else {
                m_usedAudioWritingApp = K3b::WritingAppCdrecord;
                m_usedDataWritingApp = K3b::WritingAppCdrecord;
            }
        }
        else {
            if( cdrecordUsable ) {
                m_usedAudioWritingApp = K3b::WritingAppCdrecord;
                m_usedDataWritingApp = K3b::WritingAppCdrecord;
            }
            else {
                m_usedAudioWritingApp = K3b::WritingAppCdrdao;
                m_usedDataWritingApp = K3b::WritingAppCdrdao;
            }
        }
    }
    else {
        m_usedAudioWritingApp = writingApp();
        m_usedDataWritingApp = writingApp();
    }

    // TODO: use K3b::Exceptions::brokenDaoAudio

    // Writing Mode (TAO/DAO/RAW)
    // --------------------------------------------------------------
    if( m_doc->writingMode() == K3b::WritingModeAuto ) {

        if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION ) {
            if( m_usedDataWritingApp == K3b::WritingAppCdrecord )
                m_usedDataWritingMode = K3b::WritingModeTao;
            else
                m_usedDataWritingMode = K3b::WritingModeSao;

            // default to Session at once for the audio part
            m_usedAudioWritingMode = K3b::WritingModeSao;
        }
        else {
            m_usedDataWritingMode = K3b::WritingModeTao;
            m_usedAudioWritingMode = K3b::WritingModeTao;
        }
    }
    else {
        m_usedAudioWritingMode = m_doc->writingMode();
        m_usedDataWritingMode = m_doc->writingMode();
    }


    if( m_usedDataWritingApp == K3b::WritingAppCdrecord ) {
        if( !cdrecordOnTheFly && m_doc->onTheFly() ) {
            m_doc->setOnTheFly( false );
            emit infoMessage( i18n("On-the-fly writing with cdrecord < 2.01a13 not supported."), MessageError );
        }

        if( m_doc->audioDoc()->cdText() ) {
            if( !cdrecordCdText ) {
                m_doc->audioDoc()->writeCdText( false );
                emit infoMessage( i18n("Cdrecord %1 does not support CD-Text writing.",k3bcore->externalBinManager()->binObject("cdrecord")->version()), MessageError );
            }
            else if( m_usedAudioWritingMode == K3b::WritingModeTao ) {
                emit infoMessage( i18n("It is not possible to write CD-Text in TAO mode. Try DAO or RAW."), MessageWarning );
            }
        }
    }
}


void K3b::MixedJob::normalizeFiles()
{
    if( !m_normalizeJob ) {
        m_normalizeJob = new K3b::AudioNormalizeJob( this, this );

        connect( m_normalizeJob, SIGNAL(infoMessage(const QString&, int)),
                 this, SIGNAL(infoMessage(const QString&, int)) );
        connect( m_normalizeJob, SIGNAL(percent(int)), this, SLOT(slotNormalizeProgress(int)) );
        connect( m_normalizeJob, SIGNAL(subPercent(int)), this, SLOT(slotNormalizeSubProgress(int)) );
        connect( m_normalizeJob, SIGNAL(finished(bool)), this, SLOT(slotNormalizeJobFinished(bool)) );
        connect( m_normalizeJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
        connect( m_normalizeJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
                 this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
    }

    // add all the files
    QList<QString> files;
    K3b::AudioTrack* track = m_doc->audioDoc()->firstTrack();
    while( track ) {
        files.append( m_tempData->bufferFileName(track) );
        track = track->next();
    }

    m_normalizeJob->setFilesToNormalize( files );

    emit newTask( i18n("Normalizing volume levels") );
    m_normalizeJob->start();
}

void K3b::MixedJob::slotNormalizeJobFinished( bool success )
{
    if( m_canceled || m_errorOccuredAndAlreadyReported )
        return;

    if( success ) {
        if( m_doc->mixedType() == K3b::MixedDoc::DATA_FIRST_TRACK )
            m_currentAction = WRITING_ISO_IMAGE;
        else
            m_currentAction = WRITING_AUDIO_IMAGE;

        if( !prepareWriter() || !startWriting() ) {
            cleanupAfterError();
            jobFinished(false);
        }
    }
    else {
        cleanupAfterError();
        jobFinished(false);
    }
}

void K3b::MixedJob::slotNormalizeProgress( int p )
{
    double totalTasks = d->copies+2.0;
    double tasksDone = 0;

    if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION ) {
        // the audio imager finished (m_audioDocPartOfProcess*1 task)
        // plus the normalize progress
        tasksDone = m_audioDocPartOfProcess;
    }
    else {
        // the iso and audio imagers already finished (one task)
        // plus the normalize progress
        tasksDone = 1.0;
    }

    emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3b::MixedJob::slotNormalizeSubProgress( int p )
{
    emit subPercent( p );
}


void K3b::MixedJob::prepareProgressInformation()
{
    // calculate percentage of audio and data
    // this is also used in on-the-fly mode
    double ds = (double)m_doc->dataDoc()->length().totalFrames();
    double as = (double)m_doc->audioDoc()->length().totalFrames();
    m_audioDocPartOfProcess = as/(ds+as);
}


QString K3b::MixedJob::jobDescription() const
{
    if( m_doc->mixedType() == K3b::MixedDoc::DATA_SECOND_SESSION )
        return i18n("Writing Enhanced Audio CD")
            + ( m_doc->audioDoc()->title().isEmpty()
                ? QString()
                : QString( " (%1)" ).arg(m_doc->audioDoc()->title()) );
    else
        return i18n("Writing Mixed Mode CD")
            + ( m_doc->audioDoc()->title().isEmpty()
                ? QString()
                : QString( " (%1)" ).arg(m_doc->audioDoc()->title()) );
}


QString K3b::MixedJob::jobDetails() const
{
    return ( i18ncp("%2 is of form XX:YY:ZZ, no pluralization needed"
                  ,"1 track (%2 minutes audio data, %3 ISO9660 data)"
                  ,"%1 tracks (%2 minutes audio data, %3 ISO9660 data)"
                  ,m_doc->numOfTracks()
                  ,m_doc->audioDoc()->length().toString()
                  ,KIO::convertSize(m_doc->dataDoc()->size()))
             + ( m_doc->copies() > 1 && !m_doc->dummy()
                 ? i18np(" - %1 copy", " - %1 copies", m_doc->copies())
                 : QString() ) );
}

#include "k3bmixedjob.moc"
