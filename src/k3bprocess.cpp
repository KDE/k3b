/***************************************************************************
                          k3bprocess.cpp  -
                   KProcess with enhanced stderr handling
                             -------------------
    begin                : Wed Sep  4 12:01:14 CEST 2002
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "k3bprocess.h"

#include <qstringlist.h>
#include <qsocketnotifier.h>
#include <kdebug.h>


K3bProcess::K3bProcess()
  : KProcess(),
    m_rawStdin(false),
    m_rawStdout(false)
{
  m_bSplitStdout = false;
}

K3bProcess::~K3bProcess()
{
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
  QString buffer = QString::fromLocal8Bit( data, len );
  QStringList lines = QStringList::split( "\n", buffer, true );

  QString* unfinishedLine = ( stdout ? &m_unfinishedStdoutLine : &m_unfinishedStderrLine );

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
    if( str[0] == '\r' )
      str = str.mid( 1 );

    //    kdDebug() << "(K3bProcess)         emitting line: '" << str << "'" << endl;

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
  int ok = 1;

  if (communication != NoCommunication) {
        if (communication & Stdin)
          close(in[0]);
        if (communication & Stdout)
          close(out[1]);
        if (communication & Stderr)
          close(err[1]);

        // Don't create socket notifiers and set the sockets non-blocking if
        // blocking is requested.
        if (run_mode == Block) return ok;

        if ((communication & Stdin) && !m_rawStdin) {
//        ok &= (-1 != fcntl(in[1], F_SETFL, O_NONBLOCK));
          innot =  new QSocketNotifier(in[1], QSocketNotifier::Write, this);
          Q_CHECK_PTR(innot);
          innot->setEnabled(false); // will be enabled when data has to be sent
          QObject::connect(innot, SIGNAL(activated(int)),
                                           this, SLOT(slotSendData(int)));
        }

        if (communication & Stdout) {
//        ok &= (-1 != fcntl(out[0], F_SETFL, O_NONBLOCK));
          outnot = new QSocketNotifier(out[0], QSocketNotifier::Read, this);
          Q_CHECK_PTR(outnot);
	  if( m_rawStdout ) {
	    connect( outnot, SIGNAL(activated(int)),
		     this, SIGNAL(stdoutReady(int)) );
	  }
	  else {
	    QObject::connect(outnot, SIGNAL(activated(int)),
			     this, SLOT(slotChildOutput(int)));
	    if (communication & NoRead)
              suspend();
	  }
        }

        if (communication & Stderr) {
//        ok &= (-1 != fcntl(err[0], F_SETFL, O_NONBLOCK));
          errnot = new QSocketNotifier(err[0], QSocketNotifier::Read, this );
          Q_CHECK_PTR(errnot);
          QObject::connect(errnot, SIGNAL(activated(int)),
                                           this, SLOT(slotChildError(int)));
        }
  }
  return ok;
}

int K3bProcess::stdin() const
{
  return in[1];
}

int K3bProcess::stdout() const
{
  return out[0];
}

#include "k3bprocess.moc"
