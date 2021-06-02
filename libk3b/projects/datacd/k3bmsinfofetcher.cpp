/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include "k3bmsinfofetcher.h"

#include "k3bexternalbinmanager.h"
#include "k3bdevicemanager.h"
#include "k3bdevicehandler.h"
#include "k3bdevice.h"
#include "k3bcore.h"
#include "k3bglobals.h"
#include "k3biso9660.h"
#include "k3bprocess.h"
#include "k3b_i18n.h"

#include <KProcess>

#include <QDebug>
#include <QStringList>


K3b::MsInfoFetcher::MsInfoFetcher( K3b::JobHandler* jh, QObject* parent )
    : K3b::Job( jh, parent ),
      m_process(0),
      m_device(0),
      m_dvd(false)
{
}


K3b::MsInfoFetcher::~MsInfoFetcher()
{
    delete m_process;
}


void K3b::MsInfoFetcher::start()
{
    jobStarted();

    emit infoMessage( i18n("Searching previous session"), K3b::Job::MessageInfo );

    if( !k3bcore->externalBinManager()->foundBin( "cdrecord" ) ) {
        qDebug() << "(K3b::MsInfoFetcher) could not find cdrecord executable";
        emit infoMessage( i18n("Could not find %1 executable.",QString("cdrecord")), K3b::Job::MessageError );
        jobFinished(false);
        return;
    }

    if( m_device == 0 ) {
        qDebug() << "(K3b::MsInfoFetcher) internal error: No device set!";
        jobFinished(false);
        return;
    }

    //
    // first we try to determine if it is a dvd. If so we need to
    // read the info on our own
    //

    connect( K3b::Device::sendCommand( K3b::Device::DeviceHandler::CommandDiskInfo, m_device ),
             SIGNAL(finished(K3b::Device::DeviceHandler*)),
             this,
             SLOT(slotMediaDetectionFinished(K3b::Device::DeviceHandler*)) );
}


void K3b::MsInfoFetcher::getMsInfo()
{
    delete m_process;
    m_process = new Process(this);

    const K3b::ExternalBin* bin = 0;
    if( m_dvd ) {
        // already handled
    }
    else {
        bin = k3bcore->externalBinManager()->binObject( "cdrecord" );

        if( !bin ) {
            emit infoMessage( i18n("Could not find %1 executable.", m_dvd ? QString("dvdrecord") : QString("cdrecord" )), MessageError );
            jobFinished(false);
            return;
        }

        *m_process << bin->path();

        // add the device (e.g. /dev/sg1)
        *m_process << QString("dev=") + K3b::externalBinDeviceParameter(m_device, bin);

        *m_process << "-msinfo";

        // additional user parameters from config
        *m_process << bin->userParameters();

        qDebug() << "***** " << bin->name() << " parameters:\n";
        QStringList args = m_process->program();
        args.removeFirst();
        QString s = args.join(" ");
        qDebug() << s << flush;
        emit debuggingOutput( "msinfo command:", s );


        connect( m_process, SIGNAL(finished(int)),
                 this, SLOT(slotProcessExited()) );

        m_msInfo = QString();
        m_collectedOutput = QString();
        m_canceled = false;

        m_process->start( KProcess::OnlyStdoutChannel );
    }
}


void K3b::MsInfoFetcher::slotMediaDetectionFinished( K3b::Device::DeviceHandler* h )
{
    if( h->success() ) {
        m_dvd = Device::isDvdMedia( h->diskInfo().mediaType() );
    }
    else {
        // for now we just default to cd and go on with the detecting
        m_dvd = false;
    }

    if( m_dvd ) {
        if( h->diskInfo().mediaType() & (K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR) ) {
            // get info from iso filesystem
            K3b::Iso9660 iso( m_device, h->toc().last().firstSector().lba() );
            if( iso.open() ) {
                unsigned long long nextSession = iso.primaryDescriptor().volumeSpaceSize;
                // pad to closest 32K boundary
                nextSession += 15;
                nextSession /= 16;
                nextSession *= 16;
                m_msInfo = QString::asprintf( "16,%llu", nextSession );

                jobFinished( true );
            }
            else {
                emit infoMessage( i18n("Could not open ISO 9660 filesystem in %1.",
                                       m_device->vendor() + ' ' + m_device->description() ), MessageError );
                jobFinished( false );
            }
        }
        else {
            unsigned int lastSessionStart, nextWritableAdress;
            if( m_device->getNextWritableAdress( lastSessionStart, nextWritableAdress ) ) {
                m_msInfo.sprintf( "%u,%u", lastSessionStart+16, nextWritableAdress );
                jobFinished( true );
            }
            else {
                emit infoMessage( i18n("Could not determine next writable address."), MessageError );
                jobFinished( false );
            }
        }
    }
    else // call cdrecord
        getMsInfo();
}


void K3b::MsInfoFetcher::slotProcessExited()
{
    if( m_canceled )
        return;

    if (m_process->error() == QProcess::FailedToStart) {
        emit infoMessage( i18n("Could not start %1", m_process->program().at(0)), K3b::Job::MessageError );
        jobFinished(false);
        return;
    }

    qDebug() << "(K3b::MsInfoFetcher) msinfo fetched";

    m_collectedOutput = QString::fromLocal8Bit( m_process->readAllStandardOutput() );

    emit debuggingOutput( "msinfo", m_collectedOutput );

    // now parse the output
    QString firstLine = m_collectedOutput.left( m_collectedOutput.indexOf('\n') );
    QStringList list = firstLine.split( ',' );
    if( list.count() == 2 ) {
        bool ok1, ok2;
        m_lastSessionStart = list.first().toInt( &ok1 );
        m_nextSessionStart = list[1].toInt( &ok2 );
        if( ok1 && ok2 )
            m_msInfo = firstLine.trimmed();
        else
            m_msInfo = QString();
    }
    else {
        m_msInfo = QString();
    }

    qDebug() << "(K3b::MsInfoFetcher) msinfo parsed: " << m_msInfo;

    if( m_msInfo.isEmpty() ) {
        emit infoMessage( i18n("Could not retrieve multisession information from disk."), K3b::Job::MessageError );
        emit infoMessage( i18n("The disk is either empty or not appendable."), K3b::Job::MessageError );
        jobFinished(false);
    }
    else {
        jobFinished(true);
    }
}


void K3b::MsInfoFetcher::cancel()
{
    // FIXME: this does not work if the devicehandler is running

    if( m_process )
        if( m_process->state() != QProcess::NotRunning) {
            m_canceled = true;
            m_process->terminate();
            emit canceled();
            jobFinished(false);
        }
}



