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

#include "k3bimagesink.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>


class K3bImageSink::Private
{
public:
  int fds[2];
};


K3bImageSink::K3bImageSink()
  : m_source( 0 )
{
  d = new Private;
  d->fds[0] = -1;
  d->fds[1] = -1;
}


K3bImageSink::~K3bImageSink()
{
  closeFdPair();
  delete d;
}


bool K3bImageSink::openFdPair()
{
  closeFdPair();

  if( socketpair(AF_UNIX, SOCK_STREAM, 0, d->fds) == 0 ) {
    fcntl(d->fds[0], F_SETFD, FD_CLOEXEC);
    fcntl(d->fds[1], F_SETFD, FD_CLOEXEC);

    return true;
  }
  else
    return false;
}


void K3bImageSink::closeFdPair()
{
  if( d->fds[0] != -1 ) {
    ::close( d->fds[0] );
    ::close( d->fds[1] );
    d->fds[0] = -1;
    d->fds[1] = -1;
  }
}


int K3bImageSink::fdIn() const
{
  return d->fds[1];
}


int K3bImageSink::fdOut() const
{
  return d->fds[0];
}



