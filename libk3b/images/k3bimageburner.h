/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_IMAGE_BURNER_H_
#define _K3B_IMAGE_BURNER_H_

#include <k3bimagesink.h>
#include <k3bjob.h>

#include <k3b_export.h>

#include <k3bglobals.h>

namespace K3bDevice {
  class Device;
}


/**
 * base class for K3bCDImageBurner and K3bDVDImageBurner
 */
class LIBK3B_EXPORT K3bImageBurner : public K3bBurnJob, public K3bImageSink
{
  Q_OBJECT

 public:
  K3bImageBurner( K3bJobHandler*, QObject* parent = 0 );
  virtual ~K3bImageBurner();

  K3bDevice::Device* burnDevice() const { return m_burnDevice; }
  int burnSpeed() const { return m_burnSpeed; }
  K3b::WritingMode writingMode() const { return m_writingMode; }
  K3b::WritingApp writingApp() const { return m_writingApp; }

  bool simulate() const { return m_simulate; }
  bool closeMedium() const { return m_closeMedium; }

 public slots:
  /**
   * Starts the reading of the toc from the source.
   */
  void start();

  /**
   * Emits the canceled signal if the job is running and calls 
   * cancelInternal().
   */
  void cancel();

  void setBurnDevice( K3bDevice::Device* dev ) { m_burnDevice = dev; }
  void setBurnSpeed( int speed ) { m_burnSpeed = speed; }
  void setWritingMode( K3b::WritingMode mode ) { m_writingMode = mode; }
  void setWritingApp( K3b::WritingApp app ) { m_writingApp = app; }

  void setSimulate( bool b ) { m_simulate = b; }

  /**
   * If set to true the medium will be closed after burning so no other sessions
   * can be appended.
   *
   * Be aware that this does not influence DVD+RW and DVD-RW/restricted overwrite media.
   *
   * Default: true
   */
  void setCloseMedium( bool b ) { m_closeMedium = b; }

 private slots:
  void slotTocReady( bool );

 protected:
  /**
   * Use this to determine if the job has been canceled in the error case.
   */
  bool canceled() const;

  /**
   * Reimplement this to actually start the burning process.
   * The TOC has already been successfully read.
   */
  virtual void startInternal() = 0;

  /**
   * Remimplement to actually cancel the burning process.
   * The canceled signal is already emitted in cancel()
   */
  virtual void cancelInternal() = 0;

 private:
  K3bDevice::Device* m_burnDevice;
  int m_burnSpeed;
  K3b::WritingMode m_writingMode;
  K3b::WritingApp m_writingApp;

  bool m_simulate;
  bool m_closeMedium;

  class Private;
  Private* d;
};

#endif
