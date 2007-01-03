/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bchecksumpipe.h"

#include <kmdcodec.h>
#include <kdebug.h>

#include <unistd.h>


class K3bChecksumPipe::Private
{
public:
  Private()
    : checksumType(MD5) {
  }

  void update( const char* in, int len ) {
    switch( checksumType ) {
    case MD5:
      md5.update( in, len );
      break;
    }
  }

  void reset() {
    switch( checksumType ) {
    case MD5:
      md5.reset();
      break;
    }
  }

  int checksumType;

  KMD5 md5;
};


K3bChecksumPipe::K3bChecksumPipe()
  : K3bActivePipe()
{
  d = new Private();
}


K3bChecksumPipe::~K3bChecksumPipe()
{
  delete d;
}


bool K3bChecksumPipe::open( bool closeWhenDone )
{
  return open( MD5, closeWhenDone );
}


bool K3bChecksumPipe::open( Type type, bool closeWhenDone )
{
  if( K3bActivePipe::open( closeWhenDone ) ) {
    d->reset();
    d->checksumType = type;
    return true;
  }
  else
    return false;
}


QCString K3bChecksumPipe::checksum() const
{
  switch( d->checksumType ) {
  case MD5:
    return d->md5.hexDigest();
  }

  return QCString();
}


int K3bChecksumPipe::write( char* data, int max )
{
  d->update( data, max );
  return K3bActivePipe::write( data, max );
}
