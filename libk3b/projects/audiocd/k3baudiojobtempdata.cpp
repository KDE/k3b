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

#include "k3baudiojobtempdata.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include <k3bglobals.h>
#include <k3bversion.h>
#include <k3bmsf.h>
#include <k3bcore.h>

#include <qfile.h>
#include <q3textstream.h>
#include <q3valuevector.h>

#include <kdebug.h>


class K3bAudioJobTempData::Private
{
public:
  Private( K3bAudioDoc* _doc ) 
    : doc(_doc) {
  }

  Q3ValueVector<QString> bufferFiles;
  Q3ValueVector<QString> infFiles;
  QString tocFile;

  K3bAudioDoc* doc;
};


K3bAudioJobTempData::K3bAudioJobTempData( K3bAudioDoc* doc, QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private( doc );
}


K3bAudioJobTempData::~K3bAudioJobTempData()
{
  delete d;
}


const QString& K3bAudioJobTempData::bufferFileName( int track )
{
  if( (int)d->bufferFiles.count() < track )
    prepareTempFileNames();
  return d->bufferFiles.at(track-1);
}

const QString& K3bAudioJobTempData::bufferFileName( K3bAudioTrack* track )
{
  return bufferFileName( track->trackNumber() );
}


const QString& K3bAudioJobTempData::tocFileName()
{
  if( d->tocFile.isEmpty() )
    prepareTempFileNames();
  return d->tocFile;
}


const QString& K3bAudioJobTempData::infFileName( int track )
{
  if( (int)d->infFiles.count() < track )
    prepareTempFileNames();
  return d->infFiles.at( track - 1 );
}

const QString& K3bAudioJobTempData::infFileName( K3bAudioTrack* track )
{
  return infFileName( track->trackNumber() );
}


K3bAudioDoc* K3bAudioJobTempData::doc() const
{
  return d->doc;
}


void K3bAudioJobTempData::prepareTempFileNames( const QString& path ) 
{
  d->bufferFiles.clear();
  d->infFiles.clear();

  QString prefix = K3b::findUniqueFilePrefix( "k3b_audio_", path ) + "_";

  for( int i = 0; i < d->doc->numOfTracks(); i++ ) {
    d->bufferFiles.append( prefix + QString::number( i+1 ).rightJustified( 2, '0' ) + ".wav" );
    d->infFiles.append( prefix + QString::number( i+1 ).rightJustified( 2, '0' ) + ".inf" );
  }

  d->tocFile = prefix + ".toc";
}


void K3bAudioJobTempData::cleanup()
{
  for( int i = 0; i < d->infFiles.count(); ++i ) {
    if( QFile::exists( d->infFiles[i] ) )
      QFile::remove(  d->infFiles[i] );
  }

  for( int i = 0; i < d->bufferFiles.count(); ++i ) {
    if( QFile::exists( d->bufferFiles[i] ) )
      QFile::remove(  d->bufferFiles[i] );
  }

  if( QFile::exists( d->tocFile ) )
    QFile::remove(  d->tocFile );
}


#include "k3baudiojobtempdata.moc"
