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


#include "k3bdevicehandler.h"
#include <k3bprogressinfoevent.h>
#include <k3bthread.h>
#include <k3bdevice.h>
#include <k3bcdtext.h>


class K3b::Device::DeviceHandler::Private
{
public:
    Private( bool _selfDelete )
        : selfDelete( _selfDelete ) {
    }

    bool selfDelete;

    bool success;
    int errorCode;
    int command;
    DiskInfo ngInfo;
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


K3b::Device::DeviceHandler::DeviceHandler( int command, Device* dev )
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


int K3b::Device::DeviceHandler::errorCode() const
{
    return d->errorCode;
}

bool K3b::Device::DeviceHandler::success() const
{
    return d->success;
}


const K3b::Device::DiskInfo& K3b::Device::DeviceHandler::diskInfo() const
{
    return d->ngInfo;
}


const K3b::Device::Toc& K3b::Device::DeviceHandler::toc() const
{
    return d->toc;
}

const K3b::Device::CdText& K3b::Device::DeviceHandler::cdText() const
{
    return d->cdText;
}


const QByteArray& K3b::Device::DeviceHandler::cdTextRaw() const
{
    return d->cdTextRaw;
}


K3b::Msf K3b::Device::DeviceHandler::diskSize() const
{
    return d->ngInfo.capacity();
}

K3b::Msf K3b::Device::DeviceHandler::remainingSize() const
{
    return d->ngInfo.remainingSize();
}

int K3b::Device::DeviceHandler::tocType() const
{
    return d->toc.contentType();
}

int K3b::Device::DeviceHandler::numSessions() const
{
    return d->ngInfo.numSessions();
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


void K3b::Device::DeviceHandler::sendCommand( int command )
{
    if( active() ) {
        kDebug() << "(K3b::Device::DeviceHandler) thread already running. canceling thread...";
        cancel();
        wait();
    }

    d->command = command;
    start();
}

void K3b::Device::DeviceHandler::getToc()
{
    sendCommand(DeviceHandler::TOC);
}

void K3b::Device::DeviceHandler::getDiskInfo()
{
    sendCommand(DeviceHandler::DISKINFO);
}

void K3b::Device::DeviceHandler::getDiskSize()
{
    sendCommand(DeviceHandler::DISKSIZE);
}

void K3b::Device::DeviceHandler::getRemainingSize()
{
    sendCommand(DeviceHandler::REMAININGSIZE);
}

void K3b::Device::DeviceHandler::getTocType()
{
    sendCommand(DeviceHandler::TOCTYPE);
}

void K3b::Device::DeviceHandler::getNumSessions()
{
    sendCommand(DeviceHandler::NUMSESSIONS);
}


void K3b::Device::DeviceHandler::block( bool b )
{
    sendCommand(b ? DeviceHandler::BLOCK : DeviceHandler::UNBLOCK);
}

void K3b::Device::DeviceHandler::eject()
{
    sendCommand(DeviceHandler::EJECT);
}

K3b::Device::DeviceHandler* K3b::Device::sendCommand( int command, Device* dev )
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
    kDebug() << "(K3b::Device::DeviceHandler) starting command: " << d->command;

    d->success = false;

    // clear data
    d->toc.clear();
    d->ngInfo = DiskInfo();
    d->cdText.clear();
    d->cdTextRaw.resize(0);

    if( d->dev ) {
        d->success = d->dev->open();
        if( !canceled() && d->command & DISKINFO ) {
            d->ngInfo = d->dev->diskInfo();
            if( !canceled() && !d->ngInfo.empty() ) {
                d->toc = d->dev->readToc();
                if( d->toc.contentType() == AUDIO ||
                    d->toc.contentType() == MIXED )
                    d->cdText = d->dev->readCdText();
            }
        }

        if( !canceled() && d->command & (NG_DISKINFO|
                                         DISKSIZE|
                                         REMAININGSIZE|
                                         NUMSESSIONS) ) {
            d->ngInfo = d->dev->diskInfo();
        }

        if( !canceled() && d->command & (TOC|TOCTYPE) ) {
            d->toc = d->dev->readToc();
        }

        if( !canceled() && d->command & CD_TEXT ) {
            d->cdText = d->dev->readCdText();
            d->success = (d->success && !d->cdText.isEmpty());
        }

        if( !canceled() && d->command & CD_TEXT_RAW ) {
            unsigned char* data = 0;
            unsigned int dataLen = 0;
            if( d->dev->readTocPmaAtip( &data, dataLen, 5, false, 0 ) ) {
                // we need more than the header and a multiple of 18 bytes to have valid CD-TEXT
                if( dataLen > 4 && dataLen%18 == 4 ) {
                    d->cdTextRaw = QByteArray::fromRawData( reinterpret_cast<char*>(data), dataLen );
                }
                else {
                    kDebug() << "(K3b::Device::DeviceHandler) invalid CD-TEXT length: " << dataLen;
                    delete [] data;
                    d->success = false;
                }
            }
            else
                d->success = false;
        }

        if( !canceled() && d->command & BLOCK )
            d->success = (d->success && d->dev->block( true ));

        if( !canceled() && d->command & UNBLOCK )
            d->success = (d->success && d->dev->block( false ));

        //
        // It is important that eject is performed before load
        // since the RELOAD command is a combination of both
        //

        if( !canceled() && d->command & EJECT )
            d->success = (d->success && d->dev->eject());

        if( !canceled() && d->command & LOAD )
            d->success = (d->success && d->dev->load());

        if( !canceled() && d->command & BUFFER_CAPACITY )
            d->success = d->dev->readBufferCapacity( d->bufferCapacity, d->availableBufferCapacity );

        if ( !canceled() && d->command & NEXT_WRITABLE_ADDRESS ) {
            int nwa = d->dev->nextWritableAddress();
            d->nextWritableAddress = nwa;
            d->success = ( d->success && ( nwa > 0 ) );
        }

        d->dev->close();
    }

    kDebug() << "(K3b::Device::DeviceHandler) finished command: " << d->command;

    return d->success;
}

#include "k3bdevicehandler.moc"
