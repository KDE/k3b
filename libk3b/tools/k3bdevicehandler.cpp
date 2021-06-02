/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#include "k3bdevicehandler.h"
#include "k3bprogressinfoevent.h"
#include "k3bthread.h"
#include "k3bdevice.h"
#include "k3bcdtext.h"
#include "k3bcore.h"
#include "k3bmediacache.h"


class K3b::Device::DeviceHandler::Private
{
public:
    Private( bool _selfDelete )
        : selfDelete( _selfDelete ) {
    }

    bool selfDelete;

    bool success;
    Commands command;
    DiskInfo diskInfo;
    Toc toc;
    CdText cdText;
    QByteArray cdTextRaw;
    long long bufferCapacity;
    long long availableBufferCapacity;
    Device* dev;
    K3b::Msf nextWritableAddress;
};


K3b::Device::DeviceHandler::DeviceHandler( Device* dev, QObject* parent )
    : K3b::ThreadJob( 0, parent ),
      d( new Private( false ) )
{
    d->dev = dev;
}


K3b::Device::DeviceHandler::DeviceHandler( QObject* parent )
    : K3b::ThreadJob( 0, parent ),
      d( new Private( false ) )
{
}


K3b::Device::DeviceHandler::DeviceHandler( Commands command, Device* dev )
    : K3b::ThreadJob( 0, 0 ),
      d( new Private( false ) )
{
    d->dev = dev;
    sendCommand(command);
}

K3b::Device::DeviceHandler::~DeviceHandler()
{
    delete d;
}


bool K3b::Device::DeviceHandler::success() const
{
    return d->success;
}


K3b::Device::DiskInfo K3b::Device::DeviceHandler::diskInfo() const
{
    return d->diskInfo;
}


K3b::Device::Toc K3b::Device::DeviceHandler::toc() const
{
    return d->toc;
}

K3b::Device::CdText K3b::Device::DeviceHandler::cdText() const
{
    return d->cdText;
}


QByteArray K3b::Device::DeviceHandler::cdTextRaw() const
{
    return d->cdTextRaw;
}


K3b::Msf K3b::Device::DeviceHandler::diskSize() const
{
    return d->diskInfo.capacity();
}

K3b::Msf K3b::Device::DeviceHandler::remainingSize() const
{
    return d->diskInfo.remainingSize();
}

int K3b::Device::DeviceHandler::tocType() const
{
    return d->toc.contentType();
}

int K3b::Device::DeviceHandler::numSessions() const
{
    return d->diskInfo.numSessions();
}

long long K3b::Device::DeviceHandler::bufferCapacity() const
{
    return d->bufferCapacity;
}

long long K3b::Device::DeviceHandler::availableBufferCapacity() const
{
    return d->availableBufferCapacity;
}

K3b::Msf K3b::Device::DeviceHandler::nextWritableAddress() const
{
    return d->nextWritableAddress;
}

void K3b::Device::DeviceHandler::setDevice( Device* dev )
{
    d->dev = dev;
}


void K3b::Device::DeviceHandler::sendCommand( DeviceHandler::Commands command )
{
    if( active() ) {
        qDebug() << "thread already running. canceling thread...";
        cancel();
        wait();
    }

    d->command = command;
    start();
}

void K3b::Device::DeviceHandler::getToc()
{
    sendCommand(DeviceHandler::CommandToc);
}

void K3b::Device::DeviceHandler::getDiskInfo()
{
    sendCommand(DeviceHandler::CommandDiskInfo);
}

void K3b::Device::DeviceHandler::getDiskSize()
{
    sendCommand(DeviceHandler::CommandDiskSize);
}

void K3b::Device::DeviceHandler::getRemainingSize()
{
    sendCommand(DeviceHandler::CommandRemainingSize);
}

void K3b::Device::DeviceHandler::getTocType()
{
    sendCommand(DeviceHandler::CommandTocType);
}

void K3b::Device::DeviceHandler::getNumSessions()
{
    sendCommand(DeviceHandler::CommandNumSessions);
}


void K3b::Device::DeviceHandler::block( bool b )
{
    sendCommand(b ? DeviceHandler::CommandBlock : DeviceHandler::CommandUnblock);
}

void K3b::Device::DeviceHandler::eject()
{
    sendCommand(DeviceHandler::CommandEject);
}

K3b::Device::DeviceHandler* K3b::Device::sendCommand( DeviceHandler::Commands command, Device* dev )
{
    return new DeviceHandler( command, dev );
}

void K3b::Device::DeviceHandler::jobFinished( bool success )
{
    K3b::ThreadJob::jobFinished( success );

    emit finished( this );

    if( d->selfDelete ) {
        deleteLater();
    }
}


