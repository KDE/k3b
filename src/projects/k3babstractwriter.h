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


#ifndef K3B_ABSTRACT_WRITER_H
#define K3B_ABSTRACT_WRITER_H


#include "k3bjob.h"

#include <qdatetime.h>

class K3bCdDevice::CdDevice;


class K3bAbstractWriter : public K3bJob
{
  Q_OBJECT

 public:
  virtual ~K3bAbstractWriter();

  K3bCdDevice::CdDevice* burnDevice() const;
  int burnSpeed() const { return m_burnSpeed; }
  bool burnproof() const { return m_burnproof; }
  bool simulate() const { return m_simulate; }

  virtual bool write( const char* data, int len ) = 0;

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

  void setBurnDevice( K3bCdDevice::CdDevice* dev ) { m_burnDevice = dev; }
  void setBurnSpeed( int s ) { m_burnSpeed = s; }
  void setBurnproof( bool b ) { m_burnproof = b; }
  void setSimulate( bool b ) { m_simulate = b; }

 signals:
  void burnDeviceBuffer( int );
  void buffer( int );
  void dataWritten();
  void writeSpeed( int, int );

 protected:
  K3bAbstractWriter( K3bCdDevice::CdDevice* dev, QObject* parent = 0, const char* name = 0 );

 protected slots:
  void slotUnblockWhileCancellationFinished( bool success );
  void slotEjectWhileCancellationFinished( bool success );

 private:
  K3bCdDevice::CdDevice* m_burnDevice;
  int m_burnSpeed;
  bool m_burnproof;
  bool m_simulate;
};


#endif
