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

#ifndef _K3B_DATATRACK_READER_H_
#define _K3B_DATATRACK_READER_H_


#include <k3bthreadjob.h>
#include <k3bmsf.h>

namespace K3bCdDevice {
  class CdDevice;
}


/**
 * This is a replacement for readcd. We need this since
 * it is not possible to influence the sector size used
 * by readcd and readcd is not very good to handle anyway.
 *
 * The sector size read is the following:
 * @li Mode1: 2048 bytes (onyl user data)
 * @li Mode2 Form1: 2056 bytes containing the subheader and the user data
 * @li Mode2 Form2: 2332 bytes containing the subheader and the user data
 *
 * Formless Mode2 sectors will not be read.
 */
class K3bDataTrackReader : public K3bThreadJob
{
 public:
  K3bDataTrackReader( QObject* parent = 0, const char* name = 0 );
  ~K3bDataTrackReader();

  void setDevice( K3bCdDevice::CdDevice* );

  /**
   * @param start the first sector to be read
   * @end the last sector to be read
   */
  void setSectorRange( const K3b::Msf& start, const K3b::Msf& end );
  void setRetries( int );

  /**
   * If true unreadable sectors will be replaced by zero data to always
   * maintain the track length.
   */
  void setIgnoreErrors( bool b );

  /**
   * the data gets written directly into fd instead of the imagefile.
   * Be aware that this only makes sense before starting the job.
   * To disable just set fd to -1
   */
  void writeToFd( int fd );

  void setImagePath( const QString& p );

 private:
  class WorkThread;
  WorkThread* m_thread;
};

#endif
