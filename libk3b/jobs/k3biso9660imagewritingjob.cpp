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


#include "k3biso9660imagewritingjob.h"
#include "k3bverificationjob.h"

#include <k3bdevice.h>
#include <k3bdiskinfo.h>
#include <k3bdevicehandler.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>
#include <k3bgrowisofswriter.h>
#include <k3bglobals.h>
#include <k3bcore.h>
#include <k3bversion.h>
#include <k3bexternalbinmanager.h>
#include <k3bchecksumpipe.h>
#include <k3bfilesplitter.h>
#include <k3bglobalsettings.h>

#include <kdebug.h>
#include <klocale.h>
#include <ktemporaryfile.h>
#include <kio/global.h>

#include <qstring.h>
#include <qfile.h>
#include <qapplication.h>


class K3bIso9660ImageWritingJob::Private
{
public:
    K3bChecksumPipe checksumPipe;
    K3bFileSplitter imageFile;
};


K3bIso9660ImageWritingJob::K3bIso9660ImageWritingJob( K3bJobHandler* hdl )
    : K3bBurnJob( hdl ),
      m_writingMode(K3b::WRITING_MODE_AUTO),
      m_simulate(false),
      m_device(0),
      m_noFix(false),
      m_speed(2),
      m_dataMode(K3b::DATA_MODE_AUTO),
      m_writer(0),
      m_tocFile(0),
      m_copies(1),
      m_verifyJob(0)
{
    d = new Private;
}

K3bIso9660ImageWritingJob::~K3bIso9660ImageWritingJob()
{
    delete m_tocFile;
    delete d;
}


void K3bIso9660ImageWritingJob::start()
{
    m_canceled = m_finished = false;
    m_currentCopy = 1;

    jobStarted();

    if( m_simulate )
        m_verifyData = false;

    emit newTask( i18n("Preparing data") );

    if( !QFile::exists( m_imagePath ) ) {
        emit infoMessage( i18n("Could not find image %1",m_imagePath), K3bJob::ERROR );
        jobFinished( false );
        return;
    }

    KIO::filesize_t mb = K3b::imageFilesize( m_imagePath )/1024ULL/1024ULL;

    // very rough test but since most dvd images are 4,x or 8,x GB it should be enough
    m_dvd = ( mb > 900ULL );

    startWriting();
}


void K3bIso9660ImageWritingJob::slotWriterJobFinished( bool success )
{
    if( m_canceled ) {
        m_finished = true;
        emit canceled();
        jobFinished(false);
        return;
    }

    d->checksumPipe.close();

    if( success ) {
        if( !m_simulate && m_verifyData ) {
            emit burning(false);

            // allright
            // the writerJob should have emitted the "simulation/writing successful" signal

            if( !m_verifyJob ) {
                m_verifyJob = new K3bVerificationJob( this );
                connectSubJob( m_verifyJob,
                               SLOT(slotVerificationFinished(bool)),
                               K3bJob::DEFAULT_SIGNAL_CONNECTION,
                               K3bJob::DEFAULT_SIGNAL_CONNECTION,
                               SLOT(slotVerificationProgress(int)),
                               SIGNAL(subPercent(int)) );
            }
            m_verifyJob->setDevice( m_device );
            m_verifyJob->clear();
            m_verifyJob->addTrack( 1, d->checksumPipe.checksum(), K3b::imageFilesize( m_imagePath )/2048 );

            if( m_copies == 1 )
                emit newTask( i18n("Verifying written data") );
            else
                emit newTask( i18n("Verifying written copy %1 of %2",m_currentCopy,m_copies) );

            m_verifyJob->start();
        }
        else if( m_currentCopy >= m_copies ) {
            m_finished = true;
            jobFinished(true);
        }
        else {
            m_currentCopy++;
            startWriting();
        }
    }
    else {
        m_finished = true;
        jobFinished(false);
    }
}


void K3bIso9660ImageWritingJob::slotVerificationFinished( bool success )
{
    if( m_canceled ) {
        m_finished = true;
        emit canceled();
        jobFinished(false);
        return;
    }

    if( success && m_currentCopy < m_copies ) {
        m_currentCopy++;
        connect( K3bDevice::eject( m_device ), SIGNAL(finished(bool)),
                 this, SLOT(startWriting()) );
        return;
    }

    if( k3bcore->globalSettings()->ejectMedia() )
        K3bDevice::eject( m_device );

    m_finished = true;
    jobFinished( success );
}


void K3bIso9660ImageWritingJob::slotVerificationProgress( int p )
{
    emit percent( (int)(100.0 / (double)m_copies * ( (double)(m_currentCopy-1) + 0.5 + (double)p/200.0 )) );
}


void K3bIso9660ImageWritingJob::slotWriterPercent( int p )
{
    emit subPercent( p );

    if( m_verifyData )
        emit percent( (int)(100.0 / (double)m_copies * ( (double)(m_currentCopy-1) + ((double)p/200.0) )) );
    else
        emit percent( (int)(100.0 / (double)m_copies * ( (double)(m_currentCopy-1) + ((double)p/100.0) )) );
}


