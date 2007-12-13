/* 
 *
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



#include "k3bcddblocalsubmit.h"

#include <qdir.h>
#include <qfile.h>
#include <q3textstream.h>

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
    path.replace( 0, 1, QDir::homePath() + "/" );
  else if( !path.startsWith( "/" ) )
    path.prepend( QDir::homePath() + "/" );
  if( path[path.length()-1] != '/' )
    path.append( "/" );

  if( !QFile::exists( path ) && !QDir().mkdir( path ) ) {
    kDebug() << "(K3bCddbLocalSubmit) could not create directory: " << path;
    setError( IO_ERROR );
    emit submitFinished( this );
    return;
  }

  if( QFile::exists( path ) ) {
    // if the category dir does not exists
    // create it

    path += resultEntry().category;

    if( !QFile::exists( path ) ) {
      if( !QDir().mkdir( path ) ) {
	kDebug() << "(K3bCddbLocalSubmit) could not create directory: " << path;
	setError( IO_ERROR );
	emit submitFinished( this );
	return;
      }
    }

    // we always overwrite existing entries
    path += "/" + resultEntry().discid;
    QFile entryFile( path );
    if( entryFile.exists() ) {
      kDebug() << "(K3bCddbLocalSubmit) file already exists: " << path;
    }
    
    if( !entryFile.open( QIODevice::WriteOnly ) ) {
      kDebug() << "(K3bCddbLocalSubmit) could not create file: " << path;
      setError( IO_ERROR );
      emit submitFinished( this );
    }
    else {
      kDebug() << "(K3bCddbLocalSubmit) creating file: " << path;
      Q3TextStream entryStream( &entryFile );
      entryStream.setEncoding( Q3TextStream::UnicodeUTF8 );
      entryStream << resultEntry().rawData;
      entryFile.close();

      setError( SUCCESS );
      emit submitFinished( this );
    }
  }
  else {
    kDebug() << "(K3bCddbLocalSubmit) could not find directory: " << path;
    setError( IO_ERROR );
    emit infoMessage( i18n("Could not find directory: %1",path) );
    emit submitFinished( this );
  }
}

#include "k3bcddblocalsubmit.moc"
