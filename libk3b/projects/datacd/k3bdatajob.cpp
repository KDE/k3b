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


#include "k3bdatajob.h"
#include "k3bdatadoc.h"
#include "k3bisoimager.h"
#include "k3bdatamultisessionparameterjob.h"
#include "k3bchecksumpipe.h"
#include "k3bcore.h"
#include "k3bglobals.h"
#include "k3bversion.h"
#include "k3bdevice.h"
#include "k3bdevicehandler.h"
#include "k3btoc.h"
#include "k3btrack.h"
#include "k3bdevicehandler.h"
#include "k3bexternalbinmanager.h"
#include "k3bcdrecordwriter.h"
#include "k3bcdrdaowriter.h"
#include "k3bglobalsettings.h"
#include "k3bactivepipe.h"
#include "k3bfilesplitter.h"
#include "k3bverificationjob.h"
#include "k3biso9660.h"
#include "k3bisooptions.h"
#include "k3bdeviceglobals.h"
#include "k3bgrowisofswriter.h"

#include <kapplication.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <kio/global.h>
#include <kio/job.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qdatastream.h>
#include <kdebug.h>



class K3b::DataJob::Private
{
public:
    Private()
        : usedWritingApp(K3b::WritingAppAuto),
          verificationJob( 0 ),
          pipe( 0 ) {
    }

    K3b::DataDoc* doc;

    bool initializingImager;
    bool imageFinished;
    bool canceled;

    KTemporaryFile* tocFile;

    int usedDataMode;
    K3b::WritingApp usedWritingApp;
    K3b::WritingMode usedWritingMode;

    int copies;
    int copiesDone;

    K3b::VerificationJob* verificationJob;

    K3b::FileSplitter imageFile;
    K3b::ActivePipe* pipe;

    K3b::DataMultiSessionParameterJob* multiSessionParameterJob;

    QByteArray checksumCache;
};


K3b::DataJob::DataJob( K3b::DataDoc* doc, K3b::JobHandler* hdl, QObject* parent )
    : K3b::BurnJob( hdl, parent )
{
    d = new Private;
    d->multiSessionParameterJob = new K3b::DataMultiSessionParameterJob( doc, this, this );
    connectSubJob( d->multiSessionParameterJob,
                   SLOT( slotMultiSessionParamterSetupDone( bool ) ),
                   SIGNAL( newTask( const QString& ) ),
                   SIGNAL( newSubTask( const QString& ) ) );

    d->doc = doc;
    m_writerJob = 0;
    d->tocFile = 0;
    m_isoImager = 0;
}


K3b::DataJob::~DataJob()
{
    kDebug();
    delete d->pipe;
    delete d->tocFile;
    delete d;
}


K3b::Doc* K3b::DataJob::doc() const
{
    return d->doc;
}


K3b::Device::Device* K3b::DataJob::writer() const
{
    if( doc()->onlyCreateImages() )
        return 0; // no writer needed -> no blocking on K3b::BurnJob
    else
        return doc()->burner();
}


void K3b::DataJob::start()
{
    kDebug();
    jobStarted();

    d->canceled = false;
    d->imageFinished = false;
    d->copies = d->doc->copies();
    d->copiesDone = 0;

    prepareImager();

    if( d->doc->dummy() ) {
        d->doc->setVerifyData( false );
        d->copies = 1;
    }

    emit newTask( i18n("Preparing data") );

    // there is no harm in setting these even if we write on-the-fly
    d->imageFile.setName( d->doc->tempDir() );

    d->multiSessionParameterJob->start();
}


void K3b::DataJob::slotMultiSessionParamterSetupDone( bool success )
{
    kDebug() << success;
    if ( success ) {
        prepareWriting();
    }
    else {
        if ( d->multiSessionParameterJob->hasBeenCanceled() ) {
            emit canceled();
        }
        cleanup();
        jobFinished( false );
    }
}


void K3b::DataJob::prepareWriting()
{
    kDebug();
    if( !d->doc->onlyCreateImages() &&
        ( d->multiSessionParameterJob->usedMultiSessionMode() == K3b::DataDoc::CONTINUE ||
          d->multiSessionParameterJob->usedMultiSessionMode() == K3b::DataDoc::FINISH ) ) {
        unsigned int nextSessionStart = d->multiSessionParameterJob->nextSessionStart();
        // for some reason cdrdao needs 150 additional sectors in the ms info
        if( writingApp() == K3b::WritingAppCdrdao ) {
            nextSessionStart += 150;
        }
        m_isoImager->setMultiSessionInfo( QString().sprintf( "%u,%u",
                                                             d->multiSessionParameterJob->previousSessionStart(),
                                                             nextSessionStart ),
                                          d->multiSessionParameterJob->importPreviousSession() ? d->doc->burner() : 0 );
    }
    else {
        m_isoImager->setMultiSessionInfo( QString(), 0 );
    }

    d->initializingImager = true;
    m_isoImager->init();
}


