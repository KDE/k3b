/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bvideodvdimager.h"
#include "k3bvideodvddoc.h"
#include <k3bdiritem.h>
#include <k3bprocess.h>
#include <k3bglobals.h>

#include <ktempfile.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <klocale.h>

#include <qtextstream.h>
#include <qdir.h>
#include <qfile.h>
#include <qptrlist.h>



class K3bVideoDvdImager::Private
{
public:
  K3bVideoDvdDoc* doc;

  QString tempPath;
};


K3bVideoDvdImager::K3bVideoDvdImager( K3bVideoDvdDoc* doc, K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bIsoImager( doc, jh, parent, name )
{
  d = new Private;
  d->doc = doc;
}


K3bVideoDvdImager::~K3bVideoDvdImager()
{
  delete d;
}


void K3bVideoDvdImager::start()
{
  // we need this for the VIDEO_TS HACK
  d->doc->isoOptions().setFollowSymbolicLinks(true);

  K3bIsoImager::start();
}


int K3bVideoDvdImager::writePathSpec()
{
  //
  // Create a temp dir and link all contents of the VIDEO_TS dir to make mkisofs 
  // able to handle the VideoDVD stuff.
  //
  // mkisofs is not able to create VideoDVDs from graft-points.
  //
  // We do this here since K3bIsoImager::start calls cleanup which deletes the temp files
  //
  QDir dir( KGlobal::dirs()->resourceDirs( "tmp" ).first() );
  d->tempPath = K3b::findUniqueFilePrefix( "k3bVideoDvd", dir.path() );
  kdDebug() << "(K3bVideoDvdImager) creating temp dir: " << d->tempPath << endl;
  if( !dir.mkdir( d->tempPath, true ) ) {
    emit infoMessage( i18n("Unable to create temporary directory '%1'.").arg(d->tempPath), ERROR );
    return -1;
  }

  dir.cd( d->tempPath );
  if( !dir.mkdir( "VIDEO_TS" ) ) {
    emit infoMessage( i18n("Unable to create temporary directory '%1'.").arg(d->tempPath + "/VIDEO_TS"), ERROR );
    return -1;
  }
  
  for( QPtrListIterator<K3bDataItem> it( d->doc->videoTsDir()->children() ); *it; ++it ) {
    if( (*it)->isDir() ) {
      emit infoMessage( i18n("Found invalid entry in the VIDEO_TS folder (%1).").arg((*it)->k3bName()), ERROR );
      return -1;
    }

    // convert to upper case names
    if( ::symlink( QFile::encodeName( (*it)->localPath() ), 
		   QFile::encodeName( d->tempPath + "/VIDEO_TS/" + (*it)->k3bName().upper() ) ) == -1 ) {
      emit infoMessage( i18n("Unable to link temporary file in folder %1.").arg( d->tempPath ), ERROR );
      return -1;
    }
  }


  return K3bIsoImager::writePathSpec();
}


int K3bVideoDvdImager::writePathSpecForDir( K3bDirItem* dirItem, QTextStream& stream )
{
  //
  // We handle the VIDEO_TS dir differently since otherwise mkisofs is not able to 
  // open the VideoDVD structures (see addMkisofsParameters)
  //
  if( dirItem != d->doc->videoTsDir() )
    return K3bIsoImager::writePathSpecForDir( dirItem, stream );
  else
    return 0;
}


bool K3bVideoDvdImager::addMkisofsParameters()
{
  // Here is another bad design: we assume that K3bIsoImager::start does not add additional 
  // parameters to the process. :(
  if( K3bIsoImager::addMkisofsParameters() ) {
    *m_process << "-dvd-video";
    *m_process << d->tempPath;
    return true;
  }
  else
    return false;
}


void K3bVideoDvdImager::cleanup()
{
  if( QFile::exists( d->tempPath ) ) {
    QDir dir( d->tempPath );
    dir.cd( "VIDEO_TS" );
    for( QPtrListIterator<K3bDataItem> it( d->doc->videoTsDir()->children() ); *it; ++it )
      dir.remove( (*it)->k3bName().upper() );
    dir.cdUp();
    dir.rmdir( "VIDEO_TS" );
    dir.cdUp();
    dir.rmdir( d->tempPath );
  }
  d->tempPath = QString::null;

  K3bIsoImager::cleanup();
}


void K3bVideoDvdImager::slotReceivedStderr( const QString& line )
{
  if( line.contains( "Unable to make a DVD-Video image" ) ) {
    emit infoMessage( i18n("The project does not contain all necessary VideoDVD files."), WARNING );
    emit infoMessage( i18n("The resulting DVD will most likely not be playable on a Hifi DVD player."), WARNING );
  }
  else 
    K3bIsoImager::slotReceivedStderr( line );
}

#include "k3bvideodvdimager.moc"
