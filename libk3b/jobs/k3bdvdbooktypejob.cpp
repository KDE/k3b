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

#include "k3bdvdbooktypejob.h"

#include "k3bglobals.h"
#include "k3bprocess.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3bdevicehandler.h"
#include "k3bdiskinfo.h"
#include "k3bexternalbinmanager.h"
#include "k3bcore.h"
#include "k3bversion.h"
#include "k3bglobalsettings.h"
#include "k3b_i18n.h"

#include <QtCore/QDebug>
#include <QtCore/QRegExp>

#include <errno.h>
#include <string.h>


class K3b::DvdBooktypeJob::Private
{
public:
    Private()
        : device(0),
          process(0),
          dvdBooktypeBin(0),
          running(false),
          forceNoEject(false) {
    }

    K3b::Device::Device* device;
    K3b::Process* process;
    const K3b::ExternalBin* dvdBooktypeBin;

    bool success;
    bool canceled;
    bool running;

    bool forceNoEject;

    int foundMediaType;
};


K3b::DvdBooktypeJob::DvdBooktypeJob( K3b::JobHandler* jh, QObject* parent )
    : K3b::Job( jh, parent ),
      m_action(0)
{
    d = new Private;
}


K3b::DvdBooktypeJob::~DvdBooktypeJob()
{
    delete d->process;
    delete d;
}


void K3b::DvdBooktypeJob::setForceNoEject( bool b )
{
    d->forceNoEject = b;
}


QString K3b::DvdBooktypeJob::jobDescription() const
{
    return i18n("Changing DVD Booktype"); // Changing DVD±R(W) Booktype
}


QString K3b::DvdBooktypeJob::jobDetails() const
{
    return QString();
}


void K3b::DvdBooktypeJob::start()
{
    d->canceled = false;
    d->running = true;

    jobStarted();

    if( !d->device ) {
        emit infoMessage( i18n("No device set"), MessageError );
        jobFinished(false);
        d->running = false;
        return;
    }

    //
    // In case we want to change the writers default we do not need to wait for a media
    //
    if( m_action == SET_MEDIA_DVD_ROM ||
        m_action == SET_MEDIA_DVD_R_W ) {
        emit newSubTask( i18n("Waiting for media") );
        if( waitForMedium( d->device,
                           K3b::Device::STATE_COMPLETE|K3b::Device::STATE_INCOMPLETE|K3b::Device::STATE_EMPTY,
                           K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_PLUS_R,
                           0,
                           i18n("Please insert an empty DVD+R or a DVD+RW medium into drive<p><b>%1 %2 (%3)</b>.",
                                d->device->vendor(),
                                d->device->description(),
                                d->device->blockDeviceName()) ) == Device::MEDIA_UNKNOWN ) {
            emit canceled();
            jobFinished(false);
            d->running = false;
            return;
        }

        emit infoMessage( i18n("Checking medium"), MessageInfo );
        emit newTask( i18n("Checking medium") );

        connect( K3b::Device::sendCommand( K3b::Device::DeviceHandler::CommandDiskInfo, d->device ),
                 SIGNAL(finished(K3b::Device::DeviceHandler*)),
                 this,
                 SLOT(slotDeviceHandlerFinished(K3b::Device::DeviceHandler*)) );
    }
    else {
        // change writer defaults
        startBooktypeChange();
    }
}


void K3b::DvdBooktypeJob::start( K3b::Device::DeviceHandler* dh )
{
    d->canceled = false;
    d->running = true;

    jobStarted();

    slotDeviceHandlerFinished( dh );
}


void K3b::DvdBooktypeJob::cancel()
{
    if( d->running ) {
        d->canceled = true;
        if( d->process )
            d->process->terminate();
    }
    else {
        qDebug() << "(K3b::DvdBooktypeJob) not running.";
    }
}


void K3b::DvdBooktypeJob::setDevice( K3b::Device::Device* dev )
{
    d->device = dev;
}


void K3b::DvdBooktypeJob::slotStderrLine( const QString& line )
{
    emit debuggingOutput( "dvd+rw-booktype", line );
    // FIXME
}


