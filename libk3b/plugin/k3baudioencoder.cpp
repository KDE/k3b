/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudioencoder.h"
#include "k3b_i18n.h"

#include <QDebug>
#include <QFile>


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


bool K3b::AudioEncoder::openFile( const QString& extension, const QString& filename, const K3b::Msf& length, const MetaData& metaData )
{
    closeFile();

    d->outputFile = new QFile( filename );
    if( d->outputFile->open( QIODevice::WriteOnly ) ) {
        return initEncoder( extension, length, metaData );
    }
    else {
        qDebug() << "(K3b::AudioEncoder) unable to open file " << filename;
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


qint64 K3b::AudioEncoder::encode( const char* data, qint64 len )
{
    return encodeInternal( data, len );
}


bool K3b::AudioEncoder::initEncoder( const QString& extension, const K3b::Msf& length, const MetaData& metaData )
{
    if( !isOpen() ) {
        qDebug() << "(K3b::AudioEncoder) call to initEncoder without openFile!";
        return false;
    }

    return initEncoderInternal( extension, length, metaData );
}


qint64 K3b::AudioEncoder::writeData( const char* data, qint64 len )
{
    if( d->outputFile ) {
        return d->outputFile->write( data, len );
    }
    else {
        qDebug() << "(K3b::AudioEncoder) call to writeData without opening a file first.";
        return -1;
    }
}


bool K3b::AudioEncoder::initEncoderInternal( const QString& /*extension*/, const K3b::Msf& /*length*/, const MetaData& /*metaData*/ )
{
    // do nothing
    return true;
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

#include "moc_k3baudioencoder.cpp"