void K3b::DataJob::writeImage()
{
    kDebug();
    d->initializingImager = false;

    emit burning(false);

    // get image file path
    if( d->doc->tempDir().isEmpty() )
        d->doc->setTempDir( K3b::findUniqueFilePrefix( d->doc->isoOptions().volumeID() ) + ".iso" );

    // TODO: check if the image file is part of the project and if so warn the user
    //       and append some number to make the path unique.

    //
    // Check the image file
    if( !d->doc->onTheFly() || d->doc->onlyCreateImages() ) {
        d->imageFile.setName( d->doc->tempDir() );
        if( !d->imageFile.open( QIODevice::WriteOnly ) ) {
            emit infoMessage( i18n("Could not open %1 for writing", d->doc->tempDir() ), MessageError );
            cleanup();
            jobFinished(false);
            return;
        }
    }

    emit newTask( i18n("Creating image file") );
    emit newSubTask( i18n("Track 1 of 1") );
    emit infoMessage( i18n("Creating image file in %1",d->doc->tempDir()), MessageInfo );

    m_isoImager->start();
    startPipe();
}


void K3b::DataJob::startPipe()
{
    kDebug();
    //
    // Open the active pipe which does the streaming
    //
    delete d->pipe;
    if ( d->imageFinished || !d->doc->verifyData() )
        d->pipe = new K3b::ActivePipe();
    else
        d->pipe = new K3b::ChecksumPipe();

#ifdef __GNUC__
#warning Growisofs needs stdin to be closed in order to exit gracefully. Cdrecord does not. However,  if closed with cdrecord we loose parts of stderr. Why?
#endif
    if( d->imageFinished || ( d->doc->onTheFly() && !d->doc->onlyCreateImages() ) )
        d->pipe->writeTo( m_writerJob->ioDevice(), d->usedWritingApp != K3b::WritingAppCdrecord );
    else
        d->pipe->writeTo( &d->imageFile, true );

    if ( d->imageFinished )
        d->pipe->readFrom( &d->imageFile, true );
    else
        d->pipe->readFrom( m_isoImager->ioDevice(), true );

    d->pipe->open( true );
}


bool K3b::DataJob::startOnTheFlyWriting()
{
    kDebug();
    if( prepareWriterJob() ) {
        if( startWriterJob() ) {
            d->initializingImager = false;
            m_isoImager->start();
            startPipe();
            return true;
        }
    }
    return false;
}


void K3b::DataJob::cancel()
{
    kDebug();

    emit canceled();

    d->canceled = true;

    //
    // Just cancel all and return, let slotMultiSessionParamterSetupDone,
    // slotIsoImagerFinished, and slotWriterJobFinished take care of the rest
    //
    if ( active() && !cancelAll() ) {
        kDebug() << "cancellation already done";
        cleanup();
        jobFinished( false );
    }
}


bool K3b::DataJob::cancelAll()
{
    kDebug();
    bool somethingCanceled = false;
    if ( m_isoImager->active() ) {
        kDebug() << "cancelling iso imager";
        m_isoImager->cancel();
        somethingCanceled = true;
    }
    if( m_writerJob && m_writerJob->active() ) {
        kDebug() << "cancelling writing job";
        m_writerJob->cancel();
        somethingCanceled = true;
    }
    if( d->verificationJob && d->verificationJob->active() ) {
        kDebug() << "cancelling verification job";
        d->verificationJob->cancel();
        somethingCanceled = true;
    }
    if ( d->multiSessionParameterJob && d->multiSessionParameterJob->active() ) {
        kDebug() << "cancelling multiSessionParameterJob";
        d->multiSessionParameterJob->cancel();
        somethingCanceled = true;
    }

    kDebug() << somethingCanceled;
    return somethingCanceled;
}


void K3b::DataJob::slotIsoImagerPercent( int p )
{
    if( d->doc->onlyCreateImages() ) {
        emit subPercent( p );
        emit percent( p );
    }
    else if( !d->doc->onTheFly() ) {
        double totalTasks = d->copies;
        double tasksDone = d->copiesDone; // =0 when creating an image
        if( d->doc->verifyData() ) {
            totalTasks*=2;
            tasksDone*=2;
        }
        if( !d->doc->onTheFly() ) {
            totalTasks+=1.0;
        }

        emit subPercent( p );
        emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
    }
}


