/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_THREAD_JOB_H_
#define _K3B_THREAD_JOB_H_

#include "k3bjob.h"

class QCustomEvent;
class K3bThread;


/**
 * A Wrapper to use a K3bThread just like a K3bJob.
 * Usage:
 * <pre>
 *   K3bThread* thread = new MySuperThread(...);
 *   K3bThreadJob* job = new K3bThreadJob( thread, ... );
 *   K3bBurnProgressDialog d;
 *   d.setJob(job);
 *   job->start();
 *   d.exec();
 *   delete job;
 * </pre>
 * Be aware that K3bThreadJob'd destructor does NOT delete the thread.
 */
class K3bThreadJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bThreadJob( QObject* parent = 0, const char* name = 0 );
  K3bThreadJob( K3bThread*, QObject* parent = 0, const char* name = 0 );
  virtual ~K3bThreadJob();

  void setThread( K3bThread* t );
  K3bThread* thread() const { return m_thread; }

 public slots:
  void start();
  void cancel();

 protected:
  /**
   * converts K3bThread events to K3bJob signals
   */
  virtual void customEvent( QCustomEvent* );

 private:
  K3bThread* m_thread;
};

#endif

