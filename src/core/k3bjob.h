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
#include "k3bjobhandler.h"


class K3bDoc;
namespace K3bCdDevice {
  class CdDevice;
}


/**This is the baseclass for all the jobs in K3b which actually do the work like burning a cd!
  *@author Sebastian Trueg
  */

class K3bJob : public QObject, public K3bJobHandler
{
  Q_OBJECT

 public:
  virtual ~K3bJob();

  virtual bool active() const { return false; }

  virtual QString jobDescription() const { return "K3bJob"; }
  virtual QString jobDetails() const { return QString::null; }

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
  int waitForMedia( K3bCdDevice::CdDevice*,
		    int mediaState = K3bCdDevice::STATE_EMPTY,
		    int mediaType = K3bCdDevice::MEDIA_WRITABLE_CD,
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

 private:
  K3bJobHandler* m_jobHandler;
};


class K3bBurnJob : public K3bJob
{
  Q_OBJECT
	
 public:
  K3bBurnJob( K3bJobHandler* hdl, QObject* parent = 0, const char* name = 0 );
	
  virtual K3bDoc* doc() const { return 0; }
  virtual K3bCdDevice::CdDevice* writer() const { return 0; }

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
