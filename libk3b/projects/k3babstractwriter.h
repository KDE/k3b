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


#ifndef K3B_ABSTRACT_WRITER_H
#define K3B_ABSTRACT_WRITER_H


#include "k3bjob.h"

#include <qdatetime.h>

class K3bDevice::Device;
class K3bJobHandler;


class K3bAbstractWriter : public K3bJob
{
  Q_OBJECT

 public:
  virtual ~K3bAbstractWriter();

  K3bDevice::Device* burnDevice() const;
  int burnSpeed() const { return m_burnSpeed; }
  bool simulate() const { return m_simulate; }

  /**
   * This can be used to setup direct streaming between two processes
   * for example the cdrecordwriter returnes the stdin fd which can be
   * connected to the stdout fd of mkisofs in the isoimager
   */
  virtual int fd() const { return -1; }
  virtual bool closeFd() { return false; }

 public slots:
  /**
   * If the burnDevice is set this will try to unlock the drive and
   * eject the disk if K3b is configured to do so.
   * Will also emit canceled and finished signals.
   * may be called by subclasses.
   */
  void cancel();

  void setBurnDevice( K3bDevice::Device* dev ) { m_burnDevice = dev; }
  void setBurnSpeed( int s ) { m_burnSpeed = s; }
  void setSimulate( bool b ) { m_simulate = b; }

  /**
   * Used to inform the writer that the source (especially useful when reading from 
   * another cd/dvd media) could not be read.
   *
   * Basicly it should be used to make sure no "write an email" message is thrown.
   */
  void setSourceUnreadable( bool b = true ) { m_sourceUnreadable = b; }

 signals:
  void buffer( int );
  void deviceBuffer( int );
  void writeSpeed( int, int );

 protected:
  K3bAbstractWriter( K3bDevice::Device* dev, K3bJobHandler* hdl, 
		     QObject* parent = 0, const char* name = 0 );

  bool wasSourceUnreadable() const { return m_sourceUnreadable; }

 protected slots:
  void slotUnblockWhileCancellationFinished( bool success );
  void slotEjectWhileCancellationFinished( bool success );

 private:
  K3bDevice::Device* m_burnDevice;
  int m_burnSpeed;
  bool m_simulate;
  bool m_sourceUnreadable;
};


#endif
