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

  return KProcess::start( run, com );
}


void K3bProcess::slotSplitStderr( KProcess*, char* data, int len )
{
  QString buffer = QString::fromLatin1( data, len );
  QStringList lines = QStringList::split( "\n", buffer );

  if( !m_notFinishedLine.isEmpty() ) {
    kdDebug() << "(K3bProcess) joining line: " << (m_notFinishedLine + lines.front()) << endl;

    lines.first().prepend( m_notFinishedLine );
    m_notFinishedLine = "";
  }

  QStringList::iterator it;

  // check if line ends with a newline
  // if not save the last line because it is not finished
  bool notFinishedLine = ( buffer.right(1) != "\n" && buffer.right(1) != "\r" );
  if( notFinishedLine ) {
    kdDebug() << "(K3bProcess) found unfinished line: " << lines.last() << endl;
    m_notFinishedLine = lines.last();
    it = lines.end();
    --it;
    lines.remove( it );
  }
  
  for( it = lines.begin(); it != lines.end(); ++it ) {
    QString& str = *it;
    if( str[0] == '\r' )
      str = str.mid( 1 );
    emit stderrLine( str );
  }
}


#include "k3bprocess.moc"
