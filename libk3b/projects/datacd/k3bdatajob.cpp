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

#include <k3bcore.h>
#include <k3bglobals.h>
#include <k3bversion.h>
#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bdevicehandler.h>
#include <k3bexternalbinmanager.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>
#include <k3bglobalsettings.h>
#include <k3bactivepipe.h>
#include <k3bfilesplitter.h>
#include <k3bverificationjob.h>
#include <k3biso9660.h>
#include <k3bdeviceglobals.h>
#include <k3bgrowisofswriter.h>

#include <k3process.h>
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



class K3bDataJob::Private
{
public:
    Private()
        : usedWritingApp(K3b::WRITING_APP_CDRECORD),
          verificationJob(0) {
    }

    K3bDataDoc* doc;

    bool initializingImager;
    bool imageFinished;
    bool canceled;

    KTemporaryFile* tocFile;

    int usedDataMode;
    K3b::WritingApp usedWritingApp;
    K3b::WritingMode usedWritingMode;

    int copies;
    int copiesDone;

    K3bVerificationJob* verificationJob;

    K3bFileSplitter imageFile;
    K3bActivePipe pipe;

    K3bDataMultiSessionParameterJob* multiSessionParameterJob;
};


K3bDataJob::K3bDataJob( K3bDataDoc* doc, K3bJobHandler* hdl, QObject* parent )
    : K3bBurnJob( hdl, parent )
{
    d = new Private;
    d->multiSessionParameterJob = new K3bDataMultiSessionParameterJob( doc, this, this );
    connectSubJob( d->multiSessionParameterJob,
                   SLOT( slotMultiSessionParamterSetupDone( bool ) ),
                   SIGNAL( newTask( const QString& ) ),
                   SIGNAL( newSubTask( const QString& ) ) );

    d->doc = doc;
    m_writerJob = 0;
    d->tocFile = 0;

    m_isoImager = 0;
    d->imageFinished = true;
}

K3bDataJob::~K3bDataJob()
{
    delete d->tocFile;
    delete d;
}


K3bDoc* K3bDataJob::doc() const
{
    return d->doc;
}


K3bDevice::Device* K3bDataJob::writer() const
{
    if( doc()->onlyCreateImages() )
        return 0; // no writer needed -> no blocking on K3bBurnJob
    else
        return doc()->burner();
}


void K3bDataJob::start()
{
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
    d->pipe.readFromIODevice( &d->imageFile );

    d->multiSessionParameterJob->start();
}


void K3bDataJob::slotMultiSessionParamterSetupDone( bool success )
{
    if ( success ) {
        prepareWriting();
    }
    else {
        if ( d->multiSessionParameterJob->hasBeenCanceled() ) {
            emit canceled();
        }
        jobFinished( false );
    }
}


