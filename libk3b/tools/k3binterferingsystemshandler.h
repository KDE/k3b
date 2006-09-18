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

#ifndef _K3B_INTERFERING_SYSTEMS_HANDLER_H_
#define _K3B_INTERFERING_SYSTEMS_HANDLER_H_

#include <qobject.h>
#include "k3b_export.h"

namespace K3bDevice {
  class Device;
}
class K3bJobHandler;


/**
 * This class can disable and enable local systems that
 * interfere with CD/DVD burning.
 *
 * Currently supported systems are:
 * \li KDED module mediamanager (disabled through dcop)
 * \li SuSEPlugger (killed by pid and fired up again by desktop service)
 * \li automounting systems subfs, supermount (handled through 
 *     a script running suid root)
 * \li Warns the user about other applications using the device.
 *
 * The K3bInterferingSystemsHandler will cache multiple calls to disable
 * and remember them. The interfering systems are enabled when enable
 * has been called for every call of disable.
 */
class LIBK3B_EXPORT K3bInterferingSystemsHandler : public QObject
{
  Q_OBJECT

 public:
  ~K3bInterferingSystemsHandler();

  static K3bInterferingSystemsHandler* instance();

  /**
   * Disable all interfering systems for as specific device.
   *
   * Multiple calls to disable will be cached.
   *
   * \param dev The device that should be freed of any interfering systems.
   * \param hdl If a  Job Handler is set it is used to interact with the 
   *            user. For now the only thing used is the blocking information
   *            call to notify of other applications using the device.
   */
  void disable( K3bDevice::Device* dev, K3bJobHandler* hdl = 0 );

  /**
   * Reenable all interfering systems which have been disabled by disable().
   * Does nothing if called before disable()
   *
   * \param dev The device for which the system should be reenabled.
   * \param hdl Unused for now.
   */
  void enable( K3bDevice::Device* dev, K3bJobHandler* hdl = 0 );

  static void threadSafeEnable( K3bDevice::Device* dev );
  static void threadSafeDisable( K3bDevice::Device* dev );

 signals:
  /**
   * The K3bInterferingSystemsHandler emits info messages
   * like a K3bJob. Connect to this signal to inform the
   * user about changes.
   */
  void infoMessage( const QString& message, int type );

 private:
  K3bInterferingSystemsHandler();

  void disableInternal( K3bDevice::Device* dev );
  void enableInternal( K3bDevice::Device* dev );

  int startStopMediaManager( bool start );
  int startStopMediaNotifier( bool start );
  int startStopSuSEPlugger( bool start );
  //  int startStopAutomounting( bool start, K3bDevice::Device* dev );
  int blockUnblockPmount( bool block, K3bDevice::Device* dev );

  void customEvent( QCustomEvent* e );

  static K3bInterferingSystemsHandler* s_instance;

  class Private;
  Private* d;
};

#endif
