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
#include <qptrlist.h>




#define LIBK3B_VERSION "0.12.15"

#define k3bcore K3bCore::k3bCore()


class K3bExternalBinManager;
class K3bVersion;
class KConfig;
class KAboutData;
class K3bJob;
class K3bGlobalSettings;
class K3bPluginManager;


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
  /**
   * Although K3bCore is a singlelton it's constructor is not private to make inheritance
   * possible. Just make sure to only create one instance.
   */
  K3bCore( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bCore();

  const QPtrList<K3bJob>& runningJobs() const;

  /**
   * Equals to !runningJobs().isEmpty()
   */
  bool jobsRunning() const;

  /**
   * The default implementation scans for devices, applications, and reads the global settings.
   */
  virtual void init();

  /**
   * @param c if 0 K3bCore uses the K3b configuration
   */
  virtual void readSettings( KConfig* c = 0 );

  /**
   * @param c if 0 K3bCore uses the K3b configuration
   */
  virtual void saveSettings( KConfig* c = 0 );

  K3bDevice::DeviceManager* deviceManager() const;

  /**
   * Returns the external bin manager from K3bCore.
   *
   * By default K3bCore only adds the default programs:
   * cdrecord, cdrdao, growisofs, mkisofs, dvd+rw-format, readcd
   *
   * If you need other programs you have to add them manually like this:
   * <pre>externalBinManager()->addProgram( new K3bNormalizeProgram() );</pre>
   */
  K3bExternalBinManager* externalBinManager() const;
  K3bPluginManager* pluginManager() const;

  /**
   * Global settings used throughout libk3b. Change the settings directly in the
   * K3bGlobalSettings object. They will be saved by K3bCore::saveSettings
   */
  K3bGlobalSettings* globalSettings() const;

  /**
   * returns the version of the library as defined by LIBK3B_VERSION
   */
  const K3bVersion& version() const;

  /**
   * Default implementation returns the K3b configuration from k3brc.
   * Normally this should not be used.
   */
  virtual KConfig* config() const;

  static K3bCore* k3bCore() { return s_k3bCore; }

 public slots:
  /**
   * Every running job registers itself with the core.
   * For now this is only used to determine if some job
   * is running.
   */
  void registerJob( K3bJob* job );
  void unregisterJob( K3bJob* job );

 private:
  class Private;
  Private* d;

  static K3bCore* s_k3bCore;
};

#endif
