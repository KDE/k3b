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

#include "k3bclonejob.h"

#include "k3breadcdreader.h"
#include "k3bcdrecordwriter.h"
#include "k3bexternalbinmanager.h"
#include "k3bdevice.h"
#include "k3bdevicehandler.h"
#include "k3bglobals.h"
#include "k3bcore.h"
#include "k3bclonetocreader.h"
#include "k3bglobalsettings.h"

#include <kdebug.h>
#include <klocale.h>

#include <qfile.h>
#include <qfileinfo.h>



class K3b::CloneJob::Private
{
public:
    Private()
        : doneCopies(0) {
    }

    int doneCopies;
};


K3b::CloneJob::CloneJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::BurnJob( hdl, parent ),
      m_writerDevice(0),
      m_readerDevice(0),
      m_writerJob(0),
      m_readcdReader(0),
      m_removeImageFiles(false),
      m_canceled(false),
      m_running(false),
      m_simulate(false),
      m_speed(1),
      m_copies(1),
      m_onlyCreateImage(false),
      m_onlyBurnExistingImage(false),
      m_readRetries(128)
{
    d = new Private;
}


K3b::CloneJob::~CloneJob()
{
    delete d;
}


void K3b::CloneJob::start()
{
    jobStarted();

    m_canceled = false;
    m_running = true;


    // TODO: check the cd size and warn the user if not enough space

    //
    // We first check if cdrecord has clone support
    // The readcdReader will check the same for readcd
    //
    const K3b::ExternalBin* cdrecordBin = k3bcore->externalBinManager()->binObject( "cdrecord" );
    if( !cdrecordBin ) {
        emit infoMessage( i18n("Could not find %1 executable.",QString("cdrecord")), MessageError );
        jobFinished(false);
        m_running = false;
        return;
    }
    else if( !cdrecordBin->hasFeature( "clone" ) ) {
        emit infoMessage( i18n("Cdrecord version %1 does not have cloning support.",cdrecordBin->version()), MessageError );
        jobFinished(false);
        m_running = false;
        return;
    }

    if( (!m_onlyCreateImage && !writer()) ||
        (!m_onlyBurnExistingImage && !readingDevice()) ) {
        emit infoMessage( i18n("No device set."), MessageError );
        jobFinished(false);
        m_running = false;
        return;
    }

    if( !m_onlyCreateImage ) {
        if( !writer()->supportsWritingMode( K3b::Device::WRITINGMODE_RAW_R96R ) &&
            !writer()->supportsWritingMode( K3b::Device::WRITINGMODE_RAW_R16 ) ) {
            emit infoMessage( i18n("CD writer %1 (%2) does not support cloning.",
                                   writer()->vendor(),
                                   writer()->description()), MessageError );
            m_running = false;
            jobFinished(false);
            return;
        }
    }

    if( m_imagePath.isEmpty() ) {
        m_imagePath = K3b::findTempFile( "img" );
    }
    else if( QFileInfo(m_imagePath).isDir() ) {
        m_imagePath = K3b::findTempFile( "img", m_imagePath );
    }

    if( m_onlyBurnExistingImage ) {
        startWriting();
    }
    else {
        emit burning( false );

        prepareReader();

        if( waitForMedium( readingDevice(),
                          K3b::Device::STATE_COMPLETE,
                          K3b::Device::MEDIA_WRITABLE_CD|K3b::Device::MEDIA_CD_ROM ) == Device::MEDIA_UNKNOWN ) {
            m_running = false;
            emit canceled();
            jobFinished(false);
            return;
        }

        emit newTask( i18n("Reading clone image") );

        m_readcdReader->start();
    }
}


