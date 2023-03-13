/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bexternalencoder.h"
#include "k3bexternalencodercommand.h"
#include "k3bplugin_i18n.h"

#include <config-k3b.h>

#include "k3bcore.h"
#include "k3bprocess.h"

#include <KConfig>

#include <QDebug>
#include <QFile>
#include <QList>
#include <QRegularExpression>
#include <QStandardPaths>

#include <sys/types.h>


K_PLUGIN_CLASS_WITH_JSON(K3bExternalEncoder, "k3bexternalencoder.json")


Q_DECLARE_METATYPE( QProcess::ExitStatus )


static const unsigned char s_riffHeader[] =
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

    qDebug() << "(K3bExternalEncoder) could not find command for extension " << extension;

    return K3bExternalEncoderCommand();
}


class K3bExternalEncoder::Private
{
public:
    K3b::Process* process;
    QString fileName;
    K3b::Msf length;

    K3bExternalEncoderCommand cmd;

    bool initialized;
};


K3bExternalEncoder::K3bExternalEncoder( QObject* parent, const QVariantList& )
    : K3b::AudioEncoder( parent ),
      d( new Private() )
{
    d->process = 0;
    d->initialized = false;

    qRegisterMetaType<QProcess::ExitStatus>();
}


K3bExternalEncoder::~K3bExternalEncoder()
{
    if( d->process ) {
        disconnect( d->process );
        d->process->deleteLater();
    }
    delete d;
}


void K3bExternalEncoder::finishEncoderInternal()
{
    if( d->process && d->process->state() == QProcess::Running ) {
        d->process->closeWriteChannel();

        // this is kind of evil...
        // but we need to be sure the process exited when this method returns
        d->process->waitForFinished(-1);
    }
    d->initialized = false;
}


void K3bExternalEncoder::slotExternalProgramFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    if( (exitStatus != QProcess::NormalExit) || (exitCode != 0) )
        qDebug() << "(K3bExternalEncoder) program exited with error.";
}


bool K3bExternalEncoder::openFile( const QString& ext, const QString& filename, const K3b::Msf& length, const MetaData& metaData )
{
    d->fileName = filename;
    d->length = length;

    // delete existing files as some programs (like flac for example) might refuse to overwrite files
    if( QFile::exists( filename ) )
        QFile::remove( filename );

    return initEncoderInternal( ext, length, metaData );
}


bool K3bExternalEncoder::isOpen() const
{
    return d->initialized;
}


void K3bExternalEncoder::closeFile()
{
    finishEncoderInternal();
}


bool K3bExternalEncoder::initEncoderInternal( const QString& extension, const K3b::Msf& /*length*/, const MetaData& metaData )
{
    // find the correct command
    d->cmd = commandByExtension( extension );

    if( !d->cmd.command.isEmpty() ) {
        // create the commandline
        QStringList params = d->cmd.command.split( ' ' );
        for( QStringList::iterator it = params.begin(); it != params.end(); ++it ) {
            (*it).replace( "%f", d->fileName );
            (*it).replace( "%a", metaData[META_TRACK_ARTIST].toString() );
            (*it).replace( "%t", metaData[META_TRACK_TITLE].toString() );
            (*it).replace( "%c", metaData[META_TRACK_COMMENT].toString() );
            (*it).replace( "%y", metaData[META_YEAR].toString() );
            (*it).replace( "%m", metaData[META_ALBUM_TITLE].toString() );
            (*it).replace( "%r", metaData[META_ALBUM_ARTIST].toString() );
            (*it).replace( "%x", metaData[META_ALBUM_COMMENT].toString() );
            (*it).replace( "%n", metaData[META_TRACK_NUMBER].toString() );
            (*it).replace( "%g", metaData[META_GENRE].toString() );
        }


        qDebug() << "***** external parameters:";
        qDebug() << params.join( " " ) << Qt::flush;

        // set one general error message
        setLastError( i18n("Command failed: %1", params.join( " " ) ) );

        // always create a new process since we are called in a separate thread
        if( d->process ) {
            disconnect( d->process );
            d->process->deleteLater();
        }
        d->process = new K3b::Process();
        d->process->setSplitStdout( true );
        connect( d->process, SIGNAL(finished(int,QProcess::ExitStatus)),
                this, SLOT(slotExternalProgramFinished(int,QProcess::ExitStatus)) );
        connect( d->process, SIGNAL(stderrLine(QString)),
                 this, SLOT(slotExternalProgramOutput(QString)) );
        connect( d->process, SIGNAL(stdoutLine(QString)),
                 this, SLOT(slotExternalProgramOutput(QString)) );

        d->process->setProgram( params );
        d->process->start( KProcess::SeparateChannels );

        if( d->process->waitForStarted() ) {
            if( d->cmd.writeWaveHeader )
                d->initialized = writeWaveHeader();
            else
                d->initialized = true;
        }
        else {
            static const QRegularExpression rx("\\s+");
            QString commandName = d->cmd.command.section( rx, 0 );
            if( !QStandardPaths::findExecutable( commandName ).isEmpty() )
                setLastError( i18n("Could not find program '%1'",commandName) );

            d->initialized = false;
        }

    }
    else {
        setLastError( i18n("Invalid command: the command is empty.") );
        d->initialized = false;
    }

    return d->initialized;
}


bool K3bExternalEncoder::writeWaveHeader()
{
    qDebug() << "(K3bExternalEncoder) writing wave header";

    // write the RIFF thing
    if( d->process->write( (const char*) s_riffHeader, 4 ) != 4 ) {
        qDebug() << "(K3bExternalEncoder) failed to write riff header.";
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
        qDebug() << "(K3bExternalEncoder) failed to write wave size.";
        return false;
    }

    // write static part of the header
    if( d->process->write( (const char*) s_riffHeader + 8, 32 ) != 32 ) {
        qDebug() << "(K3bExternalEncoder) failed to write wave header.";
        return false;
    }

    c[0] = (dataSize   >> 0 ) & 0xff;
    c[1] = (dataSize   >> 8 ) & 0xff;
    c[2] = (dataSize   >> 16) & 0xff;
    c[3] = (dataSize   >> 24) & 0xff;

    if( d->process->write( c, 4 ) != 4 ) {
        qDebug() << "(K3bExternalEncoder) failed to write data size.";
        return false;
    }

    return d->process->waitForBytesWritten( -1 );
}


qint64 K3bExternalEncoder::encodeInternal( const char* data, qint64 len )
{
    if( !d->initialized )
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


void K3bExternalEncoder::slotExternalProgramOutput( const QString& line )
{
    qDebug() << "(" << d->cmd.name << ") " << line;
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
