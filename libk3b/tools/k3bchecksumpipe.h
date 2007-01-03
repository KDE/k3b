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

#ifndef _K3B_CHECKSUM_PIPE_H_
#define _K3B_CHECKSUM_PIPE_H_

#include <k3bactivepipe.h>

#include <k3b_export.h>


/**
 * The checksum pipe calculates the checksum of the data
 * passed through it.
 */
class LIBK3B_EXPORT K3bChecksumPipe : public K3bActivePipe
{
 public:
  K3bChecksumPipe();
  ~K3bChecksumPipe();

  enum Type {
    MD5
  };

  /**
   * \reimplemented
   * Defaults to MD5 checksum
   */
  bool open( bool closeWhenDone = false );

  /**
   * Opens the pipe and thus starts the 
   * checksum calculation
   *
   * \param closeWhenDone If true the pipes will be closed
   *        once all data has been read.
   */
  bool open( Type type, bool closeWhenDone = false );

  /**
   * Get the calculated checksum
   */
  QCString checksum() const;

 protected:
  int write( char* data, int max );

 private:
  class Private;
  Private* d;
};

#endif
