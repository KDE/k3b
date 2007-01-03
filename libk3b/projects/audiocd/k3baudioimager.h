/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_IMAGER_H_
#define _K3B_AUDIO_IMAGER_H_

#include <k3bthreadjob.h>

class K3bAudioDoc;

class K3bAudioImager : public K3bThreadJob
{
  Q_OBJECT

 public:
  K3bAudioImager( K3bAudioDoc*, K3bJobHandler*, QObject* parent = 0, const char* name = 0 );
  ~K3bAudioImager();

  /**
   * the data gets written directly into fd instead of the imagefile.
   * Be aware that this only makes sense before starting the job.
   * To disable just set fd to -1
   */
  void writeToFd( int fd );

  /**
   * Image path. Should be an empty directory or a non-existing
   * directory in which case it will be created.
   */
  void setImageFilenames( const QStringList& p );

  enum ErrorType {
    ERROR_FD_WRITE,
    ERROR_DECODING_TRACK,
    ERROR_UNKNOWN
  };

  ErrorType lastErrorType() const;

 private:
  K3bAudioDoc* m_doc;

  class WorkThread;
  WorkThread* m_thread;
};

#endif
