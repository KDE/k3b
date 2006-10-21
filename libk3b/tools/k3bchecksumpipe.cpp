/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bchecksumpipe.h"

#include <k3bpipe.h>

#include <kmdcodec.h>
#include <kdebug.h>

#include <qthread.h>

#include <unistd.h>


class K3bChecksumPipe::Private : public QThread
{
public:
  Private() :
    fdToReadFrom(-1),
    fdToWriteTo(-1),
    closeFdToReadFrom(false),
    closeFdToWriteTo(false) {
  }

  void run() {
    kdDebug() << "(K3bChecksumPipe) started thread." << endl;
    buffer.resize( 10*2048 );
    ssize_t r = 0;
    ssize_t total = 0;
    while( ( r = ::read( readFd(), buffer.data(), buffer.size() ) ) > 0 ) {

      // write it out
      ssize_t w = 0;
      ssize_t ww = 0;
      while( w < r ) {
	if( ( ww = ::write( writeFd(), buffer.data()+w, r-w ) ) > 0 ) {
	  w += ww;
	}
	else {
	  kdDebug() << "(K3bChecksumPipe) write failed." << endl;
	  if( closeWhenDone )
	    close();
	  return;
	}
      }

      // update checksum
      update( buffer.data(), r );

      total += r;
    }
    kdDebug() << "(K3bChecksumPipe) thread done: " << r << " (total bytes: " << total << ")" << endl;

    if( closeWhenDone )
      close();
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

  int readFd() const {
    if( fdToReadFrom == -1 )
      return pipeIn.out();
    else
      return fdToReadFrom;
  }

  int writeFd() const {
    if( fdToWriteTo == -1 )
      return pipeOut.in();
    else
      return fdToWriteTo;
  }

  void close() {
    pipeIn.close();
    pipeOut.close();

    if( fdToWriteTo != -1 &&
	closeFdToWriteTo )
      ::close( fdToWriteTo );

    if( fdToReadFrom != -1 &&
	closeFdToReadFrom )
      ::close( fdToReadFrom );
  }

  int checksumType;

  int fdToReadFrom;
  int fdToWriteTo;
  K3bPipe pipeIn;
  K3bPipe pipeOut;

  bool closeWhenDone;
  bool closeFdToReadFrom;
  bool closeFdToWriteTo;

  KMD5 md5;

  QByteArray buffer;
};


K3bChecksumPipe::K3bChecksumPipe()
{
  d = new Private();
}


K3bChecksumPipe::~K3bChecksumPipe()
{
  delete d;
}


bool K3bChecksumPipe::open( int type, bool closeWhenDone )
{
  if( d->running() )
    return false;

  d->checksumType = type;
  d->closeWhenDone = closeWhenDone;

  if( d->fdToReadFrom == -1 && !d->pipeIn.open() ) {
    return false;
  }

  if( d->fdToWriteTo == -1 && !d->pipeOut.open() ) {
    close();
    return false;
  }

  kdDebug() << "(K3bChecksumPipe) successfully opened pipe." << endl;

  d->start();
  return true;
}


void K3bChecksumPipe::close()
{
  d->pipeIn.closeIn();
  d->wait();
  d->close();
}


void K3bChecksumPipe::readFromFd( int fd, bool close )
{
  d->fdToReadFrom = fd;
  d->closeFdToReadFrom = close;
}


void K3bChecksumPipe::writeToFd( int fd, bool close )
{
  d->fdToWriteTo = fd;
  d->closeFdToWriteTo = close;
}


int K3bChecksumPipe::in() const
{
  return d->pipeIn.in();
}


int K3bChecksumPipe::out() const
{
  return d->pipeOut.out();
}


QCString K3bChecksumPipe::checksum() const
{
  switch( d->checksumType ) {
  case MD5:
    return d->md5.hexDigest();
  }

  return QCString();
}

