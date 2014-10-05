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


#include "k3bmd5job.h"
#include "k3biso9660.h"
#include "k3bglobals.h"
#include "k3bdevice.h"
#include "k3bfilesplitter.h"
#include "k3b_i18n.h"

#include <KCodecs/KCodecs>

#include <QtCore/QCryptographicHash>
#include <QtCore/QDebug>
#include <QtCore/QIODevice>
#include <QtCore/QTimer>


class K3b::Md5Job::Private
{
public:
    Private()
		: md5(QCryptographicHash::Md5),
		  ioDevice(0),
          finished(true),
          data(0),
          isoFile(0),
          maxSize(0),
          lastProgress(0) {
    }

	QCryptographicHash md5;
    K3b::FileSplitter file;
    QTimer timer;
    QString filename;
    QIODevice* ioDevice;
    K3b::Device::Device* device;

    bool finished;
    char* data;
    const K3b::Iso9660File* isoFile;

    qint64 maxSize;
    qint64 readData;

    int lastProgress;

    KIO::filesize_t imageSize;

    static const int BUFFERSIZE = 2048*10;
};


K3b::Md5Job::Md5Job( K3b::JobHandler* jh, QObject* parent )
    : K3b::Job( jh, parent ),
      d( new Private() )
{
    d->data = new char[Private::BUFFERSIZE];
    connect( &d->timer, SIGNAL(timeout()),
             this, SLOT(slotUpdate()) );
}


K3b::Md5Job::~Md5Job()
{
    delete [] d->data;
    delete d;
}


void K3b::Md5Job::start()
{
    cancel();

    jobStarted();
    d->readData = 0;

    if( d->isoFile ) {
        d->imageSize = d->isoFile->size();
    }
    else if( !d->filename.isEmpty() ) {
        if( !QFile::exists( d->filename ) ) {
            emit infoMessage( i18n("Could not find file %1",d->filename), MessageError );
            jobFinished(false);
            return;
        }

        d->file.setName( d->filename );
        if( !d->file.open( QIODevice::ReadOnly ) ) {
            emit infoMessage( i18n("Could not open file %1",d->filename), MessageError );
            jobFinished(false);
            return;
        }

        d->imageSize = K3b::filesize( d->filename );
    }
    else
        d->imageSize = 0;

    if( d->device ) {
        //
        // Let the drive determine the optimal reading speed
        //
        d->device->setSpeed( 0xffff, 0xffff );
    }

    d->md5.reset();
    d->finished = false;
    if( d->ioDevice )
        connect( d->ioDevice, SIGNAL(readyRead()), this, SLOT(slotUpdate()) );
    else
        d->timer.start(0);
}


void K3b::Md5Job::cancel()
{
    if( !d->finished ) {
        stopAll();

        emit canceled();
        jobFinished( false );
    }
}


void K3b::Md5Job::setFile( const QString& filename )
{
    d->filename = filename;
    d->isoFile = 0;
    d->ioDevice = 0;
    d->device = 0;
}


void K3b::Md5Job::setFile( const K3b::Iso9660File* file )
{
    d->isoFile = file;
    d->ioDevice = 0;
    d->filename.truncate(0);
    d->device = 0;
}


void K3b::Md5Job::setIODevice( QIODevice* dev )
{
    d->ioDevice = dev;
    d->filename.truncate(0);
    d->isoFile = 0;
    d->device = 0;
}


void K3b::Md5Job::setDevice( K3b::Device::Device* dev )
{
    d->device = dev;
    d->ioDevice = 0;
    d->filename.truncate(0);
    d->isoFile = 0;
}


void K3b::Md5Job::setMaxReadSize( qint64 size )
{
    d->maxSize = size;
}


void K3b::Md5Job::slotUpdate()
{
    if( !d->finished ) {

        // determine bytes to read
        qint64 readSize = Private::BUFFERSIZE;
        if( d->maxSize > 0 )
            readSize = qMin( readSize, d->maxSize - d->readData );

        if( readSize <= 0 ) {
            //      qDebug() << "(K3b::Md5Job) reached max size of " << d->maxSize << ". Stopping.";
            emit debuggingOutput( "K3b::Md5Job", QString("Reached max read of %1. Stopping after %2 bytes.").arg(d->maxSize).arg(d->readData) );
            stopAll();
            emit percent( 100 );
            jobFinished(true);
        }
        else {
            int read = 0;

            //
            // read from the iso9660 file
            //
            if( d->isoFile ) {
                read = d->isoFile->read( d->readData, d->data, readSize );
            }

            //
            // read from the device
            //
            else if( d->device ) {
                //
                // when reading from a device we always read multiples of 2048 bytes.
                // Only the last sector may not be used completely.
                //
                qint64 sector = d->readData/2048;
                qint64 sectorCnt = qMax( readSize/2048, ( qint64 )1 );
                read = -1;
                if( d->device->read10( reinterpret_cast<unsigned char*>(d->data),
                                       sectorCnt*2048,
                                       sector,
                                       sectorCnt ) )
                    read = qMin( readSize, sectorCnt*2048 );
            }

            //
            // read from the file
            //
            else if( !d->ioDevice ) {
                read = d->file.read( d->data, readSize );
            }

            //
            // reading from the io device
            //
            else {
                read = d->ioDevice->read( d->data, readSize );
            }

            if( read < 0 ) {
                emit infoMessage( i18n("Error while reading from file %1", d->filename), MessageError );
                stopAll();
                jobFinished(false);
            }
            else if( read == 0 ) {
                //	qDebug() << "(K3b::Md5Job) read all data. Total size: " << d->readData << ". Stopping.";
                emit debuggingOutput( "K3b::Md5Job", QString("All data read. Stopping after %1 bytes.").arg(d->readData) );
                stopAll();
                emit percent( 100 );
                jobFinished(true);
            }
            else {
                d->readData += read;
				d->md5.addData( d->data, read );
                int progress = 0;
                if( d->isoFile || !d->filename.isEmpty() )
                    progress = (int)((double)d->readData * 100.0 / (double)d->imageSize);
                else if( d->maxSize > 0 )
                    progress = (int)((double)d->readData * 100.0 / (double)d->maxSize);

                if( progress != d->lastProgress ) {
                    d->lastProgress = progress;
                    emit percent( progress );
                }
            }
        }
    }
}


QByteArray K3b::Md5Job::hexDigest()
{
    if( d->finished )
		return d->md5.result().toHex();
    else
        return "";
}


QByteArray K3b::Md5Job::base64Digest()
{
	if( d->finished )
		return d->md5.result().toBase64();
	else
		return "";
}


void K3b::Md5Job::stop()
{
    emit debuggingOutput( "K3b::Md5Job", QString("Stopped manually after %1 bytes.").arg(d->readData) );
    stopAll();
    jobFinished( true );
}


void K3b::Md5Job::stopAll()
{
    if( d->ioDevice )
        disconnect( d->ioDevice, SIGNAL(readyRead()), this, SLOT(slotUpdate()) );
    if( d->file.isOpen() )
        d->file.close();
    d->timer.stop();
    d->finished = true;
}


