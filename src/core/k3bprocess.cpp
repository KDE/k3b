/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#include "k3bprocess.h"
#include "k3bexternalbinmanager.h"

#include <qstringlist.h>
#include <qsocketnotifier.h>

#include <kdebug.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>



class K3bProcess::Private
{
public:
  QString unfinishedStdoutLine;
  QString unfinishedStderrLine;

  int dupStdoutFd;
  int dupStdinFd;
};


K3bProcess::K3bProcess()
  : KProcess(),
    m_bSplitStdout(false),
    m_rawStdin(false),
    m_rawStdout(false),
    m_suppressEmptyLines(true)
{
  d = new Private();
  d->dupStdinFd = d->dupStdoutFd = -1;
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
  static_cast<KProcess*>(this)->operator<<( arg );
  return *this;
}

K3bProcess& K3bProcess::operator<<( const char* arg )
{
  static_cast<KProcess*>(this)->operator<<( arg );
  return *this;
}

K3bProcess& K3bProcess::operator<<( const QCString& arg )
{
  static_cast<KProcess*>(this)->operator<<( arg );
  return *this;
}

K3bProcess& K3bProcess::operator<<( const QStringList& args )
{
  static_cast<KProcess*>(this)->operator<<( args );
  return *this;
}


bool K3bProcess::start( RunMode run, Communication com )
{
  if( com & Stderr ) {
    connect( this, SIGNAL(receivedStderr(KProcess*, char*, int)),
	     this, SLOT(slotSplitStderr(KProcess*, char*, int)) );
  }
  if( com & Stdout ) {
    connect( this, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotSplitStdout(KProcess*, char*, int)) );
  }

  return KProcess::start( run, com );
}


void K3bProcess::slotSplitStdout( KProcess*, char* data, int len )
{
  if( m_bSplitStdout )
    splitOutput( data, len, true );
}


void K3bProcess::slotSplitStderr( KProcess*, char* data, int len )
{
  splitOutput( data, len, false );
}


void K3bProcess::splitOutput( char* data, int len, bool stdout )
{
  //
  // The stderr splitting is mainly used for parsing of messages
  // That's why we simplify the data before proceeding
  //

  QString buffer;
  for( int i = 0; i < len; i++ ) {
    if( data[i] == '\b' ) {
      while( data[i] == '\b' )  // we replace multible backspaces with a single line feed
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

  QStringList lines = QStringList::split( '\n', buffer, !m_suppressEmptyLines );

  // in case we suppress empty lines we need to handle the following separately
  // to make sure we join unfinished lines correctly
  if( m_suppressEmptyLines && buffer[0] == '\n' )
    lines.prepend( QString::null );

  QString* unfinishedLine = ( stdout ? &d->unfinishedStdoutLine : &d->unfinishedStderrLine );

  if( !unfinishedLine->isEmpty() ) {
    lines.first().prepend( *unfinishedLine );
    *unfinishedLine = "";

    kdDebug() << "(K3bProcess)           joined line: '" << (lines.first()) << "'" << endl;
  }

  QStringList::iterator it;

  // check if line ends with a newline
  // if not save the last line because it is not finished
  QChar c = buffer.right(1).at(0);
  bool hasUnfinishedLine = ( c != '\n' && c != '\r' && c != QChar(46) );  // What is unicode 46?? It is printed as a point
  if( hasUnfinishedLine ) {
    kdDebug() << "(K3bProcess) found unfinished line: '" << lines.last() << "'" << endl;
    kdDebug() << "(K3bProcess)             last char: '" << buffer.right(1) << "'" << endl;
    *unfinishedLine = lines.last();
    it = lines.end();
    --it;
    lines.remove(it);
  }

  for( it = lines.begin(); it != lines.end(); ++it ) {
    QString& str = *it;

    // just to be sure since something above does not do this right
    if( str.isEmpty() )
      continue;

    if( stdout )
      emit stdoutLine( str );
    else
      emit stderrLine( str );
  }
}


int K3bProcess::commSetupDoneP()
{
  int ok = KProcess::commSetupDoneP();

  if( communication & Stdin ) {
    // this fixed an issue introduced in kde 3.2 rc1 which for some reason 
    // makes in[1] O_NONBLOCK which causes our whole K3bProcess<->K3bProcess
    // communication to fail
    fcntl(in[1], F_SETFL, ~O_NONBLOCK & fcntl(in[1], F_GETFL));
  }

  if( m_rawStdout ) {
    disconnect( outnot, SIGNAL(activated(int)),
		this, SLOT(slotChildOutput(int)));
    connect( outnot, SIGNAL(activated(int)),
	     this, SIGNAL(stdoutReady(int)) );
  }
  if( m_rawStdin || d->dupStdinFd != -1 ) {
    delete innot;
    innot = 0;
  }

  if( d->dupStdoutFd != -1 ) {
    delete outnot;
    outnot = 0;
  }

  return ok;
}


int K3bProcess::commSetupDoneC()
{
  int ok = KProcess::commSetupDoneC();

  if( d->dupStdoutFd != -1 ) {
    if( ::dup2( d->dupStdoutFd, STDOUT_FILENO ) != -1 ) {
      kdDebug() << "(K3bProcess) Successfully duplicated " << d->dupStdoutFd << " to " << STDOUT_FILENO << endl;
    }
    else {
      kdDebug() << "(K3bProcess) Error while dup( " << d->dupStdoutFd << ", " << STDOUT_FILENO << endl;
      ok = 0;
      communication = (Communication) (communication & ~Stdout);
    }
  }
  if( d->dupStdinFd != -1 ) {
    if( ::dup2( d->dupStdinFd, STDIN_FILENO ) != -1 ) {
      kdDebug() << "(K3bProcess) Successfully duplicated " << d->dupStdinFd << " to " << STDIN_FILENO << endl;
    }
    else {
      kdDebug() << "(K3bProcess) Error while dup( " << d->dupStdinFd << ", " << STDIN_FILENO << endl;
      ok = 0;
      communication = (Communication) (communication & ~Stdin);
    }
  }

  return ok;
}



int K3bProcess::stdinFd() const
{
  return in[1];
}

int K3bProcess::stdoutFd() const
{
  return out[0];
}


void K3bProcess::dupStdout( int fd )
{
  d->dupStdoutFd = fd;
}

void K3bProcess::dupStdin( int fd )
{
  d->dupStdinFd = fd;
}



K3bProcess::OutputCollector::OutputCollector( KProcess* p )
  : m_process(0)
{
  setProcess( p );
}

void K3bProcess::OutputCollector::setProcess( KProcess* p )
{
  if( m_process )
    m_process->disconnect( this );

  m_process = p;
  if( p ) {
    connect( p, SIGNAL(receivedStdout(KProcess*, char*, int)), 
	     this, SLOT(slotGatherOutput(KProcess*, char*, int)) );
    connect( p, SIGNAL(receivedStderr(KProcess*, char*, int)), 
	     this, SLOT(slotGatherOutput(KProcess*, char*, int)) );
  }

  m_gatheredOutput = "";
}

void K3bProcess::OutputCollector::slotGatherOutput( KProcess*, char* data, int len )
{
  m_gatheredOutput.append( QString::fromLatin1( data, len ) );
}



#include "k3bprocess.moc"
