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

#include "k3bactivepipe.h"

#include <k3bpipe.h>

#include <kdebug.h>

#include <qthread.h>
#include <qiodevice.h>

#include <unistd.h>


class K3bActivePipe::Private : public QThread
{
public:
  Private( K3bActivePipe* pipe ) :
    m_pipe( pipe ),
    fdToReadFrom(-1),
    fdToWriteTo(-1),
    sourceIODevice(0),
    sinkIODevice(0),
    closeFdToReadFrom(false),
    closeFdToWriteTo(false) {
  }

  void run() {
    kdDebug() << "(K3bActivePipe) started thread." << endl;
    bytesRead = bytesWritten = 0;
    buffer.resize( 10*2048 );
    ssize_t r = 0;
    while( ( r = m_pipe->read( buffer.data(), buffer.size() ) ) > 0 ) {

      bytesRead += r;

      // write it out
      ssize_t w = 0;
      ssize_t ww = 0;
      while( w < r ) {
	if( ( ww = m_pipe->write( buffer.data()+w, r-w ) ) > 0 ) {
	  w += ww;
	  bytesWritten += ww;
	}
	else {
	  kdDebug() << "(K3bActivePipe) write failed." << endl;
	  close( closeWhenDone );
	  return;
	}
      }
    }
    //    kdDebug() << "(K3bActivePipe) thread done: " << r << " (total bytes read/written: " << bytesRead << "/" << bytesWritten << ")" << endl;
    close( closeWhenDone );
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

  void close( bool closeAll ) {
    if( sourceIODevice )
      sourceIODevice->close();
    if( sinkIODevice )
      sinkIODevice->close();

    if( closeAll ) {
      pipeIn.close();
      pipeOut.close();
      if( fdToWriteTo != -1 &&
	  closeFdToWriteTo )
	::close( fdToWriteTo );
      
      if( fdToReadFrom != -1 &&
	  closeFdToReadFrom )
	::close( fdToReadFrom );
    }
  }

private:
  K3bActivePipe* m_pipe;

public:
  int fdToReadFrom;
  int fdToWriteTo;
  K3bPipe pipeIn;
  K3bPipe pipeOut;

  QIODevice* sourceIODevice;
  QIODevice* sinkIODevice;

  bool closeWhenDone;
  bool closeFdToReadFrom;
  bool closeFdToWriteTo;

  QByteArray buffer;

  Q_UINT64 bytesRead;
  Q_UINT64 bytesWritten;
};


K3bActivePipe::K3bActivePipe()
{
  d = new Private( this );
}


K3bActivePipe::~K3bActivePipe()
{
  delete d;
}


bool K3bActivePipe::open( bool closeWhenDone )
{
  if( d->running() )
    return false;

  d->closeWhenDone = closeWhenDone;

  if( d->sourceIODevice ) {
    if( !d->sourceIODevice->open( IO_ReadOnly ) )
      return false;
  }
  else if( d->fdToReadFrom == -1 && !d->pipeIn.open() ) {
    return false;
  }

  if( d->sinkIODevice ) {
    if( !d->sinkIODevice->open( IO_WriteOnly ) )
      return false;
  }
  else if( d->fdToWriteTo == -1 && !d->pipeOut.open() ) {
    close();
    return false;
  }

  kdDebug() << "(K3bActivePipe) successfully opened pipe." << endl;

  d->start();
  return true;
}


void K3bActivePipe::close()
{
  d->pipeIn.closeIn();
  d->wait();
  d->close( true );
}


void K3bActivePipe::readFromFd( int fd, bool close )
{
  d->fdToReadFrom = fd;
  d->sourceIODevice = 0;
  d->closeFdToReadFrom = close;
}


void K3bActivePipe::writeToFd( int fd, bool close )
{
  d->fdToWriteTo = fd;
  d->sinkIODevice = 0;
  d->closeFdToWriteTo = close;
}


void K3bActivePipe::readFromIODevice( QIODevice* dev )
{
  d->fdToReadFrom = -1;
  d->sourceIODevice = dev;
}


void K3bActivePipe::writeToIODevice( QIODevice* dev )
{
  d->fdToWriteTo = -1;
  d->sinkIODevice = dev;
}


int K3bActivePipe::in() const
{
  return d->pipeIn.in();
}


int K3bActivePipe::out() const
{
  return d->pipeOut.out();
}


int K3bActivePipe::read( char* data, int max )
{
  if( d->sourceIODevice )
    return d->sourceIODevice->readBlock( data, max );
  else
    return ::read( d->readFd(), data, max );
}


int K3bActivePipe::write( char* data, int max )
{
  if( d->sinkIODevice )
    return d->sinkIODevice->writeBlock( data, max );
  else
    return ::write( d->writeFd(), data, max );
}


bool K3bActivePipe::pumpSync()
{
  if( open( true ) )
    d->wait();
  else
    return false;
  return true;
}


Q_UINT64 K3bActivePipe::bytesRead() const
{
  return d->bytesRead;
}


Q_UINT64 K3bActivePipe::bytesWritten() const
{
  return d->bytesWritten;
}