void K3b::CloneJob::prepareReader()
{
    if( !m_readcdReader ) {
        m_readcdReader = new K3b::ReadcdReader( this, this );
        connect( m_readcdReader, SIGNAL(percent(int)), this, SLOT(slotReadingPercent(int)) );
        connect( m_readcdReader, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
        connect( m_readcdReader, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
        connect( m_readcdReader, SIGNAL(finished(bool)), this, SLOT(slotReadingFinished(bool)) );
        connect( m_readcdReader, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
        connect( m_readcdReader, SIGNAL(newTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
        connect( m_readcdReader, SIGNAL(debuggingOutput(const QString&, const QString&)),
                 this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
    }

    m_readcdReader->setReadDevice( readingDevice() );
    m_readcdReader->setReadSpeed( 0 ); // MAX
    m_readcdReader->setDisableCorrection( m_noCorrection );
    m_readcdReader->setImagePath( m_imagePath );
    m_readcdReader->setClone( true );
    m_readcdReader->setRetries( m_readRetries );
}


void K3b::CloneJob::prepareWriter()
{
    if( !m_writerJob ) {
        m_writerJob = new K3b::CdrecordWriter( writer(), this, this );
        connect( m_writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
        connect( m_writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterPercent(int)) );
        connect( m_writerJob, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
        connect( m_writerJob, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
        connect( m_writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
        connect( m_writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
        connect( m_writerJob, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
        connect( m_writerJob, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)), this, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)) );
        connect( m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
        //    connect( m_writerJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
        connect( m_writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
        connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
                 this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
    }

    m_writerJob->clearArguments();
    m_writerJob->setWritingMode( K3b::WritingModeRaw );
    m_writerJob->setClone( true );
    m_writerJob->setSimulate( m_simulate );
    m_writerJob->setBurnSpeed( m_speed );
    m_writerJob->addArgument( m_imagePath );
}


void K3b::CloneJob::cancel()
{
    if( m_running ) {
        m_canceled = true;
        if( m_readcdReader )
            m_readcdReader->cancel();
        if( m_writerJob )
            m_writerJob->cancel();
    }
}


void K3b::CloneJob::slotWriterPercent( int p )
{
    if( m_onlyBurnExistingImage )
        emit percent( (int)((double)(d->doneCopies)*100.0/(double)(m_copies) + (double)p/(double)(m_copies)) );
    else
        emit percent( (int)((double)(1+d->doneCopies)*100.0/(double)(1+m_copies) + (double)p/(double)(1+m_copies)) );
}


void K3b::CloneJob::slotWriterNextTrack( int t, int tt )
{
    emit newSubTask( i18n("Writing Track %1 of %2",t,tt) );
}


void K3b::CloneJob::slotWriterFinished( bool success )
{
    if( m_canceled ) {
        removeImageFiles();
        m_running = false;
        emit canceled();
        jobFinished(false);
        return;
    }

    if( success ) {
        d->doneCopies++;

        emit infoMessage( i18n("Successfully written clone copy %1.",d->doneCopies), MessageInfo );

        if( d->doneCopies < m_copies ) {
            K3b::Device::eject( writer() );
            startWriting();
        }
        else {
            if ( k3bcore->globalSettings()->ejectMedia() ) {
                K3b::Device::eject( writer() );
            }

            if( m_removeImageFiles )
                removeImageFiles();
            m_running = false;
            jobFinished(true);
        }
    }
    else {
        removeImageFiles();
        m_running = false;
        jobFinished(false);
    }
}


void K3b::CloneJob::slotReadingPercent( int p )
{
    emit percent( m_onlyCreateImage ? p : (int)((double)p/(double)(1+m_copies)) );
}


void K3b::CloneJob::slotReadingFinished( bool success )
{
    if( m_canceled ) {
        removeImageFiles();
        m_running = false;
        emit canceled();
        jobFinished(false);
        return;
    }

    if( success ) {
        //
        // Make a quick test if the image is really valid.
        // Readcd does not seem to have proper exit codes
        //
        K3b::CloneTocReader ctr( m_imagePath );
        if( ctr.isValid() ) {
            emit infoMessage( i18n("Successfully read disk."), MessageInfo );
            if( m_onlyCreateImage ) {
                m_running = false;
                jobFinished(true);
            }
            else {
                if( writer() == readingDevice() )
                    K3b::Device::eject( writer() );
                startWriting();
            }
        }
        else {
            emit infoMessage( i18n("Failed to read disk completely in clone mode."), MessageError );
            removeImageFiles();
            m_running = false;
            jobFinished(false);
        }
    }
    else {
        emit infoMessage( i18n("Error while reading disk."), MessageError );
        removeImageFiles();
        m_running = false;
        jobFinished(false);
    }
}


void K3b::CloneJob::startWriting()
{
    emit burning( true );

    // start writing
    prepareWriter();

    if( waitForMedium( writer(),
                      K3b::Device::STATE_EMPTY,
                      K3b::Device::MEDIA_WRITABLE_CD ) == Device::MEDIA_UNKNOWN ) {
        removeImageFiles();
        m_running = false;
        emit canceled();
        jobFinished(false);
        return;
    }

    if( m_simulate )
        emit newTask( i18n("Simulating clone copy") );
    else
        emit newTask( i18n("Writing clone copy %1",d->doneCopies+1) );

    m_writerJob->start();
}


void K3b::CloneJob::removeImageFiles()
{
    if( !m_onlyBurnExistingImage ) {
        emit infoMessage( i18n("Removing image files."), MessageInfo );
        if( QFile::exists( m_imagePath ) )
            QFile::remove( m_imagePath );
        if( QFile::exists( m_imagePath + ".toc" ) )
            QFile::remove( m_imagePath + ".toc"  );
    }
}


QString K3b::CloneJob::jobDescription() const
{
    if( m_onlyCreateImage )
        return i18n("Creating Clone Image");
    else if( m_onlyBurnExistingImage ) {
        if( m_simulate )
            return i18n("Simulating Clone Image");
        else
            return i18n("Burning Clone Image");
    }
    else if( m_simulate )
        return i18n("Simulating CD Cloning");
    else
        return i18n("Cloning CD");
}


QString K3b::CloneJob::jobDetails() const
{
    return i18np("Creating 1 clone copy",
                 "Creating %1 clone copies",
                 (m_simulate||m_onlyCreateImage) ? 1 : m_copies );
}

#include "k3bclonejob.moc"
