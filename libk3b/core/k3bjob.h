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


#ifndef K3BJOB_H
#define K3BJOB_H

#include <qobject.h>
#include <qptrlist.h>
#include "k3bjobhandler.h"
#include "k3b_export.h"

class K3bDoc;
namespace K3bDevice {
  class Device;
}


/**
 * This is the baseclass for all the jobs in K3b which actually do the work like burning a cd!
 * The K3bJob object takes care of registering with the k3bcore or with a parent K3bJob.
 *
 * A K3bJob ALWAYS has to emit the signals started() and finished(bool). All other signals are
 * optional but without these two the whole registering mechanism won't work.
 *
 * @author Sebastian Trueg
 */
class LIBK3B_EXPORT K3bJob : public QObject, public K3bJobHandler
{
  Q_OBJECT

 public:
  virtual ~K3bJob();

  /**
   * \reimplemented from K3bJobHandler
   */
  bool isJob() const { return true; }

  K3bJobHandler* jobHandler() const { return m_jobHandler; }

  /**
   * Is the job active?
   * The default implementation uses the started and finished signals
   * to set an internal flag.
   */
  virtual bool active() const { return m_active; }

  /**
   * The default implementation is based on the canceled() signal.
   *
   * This means that one cannot count on this value beeing valid
   * in a slot connected to the canceled() signal.
   */
  virtual bool hasBeenCanceled() const { return m_canceled; }

  virtual QString jobDescription() const { return "K3bJob"; }
  virtual QString jobDetails() const { return QString::null; }

  /**
   * @returns the number of running subjobs.
   * this is useful for proper canceling of jobs.
   *
   * BE AWARE that the slots connected to the finished signal
   * are called in an arbitrary order. Since the running subjob
   * list is maintained via the started() and finished() signals
   * you cannot rely on numRunningSubJobs() to be acurat when called
   * from a slot connected to a subjob's finished signal.
   * You may do something like this:
   * 
   * FIXME: this blows!
   *
   * if( numRunningSubJobs() == 0 || ( numRunningSubJobs() == 1 && runningSubJobs().containsRef(job) ) )
   */
  unsigned int numRunningSubJobs() const;

  const QPtrList<K3bJob>& runningSubJobs() const { return m_runningSubJobs; }

  /**
   * Setup the following connections:
   * <table>
   * <th><td>subJob</td><td>this</td></th>
   * </table>
   */
  virtual void connectSubJob( K3bJob* subJob,
			      const char* finishedSlot = 0,
			      bool progress = false,
			      const char* progressSlot = 0,
			      const char* subProgressSot = 0,
			      const char* processedSizeSlot = 0,
			      const char* processedSubSizeSlot = 0 );

  enum MessageType { INFO, WARNING, ERROR, SUCCESS };

  /**
   * reimplemented from K3bJobHandler
   */
  int waitForMedia( K3bDevice::Device*,
		    int mediaState = K3bDevice::STATE_EMPTY,
		    int mediaType = K3bDevice::MEDIA_WRITABLE_CD,
		    const QString& message = QString::null );
  
  /**
   * reimplemented from K3bJobHandler
   */
  bool questionYesNo( const QString& text,
		      const QString& caption = QString::null );

 protected:
  K3bJob( K3bJobHandler* hdl, QObject* parent = 0, const char* name = 0 );

 public slots:
  virtual void start() = 0;
  virtual void cancel() = 0;

 signals:
  void infoMessage( const QString& msg, int type );
  void percent( int p );
  void subPercent( int p );
  void started();
  void canceled();
  void finished( bool success );
  void processedSize( int processed, int size );
  void processedSubSize( int processed, int size );
  void newTask( const QString& job );
  void newSubTask( const QString& job );
  void debuggingOutput(const QString&, const QString&);
  void data( const char* data, int len );
  void nextTrack( int track, int numTracks );

 protected slots:
  /**
   * simply converts into an infoMessage
   */
  void slotNewSubTask( const QString& str );

  /**
   * Register a subjob. Do not call this directly. The Job takes care of it
   * itself.
   */
  void registerSubJob( K3bJob* );

  /**
   * Unregister a subjob. Do not call this directly. The Job takes care of it
   * itself.
   */
  void unregisterSubJob( K3bJob* );

 private slots:
  void slotStarted();
  void slotFinished( bool );
  void slotCanceled();

 private:
  K3bJobHandler* m_jobHandler;
  QPtrList<K3bJob> m_runningSubJobs;

  bool m_canceled;
  bool m_active;
};


class LIBK3B_EXPORT K3bBurnJob : public K3bJob
{
  Q_OBJECT
	
 public:
  K3bBurnJob( K3bJobHandler* hdl, QObject* parent = 0, const char* name = 0 );
	
  /**
   * FIXME: what does this here?
   */
  virtual K3bDoc* doc() const { return 0; }
  virtual K3bDevice::Device* writer() const { return 0; }

  /**
   * use K3b::WritingApp
   */
  int writingApp() const { return m_writeMethod; }

  /**
   * K3b::WritingApp "ored" together
   */
  virtual int supportedWritingApps() const;

 public slots:
  /**
   * use K3b::WritingApp
   */
  void setWritingApp( int w ) { m_writeMethod = w; }

 signals:
  void bufferStatus( int );

  void deviceBuffer( int );

  /**
   * @param speed current writing speed in Kb
   * @param multiplicator use 150 for CDs and 1380 for DVDs
   */
  void writeSpeed( int speed, int multiplicator );

  /**
   * This signal may be used to inform when the burning starts or ends
   * The BurningProgressDialog for example uses it to enable and disable
   * the buffer and writing speed displays.
   */
  void burning(bool);

 private:
  int m_writeMethod;
};
#endif
