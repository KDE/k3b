/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bpipebuffer.h"

#include <k3bthread.h>

#include <klocale.h>
#include <kdebug.h>

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>


//
// This one is based on the little pipebuf2 program by Peter Osterlund <petero2@telia.com>
//


class K3bPipeBuffer::WorkThread : public K3bThread
{
public:
  WorkThread()
    : K3bThread(),
      buffer(0),
      bufSize(4*1024*1024),
      canceled(false) {
    outFd = inFd = -1;
    inFdPair[0] = inFdPair[1] = -1;
  }

  ~WorkThread() {
    delete [] buffer;
  }

  bool initFds() {
    if( inFd == -1 ) {
      if( ::socketpair(AF_UNIX, SOCK_STREAM, 0, inFdPair) ) {
      //      if( ::pipe( inFdPair ) ) {
	kdDebug() << "(K3bPipeBuffer::WorkThread) unable to create socketpair" << endl;
	inFdPair[0] = inFdPair[1] = -1;
	return false;
      }
      else {
	::fcntl(inFdPair[0], F_SETFL, O_NONBLOCK);
	::fcntl(outFd, F_SETFL, O_NONBLOCK);
      }
    }
    else {
      ::fcntl(inFd, F_SETFL, O_NONBLOCK);
    }

    delete [] buffer;
    buffer = new char[bufSize];

    return (buffer != 0);
  }

  void run() {
    emitStarted();

    int usedInFd = -1;
    if( inFd > 0 )
      usedInFd = inFd;
    else
      usedInFd = inFdPair[0];

    kdDebug() << "(K3bPipeBuffer::WorkThread) reading from " << usedInFd 
	      << " and writing to " << outFd << endl;
    kdDebug() << "(K3bPipeBuffer::WorkThread) using buffer size of " << bufSize << endl;

    // start the buffering
    unsigned int bufPos = 0;
    unsigned int dataLen = 0;
    bool eof = false;
    bool error = false;
    canceled = false;
    int oldPercent = 0;

    static const unsigned int MAX_BUFFER_READ = 2048*3;

    while( !canceled && !error && (!eof || dataLen > 0) ) {
      //
      // create two fd sets
      //
      fd_set readFds, writeFds;
      FD_ZERO(&readFds);
      FD_ZERO(&writeFds);

      //
      // fill the fd sets
      //
      if( !eof && dataLen < bufSize )
	FD_SET(usedInFd, &readFds);
      if( dataLen > 0 )
	FD_SET(outFd, &writeFds);

      //
      // wait for data
      //
      int ret = select( QMAX(usedInFd, outFd) + 1, &readFds, &writeFds, NULL, NULL);

      //
      // Do the buffering
      //
      if( !canceled && ret > 0 ) {

	int percent = -1;

	//
	// Read from the buffer and write to the output
	//
	if( FD_ISSET(outFd, &writeFds) ) {
	  unsigned int maxLen = QMIN(bufSize - bufPos, dataLen);

	  ret = ::write( outFd, &buffer[bufPos], maxLen );
	  
	  if( ret < 0 ) {
	    if( (errno != EINTR) && (errno != EAGAIN) ) {
	      kdDebug() << "(K3bPipeBuffer::WorkThread) error while writing to " << outFd << endl;
	      error = true;
	    }
	  }
	  else {
	    //
	    // we always emit before the reading from the buffer since
	    // it makes way more sense to show the buffer before the reading.
	    //
	    percent = (int)((double)dataLen*100.0/(double)bufSize);

	    bufPos = (bufPos + ret) % bufSize;
	    dataLen -= ret;
	  }
	}

	//
	// Read into the buffer
	//
	else if( FD_ISSET(usedInFd, &readFds) ) {
	  unsigned int readPos = (bufPos + dataLen) % bufSize;
	  unsigned int maxLen = QMIN(bufSize - readPos, bufSize - dataLen);
	  //
	  // never read more than xxx bytes
	  // This is some tuning to prevent the reading from blocking the whole thread
	  // 
	  if( maxLen > MAX_BUFFER_READ ) // some dummy value below 1 MB
	    maxLen = MAX_BUFFER_READ;
	  ret = ::read( usedInFd, &buffer[readPos], maxLen );
	  if( ret < 0 ) {
	    if( (errno != EINTR) && (errno != EAGAIN) ) {
	      kdDebug() << "(K3bPipeBuffer::WorkThread) error while reading from " << usedInFd << endl;
	      error = true;
	    }
	  } 
	  else if( ret == 0 ) {
	    kdDebug() << "(K3bPipeBuffer::WorkThread) end of input." << endl;
	    eof = true;
	  }
	  else {
	    dataLen += ret;

	    percent = (int)((double)dataLen*100.0/(double)bufSize);
	  }
	}
 
	// A little hack to keep the buffer display from flickering
	if( percent == 99 )
	  percent = 100;

	if( percent != -1 && percent != oldPercent ) {
	  emitPercent( percent );
	  oldPercent = percent;
	}
      }
      else if( !canceled ) {
	error = true;
	kdDebug() << "(K3bPipeBuffer::WorkThread) select: " << ::strerror(errno) << endl;
      }
    }

    if( inFd == -1 ) {
      ::close( inFdPair[0] );
      ::close( inFdPair[1] );
      inFdPair[0] = inFdPair[1] = -1;
    }

    //
    // close the fd we are writing to (this is need to make growisofs happy
    // TODO: perhaps make this configurable
    //
    ::close( outFd );

    if( canceled )
      emitCanceled();
    emitFinished( !error && !canceled );
  }

  char* buffer;
  size_t bufSize;
  int outFd;
  int inFd;
  int inFdPair[2];
  bool canceled;
};


K3bPipeBuffer::K3bPipeBuffer( K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bThreadJob( jh, parent, name )
{
  m_thread = new WorkThread();
  setThread( m_thread );
}


K3bPipeBuffer::~K3bPipeBuffer()
{
  delete m_thread;
}


void K3bPipeBuffer::start()
{
  //
  // Create the socketpair in the gui thread to be sure it's available after 
  // this method returns.
  //
  if( !m_thread->initFds() )
    jobFinished(false);
  else
    K3bThreadJob::start();
}


void K3bPipeBuffer::cancel()
{
  m_thread->canceled = true;
}


void K3bPipeBuffer::setBufferSize( int mb )
{
  m_thread->bufSize = mb * 1024 * 1024;
}


void K3bPipeBuffer::readFromFd( int fd )
{
  m_thread->inFd = fd;
}


void K3bPipeBuffer::writeToFd( int fd )
{
  m_thread->outFd = fd;
}


int K3bPipeBuffer::inFd() const
{
  if( m_thread->inFd == -1 )
    return m_thread->inFdPair[1];
  else
    return m_thread->inFd;
}
