/* 
 *
 * $Id: $
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

  QString path = m_cddbDir;
  if( path.startsWith( "~" ) )
    path.replace( 0, 1, QDir::homeDirPath() + "/" );
  else if( !path.startsWith( "/" ) )
    path.prepend( QDir::homeDirPath() + "/" );
  if( path[path.length()-1] != '/' )
    path.append( "/" );

  for( QStringList::const_iterator it = categories().begin();
       it != categories().end(); ++it ) {

    QString file = path + *it + "/" +  QString::number( toc().discId(), 16 );

    if( QFile::exists( file ) ) {
      // found file
      
      QFile f( file );
      if( !f.open( IO_ReadOnly ) ) {
	kdDebug() << "(K3bCddbLocalQuery) Could not open file" << endl;
      }
      else {
	QTextStream t( &f );
	
	K3bCddbResultEntry entry;
	entry.category = *it;
	entry.discid = QString::number( toc().discId(), 16 );
	parseEntry( t, entry );
	queryResult().addEntry( entry );
      }
    }
    else {
      kdDebug() << "(K3bCddbLocalQuery) Could not find local entry in category " << *it << endl;
    }
  }

  if( queryResult().foundEntries() > 0 ) {
    setError( SUCCESS );
  }
  else {
    setError( NO_ENTRY_FOUND );
  }

  emit queryFinished( this );
}


#include "k3bcddblocalquery.moc"