bool K3b::Device::DeviceHandler::run()
{
    qDebug() << "starting command: " << d->command;

    d->success = false;

    // clear data
    d->toc.clear();
    d->diskInfo = DiskInfo();
    d->cdText.clear();
    d->cdTextRaw.clear();

    if( d->dev ) {
        d->success = d->dev->open();
        if( !canceled() && d->command & CommandBlock )
            d->success = (d->success && d->dev->block( true ));

        if( !canceled() && d->command & CommandUnblock )
            d->success = (d->success && d->dev->block( false ));

        //
        // It is important that eject is performed before load
        // since the CommandReload command is a combination of both
        //

        if( !canceled() && d->command & CommandEject ) {
            d->success = (d->success && d->dev->eject());

            // to be on the safe side, especially with respect to the EmptyDiscWaiter
            // we reset the device in the cache.
            k3bcore->mediaCache()->resetDevice( d->dev );
        }

        if( !canceled() && d->command & CommandLoad )
            d->success = (d->success && d->dev->load());

        if( !canceled() && d->command & (CommandDiskInfo|
                                         CommandDiskSize|
                                         CommandRemainingSize|
                                         CommandNumSessions) ) {
            d->diskInfo = d->dev->diskInfo();
        }

        if( !canceled() && d->command & (CommandToc|CommandTocType) ) {
            d->toc = d->dev->readToc();
        }

        if( !canceled() &&
            d->command & CommandCdText &&
              !( d->command & CommandToc &&
                 d->toc.contentType() == DATA )
            ) {
            d->cdText = d->dev->readCdText();
            if ( d->command != CommandMediaInfo )
                d->success = (d->success && !d->cdText.isEmpty());
        }

        if( !canceled() && d->command & CommandCdTextRaw ) {
            bool cdTextSuccess = true;
            d->cdTextRaw = d->dev->readRawCdText( &cdTextSuccess );
            d->success = d->success && cdTextSuccess;
        }

        if( !canceled() && d->command & CommandBufferCapacity )
            d->success = d->dev->readBufferCapacity( d->bufferCapacity, d->availableBufferCapacity );

        if ( !canceled() && d->command & CommandNextWritableAddress ) {
            int nwa = d->dev->nextWritableAddress();
            d->nextWritableAddress = nwa;
            d->success = ( d->success && ( nwa > 0 ) );
        }

        d->dev->close();
    }

    qDebug() << "finished command: " << d->command;

    return d->success;
}


QDebug operator<<( QDebug dbg, K3b::Device::DeviceHandler::Commands commands )
{
    QStringList commandStrings;
    if ( commands & K3b::Device::DeviceHandler::CommandDiskInfo )
        commandStrings << QLatin1String( "CommandDiskInfo" );
    if ( commands & K3b::Device::DeviceHandler::CommandToc )
        commandStrings << QLatin1String( "CommandToc" );
    if ( commands & K3b::Device::DeviceHandler::CommandCdText )
        commandStrings << QLatin1String( "CommandCdText" );
    if ( commands & K3b::Device::DeviceHandler::CommandCdTextRaw )
        commandStrings << QLatin1String( "CommandCdTextRaw" );
    if ( commands & K3b::Device::DeviceHandler::CommandDiskSize )
        commandStrings << QLatin1String( "CommandDiskSize" );
    if ( commands & K3b::Device::DeviceHandler::CommandRemainingSize )
        commandStrings << QLatin1String( "CommandRemainingSize" );
    if ( commands & K3b::Device::DeviceHandler::CommandTocType )
        commandStrings << QLatin1String( "CommandTocType" );
    if ( commands & K3b::Device::DeviceHandler::CommandNumSessions )
        commandStrings << QLatin1String( "CommandNumSessions" );
    if ( commands & K3b::Device::DeviceHandler::CommandBlock )
        commandStrings << QLatin1String( "CommandBlock" );
    if ( commands & K3b::Device::DeviceHandler::CommandUnblock )
        commandStrings << QLatin1String( "CommandUnblock" );
    if ( commands & K3b::Device::DeviceHandler::CommandEject )
        commandStrings << QLatin1String( "CommandEject" );
    if ( commands & K3b::Device::DeviceHandler::CommandLoad )
        commandStrings << QLatin1String( "CommandLoad" );
    if ( commands & K3b::Device::DeviceHandler::CommandBufferCapacity )
        commandStrings << QLatin1String( "CommandBufferCapacity" );
    if ( commands & K3b::Device::DeviceHandler::CommandNextWritableAddress )
        commandStrings << QLatin1String( "CommandNextWritableAddress" );
    dbg.nospace() << '(' + commandStrings.join( "|" ) + ')';
    return dbg.space();
}


