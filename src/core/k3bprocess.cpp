/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#include "k3bprocess.h"

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

  // in case we suppress empty lines we need to handle the following seperately
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

    if( stdout )
      emit stdoutLine( str );
    else
      emit stderrLine( str );
  }
}


/**
 * This has mainly been copied from KProcess with some little changes:
 * If the user requested direct control over stdin or stdout no socketNotifier 
 * is created
 */
int K3bProcess::commSetupDoneP()
{
//   int ok = 1;

//   if (communication != NoCommunication) {
//         if (communication & Stdin)
//           close(in[0]);
//         if (communication & Stdout)
//           close(out[1]);
//         if (communication & Stderr)
//           close(err[1]);

//         // Don't create socket notifiers and set the sockets non-blocking if
//         // blocking is requested.
//         if (run_mode == Block) return ok;

//         if ((communication & Stdin) && !m_rawStdin) {
// //        ok &= (-1 != fcntl(in[1], F_SETFL, O_NONBLOCK));
//           innot =  new QSocketNotifier(in[1], QSocketNotifier::Write, this);
//           Q_CHECK_PTR(innot);
//           innot->setEnabled(false); // will be enabled when data has to be sent
//           QObject::connect(innot, SIGNAL(activated(int)),
//                                            this, SLOT(slotSendData(int)));
//         }

//         if (communication & Stdout) {
// //        ok &= (-1 != fcntl(out[0], F_SETFL, O_NONBLOCK));
//           outnot = new QSocketNotifier(out[0], QSocketNotifier::Read, this);
//           Q_CHECK_PTR(outnot);
// 	  if( m_rawStdout ) {
// 	    connect( outnot, SIGNAL(activated(int)),
// 		     this, SIGNAL(stdoutReady(int)) );
// 	  }
// 	  else {
// 	    QObject::connect(outnot, SIGNAL(activated(int)),
// 			     this, SLOT(slotChildOutput(int)));
// 	    if (communication & NoRead)
//               suspend();
// 	  }
//         }

//         if (communication & Stderr) {
// //        ok &= (-1 != fcntl(err[0], F_SETFL, O_NONBLOCK));
//           errnot = new QSocketNotifier(err[0], QSocketNotifier::Read, this );
//           Q_CHECK_PTR(errnot);
//           QObject::connect(errnot, SIGNAL(activated(int)),
//                                            this, SLOT(slotChildError(int)));
//         }
//   }

  int ok = KProcess::commSetupDoneP();

  if( m_rawStdout ) {
    disconnect( outnot, SIGNAL(activated(int)),
		this, SLOT(slotChildOutput(int)));
    connect( outnot, SIGNAL(activated(int)),
	     this, SIGNAL(stdoutReady(int)) );
  }
  if( m_rawStdin ) {
    delete innot;
    innot = 0;
  }

  return ok;
}


// this is just the same as in KProcess except the additional
// d->dupStdoutFd stuff
int K3bProcess::commSetupDoneC()
{
//   int ok = 1;
//   struct linger so;
//   memset(&so, 0, sizeof(so));

// //   if (communication & Stdin)
// //     close(in[1]);
// //   if (communication & Stdout)
// //     close(out[0]);
// //   if (communication & Stderr)
// //     close(err[0]);

//   if (communication & Stdin) {
//     ok &= (dup2( in[0], STDIN_FILENO ) != -1);
//   }
//   else {
//     int null_fd = open( "/dev/null", O_RDONLY );
//     ok &= dup2( null_fd, STDIN_FILENO ) != -1;
//     close( null_fd );
//   }


//   if( d->dupStdoutFd != -1 ) {
//     if( ::dup2( d->dupStdoutFd, STDOUT_FILENO ) != -1 ) {
//       kdDebug() << "(K3bProcess) Successfully duplicated " << d->dupStdoutFd << " to " << STDOUT_FILENO << endl;
//     }
//     else {
//       kdDebug() << "(K3bProcess) Error while dup( " << d->dupStdoutFd << ", " << STDOUT_FILENO << endl;
//       ok = 0;
//       communication = (Communication) (communication & ~Stdout);
//     }
//   }
//   else if (communication & Stdout) {
//     ok &= dup2(out[1], STDOUT_FILENO) != -1;
//     ok &= !setsockopt(out[1], SOL_SOCKET, SO_LINGER, (char*)&so, sizeof(so));
//   }
//   else {
//     int null_fd = open( "/dev/null", O_WRONLY );
//     ok &= dup2( null_fd, STDOUT_FILENO ) != -1;
//     close( null_fd );
//   }
//   if (communication & Stderr) {
//     ok &= dup2(err[1], STDERR_FILENO) != -1;
//     ok &= !setsockopt(err[1], SOL_SOCKET, SO_LINGER, reinterpret_cast<char *>(&so), sizeof(so));
//   }
//   else {
//     int null_fd = open( "/dev/null", O_WRONLY );
//     ok &= dup2( null_fd, STDERR_FILENO ) != -1;
//     close( null_fd );
//   }
//   return ok;

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

#include "k3bprocess.moc"