void K3bIso9660ImageWritingJob::slotNextTrack( int, int )
{
    if( m_copies == 1 )
        emit newSubTask( i18n("Writing image") );
    else
        emit newSubTask( i18n("Writing copy %1 of %2",m_currentCopy,m_copies) );
}


void K3bIso9660ImageWritingJob::cancel()
{
    if( !m_finished ) {
        m_canceled = true;

        if( m_writer )
            m_writer->cancel();
        if( m_verifyData && m_verifyJob )
            m_verifyJob->cancel();
    }
}


void K3bIso9660ImageWritingJob::startWriting()
{
    emit newSubTask( i18n("Waiting for medium") );

    // we wait for the following:
    // 1. if writing mode auto and writing app auto: all writable media types
    // 2. if writing mode auto and writing app not growisofs: all writable cd types
    // 3. if writing mode auto and writing app growisofs: all writable dvd types
    // 4. if writing mode TAO or RAW: all writable cd types
    // 5. if writing mode DAO and writing app auto: all writable cd types and DVD-R(W)
    // 6. if writing mode DAO and writing app GROWISOFS: DVD-R(W)
    // 7. if writing mode DAO and writing app CDRDAO or CDRECORD: all writable cd types
    // 8. if writing mode WRITING_MODE_INCR_SEQ: DVD-R(W)
    // 9. if writing mode WRITING_MODE_RES_OVWR: DVD-RW or DVD+RW

    int mt = 0;
    if( m_writingMode == K3b::WRITING_MODE_AUTO ) {
        if( writingApp() == K3b::WRITING_APP_DEFAULT ) {
            if( m_dvd )
                mt = K3bDevice::MEDIA_WRITABLE_DVD;
            else
                mt = K3bDevice::MEDIA_WRITABLE_CD;
        }
        else if( writingApp() != K3b::WRITING_APP_GROWISOFS )
            mt = K3bDevice::MEDIA_WRITABLE_CD;
        else
            mt = K3bDevice::MEDIA_WRITABLE_DVD;
    }
    else if( m_writingMode == K3b::WRITING_MODE_TAO || m_writingMode == K3b::WRITING_MODE_RAW ) {
        mt = K3bDevice::MEDIA_WRITABLE_CD;
    }
    else if( m_writingMode == K3b::WRITING_MODE_DAO ) {
        if( writingApp() == K3b::WRITING_APP_DEFAULT ) {
            if( m_dvd )
                mt = K3bDevice::MEDIA_WRITABLE_DVD;
            else
                mt = K3bDevice::MEDIA_WRITABLE_CD;
        }
        else if( writingApp() == K3b::WRITING_APP_GROWISOFS )
            mt = K3bDevice::MEDIA_WRITABLE_DVD;
        else
            mt = K3bDevice::MEDIA_WRITABLE_CD;
    }
    else if( m_writingMode == K3b::WRITING_MODE_RES_OVWR ) {
        mt = K3bDevice::MEDIA_DVD_PLUS_R|K3bDevice::MEDIA_DVD_PLUS_R_DL|K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR;
    }
    else {
        mt = K3bDevice::MEDIA_WRITABLE_DVD;
    }


    // wait for the media
    int media = waitForMedia( m_device, K3bDevice::STATE_EMPTY, mt );
    if( media < 0 ) {
        m_finished = true;
        emit canceled();
        jobFinished(false);
        return;
    }

    // we simply always calculate the checksum, thus making the code simpler
    d->imageFile.close();
    d->imageFile.setName( m_imagePath );
    d->imageFile.open( QIODevice::ReadOnly );
    d->checksumPipe.close();
    d->checksumPipe.readFromIODevice( &d->imageFile );

    if( prepareWriter( media ) ) {
        emit burning(true);
        m_writer->start();
        d->checksumPipe.writeToFd( m_writer->fd(), true );
        d->checksumPipe.open( K3bChecksumPipe::MD5, true );
    }
    else {
        m_finished = true;
        jobFinished(false);
    }
}


