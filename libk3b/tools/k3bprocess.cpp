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



#include "k3bprocess.h"
#include "k3bexternalbinmanager.h"

#include <qbytearray.h>
#include <qstringlist.h>
#include <qapplication.h>

#include <kdebug.h>


namespace {
    QStringList splitOutput( const QByteArray& data,
                             QString& unfinishedLine,
                             bool suppressEmptyLines )
    {
        //
        // The stderr splitting is mainly used for parsing of messages
        // That's why we simplify the data before proceeding
        //
        int len = data.length();

        QByteArray buffer;
        for( int i = 0; i < len; i++ ) {
            if( data[i] == '\b' ) {
                while( i < len &&
                       data[i] == '\b' )  // we replace multiple backspaces with a single line feed
                    i++;
                buffer += '\n';
            }
            if ( i < len ) {
                if( data[i] == '\r' )
                    buffer += '\n';
                else if( data[i] == '\t' )  // replace tabs with a single space
                    buffer += " ";
                else
                    buffer += data[i];
            }
        }

        QStringList lines = QString::fromLocal8Bit( buffer ).split( '\n', suppressEmptyLines ? QString::SkipEmptyParts : QString::KeepEmptyParts );

        // in case we suppress empty lines we need to handle the following separately
        // to make sure we join unfinished lines correctly
        if( suppressEmptyLines && buffer.startsWith( '\n' ) )
            lines.prepend( QString() );

        if( !unfinishedLine.isEmpty() ) {
            lines.first().prepend( unfinishedLine );
            unfinishedLine.truncate(0);

            kDebug() << "(K3b::Process)           joined line: '" << (lines.first()) << "'";
        }

        QStringList::iterator it;

        // check if line ends with a newline
        // if not save the last line because it is not finished
        if ( !buffer.isEmpty() ) {
            QByteRef c = buffer[buffer.length()-1];
            bool hasUnfinishedLine = ( c != '\n' && c != '\r' && QChar( c ) != QChar(46) );  // What is unicode 46?? It is printed as a point
            if( hasUnfinishedLine ) {
                kDebug() << "(K3b::Process) found unfinished line: '" << lines.last() << "'";
                kDebug() << "(K3b::Process)             last char: '" << buffer.right(1) << "'";
                unfinishedLine = lines.takeLast();
            }
        }

        return lines;
    }
}


class K3b::Process::Private
{
public:
    QString unfinishedStdoutLine;
    QString unfinishedStderrLine;

    bool suppressEmptyLines;

    bool bSplitStdout;
};


K3b::Process::Process( QObject* parent )
    : K3bKProcess( parent ),
      d( new Private() )
{
    setNextOpenMode( ReadWrite|Unbuffered );
    d->suppressEmptyLines = true;
    d->bSplitStdout = false;

    connect( this, SIGNAL(readyReadStandardError()),
             this, SLOT(slotReadyReadStandardError()) );
    connect( this, SIGNAL(readyReadStandardOutput()),
             this, SLOT(slotReadyReadStandardOutput()) );
}


K3b::Process::~Process()
{
    delete d;
}


K3b::Process& K3b::Process::operator<<( const K3b::ExternalBin* bin )
{
    return static_cast<Process&>( K3bKProcess::operator<<( bin->path() ) );
}


K3b::Process& K3b::Process::operator<<( const char* arg )
{
    return static_cast<Process&>( K3bKProcess::operator<<( QLatin1String( arg ) ) );
}


K3b::Process& K3b::Process::operator<<( const QByteArray& arg )
{
    return static_cast<Process&>( K3bKProcess::operator<<( QLatin1String( arg ) ) );
}


K3b::Process& K3b::Process::operator<<( const QLatin1String& arg )
{
    return static_cast<Process&>( K3bKProcess::operator<<( arg ) );
}


void K3b::Process::setSplitStdout( bool b )
{
    d->bSplitStdout = b;
}


void K3b::Process::slotReadyReadStandardOutput()
{
    if( d->bSplitStdout ) {
        QStringList lines = splitOutput( readAllStandardOutput(), d->unfinishedStdoutLine, d->suppressEmptyLines );

        for( QStringList::iterator it = lines.begin(); it != lines.end(); ++it ) {
            QString& str = *it;

            // just to be sure since something in splitOutput does not do this right
            if( d->suppressEmptyLines && str.isEmpty() )
                continue;

            emit stdoutLine( str );
        }
    }
}


void K3b::Process::slotReadyReadStandardError()
{
    QStringList lines = splitOutput( readAllStandardError(), d->unfinishedStderrLine, d->suppressEmptyLines );

    for( QStringList::iterator it = lines.begin(); it != lines.end(); ++it ) {
        QString& str = *it;

        // just to be sure since something in splitOutput does not do this right
        if( d->suppressEmptyLines && str.isEmpty() )
            continue;

        emit stderrLine( str );
    }
}


void K3b::Process::setSuppressEmptyLines( bool b )
{
    d->suppressEmptyLines = b;
}


QString K3b::Process::joinedArgs()
{
    return program().join( " " );
}


void K3b::Process::close()
{
    kDebug();
    closeWriteChannel();
    closeReadChannel( QProcess::StandardOutput );
}


bool K3b::Process::start( KProcess::OutputChannelMode mode )
{
    kDebug();
    setOutputChannelMode( mode );
    K3bKProcess::start();
    kDebug() << "started";
    return K3bQProcess::waitForStarted();
}

#include "k3bprocess.moc"
