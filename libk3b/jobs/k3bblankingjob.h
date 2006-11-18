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

#ifndef K3B_BLANKING_JOB_H
#define K3B_BLANKING_JOB_H

#include <k3bjob.h>
#include "k3b_export.h"
class KProcess;
class QString;
class K3bDevice::Device;
class K3bAbstractWriter;


class LIBK3B_EXPORT K3bBlankingJob : public K3bBurnJob
{
  Q_OBJECT

 public:
  K3bBlankingJob( K3bJobHandler*, QObject* parent = 0 );
  ~K3bBlankingJob();

  K3bDevice::Device* writer() const;

  bool hasBeenCanceled() const { return m_canceled; }

  enum blank_mode { Fast, Complete, Track, Unclose, Session };

 public slots:
  void start();
  void cancel();
  void setForce( bool f ) { m_force = f; }
  void setDevice( K3bDevice::Device* d );
  void setSpeed( int s ) { m_speed = s; }
  void setMode( int m ) { m_mode = m; }
  void setWritingApp (int app) { m_writingApp = app; }

  /**
   * If set true the job ignores the global K3b setting
   * and does not eject the CD-RW after finishing
   */
  void setForceNoEject( bool b ) { m_forceNoEject = b; }

 private slots:
  void slotFinished(bool);
  void slotStartErasing();

 private:
  K3bAbstractWriter* m_writerJob;
  bool m_force;
  K3bDevice::Device* m_device;
  int m_speed;
  int m_mode;
  int m_writingApp;
  bool m_canceled;
  bool m_forceNoEject;
};

#endif