void K3b::DataJob::slotIsoImagerFinished( bool success )
{
    kDebug();
    if( d->initializingImager ) {
        if( success ) {
            if( d->doc->onTheFly() && !d->doc->onlyCreateImages() ) {
                if( !startOnTheFlyWriting() ) {
                    cleanup();
                    jobFinished( false );
                }
            }
            else {
                writeImage();
            }
        }
        else {
            if( m_isoImager->hasBeenCanceled() ) {
                cancel();
            }
            else if ( !cancelAll() ) {
                cleanup();
                jobFinished( false );
            }
        }
    }
    else {
        // cache the calculated checksum since the ChecksumPipe may be deleted below
        if ( ChecksumPipe* cp = qobject_cast<ChecksumPipe*>( d->pipe ) )
            d->checksumCache = cp->checksum();

        if( !d->doc->onTheFly() ||
            d->doc->onlyCreateImages() ) {

            if( success ) {
                emit infoMessage( i18n("Image successfully created in %1", d->doc->tempDir()), K3b::Job::MessageSuccess );
                d->imageFinished = true;

                if( d->doc->onlyCreateImages() ) {
                    jobFinished( true );
                }
                else if( !d->imageFile.open( QIODevice::ReadOnly ) ) {
                    emit infoMessage( i18n("Could not open file %1", d->doc->tempDir() ), MessageError );
                    cleanup();
                    jobFinished(false);
                }
                else if( prepareWriterJob() ) {
                    startWriterJob();
                    startPipe();
                }
            }
            else {
                if( m_isoImager->hasBeenCanceled() )
                    emit canceled();
                else
                    emit infoMessage( i18n("Error while creating ISO image"), MessageError );

                cancelAll();
                cleanup();
                jobFinished( false );
            }
        }
        else { // on-the-fly
            if( success ) {
                if ( !m_writerJob->active() )
                    finishCopy();
            }
            else {
                //
                // In case the imager failed let's make sure the writer does not emit an unusable
                // error message.
                //
                if( m_writerJob && m_writerJob->active() )
                    m_writerJob->setSourceUnreadable( true );

                // there is one special case which we need to handle here: the iso imager might be canceled
                // FIXME: the iso imager should not be able to cancel itself
                if( m_isoImager->hasBeenCanceled() && !this->hasBeenCanceled() )
                    cancel();
            }
        }
    }
}


bool K3b::DataJob::startWriterJob()
{
    kDebug();
    if( d->doc->dummy() )
        emit newTask( i18n("Simulating") );
    else if( d->copies > 1 )
        emit newTask( i18n("Writing Copy %1",d->copiesDone+1) );
    else
        emit newTask( i18n("Writing") );

    emit burning(true);
    m_writerJob->start();
    return true;
}


