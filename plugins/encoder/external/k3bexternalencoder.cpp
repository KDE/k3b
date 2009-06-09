/*
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

#include "k3bexternalencoder.h"
#include "k3bexternalencodercommand.h"

#include <config-k3b.h>

#include "k3bcore.h"

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <KProcess>

#include <QtCore/QRegExp>
#include <QtCore/QFile>
#include <QtCore/QList>

#include <sys/types.h>


K3B_EXPORT_PLUGIN(k3bexternalencoder, K3bExternalEncoder)

Q_DECLARE_METATYPE( QProcess::ExitStatus )


static const char s_riffHeader[] =
{
    0x52, 0x49, 0x46, 0x46, // 0  "RIFF"
    0x00, 0x00, 0x00, 0x00, // 4  wavSize
    0x57, 0x41, 0x56, 0x45, // 8  "WAVE"
    0x66, 0x6d, 0x74, 0x20, // 12 "fmt "
    0x10, 0x00, 0x00, 0x00, // 16
    0x01, 0x00, 0x02, 0x00, // 20
    0x44, 0xac, 0x00, 0x00, // 24
    0x10, 0xb1, 0x02, 0x00, // 28
    0x04, 0x00, 0x10, 0x00, // 32
    0x64, 0x61, 0x74, 0x61, // 36 "data"
    0x00, 0x00, 0x00, 0x00  // 40 byteCount
};





static K3bExternalEncoderCommand commandByExtension( const QString& extension )
{
    QList<K3bExternalEncoderCommand> cmds( K3bExternalEncoderCommand::readCommands() );
    for( QList<K3bExternalEncoderCommand>::iterator it = cmds.begin(); it != cmds.end(); ++it )
        if( (*it).extension == extension )
            return *it;

    kDebug() << "(K3bExternalEncoder) could not find command for extension " << extension;

    return K3bExternalEncoderCommand();
}


class K3bExternalEncoder::Private
{
public:
    KProcess* process;
    QString fileName;
    QString extension;
    K3b::Msf length;

    K3bExternalEncoderCommand cmd;

    bool initialized;

    // the metaData we support
    QString artist;
    QString title;
    QString comment;
    QString trackNumber;
    QString cdArtist;
    QString cdTitle;
    QString cdComment;
    QString year;
    QString genre;
};


K3bExternalEncoder::K3bExternalEncoder( QObject* parent, const QVariantList& )
    : K3b::AudioEncoder( parent ),
      d( new Private() )
{
    d->process = 0;

    qRegisterMetaType<QProcess::ExitStatus>();
}


K3bExternalEncoder::~K3bExternalEncoder()
{
    delete d->process;
    delete d;
}


void K3bExternalEncoder::setMetaDataInternal( K3b::AudioEncoder::MetaDataField f, const QString& value )
{
    switch( f ) {
    case META_TRACK_TITLE:
        d->title = value;
        break;
    case META_TRACK_ARTIST:
        d->artist = value;
        break;
    case META_TRACK_COMMENT:
        d->comment = value;
        break;
    case META_TRACK_NUMBER:
        d->trackNumber = value;
        break;
    case META_ALBUM_TITLE:
        d->cdTitle = value;
        break;
    case META_ALBUM_ARTIST:
        d->cdArtist = value;
        break;
    case META_ALBUM_COMMENT:
        d->cdComment = value;
        break;
    case META_YEAR:
        d->year = value;
        break;
    case META_GENRE:
        d->genre = value;
        break;
    }
}


void K3bExternalEncoder::finishEncoderInternal()
{
    if( d->process->state() == QProcess::Running ) {
        d->process->closeWriteChannel();

        // this is kind of evil...
        // but we need to be sure the process exited when this method returnes
        d->process->waitForFinished(-1);
    }
}


void K3bExternalEncoder::slotExternalProgramFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    if( (exitStatus != QProcess::NormalExit) || (exitCode != 0) )
        kDebug() << "(K3bExternalEncoder) program exited with error.";
}


bool K3bExternalEncoder::openFile( const QString& ext, const QString& filename, const K3b::Msf& length )
{
    d->fileName = filename;
    d->extension = ext;
    d->initialized = false;
    d->length = length;

    // delete existing files as some programs (like flac for example) might refuse to overwrite files
    if( QFile::exists( filename ) )
        QFile::remove( filename );

    return true;
}


void K3bExternalEncoder::closeFile()
{
    finishEncoderInternal();
}


bool K3bExternalEncoder::initExternalEncoder( const QString& extension )
{
    d->initialized = true;

    // find the correct command
    d->cmd = commandByExtension( extension );

    if( d->cmd.command.isEmpty() ) {
        setLastError( i18n("Invalid command: the command is empty.") );
        return false;
    }

    // create the commandline
    QStringList params = d->cmd.command.split( ' ' );
    for( QStringList::iterator it = params.begin(); it != params.end(); ++it ) {
        (*it).replace( "%f", d->fileName );
        (*it).replace( "%a", d->artist );
        (*it).replace( "%t", d->title );
        (*it).replace( "%c", d->comment );
        (*it).replace( "%y", d->year );
        (*it).replace( "%m", d->cdTitle );
        (*it).replace( "%r", d->cdArtist );
        (*it).replace( "%x", d->cdComment );
        (*it).replace( "%n", d->trackNumber );
        (*it).replace( "%g", d->genre );
    }


    kDebug() << "***** external parameters:";
    kDebug() << params.join( " " ) << flush;

    // set one general error message
    setLastError( i18n("Command failed: %1", params.join( " " ) ) );

    // always create a new process since we are called in a separate thread
    delete d->process;
    d->process = new KProcess();
    d->process->setOutputChannelMode( KProcess::MergedChannels );
    connect( d->process, SIGNAL(finished(int, QProcess::ExitStatus)),
             this, SLOT(slotExternalProgramFinished(int, QProcess::ExitStatus)) );
    connect( d->process, SIGNAL(readyRead()),
             this, SLOT(slotExternalProgramOutput()) );

    d->process->setProgram( params );
    d->process->start();

    if( d->process->waitForStarted() ) {
        if( d->cmd.writeWaveHeader )
            return writeWaveHeader();
        else
            return true;
    }
    else {
        QString commandName = d->cmd.command.section( QRegExp("\\s+"), 0 );
        if( !KStandardDirs::findExe( commandName ).isEmpty() )
            setLastError( i18n("Could not find program '%1'",commandName) );

        return false;
    }
}


bool K3bExternalEncoder::writeWaveHeader()
{
    kDebug() << "(K3bExternalEncoder) writing wave header";

    // write the RIFF thing
    if( d->process->write( s_riffHeader, 4 ) != 4 ) {
        kDebug() << "(K3bExternalEncoder) failed to write riff header.";
        return false;
    }

    // write the wave size
    qint32 dataSize( d->length.audioBytes() );
    qint32 wavSize( dataSize + 44 - 8 );
    char c[4];

    c[0] = (wavSize   >> 0 ) & 0xff;
    c[1] = (wavSize   >> 8 ) & 0xff;
    c[2] = (wavSize   >> 16) & 0xff;
    c[3] = (wavSize   >> 24) & 0xff;

    if( d->process->write( c, 4 ) != 4 ) {
        kDebug() << "(K3bExternalEncoder) failed to write wave size.";
        return false;
    }

    // write static part of the header
    if( d->process->write( s_riffHeader + 8, 32 ) != 32 ) {
        kDebug() << "(K3bExternalEncoder) failed to write wave header.";
        return false;
    }

    c[0] = (dataSize   >> 0 ) & 0xff;
    c[1] = (dataSize   >> 8 ) & 0xff;
    c[2] = (dataSize   >> 16) & 0xff;
    c[3] = (dataSize   >> 24) & 0xff;

    if( d->process->write( c, 4 ) != 4 ) {
        kDebug() << "(K3bExternalEncoder) failed to write data size.";
        return false;
    }

    return d->process->waitForBytesWritten( -1 );
}


long K3bExternalEncoder::encodeInternal( const char* data, Q_ULONG len )
{
    if( !d->initialized )
        if( !initExternalEncoder( d->extension ) )
            return -1;

    if( d->process->state() == QProcess::Running ) {

        long written = 0;

        if( d->cmd.swapByteOrder ) {
            char* buffer = new char[len];
            for( unsigned int i = 0; i < len-1; i+=2 ) {
                buffer[i] = data[i+1];
                buffer[i+1] = data[i];
            }

            written = d->process->write( buffer, len );
            delete [] buffer;
        }
        else
            written = d->process->write( data, len );

        d->process->waitForBytesWritten( -1 );

        return written;
    }
    else
        return -1;
}


void K3bExternalEncoder::slotExternalProgramOutput()
{
    while ( d->process->canReadLine() )
        kDebug() << "(" << d->cmd.name << ") " << d->process->readLine();
}


QStringList K3bExternalEncoder::extensions() const
{
    QStringList el;
    QList<K3bExternalEncoderCommand> cmds( K3bExternalEncoderCommand::readCommands() );
    for( QList<K3bExternalEncoderCommand>::iterator it = cmds.begin(); it != cmds.end(); ++it )
        el.append( (*it).extension );

    return el;
}


QString K3bExternalEncoder::fileTypeComment( const QString& ext ) const
{
    return commandByExtension( ext ).name;
}

#include "k3bexternalencoder.moc"
