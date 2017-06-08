/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bgrowisofshandler.h"
#include "k3bjob.h"
#include "k3bcore.h"
#include "k3bglobalsettings.h"
#include "k3bdevicehandler.h"
#include "k3b_i18n.h"

#include <QtCore/QDebug>
#include <QtCore/QLocale>
#include <QtCore/QTimer>

#include <errno.h>
#include <string.h>


class K3b::GrowisofsHandler::Private
{
public:
    int lastBuffer;
    int lastDeviceBuffer;
};


K3b::GrowisofsHandler::GrowisofsHandler( QObject* parent )
    : QObject( parent ),
      m_mediaType(Device::MEDIA_DVD_ALL)
{
    d = new Private;
    reset();
}


K3b::GrowisofsHandler::~GrowisofsHandler()
{
    delete d;
}


void K3b::GrowisofsHandler::reset( K3b::Device::Device* dev, bool dao )
{
    m_device = dev;
    m_error = ERROR_UNKNOWN;
    m_dao = dao;
    d->lastBuffer = 0;
    d->lastDeviceBuffer = 0;
}


void K3b::GrowisofsHandler::handleStart()
{
//  QTimer::singleShot( 2000, this, SLOT(slotCheckBufferStatus()) );
}


void K3b::GrowisofsHandler::handleLine( const QString& line )
{
    int pos = 0;

    if( line.startsWith( ":-[" ) ) {
        // Error

        if( line.contains( "ASC=30h" ) )
            m_error = ERROR_MEDIA;

        // :-[ PERFORM OPC failed with SK=3h/ASC=73h/ASCQ=03h
        else if( line.startsWith( ":-[ PERFORM OPC failed" ) )
            emit infoMessage( i18n("OPC failed. Please try writing speed 1x."), K3b::Job::MessageError );

        // :-[ attempt -blank=full or re-run with -dvd-compat -dvd-compat to engage DAO ]
        else if( !m_dao &&
                 ( line.contains( "engage DAO" ) || line.contains( "media is not formatted or unsupported" ) ) )
            emit infoMessage( i18n("Please try again with writing mode DAO."), K3b::Job::MessageError );

        else if( line.startsWith( ":-[ Failed to change write speed" ) ) {
            m_error = ERROR_SPEED_SET_FAILED;
        }
    }
    else if( line.startsWith( ":-(" ) ) {
        if( line.contains( "No space left on device" ) )
            m_error = ERROR_OVERSIZE;

        else if( line.contains( "blocks are free" ) && line.contains( "to be written" ) ) {
            m_error = ERROR_OVERSIZE;
            if( k3bcore->globalSettings()->overburn() )
                emit infoMessage( i18n("Trying to write more than the official disk capacity"), K3b::Job::MessageWarning );
        }

        else if( line.startsWith( ":-( unable to anonymously mmap" ) ) {
            m_error = ERROR_MEMLOCK;
        }

        else if( line.startsWith( ":-( write failed" ) ) {
            m_error = ERROR_WRITE_FAILED;
        }

        else
            emit infoMessage( line, K3b::Job::MessageError );
    }
    else if( line.startsWith( "PERFORM OPC" ) ) {
        m_error = ERROR_OPC;
    }
    else if( line.contains( "flushing cache" ) ) {
        // here is where we already should stop queriying the buffer fill
        // since the device is only used there so far...
        m_device = 0;

        emit flushingCache();
        emit newSubTask( i18n("Flushing Cache")  );
        emit infoMessage( i18n("Flushing the cache may take some time."), K3b::Job::MessageInfo );
    }

    // FIXME: I think this starts with dev->blockDeviceName() so we could improve parsing with:
    //        if( line.startsWith( dev->blockDeviceName() ) ) {
    //              line = line.mid( dev->blockDeviceName().length() );
    //              if( line.startsWith( "closing.....

    else if( line.contains( "closing track" ) ) {
        emit newSubTask( i18n("Closing Track")  );
    }
    else if( line.contains( "closing disc" ) ) {
        emit newSubTask( i18n("Closing Disk")  );
    }
    else if( line.contains( "closing session" ) ) {
        emit newSubTask( i18n("Closing Session")  );
    }
    else if( line.contains( "updating RMA" ) ) {
        emit newSubTask( i18n("Updating RMA") );
        emit infoMessage( i18n("Updating RMA..."), K3b::Job::MessageInfo );
    }
    else if( line.contains( "closing session" ) ) {
        emit newSubTask( i18n("Closing Session") );
        emit infoMessage( i18n("Closing Session..."), K3b::Job::MessageInfo );
    }
    else if( line.contains( "writing lead-out" ) ) {
        emit newSubTask( i18n("Writing Lead-out") );
        emit infoMessage( i18n("Writing the lead-out may take some time."), K3b::Job::MessageInfo );
    }
    else if( line.contains( "Quick Grow" ) ) {
        emit infoMessage( i18n("Removing reference to lead-out."), K3b::Job::MessageInfo );
    }
    else if( line.contains( "copying volume descriptor" ) ) {
        emit infoMessage( i18n("Modifying ISO 9660 volume descriptor"), K3b::Job::MessageInfo );
    }
    else if( line.contains( "FEATURE 21h is not on" ) ) {
        if( !m_dao ) {
            // FIXME: it's not only the writer. It may be the media: something like does not support it with this media
            //        da war was mit: wenn einmal formattiert, dann geht nur noch dao oder wenn einmal als overwrite
            //        formattiert, dann nur noch dao oder sowas
            emit infoMessage( i18n("Writing mode Incremental Streaming not available"), K3b::Job::MessageWarning );
            emit infoMessage( i18n("Engaging DAO"), K3b::Job::MessageWarning );
        }
    }
    else if( ( pos = line.indexOf( "Current Write Speed" ) ) > 0 ) {
        // parse write speed
        // /dev/sr0: "Current Write Speed" is 2.4x1385KBps for DVD or 4.1x4496KBps for BD

        pos += 24;
        int endPos = line.indexOf( 'x', pos+1 );
        bool ok = true;
        double speed = line.mid( pos, endPos-pos ).toDouble(&ok);
        if (ok) {
            emit infoMessage(i18n("Writing speed: %1 KB/s (%2x)",
                             int(speed * double(m_mediaType == Device::MEDIA_DVD_ALL ? Device::SPEED_FACTOR_DVD : Device::SPEED_FACTOR_BD)),
                             QLocale::system().toString(speed)), K3b::Job::MessageInfo);
        } else
            qDebug() << "(K3b::GrowisofsHandler) parsing error: '" << line.mid( pos, endPos-pos ) << "'";
    }
    else if( (pos = line.indexOf( "RBU" )) > 0 ) {

        // FIXME: use QRegExp

        // parse ring buffer fill for growisofs >= 6.0
        pos += 4;
        int endPos = line.indexOf( '%', pos+1 );
        bool ok = true;
        double val = line.mid( pos, endPos-pos ).toDouble( &ok );
        if( ok ) {
            int newBuffer = (int)(val+0.5);
            if( newBuffer != d->lastBuffer ) {
                d->lastBuffer = newBuffer;
                emit buffer( newBuffer );
            }

            // device buffer for growisofs >= 7.0
            pos = line.indexOf( "UBU", pos );
            endPos = line.indexOf( '%', pos+5 );
            if( pos > 0 ) {
                pos += 4;
                val = line.mid( pos, endPos-pos ).toDouble( &ok );
                if( ok ) {
                    int newBuffer = (int)(val+0.5);
                    if( newBuffer != d->lastDeviceBuffer ) {
                        d->lastDeviceBuffer = newBuffer;
                        emit deviceBuffer( newBuffer );
                    }
                }
            }
        }
        else
            qDebug() << "(K3b::GrowisofsHandler) failed to parse ring buffer fill from '" << line.mid( pos, endPos-pos ) << "'";
    }

    else {
        qDebug() << "(growisofs) " << line;
    }
}


