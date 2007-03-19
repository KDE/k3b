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


#include "k3bcddblocalquery.h"

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>

#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>


K3bCddbLocalQuery::K3bCddbLocalQuery( QObject* parent , const char* name )
  : K3bCddbQuery( parent, name )
{
}


K3bCddbLocalQuery::~K3bCddbLocalQuery()
{
}


void K3bCddbLocalQuery::doQuery()
{
  emit infoMessage( i18n("Searching entry in %1").arg( m_cddbDir ) );
  kapp->processEvents(); //BAD!

  QString path = preparePath( m_cddbDir );

  kdDebug() << "(K3bCddbLocalQuery) searching in dir " << path << " for " 
	    << QString::number( toc().discId(), 16 ).rightJustify( 8, '0' ) << endl;

  for( QStringList::const_iterator it = categories().begin();
       it != categories().end(); ++it ) {

    QString file = path + *it + "/" +  QString::number( toc().discId(), 16 ).rightJustify( 8, '0' );

    if( QFile::exists( file ) ) {
      // found file
      
      QFile f( file );
      if( !f.open( IO_ReadOnly ) ) {
	kdDebug() << "(K3bCddbLocalQuery) Could not open file" << endl;
      }
      else {
	QTextStream t( &f );

	K3bCddbResultEntry entry;
	parseEntry( t, entry );
	K3bCddbResultHeader header;
	header.discid = QString::number( toc().discId(), 16 ).rightJustify( 8, '0' );
	header.category = *it;
	header.title = entry.cdTitle;
	header.artist = entry.cdArtist;
	m_inexactMatches.append(header);
      }
    }
    else {
      kdDebug() << "(K3bCddbLocalQuery) Could not find local entry in category " << *it << endl;
    }
  }

  if( m_inexactMatches.count() > 0 ) {
    setError( SUCCESS );
    if( m_inexactMatches.count() == 1 ) {
      queryMatch( m_inexactMatches.first() );
    }
    else {
      emit inexactMatches( this );
    }
  }
  else {
    setError( NO_ENTRY_FOUND );
    emit queryFinished( this );
  }
}


void K3bCddbLocalQuery::doMatchQuery()
{
  QString path = preparePath( m_cddbDir ) + header().category + "/" + header().discid;

  QFile f( path );
  if( !f.open( IO_ReadOnly ) ) {
    kdDebug() << "(K3bCddbLocalQuery) Could not open file" << endl;
    setError( READ_ERROR );
  }
  else {
    QTextStream t( &f );
    
    parseEntry( t, result() );
    result().discid = header().discid;
    result().category = header().category;
    setError( SUCCESS );
  }
  emit queryFinished( this );
}


QString K3bCddbLocalQuery::preparePath( const QString& p ) 
{
  QString path = p;
  if( path.startsWith( "~" ) )
    path.replace( 0, 1, QDir::homeDirPath() );
  else if( !path.startsWith( "/" ) )
    path.prepend( QDir::homeDirPath() );
  if( path[path.length()-1] != '/' )
    path.append( "/" );

  return path;
}

#include "k3bcddblocalquery.moc"