void K3b::DvdBooktypeJob::slotProcessFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    if( d->canceled ) {
        emit canceled();
        d->success = false;
    }
    else if( exitStatus == QProcess::NormalExit ) {
        if( exitCode == 0 ) {
            emit infoMessage( i18n("Booktype successfully changed"), K3b::Job::MessageSuccess );
            d->success = true;
        }
        else {
            emit infoMessage( i18n("%1 returned an unknown error (code %2).",d->dvdBooktypeBin->name(), exitCode),
                              K3b::Job::MessageError );
            emit infoMessage( i18n("Please send me an email with the last output."), K3b::Job::MessageError );

            d->success = false;
        }
    }
    else {
        emit infoMessage( i18n("%1 did not exit cleanly.",d->dvdBooktypeBin->name()),
                          MessageError );
        d->success = false;
    }

    //
    // No need to eject the media if we changed the writer's default
    //
    if( m_action == SET_MEDIA_DVD_ROM ||
        m_action == SET_MEDIA_DVD_R_W ) {

        if( d->forceNoEject ||
            !k3bcore->globalSettings()->ejectMedia() ) {
            d->running = false;
            jobFinished(d->success);
        }
        else {
            emit infoMessage( i18n("Ejecting DVD..."), MessageInfo );
            connect( K3b::Device::eject( d->device ),
                     SIGNAL(finished(K3b::Device::DeviceHandler*)),
                     this,
                     SLOT(slotEjectingFinished(K3b::Device::DeviceHandler*)) );
        }
    }
    else {
        d->running = false;
        jobFinished(d->success);
    }
}


void K3b::DvdBooktypeJob::slotEjectingFinished( K3b::Device::DeviceHandler* dh )
{
    if( !dh->success() )
        emit infoMessage( i18n("Unable to eject media."), MessageError );

    d->running = false;
    jobFinished(d->success);
}


void K3b::DvdBooktypeJob::slotDeviceHandlerFinished( K3b::Device::DeviceHandler* dh )
{
    if( d->canceled ) {
        emit canceled();
        d->running = false;
        jobFinished(false);
    }

    if( dh->success() ) {

        d->foundMediaType = dh->diskInfo().mediaType();
        if( d->foundMediaType == K3b::Device::MEDIA_DVD_PLUS_R ) {
            // the media needs to be empty
            if( dh->diskInfo().empty() )
                startBooktypeChange();
            else {
                emit infoMessage( i18n("Cannot change booktype on non-empty DVD+R media."), MessageError );
                jobFinished(false);
            }
        }
        else if( d->foundMediaType == K3b::Device::MEDIA_DVD_PLUS_RW ) {
            startBooktypeChange();
        }
        else {
            emit infoMessage( i18n("No DVD+R(W) media found."), MessageError );
            jobFinished(false);
        }
    }
    else {
        emit infoMessage( i18n("Unable to determine media state."), MessageError );
        d->running = false;
        jobFinished(false);
    }
}


void K3b::DvdBooktypeJob::startBooktypeChange()
{
    delete d->process;
    d->process = new K3b::Process();
    d->process->setSuppressEmptyLines(true);
    connect( d->process, SIGNAL(stderrLine(QString)), this, SLOT(slotStderrLine(QString)) );
    connect( d->process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotProcessFinished(int,QProcess::ExitStatus)) );

    d->dvdBooktypeBin = k3bcore->externalBinManager()->binObject( "dvd+rw-booktype" );
    if( !d->dvdBooktypeBin ) {
        emit infoMessage( i18n("Could not find %1 executable.",QString("dvd+rw-booktype")), MessageError );
        d->running = false;
        jobFinished(false);
        return;
    }

    *d->process << d->dvdBooktypeBin;

    switch( m_action ) {
    case SET_MEDIA_DVD_ROM:
        *d->process << "-dvd-rom-spec"
                    << "-media";
        break;
    case SET_MEDIA_DVD_R_W:
        if( d->foundMediaType == K3b::Device::MEDIA_DVD_PLUS_RW )
            *d->process << "-dvd+rw-spec";
        else
            *d->process << "-dvd+r-spec";
        *d->process << "-media";
        break;
    case SET_UNIT_DVD_ROM_ON_NEW_DVD_R:
        *d->process << "-dvd-rom-spec"
                    << "-unit+r";
        break;
    case SET_UNIT_DVD_ROM_ON_NEW_DVD_RW:
        *d->process << "-dvd-rom-spec"
                    << "-unit+rw";
        break;
    case SET_UNIT_DVD_R_ON_NEW_DVD_R:
        *d->process << "-dvd+r-spec"
                    << "-unit+r";
        break;
    case SET_UNIT_DVD_RW_ON_NEW_DVD_RW:
        *d->process << "-dvd+rw-spec"
                    << "-unit+rw";
        break;
    }

    *d->process << d->device->blockDeviceName();

    qDebug() << "***** dvd+rw-booktype parameters:\n";
    QString s = d->process->joinedArgs();
    qDebug() << s << endl << flush;
    emit debuggingOutput( "dvd+rw-booktype command:", s );

    if( !d->process->start( KProcess::OnlyStderrChannel ) ) {
        // something went wrong when starting the program
        // it "should" be the executable
        emit infoMessage( i18n("Could not start %1.",d->dvdBooktypeBin->name()), K3b::Job::MessageError );
        d->running = false;
        jobFinished(false);
    }
    else {
        emit newTask( i18n("Changing Booktype") );
    }
}


