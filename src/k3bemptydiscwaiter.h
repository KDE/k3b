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


#ifndef K3BEMPTYDISCWAITER_H
#define K3BEMPTYDISCWAITER_H

#include <kdialogbase.h>
#include "device/k3bdevice.h"



/**
 * Tests for an empty cd in a given device.
 * emits signal discReady if an empty disc was found.
 * K3bEmptyDiscWaiter will go on testing until the
 * slot canceled was called or an empty disc was found.
 *
 * @author Sebastian Trueg
 */
class K3bEmptyDiscWaiter : public KDialogBase
{
 Q_OBJECT

 public: 
  K3bEmptyDiscWaiter( K3bDevice* device, QWidget* parent = 0, const char* name = 0 );
  ~K3bEmptyDiscWaiter();

  enum returnValue { DISK_READY, CANCELED };

  /**
   * starts the emptydiskwaiter.
   * @param appendable if true a not empty but appendable disk is also
   *                   considered as valid.
   * @returns DISK_READY or CANCELED
   */
  int waitForEmptyDisc( bool appendable = false );

  /**
   * the same as waitForEmptyDisc( false );
   */
  int exec();

  /**
   * This only openes a dialog if the first check failed.
   */
  static int wait( K3bDevice* device, bool appendable = false );

 signals:
  void canceled();
  void discReady();

 protected slots:
  void slotCancel();
  void slotUser1();
  void startDeviceHandler();
  void slotDeviceHandlerFinished( bool );

 protected:
  void closeEvent( QCloseEvent* ) {}

 private:
  void finishWaiting( int );

  class Private;
  Private* d;
};


#endif
