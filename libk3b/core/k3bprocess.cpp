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
#include <qlist.h>
#include <qstringlist.h>
#include <qsocketnotifier.h>
#include <qapplication.h>

#include <kdebug.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


class K3bProcess::Data
{
public:
    QString unfinishedStdoutLine;
    QString unfinishedStderrLine;

    int dupStdoutFd;
    int dupStdinFd;

    bool rawStdin;
    bool rawStdout;

    int in[2];
    int out[2];

    bool suppressEmptyLines;
};


K3bProcess::K3bProcess()
    : K3Process(),
      m_bSplitStdout(false)
{
    d = new Data();
    d->dupStdinFd = d->dupStdoutFd = -1;
    d->rawStdout = d->rawStdin = false;
    d->in[0] = d->in[1] = -1;
    d->out[0] = d->out[1] = -1;
    d->suppressEmptyLines = true;
}

K3bProcess::~K3bProcess()
{
    delete d;
}


K3bProcess& K3bProcess::operator<<( const K3bExternalBin* bin )
{
    return this->operator<<( bin->path );
}

K3bProcess& K3bProcess::operator<<( const QString& arg )
{
    static_cast<K3Process*>(this)->operator<<( arg );
    return *this;
}

K3bProcess& K3bProcess::operator<<( const char* arg )
{
    static_cast<K3Process*>(this)->operator<<( arg );
    return *this;
}

K3bProcess& K3bProcess::operator<<( const QByteArray& arg )
{
    static_cast<K3Process*>(this)->operator<<( arg );
    return *this;
}

K3bProcess& K3bProcess::operator<<( const QStringList& args )
{
    static_cast<K3Process*>(this)->operator<<( args );
    return *this;
}


bool K3bProcess::start( Communication com )
{
    connect( this, SIGNAL(receivedStderr(K3Process*, char*, int)),
             this, SLOT(slotSplitStderr(K3Process*, char*, int)) );
    connect( this, SIGNAL(receivedStdout(K3Process*, char*, int)),
             this, SLOT(slotSplitStdout(K3Process*, char*, int)) );

    return K3Process::start( NotifyOnExit, com );
}


void K3bProcess::slotSplitStdout( K3Process*, char* data, int len )
{
    if( m_bSplitStdout ) {
        QStringList lines = splitOutput( data, len, d->unfinishedStdoutLine, d->suppressEmptyLines );

        for( QStringList::iterator it = lines.begin(); it != lines.end(); ++it ) {
            QString& str = *it;

            // just to be sure since something in splitOutput does not do this right
            if( d->suppressEmptyLines && str.isEmpty() )
                continue;

            emit stdoutLine( str );
        }
    }
}


void K3bProcess::slotSplitStderr( K3Process*, char* data, int len )
{
    QStringList lines = splitOutput( data, len, d->unfinishedStderrLine, d->suppressEmptyLines );

    for( QStringList::iterator it = lines.begin(); it != lines.end(); ++it ) {
        QString& str = *it;

        // just to be sure since something in splitOutput does not do this right
        if( d->suppressEmptyLines && str.isEmpty() )
            continue;

        emit stderrLine( str );
    }
}


QStringList K3bProcess::splitOutput( char* data, int len,
                                     QString& unfinishedLine, bool suppressEmptyLines )
{
    //
    // The stderr splitting is mainly used for parsing of messages
    // That's why we simplify the data before proceeding
    //

    QString buffer;
    for( int i = 0; i < len; i++ ) {
        if( data[i] == '\b' ) {
            while( data[i] == '\b' )  // we replace multiple backspaces with a single line feed
                i++;
            buffer += '\n';
        }
        if( data[i] == '\r' )
            buffer += '\n';
        else if( data[i] == '\t' )  // replace tabs with a single space
            buffer += " ";
        else
            buffer += data[i];
    }

    QStringList lines = buffer.split( '\n', suppressEmptyLines ? QString::SkipEmptyParts : QString::KeepEmptyParts );

    // in case we suppress empty lines we need to handle the following separately
    // to make sure we join unfinished lines correctly
    if( suppressEmptyLines && buffer[0] == '\n' )
        lines.prepend( QString() );

    if( !unfinishedLine.isEmpty() ) {
        lines.first().prepend( unfinishedLine );
        unfinishedLine.truncate(0);

        kDebug() << "(K3bProcess)           joined line: '" << (lines.first()) << "'";
    }

    QStringList::iterator it;

    // check if line ends with a newline
    // if not save the last line because it is not finished
    QChar c = buffer.right(1).at(0);
    bool hasUnfinishedLine = ( c != '\n' && c != '\r' && c != QChar(46) );  // What is unicode 46?? It is printed as a point
    if( hasUnfinishedLine ) {
        kDebug() << "(K3bProcess) found unfinished line: '" << lines.last() << "'";
        kDebug() << "(K3bProcess)             last char: '" << buffer.right(1) << "'";
        unfinishedLine = lines.takeLast();
    }

    return lines;
}


int K3bProcess::stdinFd() const
{
    if( d->rawStdin )
        return d->in[1];
    else if( d->dupStdinFd != -1 )
        return d->dupStdinFd;
    else
        return -1;
}

void K3bProcess::writeToFd( int fd )
{
    d->dupStdoutFd = fd;
    if( fd != -1 )
        d->rawStdout = false;
}

void K3bProcess::readFromFd( int fd )
{
    d->dupStdinFd = fd;
    if( fd != -1 )
        d->rawStdin = false;
}


void K3bProcess::setRawStdin(bool b)
{
    if( b ) {
        d->rawStdin = true;
        d->dupStdinFd = -1;
    }
    else
        d->rawStdin = false;
}


void K3bProcess::setSuppressEmptyLines( bool b )
{
    d->suppressEmptyLines = b;
}


bool K3bProcess::closeStdin()
{
    if( d->rawStdin ) {
        close(d->in[1]);
        d->in[1] = -1;
        return true;
    }
    else
        return K3Process::closeStdin();
}


bool K3bProcess::closeStdout()
{
    if( d->rawStdout ) {
        close(d->out[0]);
        d->out[0] = -1;
        return true;
    }
    else
        return K3Process::closeStdout();
}

void K3bProcess::closeWriteChannel()
{
    ::close( stdinFd() );
}

bool K3bProcess::waitForFinished(int timeout)
{
    Q_ASSERT( timeout == -1 );

    ::waitpid( pid(), 0, 0 );

    return true;
}

qint64 K3bProcess::write(const char * data, qint64 maxSize)
{
    return ::write( stdinFd(), data, maxSize);
}

QString K3bProcess::joinedArgs()
{
    QList<QByteArray> a = args();
    QString s;
    Q_FOREACH( QByteArray arg, a ) {
        s += QString::fromLocal8Bit( arg ) + " ";
    }
    return s;
}

#include "k3bprocess.moc"
