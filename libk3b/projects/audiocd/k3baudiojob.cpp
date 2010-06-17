/*
 *
 * Copyright (C) 2003-2010 Sebastian Trueg <trueg@k3b.org>
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


#include "k3baudiojob.h"

#include "k3baudioimager.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"
#include "k3baudionormalizejob.h"
#include "k3baudiojobtempdata.h"
#include "k3baudiomaxspeedjob.h"
#include "k3baudiocdtracksource.h"
#include "k3baudiofile.h"
#include "k3bdevicemanager.h"
#include "k3bdevicehandler.h"
#include "k3bdevice.h"
#include "k3bcdtext.h"
#include "k3bmsf.h"
#include "k3bglobals.h"
#include "k3bexternalbinmanager.h"
#include "k3bcore.h"
#include "k3bcdrecordwriter.h"
#include "k3bcdrdaowriter.h"
#include "k3btocfilewriter.h"
#include "k3binffilewriter.h"
#include "k3bglobalsettings.h"

#include <qfile.h>

#include <kdebug.h>
#include <klocale.h>
#include <ktemporaryfile.h>
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



class K3b::AudioJob::Private
{
public:
    Private()
        : copies(1),
          copiesDone(0) {
    }

    int copies;
    int copiesDone;
    int usedSpeed;

    bool useCdText;
    bool maxSpeed;

    bool zeroPregap;
    bool less4Sec;
};


K3b::AudioJob::AudioJob( K3b::AudioDoc* doc, K3b::JobHandler* hdl, QObject* parent )
    : K3b::BurnJob( hdl, parent ),
      m_doc( doc ),
      m_normalizeJob(0),
      m_maxSpeedJob(0)
{
    d = new Private;

    m_tempData = new K3b::AudioJobTempData( m_doc, this );
    m_audioImager = new K3b::AudioImager( m_doc, m_tempData, this, this );
    connect( m_audioImager, SIGNAL(infoMessage(const QString&, int)),
             this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_audioImager, SIGNAL(percent(int)),
             this, SLOT(slotAudioDecoderPercent(int)) );
    connect( m_audioImager, SIGNAL(subPercent(int)),
             this, SLOT(slotAudioDecoderSubPercent(int)) );
    connect( m_audioImager, SIGNAL(finished(bool)),
             this, SLOT(slotAudioDecoderFinished(bool)) );
    connect( m_audioImager, SIGNAL(nextTrack(int, int)),
             this, SLOT(slotAudioDecoderNextTrack(int, int)) );

    m_writer = 0;
}


K3b::AudioJob::~AudioJob()
{
    delete d;
}


K3b::Device::Device* K3b::AudioJob::writer() const
{
    if( m_doc->onlyCreateImages() )
        return 0; // no writer needed -> no blocking on K3b::BurnJob
    else
        return m_doc->burner();
}


K3b::Doc* K3b::AudioJob::doc() const
{
    return m_doc;
}


void K3b::AudioJob::start()
{
    jobStarted();

    m_written = true;
    m_canceled = false;
    m_errorOccuredAndAlreadyReported = false;
    d->copies = m_doc->copies();
    d->copiesDone = 0;
    d->useCdText = m_doc->cdText();
    d->usedSpeed = m_doc->speed();
    d->maxSpeed = false;

    if( m_doc->dummy() )
        d->copies = 1;

    emit newTask( i18n("Preparing data") );

    //
    // Check if all files exist
    //
    QList<K3b::AudioFile*> nonExistingFiles;
    K3b::AudioTrack* track = m_doc->firstTrack();
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
    if( m_doc->numOfTracks() == 0 ) {
        emit infoMessage( i18n("Please add files to your project first."), MessageError );
        jobFinished(false);
        return;
    }

    if( m_doc->onTheFly() && !checkAudioSources() ) {
        emit infoMessage( i18n("Unable to write on-the-fly with these audio sources."), MessageWarning );
        m_doc->setOnTheFly(false);
    }


    // we don't need this when only creating image and it is possible
    // that the burn device is null
    if( !m_doc->onlyCreateImages() ) {

        //
        // there are a lot of writers out there which produce coasters
        // in dao mode if the CD contains pregaps of length 0 (or maybe already != 2 secs?)
        //
        // Also most writers do not accept cuesheets with tracks smaller than 4 seconds (a violation
        // of the red book standard) in DAO mode.
        //
        d->zeroPregap = false;
        d->less4Sec = false;
        track = m_doc->firstTrack();
        while( track ) {
            if( track->postGap() == 0 && track->next() != 0 ) // the last track's postgap is always 0
                d->zeroPregap = true;

            if( track->length() < K3b::Msf( 0, 4, 0 ) )
                d->less4Sec = true;

            track = track->next();
        }

        // determine writing mode
        if( m_doc->writingMode() == K3b::WritingModeAuto ) {
            //
            // DAO is always the first choice
            // RAW second and TAO last
            // there are none-DAO writers that are supported by cdrdao
            //
            // older cdrecord versions do not support the -shorttrack option in RAW writing mode
            //
            if( !writer()->dao() && writingApp() == K3b::WritingAppCdrecord ) {
                if(!writer()->supportsRawWriting() &&
                   ( !d->less4Sec || k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "short-track-raw" ) ) )
                    m_usedWritingMode = K3b::WritingModeRaw;
                else
                    m_usedWritingMode = K3b::WritingModeTao;
            }
            else {
                if( (d->zeroPregap||d->less4Sec) && writer()->supportsRawWriting() ) {
                    m_usedWritingMode = K3b::WritingModeRaw;
                    if( d->less4Sec )
                        emit infoMessage( i18n("Track lengths below 4 seconds violate the Red Book standard."), MessageWarning );
                }
                else
                    m_usedWritingMode = K3b::WritingModeSao;
            }
        }
        else
            m_usedWritingMode = m_doc->writingMode();

        bool cdrecordOnTheFly = false;
        bool cdrecordCdText = false;
        if( k3bcore->externalBinManager()->binObject("cdrecord") ) {
            cdrecordOnTheFly = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "audio-stdin" );
            cdrecordCdText = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "cdtext" );
        }

        // determine writing app
        if( writingApp() == K3b::WritingAppAuto ) {
            if( m_usedWritingMode == K3b::WritingModeSao ) {
                // there are none-DAO writers that are supported by cdrdao
                if( !writer()->dao() ||
                    ( !cdrecordOnTheFly && m_doc->onTheFly() ) ||
                    ( d->useCdText && !cdrecordCdText ) ||
                    m_doc->hideFirstTrack() )
                    m_usedWritingApp = K3b::WritingAppCdrdao;
                else
                    m_usedWritingApp = K3b::WritingAppCdrecord;
            }
            else
                m_usedWritingApp = K3b::WritingAppCdrecord;
        }
        else
            m_usedWritingApp = writingApp();

        // on-the-fly writing with cdrecord >= 2.01a13
        if( m_usedWritingApp == K3b::WritingAppCdrecord &&
            m_doc->onTheFly() &&
            !cdrecordOnTheFly ) {
            emit infoMessage( i18n("On-the-fly writing with cdrecord < 2.01a13 not supported."), MessageError );
            m_doc->setOnTheFly(false);
        }

        if( m_usedWritingApp == K3b::WritingAppCdrecord &&
            d->useCdText ) {
            if( !cdrecordCdText ) {
                emit infoMessage( i18n("Cdrecord %1 does not support CD-Text writing.",
                                       k3bcore->externalBinManager()->binObject("cdrecord")->version()), MessageError );
                d->useCdText = false;
            }
            else if( m_usedWritingMode == K3b::WritingModeTao ) {
                emit infoMessage( i18n("It is not possible to write CD-Text in TAO mode."), MessageWarning );
                d->useCdText = false;
            }
        }
    }


    if( !m_doc->onlyCreateImages() && m_doc->onTheFly() ) {
        if( m_doc->speed() == 0 ) {
            // try to determine the max possible speed
            emit newSubTask( i18n("Determining maximum writing speed") );
            if( !m_maxSpeedJob ) {
                m_maxSpeedJob = new K3b::AudioMaxSpeedJob( m_doc, this, this );
                connect( m_maxSpeedJob, SIGNAL(percent(int)),
                         this, SIGNAL(subPercent(int)) );
                connect( m_maxSpeedJob, SIGNAL(finished(bool)),
                         this, SLOT(slotMaxSpeedJobFinished(bool)) );
            }
            m_maxSpeedJob->start();
            return;
        }
        else {
            if( !prepareWriter() ) {
                cleanupAfterError();
                jobFinished(false);
                return;
            }

            if( startWriting() ) {

                // now the writer is running and we can get it's stdin
                // we only use this method when writing on-the-fly since
                // we cannot easily change the audioDecode device while it's working
                // which we would need to do since we write into several
                // image files.
                m_audioImager->writeTo( m_writer->ioDevice() );
            }
            else {
                // startWriting() already did the cleanup
                return;
            }
        }
    }
    else {
        emit burning(false);
        emit infoMessage( i18n("Creating image files in %1", m_doc->tempDir()), MessageInfo );
        emit newTask( i18n("Creating image files") );
        m_tempData->prepareTempFileNames( doc()->tempDir() );
    }

    m_audioImager->start();
}


void K3b::AudioJob::slotMaxSpeedJobFinished( bool success )
{
    d->maxSpeed = success;
    if( !success )
        emit infoMessage( i18n("Unable to determine maximum speed for some reason. Ignoring."), MessageWarning );

    // now start the writing
    // same code as above. See the commecnts there
    if( !prepareWriter() ) {
        cleanupAfterError();
        jobFinished(false);
        return;
    }

    if( startWriting() )
        m_audioImager->writeTo( m_writer->ioDevice() );

    m_audioImager->start();
}


void K3b::AudioJob::cancel()
{
    m_canceled = true;

    if( m_maxSpeedJob )
        m_maxSpeedJob->cancel();

    if( m_writer )
        m_writer->cancel();

    m_audioImager->cancel();
    emit infoMessage( i18n("Writing canceled."), K3b::Job::MessageError );
    removeBufferFiles();
    emit canceled();
    jobFinished(false);
}


void K3b::AudioJob::slotWriterFinished( bool success )
{
    if( m_canceled || m_errorOccuredAndAlreadyReported )
        return;

    if( !success ) {
        cleanupAfterError();
        jobFinished(false);
        return;
    }
    else {
        d->copiesDone++;

        if( d->copiesDone == d->copies ) {
            if( m_doc->onTheFly() || m_doc->removeImages() )
                removeBufferFiles();

            if ( k3bcore->globalSettings()->ejectMedia() ) {
                K3b::Device::eject( m_doc->burner() );
            }

            jobFinished(true);
        }
        else {
            if( !K3b::eject( m_doc->burner() ) ) {
                blockingInformation( i18n("K3b was unable to eject the written disk. Please do so manually.") );
            }

            if( startWriting() ) {
                if( m_doc->onTheFly() ) {
                    // now the writer is running and we can get it's stdin
                    // we only use this method when writing on-the-fly since
                    // we cannot easily change the audioDecode fd while it's working
                    // which we would need to do since we write into several
                    // image files.
                    m_audioImager->writeTo( m_writer->ioDevice() );
                    m_audioImager->start();
                }
            }
        }
    }
}


void K3b::AudioJob::slotAudioDecoderFinished( bool success )
{
    if( m_canceled || m_errorOccuredAndAlreadyReported )
        return;

    if( !success ) {
        if( m_audioImager->lastErrorType() == K3b::AudioImager::ERROR_FD_WRITE ) {
            // this means that the writer job failed so let's use the error handling there.
            return;
        }

        emit infoMessage( i18n("Error while decoding audio tracks."), MessageError );
        cleanupAfterError();
        jobFinished(false);
        return;
    }

    if( m_doc->onlyCreateImages() || !m_doc->onTheFly() ) {

        emit infoMessage( i18n("Successfully decoded all tracks."), MessageSuccess );

        if( m_doc->normalize() ) {
            normalizeFiles();
        }
        else if( !m_doc->onlyCreateImages() ) {
            if( !prepareWriter() ) {
                cleanupAfterError();
                jobFinished(false);
            }
            else
                startWriting();
        }
        else {
            jobFinished(true);
        }
    }
}


void K3b::AudioJob::slotAudioDecoderNextTrack( int t, int tt )
{
    if( m_doc->onlyCreateImages() || !m_doc->onTheFly() ) {
        K3b::AudioTrack* track = m_doc->getTrack(t);
        emit newSubTask( i18n("Decoding audio track %1 of %2%3",
                              t,
                              tt,
                              track->title().isEmpty() || track->artist().isEmpty()
                              ? QString()
                              : " (" + track->artist() + " - " + track->title() + ")" ) );
    }
}


bool K3b::AudioJob::prepareWriter()
{
    delete m_writer;

    if( m_usedWritingApp == K3b::WritingAppCdrecord ) {

        if( !writeInfFiles() ) {
            kDebug() << "(K3b::AudioJob) could not write inf-files.";
            emit infoMessage( i18n("IO Error. Most likely no space left on harddisk."), MessageError );

            return false;
        }

        K3b::CdrecordWriter* writer = new K3b::CdrecordWriter( m_doc->burner(), this, this );

        writer->setWritingMode( m_usedWritingMode );
        writer->setSimulate( m_doc->dummy() );
        writer->setBurnSpeed( d->usedSpeed );

        writer->addArgument( "-useinfo" );

        if( d->useCdText ) {
            writer->setRawCdText( m_doc->cdTextData().rawPackData() );
        }

        // add all the audio tracks
        writer->addArgument( "-audio" );

        // we only need to pad in one case. cdrecord < 2.01.01a03 cannot handle shorttrack + raw
        if( d->less4Sec ) {
            if( m_usedWritingMode == K3b::WritingModeRaw &&
                !k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "short-track-raw" ) ) {
                writer->addArgument( "-pad" );
            }
            else {
                // Allow tracks shorter than 4 seconds
                writer->addArgument( "-shorttrack" );
            }
        }

        K3b::AudioTrack* track = m_doc->firstTrack();
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
        writer->setCommand( K3b::CdrdaoWriter::WRITE );
        writer->setSimulate( m_doc->dummy() );
        writer->setBurnSpeed( d->usedSpeed );
        writer->setTocFile( m_tempData->tocFileName() );

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


void K3b::AudioJob::slotWriterNextTrack( int t, int tt )
{
    K3b::AudioTrack* track = m_doc->getTrack(t);
    // t is in range 1..tt
    if( m_doc->hideFirstTrack() )
        track = m_doc->getTrack(t+1);
    emit newSubTask( i18n("Writing track %1 of %2%3",
                          t,
                          tt,
                          track->title().isEmpty() || track->artist().isEmpty()
                          ? QString()
                          : " (" + track->artist() + " - " + track->title() + ")" ) );
}


void K3b::AudioJob::slotWriterJobPercent( int p )
{
    double totalTasks = d->copies;
    double tasksDone = d->copiesDone;
    if( m_doc->normalize() ) {
        totalTasks+=1.0;
        tasksDone+=1.0;
    }
    if( !m_doc->onTheFly() ) {
        totalTasks+=1.0;
        tasksDone+=1.0;
    }

    emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3b::AudioJob::slotAudioDecoderPercent( int p )
{
    if( m_doc->onlyCreateImages() ) {
        if( m_doc->normalize() )
            emit percent( p/2 );
        else
            emit percent( p );
    }
    else if( !m_doc->onTheFly() ) {
        double totalTasks = d->copies;
        double tasksDone = d->copiesDone; // =0 when creating an image
        if( m_doc->normalize() ) {
            totalTasks+=1.0;
        }
        if( !m_doc->onTheFly() ) {
            totalTasks+=1.0;
        }

        emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
    }
}


void K3b::AudioJob::slotAudioDecoderSubPercent( int p )
{
    // when writing on the fly the writer produces the subPercent
    if( m_doc->onlyCreateImages() || !m_doc->onTheFly() ) {
        emit subPercent( p );
    }
}


bool K3b::AudioJob::startWriting()
{
    if( m_doc->dummy() )
        emit newTask( i18n("Simulating") );
    else if( d->copies > 1 )
        emit newTask( i18n("Writing Copy %1", d->copiesDone+1) );
    else
        emit newTask( i18n("Writing") );


    emit newSubTask( i18n("Waiting for media") );
    if( waitForMedium( m_doc->burner() ) == Device::MEDIA_UNKNOWN ) {
        cancel();
        return false;
    }

    // just to be sure we did not get canceled during the async discWaiting
    if( m_canceled )
        return false;

    // in case we determined the max possible writing speed we have to reset the speed on the writer job
    // here since an inserted media is necessary
    // the Max speed job will compare the max speed value with the supported values of the writer
    if( d->maxSpeed )
        m_writer->setBurnSpeed( m_maxSpeedJob->maxSpeed() );

    emit burning(true);
    m_writer->start();
    return true;
}


void K3b::AudioJob::cleanupAfterError()
{
    m_errorOccuredAndAlreadyReported = true;
    m_audioImager->cancel();

    if( m_writer )
        m_writer->cancel();

    // remove the temp files
    removeBufferFiles();
}


void K3b::AudioJob::removeBufferFiles()
{
    if ( !m_doc->onTheFly() ) {
        emit infoMessage( i18n("Removing temporary files."), MessageInfo );
    }

    // removes buffer images and temp toc or inf files
    m_tempData->cleanup();
}


void K3b::AudioJob::normalizeFiles()
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
    // TODO: we may need to split the wave files and put them back together!
    QList<QString> files;
    K3b::AudioTrack* track = m_doc->firstTrack();
    while( track ) {
        files.append( m_tempData->bufferFileName(track) );
        track = track->next();
    }

    m_normalizeJob->setFilesToNormalize( files );

    emit newTask( i18n("Normalizing volume levels") );
    m_normalizeJob->start();
}

void K3b::AudioJob::slotNormalizeJobFinished( bool success )
{
    if( m_canceled || m_errorOccuredAndAlreadyReported )
        return;

    if( success ) {
        if( m_doc->onlyCreateImages() ) {
            jobFinished(true);
        }
        else {
            // start the writing
            if( !prepareWriter() ) {
                cleanupAfterError();
                jobFinished(false);
            }
            else
                startWriting();
        }
    }
    else {
        cleanupAfterError();
        jobFinished(false);
    }
}

void K3b::AudioJob::slotNormalizeProgress( int p )
{
    double totalTasks = d->copies+2.0;
    double tasksDone = 1; // the decoding has been finished

    emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3b::AudioJob::slotNormalizeSubProgress( int p )
{
    emit subPercent( p );
}


bool K3b::AudioJob::writeTocFile()
{
    K3b::TocFileWriter tocWriter;
    tocWriter.setData( m_doc->toToc() );
    tocWriter.setHideFirstTrack( m_doc->hideFirstTrack() );
    if( d->useCdText )
        tocWriter.setCdText( m_doc->cdTextData() );
    if( !m_doc->onTheFly() ) {
        QStringList filenames;
        for( int i = 1; i <= m_doc->numOfTracks(); ++i )
            filenames += m_tempData->bufferFileName( i );
        tocWriter.setFilenames( filenames );
    }
    return tocWriter.save( m_tempData->tocFileName() );
}


bool K3b::AudioJob::writeInfFiles()
{
    K3b::InfFileWriter infFileWriter;
    K3b::AudioTrack* track = m_doc->firstTrack();
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


// checks if the doc contains sources from an audio cd which cannot be read on-the-fly
bool K3b::AudioJob::checkAudioSources()
{
    K3b::AudioTrack* track = m_doc->firstTrack();
    K3b::AudioDataSource* source = track->firstSource();

    while( source ) {

        if( K3b::AudioCdTrackSource* cdSource = dynamic_cast<K3b::AudioCdTrackSource*>(source) ) {
            //
            // If which cases we cannot wite on-the-fly:
            // 1. the writing device contains one of the audio cds
            // 2. Well, one of the cds is missing
            //
            K3b::Device::Device* dev = cdSource->searchForAudioCD();
            if( !dev || dev == writer() )
                return false;
            else
                cdSource->setDevice( dev );
        }

        // next source
        source = source->next();
        if( !source ) {
            track = track->next();
            if( track )
                source = track->firstSource();
        }
    }

    return true;
}


QString K3b::AudioJob::jobDescription() const
{
    return i18n("Writing Audio CD")
        + ( m_doc->title().isEmpty()
            ? QString()
            : QString( " (%1)" ).arg(m_doc->title()) );
}


QString K3b::AudioJob::jobDetails() const
{
    return ( i18ncp("%2 is of form XX:YY:ZZ, no pluralization needed",
                    "1 track (%2 minutes)",
                    "%1 tracks (%2 minutes)",
                    m_doc->numOfTracks(), m_doc->length().toString())
             + ( m_doc->copies() > 1 && !m_doc->dummy()
                 ? i18np(" - %1 copy", " - %1 copies", m_doc->copies())
                 : QString() ) );
}

#include "k3baudiojob.moc"
