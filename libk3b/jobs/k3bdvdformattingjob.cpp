/*
 *
 * Copyright (C) 2003-2010 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C)      2009 Michal Malek <michalm@jabster.pl>
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

#include "k3bdvdformattingjob.h"

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

#include <klocale.h>
#include <kdebug.h>

#include <qregexp.h>

#include <errno.h>
#include <string.h>


class K3b::DvdFormattingJob::Private
{
public:
    Private()
        : formattingMode(FormattingComplete),
          force(false),
          mode(WritingModeAuto),
          device(0),
          process(0),
          dvdFormatBin(0),
          lastProgressValue(0),
          running(false),
          forceNoEject(false) {
    }

    FormattingMode formattingMode;
    bool force;
    int mode;

    Device::Device* device;
    Process* process;
    const ExternalBin* dvdFormatBin;

    int lastProgressValue;

    bool success;
    bool canceled;
    bool running;

    bool forceNoEject;

    bool error;
};


K3b::DvdFormattingJob::DvdFormattingJob( JobHandler* jh, QObject* parent )
    : BurnJob( jh, parent )
{
    d = new Private;
}


K3b::DvdFormattingJob::~DvdFormattingJob()
{
    delete d->process;
    delete d;
}


K3b::Device::Device* K3b::DvdFormattingJob::writer() const
{
    return d->device;
}


void K3b::DvdFormattingJob::setForceNoEject( bool b )
{
    d->forceNoEject = b;
}


QString K3b::DvdFormattingJob::jobDescription() const
{
    return i18n("Formatting disc");
}


QString K3b::DvdFormattingJob::jobDetails() const
{
    if( d->formattingMode == FormattingQuick )
        return i18n("Quick Format");
    else
        return QString();
}


void K3b::DvdFormattingJob::start()
{
    d->canceled = false;
    d->running = true;
    d->error = false;

    jobStarted();

    if( !d->device ) {
        emit infoMessage( i18n("No device set"), MessageError );
        d->running = false;
        jobFinished(false);
        return;
    }

    // FIXME: check the return value
    if( K3b::isMounted( d->device ) ) {
        emit infoMessage( i18n("Unmounting medium"), MessageInfo );
        K3b::unmount( d->device );
    }

    //
    // first wait for a dvd+rw, dvd-rw, or bd-re
    // Be aware that an empty DVD-RW might be reformatted to another writing mode
    // so we also wait for empty dvds
    //
    if( waitForMedium( d->device,
                       Device::STATE_COMPLETE|Device::STATE_INCOMPLETE|Device::STATE_EMPTY,
                       Device::MEDIA_REWRITABLE_DVD|Device::MEDIA_BD_RE,
                       0,
                       i18n("Please insert a rewritable DVD or Blu-ray medium into drive<p><b>%1 %2 (%3)</b>.",
                            d->device->vendor(),
                            d->device->description(),
                            d->device->blockDeviceName()) ) == Device::MEDIA_UNKNOWN ) {
        emit canceled();
        d->running = false;
        jobFinished(false);
        return;
    }

    emit infoMessage( i18n("Checking medium"), MessageInfo );
    emit newTask( i18n("Checking medium") );

    connect( Device::sendCommand( Device::DeviceHandler::CommandDiskInfo, d->device ),
             SIGNAL(finished(K3b::Device::DeviceHandler*)),
             this,
             SLOT(slotDeviceHandlerFinished(K3b::Device::DeviceHandler*)) );
}


void K3b::DvdFormattingJob::start( const Device::DiskInfo& di )
{
    d->canceled = false;
    d->running = true;

    jobStarted();

    startFormatting( di );
}


void K3b::DvdFormattingJob::cancel()
{
    if( d->running ) {
        d->canceled = true;
        if( d->process )
            d->process->terminate();
    }
    else {
        kDebug() << "(K3b::DvdFormattingJob) not running.";
    }
}


void K3b::DvdFormattingJob::setDevice( Device::Device* dev )
{
    d->device = dev;
}


void K3b::DvdFormattingJob::setMode( int m )
{
    d->mode = m;
}


void K3b::DvdFormattingJob::setFormattingMode( FormattingMode mode )
{
    d->formattingMode = mode;
}


void K3b::DvdFormattingJob::setForce( bool b )
{
    d->force = b;
}


void K3b::DvdFormattingJob::slotStderrLine( const QString& line )
{
// * BD/DVD±RW/-RAM format utility by <appro@fy.chalmers.se>, version 7.1.
// * 4.7GB DVD-RW media in Sequential mode detected.
// * blanking 100.0|

// * formatting 100.0|

    emit debuggingOutput( "dvd+rw-format", line );

    // parsing for the -gui mode (since dvd+rw-format 4.6)
    int pos = line.indexOf( "blanking" );
    if( pos < 0 )
        pos = line.indexOf( "formatting" );
    if( pos >= 0 ) {
        pos = line.indexOf( QRegExp( "\\d" ), pos );
    }
    // parsing for \b\b... stuff
    else if( !line.startsWith("*") ) {
        pos = line.indexOf( QRegExp( "\\d" ) );
    }
    else if( line.startsWith( ":-(" ) ) {
        if( line.startsWith( ":-( unable to proceed with format" ) ) {
            d->error = true;
        }
    }

    if( pos >= 0 ) {
        int endPos = line.indexOf( QRegExp("[^\\d\\.]"), pos ) - 1;
        bool ok;
        int progress = (int)(line.mid( pos, endPos - pos ).toDouble(&ok));
        if( ok ) {
            d->lastProgressValue = progress;
            emit percent( progress );
        }
        else {
            kDebug() << "(K3b::DvdFormattingJob) parsing error: '" << line.mid( pos, endPos - pos ) << "'";
        }
    }
}


void K3b::DvdFormattingJob::slotProcessFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    if( d->canceled ) {
        emit canceled();
        d->success = false;
    }
    else if( exitStatus == QProcess::NormalExit ) {
        if( !d->error && (exitCode == 0) ) {
            emit infoMessage( i18n("Formatting successfully completed"), Job::MessageSuccess );

            if( d->lastProgressValue < 100 ) {
                emit infoMessage( i18n("Do not be concerned with the progress stopping before 100%."), MessageInfo );
                emit infoMessage( i18n("The formatting will continue in the background during writing."), MessageInfo );
            }

            d->success = true;
        }
        else {
            emit infoMessage( i18n("%1 returned an unknown error (code %2).",d->dvdFormatBin->name(), exitCode),
                              Job::MessageError );
            emit infoMessage( i18n("Please send me an email with the last output."), Job::MessageError );

            d->success = false;
        }
    }
    else {
        emit infoMessage( i18n("%1 did not exit cleanly.",d->dvdFormatBin->name()),
                          MessageError );
        d->success = false;
    }

    if( d->forceNoEject ||
        !k3bcore->globalSettings()->ejectMedia() ) {
        d->running = false;
        jobFinished(d->success);
    }
    else {
        emit infoMessage( i18n("Ejecting medium..."), MessageInfo );
        connect( Device::eject( d->device ),
                 SIGNAL(finished(K3b::Device::DeviceHandler*)),
                 this,
                 SLOT(slotEjectingFinished(K3b::Device::DeviceHandler*)) );
    }
}


void K3b::DvdFormattingJob::slotEjectingFinished( Device::DeviceHandler* dh )
{
    if( !dh->success() )
        emit infoMessage( i18n("Unable to eject medium."), MessageError );

    d->running = false;
    jobFinished(d->success);
}


void K3b::DvdFormattingJob::slotDeviceHandlerFinished( Device::DeviceHandler* dh )
{
    if( d->canceled ) {
        emit canceled();
        jobFinished(false);
        d->running = false;
    }

    if( dh->success() ) {
        startFormatting( dh->diskInfo() );
    }
    else {
        emit infoMessage( i18n("Unable to determine media state."), MessageError );
        d->running = false;
        jobFinished(false);
    }
}


void K3b::DvdFormattingJob::startFormatting( const Device::DiskInfo& diskInfo )
{
    //
    // Now check the media type:
    // if DVD-RW: use d->mode
    //            emit warning if formatting is full and stuff
    //
    // in overwrite mode: emit info that progress might stop before 100% since formatting will continue
    //                    in the background once the media gets rewritten (only DVD+RW/BD-RE?)
    //


    // emit info about what kind of media has been found
    if( diskInfo.mediaType() & (Device::MEDIA_REWRITABLE_DVD | Device::MEDIA_BD_RE) ) {
        emit infoMessage( i18n("Found %1 medium.", Device::mediaTypeString(diskInfo.mediaType())),
                        MessageInfo );
    }
    else {
        emit infoMessage( i18n("No rewritable DVD or BD medium found. Unable to format."), MessageError );
        d->running = false;
        jobFinished(false);
        return;
    }

    bool format = true;  // do we need to format
    bool blank = false;  // blank is for DVD-RW sequential incremental
    // DVD-RW restricted overwrite and DVD+RW uses the force option (or no option at all)

    //
    // DVD+RW/BD-RE is quite easy to handle. There is only one possible mode and it is always recommended to not
    // format it more than once but to overwrite it once it is formatted
    // Once the initial formatting has been done it's always "appendable" (or "complete"???)
    //

    if( diskInfo.mediaType() & (Device::MEDIA_DVD_PLUS_ALL | Device::MEDIA_BD_RE) ) {

        if( diskInfo.empty() ) {
            //
            // The DVD+RW/BD is blank and needs to be initially formatted
            //
            blank = false;
        }
        else {
            emit infoMessage( i18n("No need to format %1 media more than once.",
                                   Device::mediaTypeString(diskInfo.mediaType())), MessageInfo );
            emit infoMessage( i18n("It may simply be overwritten."), MessageInfo );

            if( d->force ) {
                emit infoMessage( i18n("Forcing formatting anyway."), MessageInfo );
                emit infoMessage( i18n("It is not recommended to force formatting of %1 media.",
                                       Device::mediaTypeString(diskInfo.mediaType())), MessageInfo );
                emit infoMessage( i18n("After 10-20 reformats the media might become unusable."), MessageInfo );
                blank = false;
            }
            else {
                format = false;
            }
        }

        if( format )
            emit newSubTask( i18n("Formatting %1 medium", Device::mediaTypeString(diskInfo.mediaType())) );
    }

    //
    // DVD-RW has two modes: incremental sequential (the default which is also needed for DAO writing)
    // and restricted overwrite which compares to the DVD+RW mode.
    //

    else {  // MEDIA_DVD_RW

        if( diskInfo.currentProfile() != Device::MEDIA_UNKNOWN ) {
            emit infoMessage( i18n("Formatted in %1 mode.",Device::mediaTypeString(diskInfo.currentProfile())), MessageInfo );


            //
            // Is it possible to have an empty DVD-RW in restricted overwrite mode???? I don't think so.
            //

            if( diskInfo.empty() &&
                (d->mode == WritingModeAuto ||
                 (d->mode == WritingModeIncrementalSequential &&
                  diskInfo.currentProfile() == Device::MEDIA_DVD_RW_SEQ) ||
                 (d->mode == WritingModeRestrictedOverwrite &&
                  diskInfo.currentProfile() == Device::MEDIA_DVD_RW_OVWR) )
                ) {
                emit infoMessage( i18n("Media is already empty."), MessageInfo );
                if( d->force )
                    emit infoMessage( i18n("Forcing formatting anyway."), MessageInfo );
                else
                    format = false;
            }
            else if( diskInfo.currentProfile() == Device::MEDIA_DVD_RW_OVWR &&
                     d->mode != WritingModeIncrementalSequential ) {
                emit infoMessage( i18n("No need to format %1 media more than once.",
                                  Device::mediaTypeString(diskInfo.currentProfile())), MessageInfo );
                emit infoMessage( i18n("It may simply be overwritten."), MessageInfo );

                if( d->force )
                    emit infoMessage( i18n("Forcing formatting anyway."), MessageInfo );
                else
                    format = false;
            }

            if( format ) {
                if( d->mode == WritingModeAuto ) {
                    // just format in the same mode as the media is currently formatted
                    blank = (diskInfo.currentProfile() == Device::MEDIA_DVD_RW_SEQ);
                }
                else {
                    blank = (d->mode == WritingModeIncrementalSequential);
                }

                emit newSubTask( i18n("Formatting"
                                      " DVD-RW in %1 mode.",Device::mediaTypeString( blank ?
                                                                                     Device::MEDIA_DVD_RW_SEQ :
                                                                                     Device::MEDIA_DVD_RW_OVWR )) );
            }
        }
        else {
            emit infoMessage( i18n("Unable to determine the current formatting state of the DVD-RW medium."), MessageError );
            d->running = false;
            jobFinished(false);
            return;
        }
    }


    if( format ) {
        delete d->process;
        d->process = new Process();
        connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStderrLine(const QString&)) );
        connect( d->process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotProcessFinished(int, QProcess::ExitStatus)) );

        d->dvdFormatBin = k3bcore->externalBinManager()->binObject( "dvd+rw-format" );
        if( !d->dvdFormatBin ) {
            emit infoMessage( i18n("Could not find %1 executable.",QString("dvd+rw-format")), MessageError );
            d->running = false;
            jobFinished(false);
            return;
        }

        if( !d->dvdFormatBin->copyright().isEmpty() )
            emit infoMessage( i18n("Using %1 %2 – Copyright © %3",d->dvdFormatBin->name(),d->dvdFormatBin->version(),d->dvdFormatBin->copyright()), MessageInfo );


        *d->process << d->dvdFormatBin;

        if( d->dvdFormatBin->version() >= Version( 4, 6 ) )
            *d->process << "-gui";

        QString p;
        if( blank )
            p = "-blank";
        else
            p = "-force";
        if( d->formattingMode == FormattingComplete )
            p += "=full";

        *d->process << p;

        *d->process << d->device->blockDeviceName();

        // additional user parameters from config
        const QStringList& params = d->dvdFormatBin->userParameters();
        for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
            *d->process << *it;

        kDebug() << "***** dvd+rw-format parameters:\n";
        QString s = d->process->joinedArgs();
        kDebug() << s << endl << flush;
        emit debuggingOutput( "dvd+rw-format command:", s );

        if( !d->process->start( KProcess::OnlyStderrChannel ) ) {
            // something went wrong when starting the program
            // it "should" be the executable
            kDebug() << "(K3b::DvdFormattingJob) could not start " << d->dvdFormatBin->path();
            emit infoMessage( i18n("Could not start %1.",d->dvdFormatBin->name()), Job::MessageError );
            d->running = false;
            jobFinished(false);
        }
        else {
            emit newTask( i18n("Formatting") );
        }
    }
    else {
        // already formatted :)
        d->running = false;
        jobFinished(true);
    }
}


#include "k3bdvdformattingjob.moc"
