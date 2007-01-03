/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIOSESSION_READING_JOB_H_
#define _K3B_AUDIOSESSION_READING_JOB_H_

#include <k3bthreadjob.h>

#include <qstringlist.h>


namespace K3bDevice {
  class Device;
  class Toc;
}


class K3bAudioSessionReadingJob : public K3bThreadJob
{
  Q_OBJECT

 public:
  K3bAudioSessionReadingJob( K3bJobHandler*, QObject* parent = 0, const char* name = 0 );
  ~K3bAudioSessionReadingJob();

  /**
   * For now this simply reads all the audio tracks at the beginning
   * since we only support CD-Extra mixed mode cds.
   */
  void setDevice( K3bDevice::Device* );

  /**
   * Use for faster initialization
   */
  void setToc( const K3bDevice::Toc& toc );

  /**
   * the data gets written directly into fd instead of imagefiles.
   * To disable just set fd to -1 (the default)
   */
  void writeToFd( int fd );

  /**
   * Used if fd == -1
   */
  void setImageNames( const QStringList& l );

  void setParanoiaMode( int m );
  void setReadRetries( int );
  void setNeverSkip( bool b );

 public slots:
  void start();

 protected:
  void cleanupJob( bool success );

 private:
  class WorkThread;
  WorkThread* m_thread;
};

#endif
