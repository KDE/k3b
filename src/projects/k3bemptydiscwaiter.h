/*
 *
 * $Id$
 * Copyright (C) 2003-2004 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BEMPTYDISCWAITER_H
#define K3BEMPTYDISCWAITER_H

#include <kdialogbase.h>
#include <k3bjobhandler.h>

#include <k3bdiskinfo.h>

namespace K3bCdDevice {
  class CdDevice;
  class DeviceHandler;
}


/**
 * Tests for an empty cd in a given device.
 *
 * Use the static wait methods.
 *
 * @author Sebastian Trueg
 */
class K3bEmptyDiscWaiter : public KDialogBase, public K3bJobHandler
{
 Q_OBJECT

 public: 
  ~K3bEmptyDiscWaiter();

  /**
   * This should be replaced by the mediaType that was found or -1 for forced.
   * MEDIA_NONE if canceled.
   */
  enum returnValue { DISK_READY = 0,
		     CANCELED = -1 };

  /**
   * starts the emptydiskwaiter.
   * @param mediaState a bitwise combination of the K3bCdDevice::State enum
   * @param mediaType a bitwise combination of the MediaType enum
   * @returns the found MediaType on success, 0 if forced and -1 if canceled
   */
  int waitForDisc( int mediaState = K3bCdDevice::STATE_EMPTY,
		   int mediaType = K3bCdDevice::MEDIA_WRITABLE_CD,
		   const QString& message = QString::null );

  /**
   * the same as waitForEmptyDisc( false );
   */
  int exec();

  /**
   * @reimplemented from K3bJobHandler
   */
  int waitForMedia( K3bCdDevice::CdDevice*,
		    int mediaState = K3bCdDevice::STATE_EMPTY,
		    int mediaType = K3bCdDevice::MEDIA_WRITABLE_CD,
		    const QString& message = QString::null );
  
  /**
   * @reimplemented from K3bJobHandler
   */
  bool questionYesNo( const QString& text,
		      const QString& caption = QString::null );


  /**
   * This only openes a dialog if the first check failed.
   */
  static int wait( K3bCdDevice::CdDevice* device, 
		   bool appendable = false, 
		   int mediaType = K3bCdDevice::MEDIA_WRITABLE_CD,
		   QWidget* parent = 0 );

  static int wait( K3bCdDevice::CdDevice*,
		   int mediaState,
		   int mediaType = K3bCdDevice::MEDIA_WRITABLE_CD,
		   const QString& message = QString::null,
		   QWidget* parent = 0 );

 protected slots:
  void slotCancel();
  void slotUser1();
  void slotUser2();
  void slotUser3();
  void startDeviceHandler();
  void slotDeviceHandlerFinished( K3bCdDevice::DeviceHandler* );
  void showDialog();
  void slotErasingFinished( bool );
  void slotReloadingAfterErasingFinished( K3bCdDevice::DeviceHandler* );

 protected:
  /**
   * Use the static wait methods.
   */
  explicit K3bEmptyDiscWaiter( K3bCdDevice::CdDevice* device, QWidget* parent = 0, const char* name = 0 );

  /**
   * Nobody closes this dialog but itself!
   */
  void closeEvent( QCloseEvent* ) {}

 private:
  void finishWaiting( int );
  void prepareErasingDialog();

  QWidget* parentWidgetToUse();

  class Private;
  Private* d;
};


#endif
