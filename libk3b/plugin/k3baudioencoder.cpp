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

#include "k3baudioencoder.h"

#include <qfile.h>

#include <kdebug.h>


class K3b::AudioEncoder::Private
{
public:
    Private()
        : outputFile(0) {
    }

    QFile* outputFile;
    QString outputFilename;

    QString lastErrorString;
};


K3b::AudioEncoder::AudioEncoder( QObject* parent )
    : K3b::Plugin( parent )
{
    d = new Private();
}


K3b::AudioEncoder::~AudioEncoder()
{
    closeFile();
    delete d;
}


QString K3b::AudioEncoder::categoryName() const
{
    return i18nc( "plugin type", "Audio Encoder" );
}


bool K3b::AudioEncoder::openFile( const QString& ext, const QString& filename, const K3b::Msf& length )
{
    closeFile();

    d->outputFile = new QFile( filename );
    if( d->outputFile->open( QIODevice::WriteOnly ) ) {
        return initEncoder( ext, length );
    }
    else {
        kDebug() << "(K3b::AudioEncoder) unable to open file " << filename;
        closeFile();
        return false;
    }
}


bool K3b::AudioEncoder::isOpen() const
{
    if( d->outputFile )
        return d->outputFile->isOpen();
    else
        return false;
}


void K3b::AudioEncoder::closeFile()
{
    if( d->outputFile ) {
        finishEncoder();
        if( d->outputFile->isOpen() )
            d->outputFile->close();
        delete d->outputFile;
        d->outputFile = 0;
        d->outputFilename = QString();
    }
}


QString K3b::AudioEncoder::filename() const
{
    if( d->outputFile )
        return d->outputFilename;
    else
        return QString();
}



void K3b::AudioEncoder::setMetaData( K3b::AudioEncoder::MetaDataField f, const QString& data )
{
    if( !data.isEmpty() )
        return setMetaDataInternal( f, data );
}


long K3b::AudioEncoder::encode( const char* data, Q_ULONG len )
{
    return encodeInternal( data, len );
}


bool K3b::AudioEncoder::initEncoder( const QString& ext, const K3b::Msf& length )
{
    if( !isOpen() ) {
        kDebug() << "(K3b::AudioEncoder) call to initEncoder without openFile!";
        return false;
    }

    return initEncoderInternal( ext, length );
}


Q_LONG K3b::AudioEncoder::writeData( const char* data, Q_ULONG len )
{
    if( d->outputFile ) {
        return d->outputFile->write( data, len );
    }
    else {
        kDebug() << "(K3b::AudioEncoder) call to writeData without opening a file first.";
        return -1;
    }
}


bool K3b::AudioEncoder::initEncoderInternal( const QString&, const K3b::Msf& )
{
    // do nothing
    return true;
}


void K3b::AudioEncoder::setMetaDataInternal( K3b::AudioEncoder::MetaDataField, const QString& )
{
    // do nothing
}


void K3b::AudioEncoder::finishEncoder()
{
    if( isOpen() )
        finishEncoderInternal();
}


void K3b::AudioEncoder::finishEncoderInternal()
{
    // do nothing
}


void K3b::AudioEncoder::setLastError( const QString& e )
{
    d->lastErrorString = e;
}


QString K3b::AudioEncoder::lastErrorString() const
{
    if( d->lastErrorString.isEmpty() )
        return i18n("An unknown error occurred.");
    else
        return d->lastErrorString;
}

#include "k3baudioencoder.moc"
