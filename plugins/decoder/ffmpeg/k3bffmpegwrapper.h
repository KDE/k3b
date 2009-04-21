/* 
 *
 *
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

#ifndef _K3B_FFMPEG_WRAPPER_H_
#define _K3B_FFMPEG_WRAPPER_H_

#include "k3bmsf.h"



/**
 * Create with K3bFFMpegWrapper::open
 */
class K3bFFMpegFile
{
  friend class K3bFFMpegWrapper;

 public:
  ~K3bFFMpegFile();

  const QString& filename() const { return m_filename; }

  bool open();
  void close();

  K3b::Msf length() const;
  int sampleRate() const;
  int channels() const;

  /**
   * ffmpeg internal enumeration
   */
  int type() const;
  QString typeComment() const;

  QString title() const;
  QString author() const;
  QString comment() const;

  int read( char* buf, int bufLen );
  bool seek( const K3b::Msf& );

 private:
  K3bFFMpegFile( const QString& filename );
  int readPacket();
  int fillOutputBuffer();

  QString m_filename;

  class Private;
  Private* d;
};


class K3bFFMpegWrapper
{
 public:
  ~K3bFFMpegWrapper();

  /**
   * returns 0 on failure.
   */
  K3bFFMpegFile* open( const QString& filename ) const;

  static K3bFFMpegWrapper* instance();

 private:
  K3bFFMpegWrapper();

  static K3bFFMpegWrapper* s_instance;
};

#endif
