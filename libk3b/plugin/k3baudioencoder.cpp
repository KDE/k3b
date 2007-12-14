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

  QString lastErrorString;
};


K3bAudioEncoder::K3bAudioEncoder( QObject* parent, const char* name )
  : K3bPlugin( parent )
{
  d = new Private();
}


K3bAudioEncoder::~K3bAudioEncoder()
{
  closeFile();
  delete d;
}


bool K3bAudioEncoder::openFile( const QString& ext, const QString& filename, const K3b::Msf& length )
{
  closeFile();

  d->outputFile = new QFile( filename );
  if( d->outputFile->open( QIODevice::WriteOnly ) ) {
    return initEncoder( ext, length );
  }
  else {
    kDebug() << "(K3bAudioEncoder) unable to open file " << filename;
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



void K3bAudioEncoder::setMetaData( K3bAudioEncoder::MetaDataField f, const QString& data )
{
  if( !data.isEmpty() )
    return setMetaDataInternal( f, data );
}


long K3bAudioEncoder::encode( const char* data, Q_ULONG len )
{
  return encodeInternal( data, len );
}


bool K3bAudioEncoder::initEncoder( const QString& ext, const K3b::Msf& length )
{
  if( !isOpen() ) {
    kDebug() << "(K3bAudioEncoder) call to initEncoder without openFile!";
    return false;
  }

  return initEncoderInternal( ext, length );
}


Q_LONG K3bAudioEncoder::writeData( const char* data, Q_ULONG len )
{
  if( d->outputFile ) {
    return d->outputFile->write( data, len );
  }
  else {
    kDebug() << "(K3bAudioEncoder) call to writeData without opening a file first.";
    return -1;
  }
}


bool K3bAudioEncoder::initEncoderInternal( const QString&, const K3b::Msf& )
{
  // do nothing
  return true;
}


void K3bAudioEncoder::setMetaDataInternal( K3bAudioEncoder::MetaDataField, const QString& )
{
  // do nothing
}


void K3bAudioEncoder::finishEncoder()
{
  if( isOpen() )
    finishEncoderInternal();
}


void K3bAudioEncoder::finishEncoderInternal()
{
  // do nothing
}


void K3bAudioEncoder::setLastError( const QString& e )
{
  d->lastErrorString = e;
}


QString K3bAudioEncoder::lastErrorString() const
{
  if( d->lastErrorString.isEmpty() )
    return i18n("An unknown error occurred.");
  else
    return d->lastErrorString;
}

#include "k3baudioencoder.moc"