void K3bDataJob::prepareWriting()
{
    if( !d->doc->onlyCreateImages() &&
        ( d->multiSessionParameterJob->usedMultiSessionMode() == K3bDataDoc::CONTINUE ||
          d->multiSessionParameterJob->usedMultiSessionMode() == K3bDataDoc::FINISH ) ) {
        unsigned int nextSessionStart = d->multiSessionParameterJob->nextSessionStart();
        // for some reason cdrdao needs 150 additional sectors in the ms info
        if( writingApp() == K3b::WRITING_APP_CDRDAO ) {
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


void K3bDataJob::writeImage()
{
    d->initializingImager = false;

    emit burning(false);

    // get image file path
    if( d->doc->tempDir().isEmpty() )
        d->doc->setTempDir( K3b::findUniqueFilePrefix( d->doc->isoOptions().volumeID() ) + ".iso" );

    // TODO: check if the image file is part of the project and if so warn the user
    //       and append some number to make the path unique.

    emit newTask( i18n("Creating image file") );
    emit newSubTask( i18n("Track 1 of 1") );
    emit infoMessage( i18n("Creating image file in %1",d->doc->tempDir()), INFO );

    m_isoImager->writeToImageFile( d->doc->tempDir() );
    m_isoImager->start();
}


bool K3bDataJob::startOnTheFlyWriting()
{
    if( prepareWriterJob() ) {
        if( startWriterJob() ) {
            // try a direct connection between the processes
            if( m_writerJob->fd() != -1 )
                m_isoImager->writeToFd( m_writerJob->fd() );
            d->initializingImager = false;
            m_isoImager->start();
            return true;
        }
    }
    return false;
}


void K3bDataJob::cancel()
{
    emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
    emit canceled();

    if( m_writerJob && m_writerJob->active() ) {
        //
        // lets wait for the writer job to finish
        // and let it finish the job for good.
        //
        cancelAll();
    }
    else {
        //
        // Just cancel all and return
        // This is bad design as we should wait for all subjobs to finish
        //
        cancelAll();
        jobFinished( false );
    }
}


void K3bDataJob::slotIsoImagerPercent( int p )
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


void K3bDataJob::slotIsoImagerFinished( bool success )
{
    if( d->initializingImager ) {
        if( success ) {
            if( d->doc->onTheFly() && !d->doc->onlyCreateImages() ) {
                if( !startOnTheFlyWriting() ) {
                    cancelAll();
                    jobFinished( false );
                }
            }
            else {
                writeImage();
            }
        }
        else {
            if( m_isoImager->hasBeenCanceled() )
                emit canceled();
            jobFinished( false );
        }
    }
    else {
        // tell the writer that there won't be more data
        if( d->doc->onTheFly() && m_writerJob )
            m_writerJob->closeFd();

        if( !d->doc->onTheFly() ||
            d->doc->onlyCreateImages() ) {

            if( success ) {
                emit infoMessage( i18n("Image successfully created in %1",d->doc->tempDir()), K3bJob::SUCCESS );
                d->imageFinished = true;

                if( d->doc->onlyCreateImages() ) {
                    jobFinished( true );
                }
                else {
                    if( prepareWriterJob() ) {
                        startWriterJob();
                        d->pipe.writeToFd( m_writerJob->fd(), true );
                        d->pipe.open(true);
                    }
                }
            }
            else {
                if( m_isoImager->hasBeenCanceled() )
                    emit canceled();
                else
                    emit infoMessage( i18n("Error while creating ISO image"), ERROR );

                cancelAll();
                jobFinished( false );
            }
        }
        else if( !success ) { // on-the-fly
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


bool K3bDataJob::startWriterJob()
{
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


void K3bDataJob::slotWriterJobPercent( int p )
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


void K3bDataJob::slotWriterNextTrack( int t, int tt )
{
    emit newSubTask( i18n("Writing Track %1 of %2",t,tt) );
}


void K3bDataJob::slotWriterJobFinished( bool success )
{
    d->pipe.close();

    //
    // This is a little workaround for the bad cancellation handling in this job
    // see cancel()
    //
    if( d->canceled ) {
        if( active() )
            jobFinished( false );
    }

    if( success ) {
        // allright
        // the writerJob should have emitted the "simulation/writing successful" signal

        if( d->doc->verifyData() ) {
            if( !d->verificationJob ) {
                d->verificationJob = new K3bVerificationJob( this, this );
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
            d->verificationJob->addTrack( 0, m_isoImager->checksum(), m_isoImager->size() );

            emit burning(false);

            emit newTask( i18n("Verifying written data") );

            d->verificationJob->start();
        }
        else {
            d->copiesDone++;

            if( d->copiesDone < d->copies ) {
                if( !d->doc->burner()->eject() ) {
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
                    d->pipe.writeToFd( m_writerJob->fd(), true );
                    d->pipe.open(true);
                }
            }
            else {
                cleanup();
                if ( k3bcore->globalSettings()->ejectMedia() ) {
                    K3bDevice::eject( d->doc->burner() );
                }
                jobFinished(true);
            }
        }
    }
    else {
        cancelAll();
        jobFinished( false );
    }
}


void K3bDataJob::slotVerificationProgress( int p )
{
    double totalTasks = d->copies*2;
    double tasksDone = d->copiesDone*2 + 1; // the writing of the current copy has already been finished

    if( !d->doc->onTheFly() ) {
        totalTasks+=1.0;
        tasksDone+=1.0;
    }

    emit percent( (int)((100.0*tasksDone + (double)p) / totalTasks) );
}


void K3bDataJob::slotVerificationFinished( bool success )
{
    d->copiesDone++;

    // reconnect our imager which we deconnected for the verification
    connectImager();

    if( k3bcore->globalSettings()->ejectMedia() || d->copiesDone < d->copies )
        K3bDevice::eject( d->doc->burner() );

    if( !d->canceled && d->copiesDone < d->copies ) {
        bool failed = false;
        if( d->doc->onTheFly() )
            failed = !startOnTheFlyWriting();
        else
            failed = !prepareWriterJob() || !startWriterJob();

        if( failed )
            cancel();
        else if( !d->doc->onTheFly() ) {
            d->pipe.writeToFd( m_writerJob->fd(), true );
            d->pipe.open(true);
        }
    }
    else {
        cleanup();
        jobFinished( success );
    }
}


void K3bDataJob::setWriterJob( K3bAbstractWriter* writer )
{
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
    connect( m_writerJob, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
    connect( m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
    connect( m_writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
}


void K3bDataJob::setImager( K3bIsoImager* imager )
{
    if( m_isoImager != imager ) {
        delete m_isoImager;

        m_isoImager = imager;

        connectImager();
    }
}


void K3bDataJob::connectImager()
{
    m_isoImager->disconnect( this );
    connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
    connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
    connect( m_isoImager, SIGNAL(debuggingOutput(const QString&, const QString&)),
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
}


void K3bDataJob::prepareImager()
{
    if( !m_isoImager )
        setImager( new K3bIsoImager( d->doc, this, this ) );
}


bool K3bDataJob::prepareWriterJob()
{
    if( m_writerJob ) {
        delete m_writerJob;
        m_writerJob = 0;
    }

    // if we append a new session we asked for an appendable cd already
    if( !waitForMedium() ) {
        return false;
    }

    // It seems as if cdrecord is not able to append sessions in dao mode whereas cdrdao is
    if( d->usedWritingApp == K3b::WRITING_APP_CDRECORD )  {
        if( !setupCdrecordJob() ) {
            return false;
        }
    }
    else if ( d->usedWritingApp == K3b::WRITING_APP_CDRDAO ) {
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


void K3bDataJob::cancelAll()
{
    d->canceled = true;

    m_isoImager->cancel();
    if( m_writerJob )
        m_writerJob->cancel();
    if( d->verificationJob )
        d->verificationJob->cancel();

    d->pipe.close();

    cleanup();
}


bool K3bDataJob::waitForMedium()
{
    // start with all media types supported by the writer
    int m  = d->doc->supportedMediaTypes() & d->doc->burner()->writeCapabilities();

    // if everything goes wrong we are left with no possible media to request
    if ( !m ) {
        emit infoMessage( i18n( "Internal Error: No medium type fits. This project cannot be burned." ), ERROR );
        return false;
    }

    emit newSubTask( i18n("Waiting for a medium") );
    int foundMedium = waitForMedia( d->doc->burner(),
                                    usedMultiSessionMode() == K3bDataDoc::CONTINUE ||
                                    usedMultiSessionMode() == K3bDataDoc::FINISH ?
                                    K3bDevice::STATE_INCOMPLETE :
                                    K3bDevice::STATE_EMPTY,
                                    m );

    if( foundMedium < 0 || hasBeenCanceled() ) {
        return false;
    }

    if( foundMedium == 0 ) {
        emit infoMessage( i18n("Forced by user. Writing will continue without further checks."), INFO );
        return true;
    }
    else {
        return analyseBurnMedium( foundMedium );
    }
}


bool K3bDataJob::analyseBurnMedium( int foundMedium )
{
    // -------------------------------
    // CD-R(W)
    // -------------------------------
    if ( foundMedium & K3bDevice::MEDIA_CD_ALL ) {
        emit infoMessage( i18n( "Writing %1" , K3bDevice::mediaTypeString( foundMedium ) ), INFO );

        // first of all we determine the data mode
        if( d->doc->dataMode() == K3b::DATA_MODE_AUTO ) {
            if( !d->doc->onlyCreateImages() &&
                ( usedMultiSessionMode() == K3bDataDoc::CONTINUE ||
                  usedMultiSessionMode() == K3bDataDoc::FINISH ) ) {

                // try to get the last track's datamode
                // we already asked for an appendable cdr when fetching
                // the ms info
                kDebug() << "(K3bDataJob) determining last track's datamode...";

                // FIXME: use the DeviceHandler
                K3bDevice::Toc toc = d->doc->burner()->readToc();
                if( toc.isEmpty() ) {
                    kDebug() << "(K3bDataJob) could not retrieve toc.";
                    emit infoMessage( i18n("Unable to determine the last track's datamode. Using default."), ERROR );
                    d->usedDataMode = K3b::DATA_MODE_2;
                }
                else {
                    if( toc.back().mode() == K3bDevice::Track::MODE1 )
                        d->usedDataMode = K3b::DATA_MODE_1;
                    else
                        d->usedDataMode = K3b::DATA_MODE_2;

                    kDebug() << "(K3bDataJob) using datamode: "
                             << (d->usedDataMode == K3b::DATA_MODE_1 ? "mode1" : "mode2")
                             << endl;
                }
            }
            else if( usedMultiSessionMode() == K3bDataDoc::NONE )
                d->usedDataMode = K3b::DATA_MODE_1;
            else
                d->usedDataMode = K3b::DATA_MODE_2;
        }
        else
            d->usedDataMode = d->doc->dataMode();

        // determine the writing mode
        if( d->doc->writingMode() == K3b::WRITING_MODE_AUTO ) {
            // TODO: put this into the cdreocrdwriter and decide based on the size of the
            // track
            if( writer()->dao() && d->usedDataMode == K3b::DATA_MODE_1 &&
                usedMultiSessionMode() == K3bDataDoc::NONE )
                d->usedWritingMode = K3b::WRITING_MODE_DAO;
            else
                d->usedWritingMode = K3b::WRITING_MODE_TAO;
        }
        else
            d->usedWritingMode = d->doc->writingMode();


        if ( writingApp() == K3b::WRITING_APP_GROWISOFS ) {
            emit infoMessage( i18n( "Cannot write %1 media using %2. Falling back to default application." , QString("CD") , QString("growisofs") ), WARNING );
            setWritingApp( K3b::WRITING_APP_DEFAULT );
        }
        // cdrecord seems to have problems writing xa 1 disks in dao mode? At least on my system!
        if( writingApp() == K3b::WRITING_APP_DEFAULT ) {
            if( d->usedWritingMode == K3b::WRITING_MODE_DAO ) {
                if( usedMultiSessionMode() != K3bDataDoc::NONE )
                    d->usedWritingApp = K3b::WRITING_APP_CDRDAO;
                else if( d->usedDataMode == K3b::DATA_MODE_2 )
                    d->usedWritingApp = K3b::WRITING_APP_CDRDAO;
                else
                    d->usedWritingApp = K3b::WRITING_APP_CDRECORD;
            }
            else
                d->usedWritingApp = K3b::WRITING_APP_CDRECORD;
        }
        else {
            d->usedWritingApp = writingApp();
        }
    }

    // -------------------------------
    // DVD Plus
    // -------------------------------
    else if ( foundMedium & K3bDevice::MEDIA_DVD_ALL ) {
        if ( writingApp() == K3b::WRITING_APP_CDRDAO ) {
            emit infoMessage( i18n( "Cannot write %1 media using %2. Falling back to default application.",
                                    K3bDevice::mediaTypeString( foundMedium, true ), "cdrdao" ), WARNING );
            setWritingApp( K3b::WRITING_APP_DEFAULT );
        }

        // make sure that we use the proper parameters for cdrecord
        d->usedDataMode = K3b::DATA_MODE_1;

        d->usedWritingApp = writingApp();
        // let's default to cdrecord for the time being (except for special cases below)
        if ( d->usedWritingApp == K3b::WRITING_APP_DEFAULT ) {
            d->usedWritingApp = K3b::WRITING_APP_CDRECORD;
        }

        if( foundMedium & K3bDevice::MEDIA_DVD_PLUS_ALL ) {
            if( d->doc->dummy() ) {
                if( !questionYesNo( i18n("DVD+R(W) media do not support write simulation. "
                                         "Do you really want to continue? The media will be written "
                                         "for real."),
                                    i18n("No Simulation with DVD+R(W)") ) ) {
                    return false;
                }

                d->doc->setDummy( false );
                emit newTask( i18n("Writing") );
            }

            if( d->doc->writingMode() != K3b::WRITING_MODE_AUTO && d->doc->writingMode() != K3b::WRITING_MODE_RES_OVWR )
                emit infoMessage( i18n("Writing mode ignored when writing DVD+R(W) media."), INFO );
            d->usedWritingMode = K3b::WRITING_MODE_DAO; // since cdrecord uses -sao for DVD+R(W)

            if( foundMedium & K3bDevice::MEDIA_DVD_PLUS_RW ) {
                if( usedMultiSessionMode() == K3bDataDoc::NONE ||
                    usedMultiSessionMode() == K3bDataDoc::START )
                    emit infoMessage( i18n("Writing DVD+RW."), INFO );
                else {
                    emit infoMessage( i18n("Growing ISO9660 filesystem on DVD+RW."), INFO );
                    // we can only do this with growisofs
                    d->usedWritingApp = K3b::WRITING_APP_GROWISOFS;
                }
            }
            else if( foundMedium & K3bDevice::MEDIA_DVD_PLUS_R_DL )
                emit infoMessage( i18n("Writing Double Layer DVD+R."), INFO );
            else
                emit infoMessage( i18n("Writing DVD+R."), INFO );
        }

        // -------------------------------
        // DVD Minus
        // -------------------------------
        else if ( foundMedium & K3bDevice::MEDIA_DVD_MINUS_ALL ) {
            if( d->doc->dummy() && !d->doc->burner()->dvdMinusTestwrite() ) {
                if( !questionYesNo( i18n("Your writer (%1 %2) does not support simulation with DVD-R(W) media. "
                                         "Do you really want to continue? The media will be written "
                                         "for real.",
                                         d->doc->burner()->vendor(),
                                         d->doc->burner()->description()),
                                    i18n("No Simulation with DVD-R(W)") ) ) {
                    return false;
                }

                d->doc->setDummy( false );
            }

            // RESTRICTED OVERWRITE
            // --------------------
            if( foundMedium & K3bDevice::MEDIA_DVD_RW_OVWR ) {
                d->usedWritingMode = K3b::WRITING_MODE_RES_OVWR;
                if( usedMultiSessionMode() == K3bDataDoc::NONE ||
                    usedMultiSessionMode() == K3bDataDoc::START ) {
                    // FIXME: can cdrecord handle this?
                    emit infoMessage( i18n("Writing DVD-RW in restricted overwrite mode."), INFO );
                }
                else {
                    emit infoMessage( i18n("Growing ISO9660 filesystem on DVD-RW in restricted overwrite mode."), INFO );
                    // we can only do this with growisofs
                    d->usedWritingApp = K3b::WRITING_APP_GROWISOFS;
                }
            }

            // NORMAL
            // ------
            else {

                // FIXME: DVD-R DL jump and stuff

                if( d->doc->writingMode() == K3b::WRITING_MODE_DAO ) {
                    d->usedWritingMode = K3b::WRITING_MODE_DAO;
                    emit infoMessage( i18n("Writing %1 in DAO mode.", K3bDevice::mediaTypeString(foundMedium, true) ), INFO );
                }

                else {
                    // check if the writer supports writing sequential and thus multisession (on -1 the burner cannot handle
                    // features and we simply ignore it and hope for the best)
                    if( d->doc->burner()->featureCurrent( K3bDevice::FEATURE_INCREMENTAL_STREAMING_WRITABLE ) == 0 ) {
                        if( !questionYesNo( i18n("Your writer (%1 %2) does not support Incremental Streaming with %3 "
                                                 "media. Multisession will not be possible. Continue anyway?",
                                                 d->doc->burner()->vendor(),
                                                 d->doc->burner()->description(),
                                                 K3bDevice::mediaTypeString(foundMedium, true) ),
                                            i18n("No Incremental Streaming") ) ) {
                            return false;
                        }
                        else {
                            d->usedWritingMode = K3b::WRITING_MODE_DAO;
                            emit infoMessage( i18n("Writing %1 in DAO mode.", K3bDevice::mediaTypeString(foundMedium, true) ), INFO );
                        }
                    }
                    else {
                        d->usedWritingMode = K3b::WRITING_MODE_INCR_SEQ;
                        if( !(foundMedium & (K3bDevice::MEDIA_DVD_RW|K3bDevice::MEDIA_DVD_RW_OVWR|K3bDevice::MEDIA_DVD_RW_SEQ)) &&
                            d->doc->writingMode() == K3b::WRITING_MODE_RES_OVWR )
                            emit infoMessage( i18n("Restricted Overwrite is not possible with DVD-R media."), INFO );

                        emit infoMessage( i18n("Writing %1 in incremental mode.", K3bDevice::mediaTypeString(foundMedium, true) ), INFO );
                    }
                }
            }
        }
    }

    // --------------------
    // Blu-Ray
    // --------------------
    else if ( foundMedium & K3bDevice::MEDIA_BD_ALL ) {
        d->usedWritingApp = writingApp();
        if ( d->usedWritingApp == K3b::WRITING_APP_DEFAULT ) {
            if ( k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "blu-ray" ) )
                d->usedWritingApp = K3b::WRITING_APP_CDRECORD;
            else
                d->usedWritingApp = K3b::WRITING_APP_GROWISOFS;
        }

        if ( d->usedWritingApp == K3b::WRITING_APP_CDRECORD &&
             !k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "blu-ray" ) ) {
            d->usedWritingApp = K3b::WRITING_APP_GROWISOFS;
        }

        // FIXME: what do we need to take care of with BD media?
        emit infoMessage( i18n( "Writing %1" , K3bDevice::mediaTypeString( foundMedium, true ) ), INFO );
    }

    return true;
}


QString K3bDataJob::jobDescription() const
{
    if( d->doc->onlyCreateImages() ) {
        return i18n("Creating Data Image File");
    }
    else if( d->doc->multiSessionMode() == K3bDataDoc::NONE ||
             d->doc->multiSessionMode() == K3bDataDoc::AUTO ) {
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


QString K3bDataJob::jobDetails() const
{
    if( d->doc->copies() > 1 &&
        !d->doc->dummy() &&
        !(d->doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
          d->doc->multiSessionMode() == K3bDataDoc::FINISH) )
        return i18np("ISO9660 Filesystem (Size: %1) - %2 copy",
                     "ISO9660 Filesystem (Size: %1) - %2 copies",
                     KIO::convertSize( d->doc->size() ),
                     d->doc->copies() );
    else
        return i18n( "ISO9660 Filesystem (Size: %1)",
                     KIO::convertSize( d->doc->size() ) );
}


K3bDataDoc::MultiSessionMode K3bDataJob::usedMultiSessionMode() const
{
    return d->multiSessionParameterJob->usedMultiSessionMode();
}


void K3bDataJob::cleanup()
{
    if( !d->doc->onTheFly() && d->doc->removeImages() ) {
        if( QFile::exists( d->doc->tempDir() ) ) {
            d->imageFile.remove();
            emit infoMessage( i18n("Removed image file %1",d->doc->tempDir()), K3bJob::SUCCESS );
        }
    }

    if( d->tocFile ) {
        delete d->tocFile;
        d->tocFile = 0;
    }
}


bool K3bDataJob::hasBeenCanceled() const
{
    return d->canceled;
}


bool K3bDataJob::setupCdrecordJob()
{
    K3bCdrecordWriter* writer = new K3bCdrecordWriter( d->doc->burner(), this, this );

    // cdrecord manpage says that "not all" writers are able to write
    // multisession disks in dao mode. That means there are writers that can.

    // Does it really make sence to write Data ms cds in DAO mode since writing the
    // first session of a cd-extra in DAO mode is no problem with my writer while
    // writing the second data session is only possible in TAO mode.
    if( d->usedWritingMode == K3b::WRITING_MODE_DAO &&
        usedMultiSessionMode() != K3bDataDoc::NONE )
        emit infoMessage( i18n("Most writers do not support writing "
                               "multisession CDs in DAO mode."), INFO );

    writer->setWritingMode( d->usedWritingMode );
    writer->setSimulate( d->doc->dummy() );
    writer->setBurnSpeed( d->doc->speed() );

    // multisession
    if( usedMultiSessionMode() == K3bDataDoc::START ||
        usedMultiSessionMode() == K3bDataDoc::CONTINUE ) {
        writer->addArgument("-multi");
    }

    if( d->doc->onTheFly() &&
        ( usedMultiSessionMode() == K3bDataDoc::CONTINUE ||
          usedMultiSessionMode() == K3bDataDoc::FINISH ) )
        writer->addArgument("-waiti");

    if( d->usedDataMode == K3b::DATA_MODE_1 )
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


bool K3bDataJob::setupCdrdaoJob()
{
    // create cdrdao job
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( d->doc->burner(), this, this );
    writer->setCommand( K3bCdrdaoWriter::WRITE );
    writer->setSimulate( d->doc->dummy() );
    writer->setBurnSpeed( d->doc->speed() );
    // multisession
    writer->setMulti( usedMultiSessionMode() == K3bDataDoc::START ||
                      usedMultiSessionMode() == K3bDataDoc::CONTINUE );

    // now write the tocfile
    if( d->tocFile ) delete d->tocFile;
    d->tocFile = new KTemporaryFile();
    d->tocFile->setSuffix( ".toc" );
    d->tocFile->open();

    QTextStream s( d->tocFile );
    if( d->usedDataMode == K3b::DATA_MODE_1 ) {
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


bool K3bDataJob::setupGrowisofsJob()
{
    K3bGrowisofsWriter* writer = new K3bGrowisofsWriter( d->doc->burner(), this, this );

    // these do only make sense with DVD-R(W)
    writer->setSimulate( d->doc->dummy() );
    writer->setBurnSpeed( d->doc->speed() );

    // Andy said incremental sequential is the default mode and it seems uses have more problems with DAO anyway
    // BUT: I also had a report that incremental sequential produced unreadable media!
    if( d->doc->writingMode() == K3b::WRITING_MODE_DAO )
//     || ( d->doc->writingMode() == K3b::WRITING_MODE_AUTO &&
// 	 usedMultiSessionMode() == K3bDataDoc::NONE ) )
        writer->setWritingMode( K3b::WRITING_MODE_DAO );

    writer->setMultiSession( usedMultiSessionMode() == K3bDataDoc::CONTINUE ||
                             usedMultiSessionMode() == K3bDataDoc::FINISH );

    writer->setCloseDvd( usedMultiSessionMode() == K3bDataDoc::NONE ||
                         usedMultiSessionMode() == K3bDataDoc::FINISH );

    writer->setImageToWrite( QString() );  // read from stdin
    writer->setTrackSize( m_isoImager->size() );

    if( usedMultiSessionMode() != K3bDataDoc::NONE ) {
        //
        // growisofs wants a valid -C parameter for multisession, so we get it from the
        // K3bMsInfoFetcher (see K3bDataJob::slotMsInfoFetched)
        //
        writer->setMultiSessionInfo( m_isoImager->multiSessionInfo() );
    }

    setWriterJob( writer );

    return true;
}

#include "k3bdatajob.moc"
