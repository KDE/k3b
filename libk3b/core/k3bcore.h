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


#ifndef _K3B_CORE_H_
#define _K3B_CORE_H_

#include <qobject.h>


#define k3bcore K3bCore::k3bCore()


class K3bExternalBinManager;
class K3bVersion;
class KConfig;
class KAboutData;
class K3bJob;


namespace K3bDevice {
  class DeviceManager;
}


/**
 * The K3b core takes care of the managers. 
 * This has been separated from K3bApplication to 
 * make creating a K3bPart easy.
 * This is the heart of the K3b system. Every plugin may use this
 * to get the information it needs.
 */
class K3bCore : public QObject
{
  Q_OBJECT

 public:
  K3bCore( const K3bVersion&, KConfig* = 0, QObject* parent = 0, const char* name = 0 );
  virtual ~K3bCore();

  bool jobsRunning() const;

  void init();
  void saveConfig();

  K3bDevice::DeviceManager* deviceManager() const;
  K3bExternalBinManager* externalBinManager() const;

  const K3bVersion& version() const;

  KConfig* config() const;

  static K3bCore* k3bCore() { return s_k3bCore; }

 public slots:
  /**
   * This will just emit the busyInfoRequested signal
   * Anyone may connect to it and show the string to the
   * user in some way.
   */
  void requestBusyInfo( const QString& );
  void requestBusyFinish();

  /**
   * Every running job registers itself with the core.
   * For now this is only used to determine if some job
   * is running.
   */
  void registerJob( K3bJob* job );
  void unregisterJob( K3bJob* job );

 signals:
  /**
   * This is used for showing info in the K3b splashscreen
   * and should really be moved somewhere else!
   */
  void initializationInfo( const QString& );

  /**
   * Any component may request busy info
   * In the K3b main app this will be displayed
   * as a moving square in the taskbar
   */
  void busyInfoRequested( const QString& );

  void busyFinishRequested();

 private:
  class Private;
  Private* d;

  static K3bCore* s_k3bCore;
};

#endif
