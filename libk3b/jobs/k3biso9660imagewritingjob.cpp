/*
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "k3biso9660imagewritingjob.h"
#include "k3bverificationjob.h"
#include "k3bmetawriter.h"

#include "k3bdevice.h"
#include "k3bdiskinfo.h"
#include "k3bdevicehandler.h"
#include "k3bglobals.h"
#include "k3bcore.h"
#include "k3bversion.h"
#include "k3bexternalbinmanager.h"
#include "k3bchecksumpipe.h"
#include "k3bfilesplitter.h"
#include "k3bglobalsettings.h"
#include "k3b_i18n.h"

#include <KIO/Global>

#include <QDebug>
#include <QString>
#include <QFile>


class K3b::Iso9660ImageWritingJob::Private
{
public:
    K3b::ChecksumPipe checksumPipe;
    K3b::FileSplitter imageFile;

    bool isDvdImage;
    int currentCopy;
    bool canceled;
    bool finished;

    VerificationJob* verifyJob;
    MetaWriter* writer;
};


K3b::Iso9660ImageWritingJob::Iso9660ImageWritingJob( K3b::JobHandler* hdl )
    : K3b::BurnJob( hdl ),
      m_writingMode(K3b::WritingModeAuto),
      m_simulate(false),
      m_device(0),
      m_noFix(false),
      m_speed(2),
      m_dataMode(K3b::DataModeAuto),
      m_copies(1)
{
    d = new Private;
    d->verifyJob = 0;
    d->writer = 0;
}


K3b::Iso9660ImageWritingJob::~Iso9660ImageWritingJob()
{
    delete d->writer;
    delete d;
}


void K3b::Iso9660ImageWritingJob::start()
{
    d->canceled = d->finished = false;
    d->currentCopy = 1;

    jobStarted();

    if( m_simulate )
        m_verifyData = false;

    emit newTask( i18n("Preparing data") );

    if( !QFile::exists( m_imagePath ) ) {
        emit infoMessage( i18n("Could not find image %1", m_imagePath), K3b::Job::MessageError );
        jobFinished( false );
        return;
    }

    KIO::filesize_t mb = K3b::imageFilesize( QUrl::fromLocalFile(m_imagePath) )/1024ULL/1024ULL;

    // very rough test but since most dvd images are 4,x or 8,x GB it should be enough
    d->isDvdImage = ( mb > 900ULL );

    startWriting();
}


void K3b::Iso9660ImageWritingJob::slotWriterJobFinished( bool success )
{
    if( d->canceled ) {
        d->finished = true;
        emit canceled();
        jobFinished(false);
        return;
    }

    d->checksumPipe.close();

    if( success ) {
        if( !m_simulate && m_verifyData ) {
            emit burning(false);

            // alright
            // the writerJob should have emitted the "simulation/writing successful" signal

            if( !d->verifyJob ) {
                d->verifyJob = new K3b::VerificationJob( this );
                connectSubJob( d->verifyJob,
                               SLOT(slotVerificationFinished(bool)),
                               K3b::Job::DEFAULT_SIGNAL_CONNECTION,
                               K3b::Job::DEFAULT_SIGNAL_CONNECTION,
                               SLOT(slotVerificationProgress(int)),
                               SIGNAL(subPercent(int)) );
            }
            d->verifyJob->setDevice( m_device );
            d->verifyJob->clear();
            d->verifyJob->addTrack( 1, d->checksumPipe.checksum(), K3b::imageFilesize( QUrl::fromLocalFile(m_imagePath) )/2048 );

            if( m_copies == 1 )
                emit newTask( i18n("Verifying written data") );
            else
                emit newTask( i18n("Verifying written copy %1 of %2", d->currentCopy, m_copies) );

            d->verifyJob->start();
        }
        else if( d->currentCopy >= m_copies ) {
            if ( k3bcore->globalSettings()->ejectMedia() ) {
                K3b::Device::eject( m_device );
            }
            d->finished = true;
            jobFinished(true);
        }
        else {
            d->currentCopy++;
            if( !K3b::eject( m_device ) ) {
                blockingInformation( i18n("K3b was unable to eject the written medium. Please do so manually.") );
            }
            startWriting();
        }
    }
    else {
        if ( k3bcore->globalSettings()->ejectMedia() ) {
            K3b::Device::eject( m_device );
        }
        d->finished = true;
        jobFinished(false);
    }
}


void K3b::Iso9660ImageWritingJob::slotVerificationFinished( bool success )
{
    if( d->canceled ) {
        d->finished = true;
        emit canceled();
        jobFinished(false);
        return;
    }

    if( success && d->currentCopy < m_copies ) {
        d->currentCopy++;
        connect( K3b::Device::eject( m_device ), SIGNAL(finished(bool)),
                 this, SLOT(startWriting()) );
        return;
    }

    if( k3bcore->globalSettings()->ejectMedia() )
        K3b::Device::eject( m_device );

    d->finished = true;
    jobFinished( success );
}


void K3b::Iso9660ImageWritingJob::slotVerificationProgress( int p )
{
    emit percent( int(100.0 / double(m_copies) * ( double(d->currentCopy-1) + 0.5 + double(p)/200.0 )) );
}


void K3b::Iso9660ImageWritingJob::slotWriterPercent( int p )
{
    emit subPercent( p );

    if( m_verifyData )
        emit percent( int(100.0 / double(m_copies) * ( double(d->currentCopy-1) + (double(p)/200.0) )) );
    else
        emit percent( int(100.0 / double(m_copies) * ( double(d->currentCopy-1) + (double(p)/100.0) )) );
}


void K3b::Iso9660ImageWritingJob::slotNextTrack( int, int )
{
    if( m_copies == 1 )
        emit newSubTask( i18n("Writing image") );
    else
        emit newSubTask( i18n("Writing copy %1 of %2", d->currentCopy, m_copies) );
}


void K3b::Iso9660ImageWritingJob::cancel()
{
    if( !d->finished ) {
        d->canceled = true;

        if( d->writer )
            d->writer->cancel();
        if( m_verifyData && d->verifyJob )
            d->verifyJob->cancel();
    }
}


void K3b::Iso9660ImageWritingJob::startWriting()
{
    emit newSubTask( i18n("Waiting for medium") );

    // we wait for the following:
    // 1. If special CD features are requested: CD types only Special are:
    // K3b::WritingAppCdrdao with K3b::WritingModeAuto or K3b::WritingModeSao,
    // any WritingApp with K3b::WritingModeTao,
    // any WritingApp with K3b::WritingModeRaw
    // 2. If formatted DVD-RW is requested: formatted DVD-RW only Request is:
    // K3b::WritingModeRestrictedOverwrite
    // 3. If image is larger than 900 MiB (d->isDvdImage == true): DVD or BD
    // types See K3b::Iso9660ImageWritingJob::start()
    // 4. If image not larger than 900 MiB: All media types
    // 5. If not decided yet: DVD and BD media types.

    Device::MediaTypes mt = Device::MediaTypes();
    if (m_writingMode == K3b::WritingModeAuto ||
        m_writingMode == K3b::WritingModeSao) {
        if (writingApp() == K3b::WritingAppCdrdao)
            mt = K3b::Device::MEDIA_WRITABLE_CD;
        else if (d->isDvdImage)
            mt = K3b::Device::MEDIA_WRITABLE_DVD | K3b::Device::MEDIA_WRITABLE_BD;
        else
            mt = K3b::Device::MEDIA_WRITABLE;
    } else if (m_writingMode == K3b::WritingModeTao ||
               m_writingMode == K3b::WritingModeRaw) {
        mt = K3b::Device::MEDIA_WRITABLE_CD;
    } else if (m_writingMode == K3b::WritingModeRestrictedOverwrite) {
        mt = /*K3b::Device::MEDIA_DVD_PLUS_R | K3b::Device::MEDIA_DVD_PLUS_R_DL |*/
             K3b::Device::MEDIA_DVD_PLUS_RW | K3b::Device::MEDIA_DVD_RW_OVWR;
    } else {
        mt = K3b::Device::MEDIA_WRITABLE_DVD | K3b::Device::MEDIA_WRITABLE_BD;
    }

    // wait for the media
    Device::MediaType media = waitForMedium( m_device, K3b::Device::STATE_EMPTY, mt, K3b::imageFilesize( QUrl::fromLocalFile(m_imagePath) )/2048 );
    if( media == Device::MEDIA_UNKNOWN ) {
        d->finished = true;
        emit canceled();
        jobFinished(false);
        return;
    }

    // we simply always calculate the checksum, thus making the code simpler
    d->imageFile.close();
    d->imageFile.setName( m_imagePath );
    d->imageFile.open( QIODevice::ReadOnly );
    d->checksumPipe.close();
    d->checksumPipe.readFrom( &d->imageFile, true );

    if( prepareWriter() ) {
        emit burning(true);
        d->writer->start();
#ifdef __GNUC__
#warning Growisofs needs stdin to be closed in order to exit gracefully. Cdrecord does not. However,  if closed with cdrecord we loose parts of stderr. Why?
#endif
        d->checksumPipe.writeTo( d->writer->ioDevice(), d->writer->usedWritingApp() == K3b::WritingAppGrowisofs );
        d->checksumPipe.open( K3b::ChecksumPipe::MD5, true );
    }
    else {
        d->finished = true;
        jobFinished(false);
    }
}


