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

#include "k3baudioencoder.h"

#include <qfile.h>

#include <kdebug.h>


class K3bAudioEncoder::Private
{
public:
  Private()
    : outputFile(0) {
  }

  QFile* outputFile;
  QString outputFilename;
};


K3bAudioEncoder::K3bAudioEncoder( QObject* parent, const char* name )
  : K3bPlugin( parent, name )
{
  d = new Private();
}


K3bAudioEncoder::~K3bAudioEncoder()
{
  closeFile();
  delete d;
}


bool K3bAudioEncoder::openFile( const QString& ext, const QString& filename )
{
  closeFile();

  d->outputFile = new QFile( filename );
  if( d->outputFile->open( IO_WriteOnly ) ) {
    return initEncoder( ext );
  }
  else {
    kdDebug() << "(K3bAudioEncoder) unable to open file " << filename << endl;
    closeFile();
    return false;
  }
}


bool K3bAudioEncoder::isOpen() const
{
  if( d->outputFile )
    return d->outputFile->isOpen();
  else
    return false;
}


void K3bAudioEncoder::closeFile()
{
  if( d->outputFile ) {
    finishEncoder();
    if( d->outputFile->isOpen() )
      d->outputFile->close();
    delete d->outputFile;
    d->outputFile = 0;
    d->outputFilename = QString::null;
  }
}


const QString& K3bAudioEncoder::filename() const
{
  if( d->outputFile )
    return d->outputFilename;
  else
    return QString::null;
}



void K3bAudioEncoder::setMetaData( const QString& type, const QString& data )
{
  if( !data.isEmpty() )
    return setMetaDataInternal( type, data );
}


long K3bAudioEncoder::encode( const char* data, Q_ULONG len )
{
  return encodeInternal( data, len );
}


bool K3bAudioEncoder::initEncoder( const QString& ext )
{
  if( !isOpen() ) {
    kdDebug() << "(K3bAudioEncoder) call to initEncoder without openFile!" << endl;
    return false;
  }

  return initEncoderInternal( ext );
}


Q_LONG K3bAudioEncoder::writeData( const char* data, Q_ULONG len )
{
  if( d->outputFile ) {
    return d->outputFile->writeBlock( data, len );
  }
  else {
    kdDebug() << "(K3bAudioEncoder) call to writeData without opening a file first." << endl;
    return -1;
  }
}


bool K3bAudioEncoder::initEncoderInternal( const QString& )
{
  // do nothing
  return true;
}


void K3bAudioEncoder::setMetaDataInternal( const QString&, const QString& )
{
  // do nothing
}


void K3bAudioEncoder::finishEncoder()
{
  finishEncoderInternal();
}


void K3bAudioEncoder::finishEncoderInternal()
{
  // do nothing
}

#include "k3baudioencoder.moc"
