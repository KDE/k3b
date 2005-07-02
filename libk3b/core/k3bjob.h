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
   * The default implementation is based on the jobStarted() and jobFinished()
   * methods and there is normally no need to reimplement this.
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
   */
  unsigned int numRunningSubJobs() const;

  const QPtrList<K3bJob>& runningSubJobs() const { return m_runningSubJobs; }

  virtual void connectSubJob( K3bJob* subJob,
			      const char* finishedSlot = 0,
			      bool progress = false,
			      const char* progressSlot = 0,
			      const char* subProgressSot = 0,
			      const char* processedSizeSlot = 0,
			      const char* processedSubSizeSlot = 0 );

  /**
   * Message types to be used in combination with the infoMessage signal.
   *
   * \see infoMessage()
   */
  enum MessageType { 
    INFO,     /**< Informational message. For example a message that informs the user about what is
		 currently going on */
    WARNING,  /**< A warning message. Something did not go perfectly but the job may continue. */
    ERROR,    /**< An error. Only use this message type if the job will actually fail afterwards
		 with a call to jobFinished( false ) */
    SUCCESS   /**< This message type may be used to inform the user that a sub job has 
		 been successfully finished. */
  };

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

 public slots:
  /**
   * This is the slot that starts the job. The first call should always
   * be jobStarted().
   *
   * Once the job has finished it has to call jobFinished() with the result as
   * a parameter.
   *
   * \see jobStarted()
   * \see jobFinished()
   */
  virtual void start() = 0;

  /**
   * This slot should cancel the job. The job has to emit the canceled() signal and make a call
   * to jobFinished().
   * It is not important to do any of those two directly in this slot though.
   */
  virtual void cancel() = 0;

  void setJobHandler( K3bJobHandler* );

 signals:
  void infoMessage( const QString& msg, int type );
  void percent( int p );
  void subPercent( int p );
  void processedSize( int processed, int size );
  void processedSubSize( int processed, int size );
  void newTask( const QString& job );
  void newSubTask( const QString& job );
  void debuggingOutput(const QString&, const QString&);
  void data( const char* data, int len );
  void nextTrack( int track, int numTracks );

  void canceled();

  /**
   * Emitted once the job has been started. Never emit this signal directly.
   * Use jobStarted() instead, otherwise the job will not be properly registered
   */
  void started();

  /**
   * Emitted once the job has been finshed. Never emit this signal directly.
   * Use jobFinished() instead, otherwise the job will not be properly deregistered
   */
  void finished( bool success );

 protected:
  /**
   * \param hdl the handler of the job. This allows for some user interaction without
   *            specifying any details (like the GUI).
   *            The job handler can also be another job. In that case this job is a sub job
   *            and will be part of the parents running sub jobs.
   *
   * \see runningSubJobs()
   * \see numRunningSubJobs()
   */
  K3bJob( K3bJobHandler* hdl, QObject* parent = 0, const char* name = 0 );

  /**
   * Call this in start() to properly register the job and emit the started() signal.
   * Do never emit the started() signal manually.
   */
  void jobStarted();

  /**
   * Call this at the end of the job to properly deregister the job and emit the finished() signal.
   * Do never emit the started() signal manually.
   */
  void jobFinished( bool success );

 private slots:
  void slotCanceled();
  void slotNewSubTask( const QString& str );

 private:
  void registerSubJob( K3bJob* );
  void unregisterSubJob( K3bJob* );

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
   * FIXME: maybe one should be able to ask the burnjob if it burns a CD or a DVD and remove the 
   *        multiplicator parameter)
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