void K3b::DataJob::slotWriterJobPercent( int p )
{
    double totalTasks = d->copies;
    double tasksDone = d->copiesDone;
    if( d->doc->verifyData() ) {
        totalTasks*=2;
        tasksDone*=2;
    }
    if( !d->doc->onTheFly() ) {
        totalTasks+=1.0;
        tasksDone+=1.0;
    }

    emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3b::DataJob::slotWriterNextTrack( int t, int tt )
{
    emit newSubTask( i18n("Writing Track %1 of %2",t,tt) );
}


void K3b::DataJob::slotWriterJobFinished( bool success )
{
    kDebug();

    if( success ) {
        if ( !d->doc->onTheFly() ||
             !m_isoImager->active() ) {
            finishCopy();
        }
    }
    else {
        if ( !cancelAll() ) {
            cleanup();
            jobFinished( false );
        }
    }
}


void K3b::DataJob::finishCopy()
{
    // the writerJob should have emitted the "simulation/writing successful" signal

    if( d->doc->verifyData() ) {
        if( !d->verificationJob ) {
            d->verificationJob = new K3b::VerificationJob( this, this );
            connect( d->verificationJob, SIGNAL(infoMessage(const QString&, int)),
                     this, SIGNAL(infoMessage(const QString&, int)) );
            connect( d->verificationJob, SIGNAL(newTask(const QString&)),
                     this, SIGNAL(newSubTask(const QString&)) );
            connect( d->verificationJob, SIGNAL(newSubTask(const QString&)),
                     this, SIGNAL(newSubTask(const QString&)) );
            connect( d->verificationJob, SIGNAL(percent(int)),
                     this, SLOT(slotVerificationProgress(int)) );
            connect( d->verificationJob, SIGNAL(percent(int)),
                     this, SIGNAL(subPercent(int)) );
            connect( d->verificationJob, SIGNAL(finished(bool)),
                     this, SLOT(slotVerificationFinished(bool)) );
            connect( d->verificationJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
                     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

        }
        d->verificationJob->clear();
        d->verificationJob->setDevice( d->doc->burner() );
        d->verificationJob->setGrownSessionSize( m_isoImager->size() );
        d->verificationJob->addTrack( 0, d->checksumCache, m_isoImager->size() );

        emit burning(false);

        emit newTask( i18n("Verifying written data") );

        d->verificationJob->start();
    }
    else {
        d->copiesDone++;

        if( d->copiesDone < d->copies ) {
            if( !K3b::eject( d->doc->burner() ) ) {
                blockingInformation( i18n("K3b was unable to eject the written disk. Please do so manually.") );
            }

            bool failed = false;
            if( d->doc->onTheFly() )
                failed = !startOnTheFlyWriting();
            else
                failed = !prepareWriterJob() || !startWriterJob();

            if( failed ) {
                cancel();
            }
            else if( !d->doc->onTheFly() ) {
#ifdef __GNUC__
#warning Growisofs needs stdin to be closed in order to exit gracefully. Cdrecord does not. However,  if closed with cdrecord we loose parts of stderr. Why?
#endif
                d->pipe->writeTo( m_writerJob->ioDevice(), d->usedWritingApp != K3b::WritingAppCdrecord );
                d->pipe->open(true);
            }
        }
        else {
            cleanup();
            if ( k3bcore->globalSettings()->ejectMedia() ) {
                K3b::Device::eject( d->doc->burner() );
            }
            jobFinished(true);
        }
    }
}


void K3b::DataJob::slotVerificationProgress( int p )
{
    double totalTasks = d->copies*2;
    double tasksDone = d->copiesDone*2 + 1; // the writing of the current copy has already been finished

    if( !d->doc->onTheFly() ) {
        totalTasks+=1.0;
        tasksDone+=1.0;
    }

    emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3b::DataJob::slotVerificationFinished( bool success )
{
    kDebug();
    d->copiesDone++;

    // reconnect our imager which we deconnected for the verification
    connectImager();

    if( k3bcore->globalSettings()->ejectMedia() || d->copiesDone < d->copies )
        K3b::Device::eject( d->doc->burner() );

    if( !d->canceled && d->copiesDone < d->copies ) {
        bool failed = false;
        if( d->doc->onTheFly() )
            failed = !startOnTheFlyWriting();
        else
            failed = !prepareWriterJob() || !startWriterJob();

        if( failed )
            cancel();
        else if( !d->doc->onTheFly() ) {
#ifdef __GNUC__
#warning Growisofs needs stdin to be closed in order to exit gracefully. Cdrecord does not. However,  if closed with cdrecord we loose parts of stderr. Why?
#endif
            d->pipe->writeTo( m_writerJob->ioDevice(), d->usedWritingApp != K3b::WritingAppCdrecord );
            d->pipe->open(true);
        }
    }
    else {
        cleanup();
        jobFinished( success );
    }
}


void K3b::DataJob::setWriterJob( K3b::AbstractWriter* writer )
{
    kDebug();
    // FIXME: progressedsize for multiple copies
    m_writerJob = writer;
    connect( m_writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
    connect( m_writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
    connect( m_writerJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
    connect( m_writerJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
    connect( m_writerJob, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
    connect( m_writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
    connect( m_writerJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( m_writerJob, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)), this, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)) );
    connect( m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
    connect( m_writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
}


void K3b::DataJob::setImager( K3b::IsoImager* imager )
{
    kDebug();
    if( m_isoImager != imager ) {
        delete m_isoImager;

        m_isoImager = imager;

        connectImager();
    }
}


void K3b::DataJob::connectImager()
{
    kDebug();
    m_isoImager->disconnect( this );
    connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
    connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
    connect( m_isoImager, SIGNAL(debuggingOutput(const QString&, const QString&)),
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
}


void K3b::DataJob::prepareImager()
{
    kDebug();
    if( !m_isoImager )
        setImager( new K3b::IsoImager( d->doc, this, this ) );
}


bool K3b::DataJob::prepareWriterJob()
{
    kDebug();
    if( m_writerJob ) {
        delete m_writerJob;
        m_writerJob = 0;
    }

    // if we append a new session we asked for an appendable cd already
    if( !waitForBurnMedium() ) {
        return false;
    }

    // It seems as if cdrecord is not able to append sessions in dao mode whereas cdrdao is
    if( d->usedWritingApp == K3b::WritingAppCdrecord )  {
        if( !setupCdrecordJob() ) {
            return false;
        }
    }
    else if ( d->usedWritingApp == K3b::WritingAppCdrdao ) {
        if ( !setupCdrdaoJob() ) {
            return false;
        }
    }
    else {
        if ( !setupGrowisofsJob() ) {
            return false;
        }
    }

    return true;
}


bool K3b::DataJob::waitForBurnMedium()
{
    // start with all media types supported by the writer
    Device::MediaTypes m  = d->doc->supportedMediaTypes() & d->doc->burner()->writeCapabilities();

    // if everything goes wrong we are left with no possible media to request
    if ( !m ) {
        emit infoMessage( i18n( "Internal Error: No medium type fits. This project cannot be burned." ), MessageError );
        return false;
    }

    emit newSubTask( i18n("Waiting for a medium") );
    Device::MediaType foundMedium = waitForMedium( d->doc->burner(),
                                                   usedMultiSessionMode() == K3b::DataDoc::CONTINUE ||
                                                   usedMultiSessionMode() == K3b::DataDoc::FINISH ?
                                                   K3b::Device::STATE_INCOMPLETE :
                                                   K3b::Device::STATE_EMPTY,
                                                   m,
                                                   d->doc->burningLength() );

    if( foundMedium == Device::MEDIA_UNKNOWN || hasBeenCanceled() ) {
        return false;
    }

    // -------------------------------
    // CD-R(W)
    // -------------------------------
    else if ( foundMedium & K3b::Device::MEDIA_CD_ALL ) {
        emit infoMessage( i18n( "Writing %1" , K3b::Device::mediaTypeString( foundMedium ) ), MessageInfo );

        // first of all we determine the data mode
        if( d->doc->dataMode() == K3b::DataModeAuto ) {
            if( !d->doc->onlyCreateImages() &&
                ( usedMultiSessionMode() == K3b::DataDoc::CONTINUE ||
                  usedMultiSessionMode() == K3b::DataDoc::FINISH ) ) {

                // try to get the last track's datamode
                // we already asked for an appendable cdr when fetching
                // the ms info
                kDebug() << "(K3b::DataJob) determining last track's datamode...";

                // FIXME: use the DeviceHandler
                K3b::Device::Toc toc = d->doc->burner()->readToc();
                if( toc.isEmpty() ) {
                    kDebug() << "(K3b::DataJob) could not retrieve toc.";
                    emit infoMessage( i18n("Unable to determine the last track's datamode. Using default."), MessageError );
                    d->usedDataMode = K3b::DataMode2;
                }
                else {
                    if( toc.back().mode() == K3b::Device::Track::MODE1 )
                        d->usedDataMode = K3b::DataMode1;
                    else
                        d->usedDataMode = K3b::DataMode2;

                    kDebug() << "(K3b::DataJob) using datamode: "
                             << (d->usedDataMode == K3b::DataMode1 ? "mode1" : "mode2")
                             << endl;
                }
            }
            else if( usedMultiSessionMode() == K3b::DataDoc::NONE )
                d->usedDataMode = K3b::DataMode1;
            else
                d->usedDataMode = K3b::DataMode2;
        }
        else
            d->usedDataMode = d->doc->dataMode();

        // determine the writing mode
        if( d->doc->writingMode() == K3b::WritingModeAuto ) {
            // TODO: put this into the cdreocrdwriter and decide based on the size of the
            // track
            if( writer()->dao() && d->usedDataMode == K3b::DataMode1 &&
                usedMultiSessionMode() == K3b::DataDoc::NONE )
                d->usedWritingMode = K3b::WritingModeSao;
            else
                d->usedWritingMode = K3b::WritingModeTao;
        }
        else
            d->usedWritingMode = d->doc->writingMode();


        if ( writingApp() == K3b::WritingAppGrowisofs ) {
            emit infoMessage( i18n( "Cannot write %1 media using %2. Falling back to default application." , QString("CD") , QString("growisofs") ), MessageWarning );
            setWritingApp( K3b::WritingAppAuto );
        }
        // cdrecord seems to have problems writing xa 1 disks in dao mode? At least on my system!
        if( writingApp() == K3b::WritingAppAuto ) {
            if( d->usedWritingMode == K3b::WritingModeSao ) {
                if( usedMultiSessionMode() != K3b::DataDoc::NONE )
                    d->usedWritingApp = K3b::WritingAppCdrdao;
                else if( d->usedDataMode == K3b::DataMode2 )
                    d->usedWritingApp = K3b::WritingAppCdrdao;
                else
                    d->usedWritingApp = K3b::WritingAppCdrecord;
            }
            else
                d->usedWritingApp = K3b::WritingAppCdrecord;
        }
        else {
            d->usedWritingApp = writingApp();
        }
    }

    else if ( foundMedium & K3b::Device::MEDIA_DVD_ALL ) {
        if ( writingApp() == K3b::WritingAppCdrdao ) {
            emit infoMessage( i18n( "Cannot write %1 media using %2. Falling back to default application.",
                                    K3b::Device::mediaTypeString( foundMedium, true ), "cdrdao" ), MessageWarning );
            setWritingApp( K3b::WritingAppAuto );
        }

        // make sure that we use the proper parameters for cdrecord
        d->usedDataMode = K3b::DataMode1;

        d->usedWritingApp = writingApp();
        // let's default to cdrecord for the time being (except for special cases below)
        // but prefer growisofs to wodim for DVDs
        if ( d->usedWritingApp == K3b::WritingAppAuto ) {
            if (k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "wodim" ))
                d->usedWritingApp = K3b::WritingAppGrowisofs;
            else
                d->usedWritingApp = K3b::WritingAppCdrecord;
        }

        // -------------------------------
        // DVD Plus
        // -------------------------------
        if( foundMedium & K3b::Device::MEDIA_DVD_PLUS_ALL ) {
            if( d->doc->dummy() ) {
                if( !questionYesNo( i18n("%1 media do not support write simulation. "
                                         "Do you really want to continue? The disc will actually be "
                                         "written to.", Device::mediaTypeString(foundMedium, true)),
                                    i18n("No Simulation with %1", Device::mediaTypeString(foundMedium, true)) ) ) {
                    return false;
                }

                d->doc->setDummy( false );
            }

            if( d->doc->writingMode() != K3b::WritingModeAuto && d->doc->writingMode() != K3b::WritingModeRestrictedOverwrite )
                emit infoMessage( i18n("Writing mode ignored when writing %1 media.", Device::mediaTypeString(foundMedium, true)), MessageInfo );
            d->usedWritingMode = K3b::WritingModeSao; // since cdrecord uses -sao for DVD+R(W)

            // Cdrecord doesn't support multisession DVD+R(W) disks
            if( usedMultiSessionMode() != DataDoc::NONE &&
                d->usedWritingApp == K3b::WritingAppCdrecord ) {
                d->usedWritingApp = WritingAppGrowisofs;
            }

            if( foundMedium & K3b::Device::MEDIA_DVD_PLUS_RW &&
                ( usedMultiSessionMode() == K3b::DataDoc::CONTINUE ||
                  usedMultiSessionMode() == K3b::DataDoc::FINISH ) )
                emit infoMessage( i18n("Growing ISO9660 filesystem on %1.", Device::mediaTypeString(foundMedium, true)), MessageInfo );
            else
                emit infoMessage( i18n("Writing %1.", Device::mediaTypeString(foundMedium, true)), MessageInfo );
        }

        // -------------------------------
        // DVD Minus
        // -------------------------------
        else if ( foundMedium & K3b::Device::MEDIA_DVD_MINUS_ALL ) {
            if( d->doc->dummy() && !d->doc->burner()->dvdMinusTestwrite() ) {
                if( !questionYesNo( i18n("Your writer (%1 %2) does not support simulation with DVD-R(W) media. "
                                         "Do you really want to continue? The media will actually be "
                                         "written to.",
                                         d->doc->burner()->vendor(),
                                         d->doc->burner()->description()),
                                    i18n("No Simulation with DVD-R(W)") ) ) {
                    return false;
                }

                d->doc->setDummy( false );
            }

            // RESTRICTED OVERWRITE
            // --------------------
            if( foundMedium & K3b::Device::MEDIA_DVD_RW_OVWR ) {
                d->usedWritingMode = K3b::WritingModeRestrictedOverwrite;
                if( usedMultiSessionMode() == K3b::DataDoc::NONE ||
                    usedMultiSessionMode() == K3b::DataDoc::START ) {
                    // FIXME: can cdrecord handle this?
                    emit infoMessage( i18n("Writing DVD-RW in restricted overwrite mode."), MessageInfo );
                }
                else {
                    emit infoMessage( i18n("Growing ISO9660 filesystem on DVD-RW in restricted overwrite mode."), MessageInfo );
                    // we can only do this with growisofs
                    d->usedWritingApp = K3b::WritingAppGrowisofs;
                }
            }

            // NORMAL
            // ------
            else {

                // FIXME: DVD-R DL jump and stuff

                if( d->doc->writingMode() == K3b::WritingModeSao ) {
                    d->usedWritingMode = K3b::WritingModeSao;
                    emit infoMessage( i18n("Writing %1 in DAO mode.", K3b::Device::mediaTypeString(foundMedium, true) ), MessageInfo );
                }

                else {
                    // check if the writer supports writing sequential and thus multisession (on -1 the burner cannot handle
                    // features and we simply ignore it and hope for the best)
                    if( d->doc->burner()->featureCurrent( K3b::Device::FEATURE_INCREMENTAL_STREAMING_WRITABLE ) == 0 ) {
                        if( !questionYesNo( i18n("Your writer (%1 %2) does not support Incremental Streaming with %3 "
                                                 "media. Multisession will not be possible. Continue anyway?",
                                                 d->doc->burner()->vendor(),
                                                 d->doc->burner()->description(),
                                                 K3b::Device::mediaTypeString(foundMedium, true) ),
                                            i18n("No Incremental Streaming") ) ) {
                            return false;
                        }
                        else {
                            d->usedWritingMode = K3b::WritingModeSao;
                            emit infoMessage( i18n("Writing %1 in DAO mode.", K3b::Device::mediaTypeString(foundMedium, true) ), MessageInfo );
                        }
                    }
                    else {
                        d->usedWritingMode = K3b::WritingModeIncrementalSequential;
                        if( !(foundMedium & (K3b::Device::MEDIA_DVD_RW|K3b::Device::MEDIA_DVD_RW_OVWR|K3b::Device::MEDIA_DVD_RW_SEQ)) &&
                            d->doc->writingMode() == K3b::WritingModeRestrictedOverwrite )
                            emit infoMessage( i18n("Restricted Overwrite is not possible with DVD-R media."), MessageInfo );

                        emit infoMessage( i18n("Writing %1 in incremental mode.", K3b::Device::mediaTypeString(foundMedium, true) ), MessageInfo );
                    }
                }
            }
        }
    }

    // --------------------
    // Blu-ray
    // --------------------
    else if ( foundMedium & K3b::Device::MEDIA_BD_ALL ) {
        d->usedWritingApp = writingApp();
        if( d->usedWritingApp == K3b::WritingAppAuto ) {
            if (k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "wodim" ))
                d->usedWritingApp = K3b::WritingAppGrowisofs;
            else
                d->usedWritingApp = K3b::WritingAppCdrecord;
        }

        if ( d->usedWritingApp == K3b::WritingAppCdrecord &&
             !k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "blu-ray" ) ) {
            d->usedWritingApp = K3b::WritingAppGrowisofs;
        }

        if( d->doc->dummy() ) {
            if( !questionYesNo( i18n("%1 media do not support write simulation. "
                                     "Do you really want to continue? The disc will actually be "
                                     "written to.", Device::mediaTypeString(foundMedium, true)),
                                i18n("No Simulation with %1", Device::mediaTypeString(foundMedium, true)) ) ) {
                return false;
            }

            d->doc->setDummy( false );
        }

        if( d->doc->writingMode() != K3b::WritingModeAuto )
            emit infoMessage( i18n("Writing mode ignored when writing %1 media.", Device::mediaTypeString(foundMedium, true)), MessageInfo );
        d->usedWritingMode = K3b::WritingModeSao; // cdrecord uses -sao for DVD+R(W), let's assume it's used also for BD-R(E)

        // Cdrecord probably doesn't support multisession BD-R disks
        // FIXME: check if above is actually true
        if( usedMultiSessionMode() != DataDoc::NONE &&
            d->usedWritingApp == K3b::WritingAppCdrecord ) {
            d->usedWritingApp = WritingAppGrowisofs;
        }

        if( foundMedium & K3b::Device::MEDIA_BD_RE &&
            ( usedMultiSessionMode() == K3b::DataDoc::CONTINUE ||
              usedMultiSessionMode() == K3b::DataDoc::FINISH ) )
            emit infoMessage( i18n("Growing ISO9660 filesystem on %1.", Device::mediaTypeString(foundMedium, true)), MessageInfo );
        else
            emit infoMessage( i18n("Writing %1.", Device::mediaTypeString(foundMedium, true)), MessageInfo );
    }

    return true;
}


QString K3b::DataJob::jobDescription() const
{
    if( d->doc->onlyCreateImages() ) {
        return i18n("Creating Data Image File");
    }
    else if( d->doc->multiSessionMode() == K3b::DataDoc::NONE ||
             d->doc->multiSessionMode() == K3b::DataDoc::AUTO ) {
        return i18n("Writing Data Project")
            + ( d->doc->isoOptions().volumeID().isEmpty()
                ? QString()
                : QString( " (%1)" ).arg(d->doc->isoOptions().volumeID()) );
    }
    else {
        return i18n("Writing Multisession Project")
            + ( d->doc->isoOptions().volumeID().isEmpty()
                ? QString()
                : QString( " (%1)" ).arg(d->doc->isoOptions().volumeID()) );
    }
}


QString K3b::DataJob::jobDetails() const
{
    if( d->doc->copies() > 1 &&
        !d->doc->dummy() &&
        !(d->doc->multiSessionMode() == K3b::DataDoc::CONTINUE ||
          d->doc->multiSessionMode() == K3b::DataDoc::FINISH) )
        return i18np("ISO9660 Filesystem (Size: %2) – One copy",
                     "ISO9660 Filesystem (Size: %2) – %1 copies",
                     d->doc->copies(),
                     KIO::convertSize( d->doc->size() ) );
    else
        return i18n( "ISO9660 Filesystem (Size: %1)",
                     KIO::convertSize( d->doc->size() ) );
}


K3b::DataDoc::MultiSessionMode K3b::DataJob::usedMultiSessionMode() const
{
    return d->multiSessionParameterJob->usedMultiSessionMode();
}


void K3b::DataJob::cleanup()
{
    kDebug();
    if( !d->doc->onTheFly() && ( d->doc->removeImages() || d->canceled ) ) {
        if( QFile::exists( d->doc->tempDir() ) ) {
            d->imageFile.remove();
            emit infoMessage( i18n("Removed image file %1",d->doc->tempDir()), K3b::Job::MessageSuccess );
        }
    }

    if( d->tocFile ) {
        delete d->tocFile;
        d->tocFile = 0;
    }
}


bool K3b::DataJob::hasBeenCanceled() const
{
    return d->canceled;
}


bool K3b::DataJob::setupCdrecordJob()
{
    kDebug();
    K3b::CdrecordWriter* writer = new K3b::CdrecordWriter( d->doc->burner(), this, this );

    // cdrecord manpage says that "not all" writers are able to write
    // multisession disks in dao mode. That means there are writers that can.

    // Does it really make sence to write Data ms cds in DAO mode since writing the
    // first session of a cd-extra in DAO mode is no problem with my writer while
    // writing the second data session is only possible in TAO mode.
    if( d->usedWritingMode == K3b::WritingModeSao &&
        usedMultiSessionMode() != K3b::DataDoc::NONE )
        emit infoMessage( i18n("Most writers do not support writing "
                               "multisession CDs in DAO mode."), MessageInfo );

    writer->setWritingMode( d->usedWritingMode );
    writer->setSimulate( d->doc->dummy() );
    writer->setBurnSpeed( d->doc->speed() );

    // multisession
    writer->setMulti( usedMultiSessionMode() == K3b::DataDoc::START ||
                      usedMultiSessionMode() == K3b::DataDoc::CONTINUE );

    if( d->doc->onTheFly() &&
        ( usedMultiSessionMode() == K3b::DataDoc::CONTINUE ||
          usedMultiSessionMode() == K3b::DataDoc::FINISH ) )
        writer->addArgument("-waiti");

    if( d->usedDataMode == K3b::DataMode1 )
        writer->addArgument( "-data" );
    else {
        if( k3bcore->externalBinManager()->binObject("cdrecord") &&
            k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "xamix" ) )
            writer->addArgument( "-xa" );
        else
            writer->addArgument( "-xa1" );
    }

    writer->addArgument( QString("-tsize=%1s").arg(m_isoImager->size()) )->addArgument("-");

    setWriterJob( writer );

    return true;
}


bool K3b::DataJob::setupCdrdaoJob()
{
    // create cdrdao job
    K3b::CdrdaoWriter* writer = new K3b::CdrdaoWriter( d->doc->burner(), this, this );
    writer->setCommand( K3b::CdrdaoWriter::WRITE );
    writer->setSimulate( d->doc->dummy() );
    writer->setBurnSpeed( d->doc->speed() );
    // multisession
    writer->setMulti( usedMultiSessionMode() == K3b::DataDoc::START ||
                      usedMultiSessionMode() == K3b::DataDoc::CONTINUE );

    // now write the tocfile
    if( d->tocFile ) delete d->tocFile;
    d->tocFile = new KTemporaryFile();
    d->tocFile->setSuffix( ".toc" );
    d->tocFile->open();

    QTextStream s( d->tocFile );
    if( d->usedDataMode == K3b::DataMode1 ) {
        s << "CD_ROM" << "\n";
        s << "\n";
        s << "TRACK MODE1" << "\n";
    }
    else {
        s << "CD_ROM_XA" << "\n";
        s << "\n";
        s << "TRACK MODE2_FORM1" << "\n";
    }

    s << "DATAFILE \"-\" " << m_isoImager->size()*2048 << "\n";

    d->tocFile->close();

    writer->setTocFile( d->tocFile->fileName() );

    setWriterJob( writer );

    return true;
}


bool K3b::DataJob::setupGrowisofsJob()
{
    K3b::GrowisofsWriter* writer = new K3b::GrowisofsWriter( d->doc->burner(), this, this );

    // these do only make sense with DVD-R(W)
    writer->setSimulate( d->doc->dummy() );
    writer->setBurnSpeed( d->doc->speed() );

    // Andy said incremental sequential is the default mode and it seems uses have more problems with DAO anyway
    // BUT: I also had a report that incremental sequential produced unreadable media!
    if( d->doc->writingMode() == K3b::WritingModeSao )
//     || ( d->doc->writingMode() == K3b::WritingModeAuto &&
// 	 usedMultiSessionMode() == K3b::DataDoc::NONE ) )
        writer->setWritingMode( K3b::WritingModeSao );

    writer->setMultiSession( usedMultiSessionMode() == K3b::DataDoc::CONTINUE ||
                             usedMultiSessionMode() == K3b::DataDoc::FINISH );

    writer->setCloseDvd( usedMultiSessionMode() == K3b::DataDoc::NONE ||
                         usedMultiSessionMode() == K3b::DataDoc::FINISH );

    writer->setImageToWrite( QString() );  // read from stdin
    writer->setTrackSize( m_isoImager->size() );

    if( usedMultiSessionMode() != K3b::DataDoc::NONE ) {
        //
        // growisofs wants a valid -C parameter for multisession, so we get it from the
        // K3b::MsInfoFetcher (see K3b::DataJob::prepareWriting)
        //
        writer->setMultiSessionInfo( m_isoImager->multiSessionInfo() );
    }

    setWriterJob( writer );

    return true;
}

#include "k3bdatajob.moc"
