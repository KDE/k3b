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
#include <kdebug.h>


K3bProcess::K3bProcess()
  : KProcess()
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
  QStringList lines = QStringList::split( "\n", buffer );

  QString& unfinishedLine = m_unfinishedStderrLine;
  if( stdout )
    unfinishedLine = m_unfinishedStdoutLine;

  if( !unfinishedLine.isEmpty() ) {
    kdDebug() << "(K3bProcess) joining line: " << (unfinishedLine + lines.front()) << endl;

    lines.first().prepend( unfinishedLine );
    unfinishedLine = "";
  }

  QStringList::iterator it;

  // check if line ends with a newline
  // if not save the last line because it is not finished
  bool hasUnfinishedLine = ( buffer.right(1) != "\n" && buffer.right(1) != "\r" );
  if( hasUnfinishedLine ) {
    kdDebug() << "(K3bProcess) found unfinished line: " << lines.last() << endl;
    unfinishedLine = lines.last();
    it = lines.end();
    --it;
    lines.remove( it );
  }

  for( it = lines.begin(); it != lines.end(); ++it ) {
    QString& str = *it;
    if( str[0] == '\r' )
      str = str.mid( 1 );
    if( stdout )
      emit stdoutLine( str );
    else
      emit stderrLine( str );
  }
}


#include "k3bprocess.moc"
