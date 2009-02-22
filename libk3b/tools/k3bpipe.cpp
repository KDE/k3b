/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bpipe.h"

#include <kdebug.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


K3b::Pipe::Pipe()
{
    m_fd[0] = m_fd[1] = -1;
}


K3b::Pipe::~Pipe()
{
    close();
}


bool K3b::Pipe::open()
{
    close();

    if( ::socketpair( AF_UNIX, SOCK_STREAM, 0, m_fd ) == 0 ) {
        fcntl( m_fd[0], F_SETFD, FD_CLOEXEC );
        fcntl( m_fd[1], F_SETFD, FD_CLOEXEC );
        return true;
    }
    else {
        kDebug() << "(K3b::Pipe) failed to setup socket pair.";
        return false;
    }
}


void K3b::Pipe::closeIn()
{
    if( m_fd[1] != -1 ) {
        ::close( m_fd[1] );
        m_fd[1] = -1;
    }
}


void K3b::Pipe::closeOut()
{
    if( m_fd[0] != -1 ) {
        ::close( m_fd[0] );
        m_fd[0] = -1;
    }
}


void K3b::Pipe::close()
{
    closeIn();
    closeOut();
}