void K3b::GrowisofsHandler::handleExit( int exitCode )
{
    switch( m_error ) {
    case ERROR_MEDIA:
        emit infoMessage( i18n("K3b detected a problem with the medium."), K3b::Job::MessageError );
        emit infoMessage( i18n("Please try another brand of media, preferably one explicitly recommended by your writer's vendor."), K3b::Job::MessageError );
        emit infoMessage( i18n("Report the problem if it persists anyway."), K3b::Job::MessageError );
        break;

    case ERROR_OVERSIZE:
        if( k3bcore->globalSettings()->overburn() )
            emit infoMessage( i18n("Data did not fit on disk."), K3b::Job::MessageError );
        else
            emit infoMessage( i18n("Data does not fit on disk."), K3b::Job::MessageError );
        break;

    case ERROR_SPEED_SET_FAILED:
        emit infoMessage( i18n("Unable to set writing speed."), K3b::Job::MessageError );
        emit infoMessage( i18n("Please try again with the 'ignore speed' setting."), K3b::Job::MessageError );
        break;

    case ERROR_OPC:
        emit infoMessage( i18n("Optimum Power Calibration failed."), K3b::Job::MessageError );
        emit infoMessage( i18n("Try adding '-use-the-force-luke=noopc' to the "
                               "growisofs user parameters in the K3b settings."), K3b::Job::MessageError );
        break;

    case ERROR_MEMLOCK:
        emit infoMessage( i18n("Unable to allocate software buffer."), K3b::Job::MessageError );
        emit infoMessage( i18n("This error is caused by the low memorylocked resource limit."), K3b::Job::MessageError );
        emit infoMessage( i18n("It can be solved by issuing the command 'ulimit -l unlimited'..."), K3b::Job::MessageError );
        emit infoMessage( i18n("...or by lowering the used software buffer size in the advanced K3b settings."), K3b::Job::MessageError );
        break;

    case ERROR_WRITE_FAILED:
        emit infoMessage( i18n("Write error"), K3b::Job::MessageError );
        break;

    default:

        //
        // The growisofs error codes:
        //
        // 128 + errno: fatal error upon program startup
        // errno      : fatal error during recording
        //

        if( exitCode > 128 ) {
            // for now we just emit a message with the error
            // in the future when I know more about what kinds of errors may occur
            // we will enhance this
            emit infoMessage( i18n( "Fatal error at startup: %1", QString::fromLocal8Bit( ::strerror( exitCode-128 ) ) ),
                              K3b::Job::MessageError );
        }
        else if( exitCode == 1 ) {
            // Doku says: warning at exit
            // Example: mkisofs error
            //          unable to reload
            // So basically this is just for mkisofs failure since we do not let growisofs reload the media
            emit infoMessage( i18n("Warning at exit: (1)"), K3b::Job::MessageError );
            emit infoMessage( i18n("Most likely mkisofs failed in some way."), K3b::Job::MessageError );
        }
        else {
            emit infoMessage( i18n( "Fatal error during recording: %1", QString::fromLocal8Bit( ::strerror(exitCode) ) ),
                              K3b::Job::MessageError );
        }
    }

    reset();
}


void K3b::GrowisofsHandler::slotCheckBufferStatus()
{
    connect( K3b::Device::sendCommand( K3b::Device::DeviceHandler::CommandBufferCapacity, m_device ),
             SIGNAL(finished(K3b::Device::DeviceHandler*)),
             this,
             SLOT(slotCheckBufferStatusDone(K3b::Device::DeviceHandler*)) );
}


void K3b::GrowisofsHandler::slotCheckBufferStatusDone( K3b::Device::DeviceHandler* dh )
{
    if( dh->success() && dh->bufferCapacity() > 0 ) {
        emit deviceBuffer( 100 * (dh->bufferCapacity() - dh->availableBufferCapacity() ) / dh->bufferCapacity() );
        QTimer::singleShot( 500, this, SLOT(slotCheckBufferStatus()) );
    }
    else {
        qDebug() << "(K3b::GrowisofsHandler) stopping buffer check.";
    }
}

void K3b::GrowisofsHandler::setMediaType(Device::MediaType mediaType) 
{
    m_mediaType = mediaType;
}
