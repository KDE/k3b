/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include <q3ptrqueue.h>
#include <qapplication.h>
//Added by qt3to4:
#include <Q3CString>

#include <kdebug.h>

#include <sys/types.h>
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

K3bProcess& K3bProcess::operator<<( const Q3CString& arg )
{
  static_cast<K3Process*>(this)->operator<<( arg );
  return *this;
}

K3bProcess& K3bProcess::operator<<( const QStringList& args )
{
  static_cast<K3Process*>(this)->operator<<( args );
  return *this;
}


bool K3bProcess::start( RunMode run, Communication com )
{
  if( com & Stderr ) {
    connect( this, SIGNAL(receivedStderr(K3Process*, char*, int)),
	     this, SLOT(slotSplitStderr(K3Process*, char*, int)) );
  }
  if( com & Stdout ) {
    connect( this, SIGNAL(receivedStdout(K3Process*, char*, int)),
	     this, SLOT(slotSplitStdout(K3Process*, char*, int)) );
  }

  return K3Process::start( run, com );
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

  QStringList lines = QStringList::split( '\n', buffer, !suppressEmptyLines );

  // in case we suppress empty lines we need to handle the following separately
  // to make sure we join unfinished lines correctly
  if( suppressEmptyLines && buffer[0] == '\n' )
    lines.prepend( QString::null );

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
    unfinishedLine = lines.last();
    it = lines.end();
    --it;
    lines.remove(it);
  }

  return lines;
}


int K3bProcess::setupCommunication( Communication comm )
{
  if( K3Process::setupCommunication( comm ) ) {

    //
    // Setup our own socketpair
    //

    if( d->rawStdin ) {
      if( socketpair(AF_UNIX, SOCK_STREAM, 0, d->in) == 0 ) {
	fcntl(d->in[0], F_SETFD, FD_CLOEXEC);
	fcntl(d->in[1], F_SETFD, FD_CLOEXEC);
      }
      else
	return 0;
    }

    if( d->rawStdout ) {
      if( socketpair(AF_UNIX, SOCK_STREAM, 0, d->out) == 0 ) {
	fcntl(d->out[0], F_SETFD, FD_CLOEXEC);
	fcntl(d->out[1], F_SETFD, FD_CLOEXEC);
      }
      else {
	if( d->rawStdin || d->dupStdinFd ) {
	  close(d->in[0]);
	  close(d->in[1]);
	}
	return 0;
      }
    }

    return 1;
  }
  else
    return 0;
}


void K3bProcess::commClose()
{
  if( d->rawStdin ) {
    close(d->in[1]);
    d->in[1] = -1;
  }
  if( d->rawStdout ) {
    close(d->out[0]);
    d->out[0] = -1;
  }

  K3Process::commClose();
}


int K3bProcess::commSetupDoneP()
{
  int ok = K3Process::commSetupDoneP();

  if( d->rawStdin )
    close(d->in[0]);
  if( d->rawStdout )
    close(d->out[1]);

  d->in[0] = d->out[1] = -1;

  return ok;
}


int K3bProcess::commSetupDoneC()
{
  int ok = K3Process::commSetupDoneC();

  if( d->dupStdoutFd != -1 ) {
    //
    // make STDOUT_FILENO a duplicate of d->dupStdoutFd such that writes to STDOUT_FILENO are "redirected"
    // to d->dupStdoutFd
    //
    if( ::dup2( d->dupStdoutFd, STDOUT_FILENO ) < 0 ) {
      kDebug() << "(K3bProcess) Error while dup( " << d->dupStdoutFd << ", " << STDOUT_FILENO;
      ok = 0;
    }
  }
  else if( d->rawStdout ) {
    if( ::dup2( d->out[1], STDOUT_FILENO ) < 0 ) {
      kDebug() << "(K3bProcess) Error while dup( " << d->out[1] << ", " << STDOUT_FILENO;
      ok = 0;
    }
  }

  if( d->dupStdinFd != -1 ) {
    if( ::dup2( d->dupStdinFd, STDIN_FILENO ) < 0 ) {
      kDebug() << "(K3bProcess) Error while dup( " << d->dupStdinFd << ", " << STDIN_FILENO;
      ok = 0;
    }
  }
  else if( d->rawStdin ) {
    if( ::dup2( d->in[0], STDIN_FILENO ) < 0 ) {
      kDebug() << "(K3bProcess) Error while dup( " << d->in[0] << ", " << STDIN_FILENO;
      ok = 0;
    }
  }

  return ok;
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

int K3bProcess::stdoutFd() const
{
  if( d->rawStdout )
    return d->out[0];
  else if( d->dupStdoutFd != -1 )
    return d->dupStdoutFd;
  else
    return -1;
}


void K3bProcess::dupStdout( int fd )
{
  writeToFd( fd );
}

void K3bProcess::dupStdin( int fd )
{
  readFromFd( fd );
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


void K3bProcess::setRawStdout(bool b)
{
  if( b ) {
    d->rawStdout = true;
    d->dupStdoutFd = -1;
  }
  else
    d->rawStdout = false;
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


K3bProcessOutputCollector::K3bProcessOutputCollector( K3Process* p )
  : m_process(0)
{
  setProcess( p );
}

void K3bProcessOutputCollector::setProcess( K3Process* p )
{
  if( m_process )
    m_process->disconnect( this );

  m_process = p;
  if( p ) {
    connect( p, SIGNAL(receivedStdout(K3Process*, char*, int)), 
	     this, SLOT(slotGatherStdout(K3Process*, char*, int)) );
    connect( p, SIGNAL(receivedStderr(K3Process*, char*, int)), 
	     this, SLOT(slotGatherStderr(K3Process*, char*, int)) );
  }

  m_gatheredOutput.truncate( 0 );
  m_stderrOutput.truncate( 0 );
  m_stdoutOutput.truncate( 0 );
}

void K3bProcessOutputCollector::slotGatherStderr( K3Process*, char* data, int len )
{
  m_gatheredOutput.append( QString::fromLocal8Bit( data, len ) );
  m_stderrOutput.append( QString::fromLocal8Bit( data, len ) );
}

void K3bProcessOutputCollector::slotGatherStdout( K3Process*, char* data, int len )
{
  m_gatheredOutput.append( QString::fromLocal8Bit( data, len ) );
  m_stdoutOutput.append( QString::fromLocal8Bit( data, len ) );
}


#include "k3bprocess.moc"