bool K3b::Iso9660ImageWritingJob::prepareWriter()
{
    delete d->writer;

    d->writer = new MetaWriter( m_device, this );

    d->writer->setWritingMode( m_writingMode );
    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << writingApp();
    d->writer->setWritingApp( writingApp() );
    d->writer->setSimulate( m_simulate );
    d->writer->setBurnSpeed( m_speed );
    d->writer->setMultiSession( m_noFix );

    Device::Toc toc;
    toc << Device::Track( 0, Msf(K3b::imageFilesize( QUrl::fromLocalFile(m_imagePath) )/2048)-1,
                          Device::Track::TYPE_DATA,
                          ( m_dataMode == K3b::DataModeAuto && m_noFix ) ||
                          m_dataMode == K3b::DataMode2
                          ? Device::Track::XA_FORM2
                          : Device::Track::MODE1 );
    d->writer->setSessionToWrite( toc );

    connect( d->writer, SIGNAL(infoMessage(QString,int)), this, SIGNAL(infoMessage(QString,int)) );
    connect( d->writer, SIGNAL(nextTrack(int,int)), this, SLOT(slotNextTrack(int,int)) );
    connect( d->writer, SIGNAL(percent(int)), this, SLOT(slotWriterPercent(int)) );
    connect( d->writer, SIGNAL(processedSize(int,int)), this, SIGNAL(processedSize(int,int)) );
    connect( d->writer, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
    connect( d->writer, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( d->writer, SIGNAL(writeSpeed(int,K3b::Device::SpeedMultiplicator)), this, SIGNAL(writeSpeed(int,K3b::Device::SpeedMultiplicator)) );
    connect( d->writer, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
    connect( d->writer, SIGNAL(newTask(QString)), this, SIGNAL(newTask(QString)) );
    connect( d->writer, SIGNAL(newSubTask(QString)), this, SIGNAL(newSubTask(QString)) );
    connect( d->writer, SIGNAL(debuggingOutput(QString,QString)),
             this, SIGNAL(debuggingOutput(QString,QString)) );

    return true;
}


QString K3b::Iso9660ImageWritingJob::jobDescription() const
{
    if( m_simulate )
        return i18n("Simulating ISO 9660 Image");
    else
        return ( i18n("Burning ISO 9660 Image")
                 + ( m_copies > 1
                     ? i18np(" - %1 Copy", " - %1 Copies", m_copies)
                     : QString() ) );
}


QString K3b::Iso9660ImageWritingJob::jobDetails() const
{
    return m_imagePath.section('/', -1) + QString( " (%1)" ).arg(KIO::convertSize(K3b::filesize(QUrl::fromLocalFile(m_imagePath))));
}


QString K3b::Iso9660ImageWritingJob::jobSource() const
{
    return m_imagePath;
}


QString K3b::Iso9660ImageWritingJob::jobTarget() const
{
    if( m_device )
        return m_device->vendor() + ' ' + m_device->description();
    else
        return QString ();
}

#include "moc_k3biso9660imagewritingjob.cpp"
