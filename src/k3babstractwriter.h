/***************************************************************************
                          k3babstractwriter.h  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3B_ABSTRACT_WRITER_H
#define K3B_ABSTRACT_WRITER_H


#include "k3bjob.h"

#include <qdatetime.h>

class K3bDevice;


class K3bAbstractWriter : public K3bJob
{
  Q_OBJECT

 public:
  virtual ~K3bAbstractWriter();

  K3bDevice* burnDevice() const;
  int burnSpeed() const { return m_burnSpeed; }
  bool burnproof() const { return m_burnproof; }
  bool simulate() const { return m_simulate; }

  virtual bool write( const char* data, int len ) = 0;

 public slots:
  void setBurnDevice( K3bDevice* dev ) { m_burnDevice = dev; }
  void setBurnSpeed( int s ) { m_burnSpeed = s; }
  void setBurnproof( bool b ) { m_burnproof = b; }
  void setSimulate( bool b ) { m_simulate = b; }

 signals:
  void burnDeviceBuffer( int );
  void buffer( int );
  void dataWritten();
  void writeSpeed( int );

  /** 
   * writer will emit subTasks for all prewriting and postwriting stuff
   * between that only nextTrack is emitted for the job to create it's own 
   * subTasks
   */
  void nextTrack( int, int );

 protected:
  K3bAbstractWriter( K3bDevice* dev, QObject* parent = 0, const char* name = 0 );

 private slots:
  /** calculates the current write speed */
  void slotProcessedSize(int, int);
  void slotFinished(bool);

 private:
  K3bDevice* m_burnDevice;
  int m_burnSpeed;
  bool m_burnproof;
  bool m_simulate;

  // used for write speed calculation
  QTime m_lastWriteSpeedCalcTime;
  int m_lastWrittenBytes;
  bool m_started;
};


#endif
