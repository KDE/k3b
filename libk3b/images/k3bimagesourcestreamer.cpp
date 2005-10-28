/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bimagesourcestreamer.h"
#include "k3bimagesource.h"

#include <k3bthread.h>

#include <unistd.h>


class K3bImageSourceStreamer::WorkThread : public K3bThread
{
public:
  WorkThread()
    : K3bThread(),
      m_canceled(false) {
  }

  void cancel() {
    m_canceled = true;
  }

  K3bImageSource* source;

protected:
  void run() {
    emitStarted();
    m_canceled = false;
    bool success = true;

    // FIXME: count read data and cut or pad the data

    char data[10*2048];
    long read = 0;
    while( !m_canceled && (read = source->read( data, 10*2048 )) > 0 ) {
      if( ::write( source->fdToWriteTo(), data, read ) != read ) {
	success = false;
	break;
      }
    }

    if( read < 0 || m_canceled )
      success = false;

    if( m_canceled )
      emitCanceled();
    emitFinished( success );
  }

private:
  bool m_canceled;
};


K3bImageSourceStreamer::K3bImageSourceStreamer( K3bJobHandler* jh, QObject* parent )
  : K3bThreadJob( jh, parent )
{
  m_thread = new WorkThread();
  setThread( m_thread );
}


K3bImageSourceStreamer::~K3bImageSourceStreamer()
{
  delete m_thread;
}


void K3bImageSourceStreamer::setSource( K3bImageSource* s )
{
  m_thread->source = s;
}
