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


#ifndef _K3B_THREAD_H_
#define _K3B_THREAD_H_

#include <qthread.h>


class QObject;

/**
 * The threaded couterpart to K3bJob
 * instead of emitting the information signals
 * one has to use the emitXXX methods which will post
 * K3bProgressInfoEvents to the eventhandler
 */
class K3bThread : public QThread
{
 public:
  K3bThread( QObject* eventHandler = 0 );
  K3bThread( unsigned int stackSize, QObject* eventHandler = 0  );
  virtual ~K3bThread();

  void setProgressInfoEventHandler( QObject* eventHandler );

  /**
   * to provide the same api like K3bJob
   * the default implementation calls terminate and
   * emitCancled() and emitFinished(false)
   */
  virtual void cancel();

  virtual QString jobDescription() const;
  virtual QString jobDetails() const;

  /**
   * waits until all running K3bThread have finished.
   * This is used by K3bApplication.
   */
  static void waitUntilFinished();

 protected:
  virtual void run() = 0;

  /**
   * uses the K3bJob::MessageType enum
   */
  void emitInfoMessage( const QString& msg, int type );
  void emitPercent( int p );
  void emitSubPercent( int p );
  void emitStarted();
  void emitCanceled();
  void emitFinished( bool success );
  void emitProcessedSize( int processed, int size );
  void emitProcessedSubSize( int processed, int size );
  void emitNewTask( const QString& job );
  void emitNewSubTask( const QString& job );
  void emitDebuggingOutput(const QString&, const QString&);
  void emitData( const char* data, int len );
  void emitNextTrack( int track, int trackNum );

 private:
  class Private;
  Private* d;
};

#endif
