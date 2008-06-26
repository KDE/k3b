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

#include "k3bdvdbooktypejob.h"

#include <k3bglobals.h>
#include <k3bprocess.h>
#include <k3bdevice.h>
#include <k3bdeviceglobals.h>
#include <k3bdevicehandler.h>
#include <k3bdiskinfo.h>
#include <k3bexternalbinmanager.h>
#include <k3bcore.h>
#include <k3bversion.h>
#include <k3bglobalsettings.h>

#include <klocale.h>
#include <kdebug.h>

#include <qregexp.h>

#include <errno.h>
#include <string.h>


class K3bDvdBooktypeJob::Private
{
public:
    Private()
        : device(0),
          process(0),
          dvdBooktypeBin(0),
          running(false),
          forceNoEject(false) {
    }

    K3bDevice::Device* device;
    K3bProcess* process;
    const K3bExternalBin* dvdBooktypeBin;

    bool success;
    bool canceled;
    bool running;

    bool forceNoEject;

    int foundMediaType;
};


K3bDvdBooktypeJob::K3bDvdBooktypeJob( K3bJobHandler* jh, QObject* parent )
    : K3bJob( jh, parent ),
      m_action(0)
{
    d = new Private;
}


K3bDvdBooktypeJob::~K3bDvdBooktypeJob()
{
    delete d->process;
    delete d;
}


void K3bDvdBooktypeJob::setForceNoEject( bool b )
{
    d->forceNoEject = b;
}


QString K3bDvdBooktypeJob::jobDescription() const
{
    return i18n("Changing DVD Booktype"); // Changing DVD±R(W) Booktype
}


QString K3bDvdBooktypeJob::jobDetails() const
{
    return QString();
}


void K3bDvdBooktypeJob::start()
{
    d->canceled = false;
    d->running = true;

    jobStarted();

    if( !d->device ) {
        emit infoMessage( i18n("No device set"), ERROR );
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
        if( waitForMedia( d->device,
                          K3bDevice::STATE_COMPLETE|K3bDevice::STATE_INCOMPLETE|K3bDevice::STATE_EMPTY,
                          K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_PLUS_R,
                          i18n("Please insert an empty DVD+R or a DVD+RW medium into drive<p><b>%1 %2 (%3)</b>.",
                               d->device->vendor(),
                               d->device->description(),
                               d->device->blockDeviceName()) ) == -1 ) {
            emit canceled();
            jobFinished(false);
            d->running = false;
            return;
        }

        emit infoMessage( i18n("Checking media..."), INFO );
        emit newTask( i18n("Checking media") );

        connect( K3bDevice::sendCommand( K3bDevice::DeviceHandler::NG_DISKINFO, d->device ),
                 SIGNAL(finished(K3bDevice::DeviceHandler*)),
                 this,
                 SLOT(slotDeviceHandlerFinished(K3bDevice::DeviceHandler*)) );
    }
    else {
        // change writer defaults
        startBooktypeChange();
    }
}


void K3bDvdBooktypeJob::start( K3bDevice::DeviceHandler* dh )
{
    d->canceled = false;
    d->running = true;

    jobStarted();

    slotDeviceHandlerFinished( dh );
}


void K3bDvdBooktypeJob::cancel()
{
    if( d->running ) {
        d->canceled = true;
        if( d->process )
            d->process->kill();
    }
    else {
        kDebug() << "(K3bDvdBooktypeJob) not running.";
    }
}


void K3bDvdBooktypeJob::setDevice( K3bDevice::Device* dev )
{
    d->device = dev;
}


void K3bDvdBooktypeJob::slotStderrLine( const QString& line )
{
    emit debuggingOutput( "dvd+rw-booktype", line );
    // FIXME
}


void K3bDvdBooktypeJob::slotProcessFinished( K3Process* p )
{
    if( d->canceled ) {
        emit canceled();
        d->success = false;
    }
    else if( p->normalExit() ) {
        if( p->exitStatus() == 0 ) {
            emit infoMessage( i18n("Booktype successfully changed"), K3bJob::SUCCESS );
            d->success = true;
        }
        else {
            emit infoMessage( i18n("%1 returned an unknown error (code %2).",d->dvdBooktypeBin->name(),p->exitStatus()),
                              K3bJob::ERROR );
            emit infoMessage( i18n("Please send me an email with the last output."), K3bJob::ERROR );

            d->success = false;
        }
    }
    else {
        emit infoMessage( i18n("%1 did not exit cleanly.",d->dvdBooktypeBin->name()),
                          ERROR );
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
            emit infoMessage( i18n("Ejecting DVD..."), INFO );
            connect( K3bDevice::eject( d->device ),
                     SIGNAL(finished(K3bDevice::DeviceHandler*)),
                     this,
                     SLOT(slotEjectingFinished(K3bDevice::DeviceHandler*)) );
        }
    }
    else {
        d->running = false;
        jobFinished(d->success);
    }
}


void K3bDvdBooktypeJob::slotEjectingFinished( K3bDevice::DeviceHandler* dh )
{
    if( !dh->success() )
        emit infoMessage( i18n("Unable to eject media."), ERROR );

    d->running = false;
    jobFinished(d->success);
}


void K3bDvdBooktypeJob::slotDeviceHandlerFinished( K3bDevice::DeviceHandler* dh )
{
    if( d->canceled ) {
        emit canceled();
        d->running = false;
        jobFinished(false);
    }

    if( dh->success() ) {

        d->foundMediaType = dh->diskInfo().mediaType();
        if( d->foundMediaType == K3bDevice::MEDIA_DVD_PLUS_R ) {
            // the media needs to be empty
            if( dh->diskInfo().empty() )
                startBooktypeChange();
            else {
                emit infoMessage( i18n("Cannot change booktype on non-empty DVD+R media."), ERROR );
                jobFinished(false);
            }
        }
        else if( d->foundMediaType == K3bDevice::MEDIA_DVD_PLUS_RW ) {
            startBooktypeChange();
        }
        else {
            emit infoMessage( i18n("No DVD+R(W) media found."), ERROR );
            jobFinished(false);
        }
    }
    else {
        emit infoMessage( i18n("Unable to determine media state."), ERROR );
        d->running = false;
        jobFinished(false);
    }
}


void K3bDvdBooktypeJob::startBooktypeChange()
{
    delete d->process;
    d->process = new K3bProcess();
    d->process->setRunPrivileged(true);
    d->process->setSuppressEmptyLines(true);
    connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStderrLine(const QString&)) );
    connect( d->process, SIGNAL(processExited(K3Process*)), this, SLOT(slotProcessFinished(K3Process*)) );

    d->dvdBooktypeBin = k3bcore->externalBinManager()->binObject( "dvd+rw-booktype" );
    if( !d->dvdBooktypeBin ) {
        emit infoMessage( i18n("Could not find %1 executable.",QString("dvd+rw-booktype")), ERROR );
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
        if( d->foundMediaType == K3bDevice::MEDIA_DVD_PLUS_RW )
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

    kDebug() << "***** dvd+rw-booktype parameters:\n";
    QString s = d->process->joinedArgs();
    kDebug() << s << endl << flush;
    emit debuggingOutput( "dvd+rw-booktype command:", s );


    if( !d->process->start( K3Process::All ) ) {
        // something went wrong when starting the program
        // it "should" be the executable
        emit infoMessage( i18n("Could not start %1.",d->dvdBooktypeBin->name()), K3bJob::ERROR );
        d->running = false;
        jobFinished(false);
    }
    else {
        emit newTask( i18n("Changing Booktype") );
    }
}

#include "k3bdvdbooktypejob.moc"
