/*
 *
 * Copyright (C) 2003 Klaus-Dieter Krannich <kd@k3b.org>
 * Copyright (C) 2009 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bbinimagewritingjob.h"
#include "k3bcdrecordwriter.h"
#include "k3bcdrdaowriter.h"
#include "k3bcore.h"
#include "k3bdevice.h"
#include "k3bglobals.h"
#include "k3bexternalbinmanager.h"
#include "k3bglobalsettings.h"
#include "k3bdevicehandler.h"

#include <KLocale>
#include <KDebug>

#include <QFile>
#include <QTextStream>



K3b::BinImageWritingJob::BinImageWritingJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::BurnJob( hdl, parent ),
      m_device(0),
      m_simulate(false),
      m_noFix(false),
      m_tocFile(),
      m_speed(2),
      m_copies(1),
      m_writer(0)
{
}

K3b::BinImageWritingJob::~BinImageWritingJob()
{
}

void K3b::BinImageWritingJob::start()
{
    m_canceled =  false;

    if( m_copies < 1 )
        m_copies = 1;
    m_finishedCopies = 0;

    jobStarted();
    emit newTask( i18n("Write Binary Image") );

    if( prepareWriter() )
        writerStart();
    else
        cancel();

}

void K3b::BinImageWritingJob::cancel()
{
    m_canceled = true;
    m_writer->cancel();
    emit canceled();
    jobFinished( false );
}

bool K3b::BinImageWritingJob::prepareWriter()
{
    delete m_writer;

    int usedWritingApp = writingApp();
    const K3b::ExternalBin* cdrecordBin = k3bcore->externalBinManager()->binObject("cdrecord");
    if( usedWritingApp == K3b::WritingAppCdrecord ||
        ( usedWritingApp == K3b::WritingAppAuto && cdrecordBin && cdrecordBin->hasFeature("cuefile") && m_device->dao() ) ) {
        usedWritingApp = K3b::WritingAppCdrecord;

        // IMPROVEME: check if it's a cdrdao toc-file
        if( m_tocFile.right(4) == ".toc" ) {
            kDebug() << "(K3b::BinImageWritingJob) imagefile has ending toc.";
            usedWritingApp = K3b::WritingAppCdrdao;
        }
        else {
            // TODO: put this into K3b::CueFileParser
            // TODO: check K3b::CueFileParser::imageFilenameInCue()
            // let's see if cdrecord can handle the cue file
            QFile f( m_tocFile );
            if( f.open( QIODevice::ReadOnly ) ) {
                QTextStream fStr( &f );
                if( fStr.readAll().contains( "MODE1/2352" ) ) {
                    kDebug() << "(K3b::BinImageWritingJob) cuefile contains MODE1/2352 track. using cdrdao.";
                    usedWritingApp = K3b::WritingAppCdrdao;
                }
                f.close();
            }
            else
                kDebug() << "(K3b::BinImageWritingJob) could not open file " << m_tocFile;
        }
    }
    else
        usedWritingApp = K3b::WritingAppCdrdao;

    if( usedWritingApp == K3b::WritingAppCdrecord ) {
        // create cdrecord job
        K3b::CdrecordWriter* writer = new K3b::CdrecordWriter( m_device, this );

        writer->setDao( true );
        writer->setSimulate( m_simulate );
        writer->setBurnSpeed( m_speed );
        writer->setCueFile ( m_tocFile );
        writer->setMulti( m_noFix );

        m_writer = writer;
    }
    else {
        // create cdrdao job
        K3b::CdrdaoWriter* writer = new K3b::CdrdaoWriter( m_device, this );

        writer->setCommand( K3b::CdrdaoWriter::WRITE );
        writer->setSimulate( m_simulate );
        writer->setBurnSpeed( m_speed );
        writer->setTocFile( m_tocFile );
        writer->setMulti( m_noFix );

        m_writer = writer;
    }

    connect( m_writer, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_writer, SIGNAL(percent(int)), this, SLOT(copyPercent(int)) );
    connect( m_writer, SIGNAL(subPercent(int)), this, SLOT(copySubPercent(int)) );
    connect( m_writer, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
    connect( m_writer, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
    connect( m_writer, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
    connect( m_writer, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)), this, SIGNAL(writeSpeed(int, K3b::Device::SpeedMultiplicator)) );
    connect( m_writer, SIGNAL(finished(bool)), this, SLOT(writerFinished(bool)) );
    connect( m_writer, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
    connect( m_writer, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_writer, SIGNAL(nextTrack(int, int)), this, SLOT(slotNextTrack(int, int)) );
    connect( m_writer, SIGNAL(debuggingOutput(const QString&, const QString&)), this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

    return true;
}


void K3b::BinImageWritingJob::writerStart()
{

    if( waitForMedium( m_device ) == Device::MEDIA_UNKNOWN ) {
        cancel();
    }
    // just to be sure we did not get canceled during the async discWaiting
    else if( !m_canceled ) {
        emit burning(true);
        m_writer->start();
    }
}

void K3b::BinImageWritingJob::copyPercent(int p)
{
    emit percent( (100*m_finishedCopies + p)/m_copies );
}

void K3b::BinImageWritingJob::copySubPercent(int p)
{
    emit subPercent(p);
}

void K3b::BinImageWritingJob::writerFinished(bool ok)
{
    if( m_canceled )
        return;

    if (ok) {
        m_finishedCopies++;
        if ( m_finishedCopies == m_copies ) {
            if ( k3bcore->globalSettings()->ejectMedia() ) {
                K3b::Device::eject( m_device );
            }
            emit infoMessage( i18np("One copy successfully created", "%1 copies successfully created", m_copies),K3b::Job::MessageInfo );
            jobFinished( true );
        }
        else {
            K3b::Device::eject( m_device );
            writerStart();
        }
    }
    else {
        jobFinished(false);
    }
}


void K3b::BinImageWritingJob::slotNextTrack( int t, int tt )
{
    emit newSubTask( i18n("Writing track %1 of %2",t,tt) );
}


QString K3b::BinImageWritingJob::jobDescription() const
{
    return ( i18n("Writing cue/bin Image")
             + ( m_copies > 1
                 ? i18np(" - %1 Copy", " - %1 Copies", m_copies)
                 : QString() ) );
}


QString K3b::BinImageWritingJob::jobDetails() const
{
    return m_tocFile.section("/", -1);
}


void K3b::BinImageWritingJob::setTocFile(const QString& s)
{
    m_tocFile = s;
}

#include "k3bbinimagewritingjob.moc"
