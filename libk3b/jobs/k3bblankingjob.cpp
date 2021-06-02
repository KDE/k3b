/*

    SPDX-FileCopyrightText: 2003-2010 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bblankingjob.h"
#include "k3bcdrecordwriter.h"
#include "k3bcdrdaowriter.h"

#include "k3bglobals.h"
#include "k3bdevice.h"
#include "k3bdevicehandler.h"
#include "k3bcore.h"
#include "k3bglobalsettings.h"
#include "k3b_i18n.h"

#include <KConfig>
#include <KIO/Global>
#include <KIO/Job>

#include <QDebug>
#include <QString>



K3b::BlankingJob::BlankingJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::BurnJob( hdl, parent ),
      m_writerJob(0),
      m_force(true),
      m_device(0),
      m_speed(0),
      m_mode(FormattingQuick),
      m_writingApp(WritingAppAuto),
      m_canceled(false),
      m_forceNoEject(false)
{
}


K3b::BlankingJob::~BlankingJob()
{
    delete m_writerJob;
}


K3b::Device::Device* K3b::BlankingJob::writer() const
{
    return m_device;
}


void K3b::BlankingJob::setDevice( K3b::Device::Device* dev )
{
    m_device = dev;
}


void K3b::BlankingJob::start()
{
    if( m_device == 0 )
        return;

    jobStarted();

    emit newTask( i18n( "Erasing CD-RW" ) );
    emit infoMessage( i18n( "When erasing a CD-RW no progress information is available." ), MessageWarning );

    slotStartErasing();
}

void K3b::BlankingJob::slotStartErasing()
{
    m_canceled = false;

    if( m_writerJob )
        delete m_writerJob;

    if( m_writingApp == K3b::WritingAppCdrdao ) {
        K3b::CdrdaoWriter* writer = new K3b::CdrdaoWriter( m_device, this );
        m_writerJob = writer;

        writer->setCommand( K3b::CdrdaoWriter::BLANK );
        writer->setBlankMode( m_mode );
        writer->setForce( m_force );
        writer->setBurnSpeed( m_speed );
    }
    else {
        K3b::CdrecordWriter* writer = new K3b::CdrecordWriter( m_device, this );
        m_writerJob = writer;

        writer->setFormattingMode( m_mode );
        writer->setForce( m_force );
        writer->setBurnSpeed( m_speed );
    }

    connect(m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)));
    connect(m_writerJob, SIGNAL(infoMessage(QString,int)),
            this,SIGNAL(infoMessage(QString,int)));
    connect( m_writerJob, SIGNAL(debuggingOutput(QString,QString)),
             this, SIGNAL(debuggingOutput(QString,QString)) );

    if( waitForMedium( m_device,
                       K3b::Device::STATE_COMPLETE|K3b::Device::STATE_INCOMPLETE,
                       K3b::Device::MEDIA_CD_RW,
                       0,
                       i18n("Please insert a rewritable CD medium into drive<p><b>%1 %2 (%3)</b>.",
                            m_device->vendor(),
                            m_device->description(),
                            m_device->blockDeviceName()) ) == Device::MEDIA_UNKNOWN ) {
        emit canceled();
        jobFinished(false);
        return;
    }

    m_writerJob->start();
}


void K3b::BlankingJob::cancel()
{
    m_canceled = true;

    if( m_writerJob )
        m_writerJob->cancel();
}


void K3b::BlankingJob::slotFinished(bool success)
{
    if ( !m_forceNoEject && k3bcore->globalSettings()->ejectMedia() ) {
        K3b::Device::eject( m_device );
    }

    if( success ) {
        emit percent( 100 );
        jobFinished( true );
    }
    else {
        if( m_canceled ) {
            emit canceled();
        }
        else {
            emit infoMessage( i18n("Blanking error."), K3b::Job::MessageError );
            emit infoMessage( i18n("Sorry, no error handling yet."), K3b::Job::MessageError );
        }
        jobFinished( false );
    }
}


QString K3b::BlankingJob::jobDescription() const
{
    return i18n("Erasing CD-RW");
}


QString K3b::BlankingJob::jobDetails() const
{
    if( m_mode == FormattingQuick )
        return i18n("Quick Format");
    else
        return QString();
}