bool K3bIso9660ImageWritingJob::prepareWriter( int mediaType )
{
    if( mediaType == 0 ) { // media forced
        // just to get it going...
        if( writingApp() != K3b::WRITING_APP_GROWISOFS && !m_dvd )
            mediaType = K3bDevice::MEDIA_CD_R;
        else
            mediaType = K3bDevice::MEDIA_DVD_R;
    }

    delete m_writer;

    if( mediaType == K3bDevice::MEDIA_CD_R || mediaType == K3bDevice::MEDIA_CD_RW ) {
        K3b::WritingMode usedWritingMode = m_writingMode;
        if( usedWritingMode == K3b::WRITING_MODE_AUTO ) {
            // cdrecord seems to have problems when writing in mode2 in dao mode
            // so with cdrecord we use TAO
            if( m_noFix || m_dataMode == K3b::DATA_MODE_2 || !m_device->dao() )
                usedWritingMode = K3b::WRITING_MODE_TAO;
            else
                usedWritingMode = K3b::WRITING_MODE_DAO;
        }

        K3b::WritingApp usedApp = writingApp();
        if( usedApp == K3b::WRITING_APP_DEFAULT ) {
            if( usedWritingMode == K3b::WRITING_MODE_DAO &&
                ( m_dataMode == K3b::DATA_MODE_2 || m_noFix ) )
                usedApp = K3b::WRITING_APP_CDRDAO;
            else
                usedApp = K3b::WRITING_APP_CDRECORD;
        }


        if( usedApp == K3b::WRITING_APP_CDRECORD ) {
            K3bCdrecordWriter* writer = new K3bCdrecordWriter( m_device, this );

            writer->setWritingMode( usedWritingMode );
            writer->setSimulate( m_simulate );
            writer->setBurnSpeed( m_speed );

            if( m_noFix ) {
                writer->addArgument("-multi");
            }

            if( (m_dataMode == K3b::DATA_MODE_AUTO && m_noFix) ||
                m_dataMode == K3b::DATA_MODE_2 ) {
                if( k3bcore->externalBinManager()->binObject("cdrecord") &&
                    k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "xamix" ) )
                    writer->addArgument( "-xa" );
                else
                    writer->addArgument( "-xa1" );
            }
            else
                writer->addArgument("-data");

            // read from stdin
            writer->addArgument( QString("-tsize=%1s").arg( K3b::imageFilesize( m_imagePath )/2048 ) )->addArgument( "-" );

            m_writer = writer;
        }
        else {
            // create cdrdao job
            K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_device, this );
            writer->setCommand( K3bCdrdaoWriter::WRITE );
            writer->setSimulate( m_simulate );
            writer->setBurnSpeed( m_speed );
            // multisession
            writer->setMulti( m_noFix );

            // now write the tocfile
            delete m_tocFile;
            m_tocFile = new KTemporaryFile();
            m_tocFile->setSuffix( ".toc" );
            m_tocFile->open();

            QTextStream s( m_tocFile );
            if( (m_dataMode == K3b::DATA_MODE_AUTO && m_noFix) ||
                m_dataMode == K3b::DATA_MODE_2 ) {
                s << "CD_ROM_XA" << "\n";
                s << "\n";
                s << "TRACK MODE2_FORM1" << "\n";
            }
            else {
                s << "CD_ROM" << "\n";
                s << "\n";
                s << "TRACK MODE1" << "\n";
            }
            s << "DATAFILE \"-\" " << QString::number( K3b::imageFilesize( m_imagePath ) ) << "\n";

            m_tocFile->close();

            writer->setTocFile( m_tocFile->fileName() );

            m_writer = writer;
        }
    }
    else {  // DVD
        if( mediaType & K3bDevice::MEDIA_DVD_PLUS_ALL ) {
            if( m_simulate ) {
                if( questionYesNo( i18n("K3b does not support simulation with DVD+R(W) media. "
                                        "Do you really want to continue? The media will be written "
                                        "for real."),
                                   i18n("No Simulation with DVD+R(W)") ) ) {
                    return false;
                }
            }

            m_simulate = false;
        }

        K3bGrowisofsWriter* writer = new K3bGrowisofsWriter( m_device, this );
        writer->setSimulate( m_simulate );
        writer->setBurnSpeed( m_speed );
        writer->setWritingMode( m_writingMode == K3b::WRITING_MODE_DAO ? K3b::WRITING_MODE_DAO : K3b::WRITING_MODE_AUTO );
        writer->setImageToWrite( QString() ); // read from stdin
        writer->setCloseDvd( !m_noFix );
        writer->setTrackSize( K3b::imageFilesize( m_imagePath )/2048 );

        m_writer = writer;
    }

    connect( m_writer, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_writer, SIGNAL(nextTrack(int, int)), this, SLOT(slotNextTrack(int, int)) );
    connect( m_writer, SIGNAL(percent(int)), this, SLOT(slotWriterPercent(int)) );
    connect( m_writer, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
    connect( m_writer, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
    connect( m_writer, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( m_writer, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
    connect( m_writer, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
    connect( m_writer, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
    connect( m_writer, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_writer, SIGNAL(debuggingOutput(const QString&, const QString&)),
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

    return true;
}


QString K3bIso9660ImageWritingJob::jobDescription() const
{
    if( m_simulate )
        return i18n("Simulating ISO9660 Image");
    else
        return ( i18n("Burning ISO9660 Image")
                 + ( m_copies > 1
                     ? i18np(" - %1 Copy", " - %1 Copies", m_copies)
                     : QString() ) );
}


QString K3bIso9660ImageWritingJob::jobDetails() const
{
    return m_imagePath.section("/", -1) + QString( " (%1)" ).arg(KIO::convertSize(K3b::filesize(m_imagePath)));
}


#include "k3biso9660imagewritingjob.moc"
