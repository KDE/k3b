/***************************************************************************
                          k3bcddblocalsubmit.cpp  -  description
                             -------------------
    begin                : Sun Oct 7 2001
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


#include "k3bcddblocalsubmit.h"

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>

#include <kdebug.h>
#include <klocale.h>


K3bCddbLocalSubmit::K3bCddbLocalSubmit( QObject* parent, const char* name )
  : K3bCddbSubmit( parent, name )
{
}


K3bCddbLocalSubmit::~K3bCddbLocalSubmit()
{
}


void K3bCddbLocalSubmit::doSubmit()
{
  QString path = m_cddbDir;
  if( path.startsWith( "~" ) )
    path.replace( 0, 1, QDir::homeDirPath() + "/" );
  else if( !path.startsWith( "/" ) )
    path.prepend( QDir::homeDirPath() + "/" );
  if( path[path.length()-1] != '/' )
    path.append( "/" );

  if( QFile::exists( path ) ) {
    // if the category dir does not exists
    // create it

    path += resultEntry().category;

    if( !QFile::exists( path ) ) {
      if( !QDir().mkdir( path ) ) {
	kdDebug() << "(K3bCddbLocalSubmit) could not create directory: " << path << endl;
	setError( IO_ERROR );
	emit submitFinished( this );
	return;
      }
    }

    // we always overwrite existing entries
    path += "/" + resultEntry().discid;
    QFile entryFile( path );
    if( entryFile.exists() ) {
      kdDebug() << "(K3bCddbLocalSubmit) file already exists: " << path << endl;
      setError( SUCCESS );
      emit submitFinished( this );
    }
    else if( !entryFile.open( IO_WriteOnly ) ) {
      kdDebug() << "(K3bCddbLocalSubmit) could not create file: " << path << endl;
      setError( IO_ERROR );
      emit submitFinished( this );
    }
    else {
      kdDebug() << "(K3bCddbLocalSubmit) creating file: " << path << endl;
      QTextStream entryStream( &entryFile );
      entryStream << resultEntry().rawData;
      entryFile.close();

      setError( SUCCESS );
      emit submitFinished( this );
    }
  }
  else {
    kdDebug() << "(K3bCddbLocalSubmit) could not find directory: " << path << endl;
    setError( IO_ERROR );
    emit infoMessage( i18n("Could not find directory: %1").arg(path) );
    emit submitFinished( this );
  }
}

#include "k3bcddblocalsubmit.moc"
