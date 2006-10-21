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

#ifndef _K3B_CHECKSUM_PIPE_H_
#define _K3B_CHECKSUM_PIPE_H_

#include <qcstring.h>

/**
 * The checksum pipe calculates the checksum of the data
 * passed through it.
 */
class K3bChecksumPipe
{
 public:
  K3bChecksumPipe();
  ~K3bChecksumPipe();

  enum Type {
    MD5
  };

  /**
   * Opens the pipe and thus starts the 
   * checksum calculation
   *
   * \param closeWhenDone If true the pipes will be closed
   *        once all data has been read.
   */
  bool open( int type = MD5, bool closeWhenDone = false );

  /**
   * Close the pipe and finish the checksum.
   */
  void close();

  /**
   * Set the file descriptor to read from. If this is -1 (the default) then
   * data has to be piped into the in() file descriptor.
   *
   * \param fd The file descriptor to read from.
   * \param close If true the reading file descriptor will be closed on a call to close()
   */
  void readFromFd( int fd, bool close = false );

  /**
   * Set the file descriptor to write to. If this is -1 (the default) then
   * data has to read from the out() file descriptor.
   *
   * \param fd The file descriptor to write to.
   * \param close If true the reading file descriptor will be closed on a call to close()
   */
  void writeToFd( int fd, bool close = false );

  /**
   * The file descriptor to write into
   * Only valid if no fd to read from has been set
   */
  int in() const;

  /**
   * The file descriptor to read from
   * Only valid if no fd to write to has been set
   */
  int out() const;

  /**
   * Get the calculated checksum
   */
  QCString checksum() const;

 private:
  class Private;
  Private* d;
};

#endif
