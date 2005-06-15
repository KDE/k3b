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


#ifndef _K3B_APPLICATION_H_
#define _K3B_APPLICATION_H_

#include <kuniqueapplication.h>
#include <k3bcore.h>

#define k3bappcore K3bApplication::Core::k3bAppCore()


class K3bMainWindow;
class K3bInterface;
class K3bAudioServer;
class K3bThemeManager;
class K3bProjectManager;
class K3bAppDeviceManager;


class K3bApplication : public KUniqueApplication
{
  Q_OBJECT

 public:
  K3bApplication();
  ~K3bApplication();

  void init();

  int newInstance();

  class Core;

 signals:
  void initializationInfo( const QString& );
  void initializationDone();

 private slots:
  void slotShutDown();

 private:
  bool processCmdLineArgs();

  K3bInterface* m_interface;
  Core* m_core;
  K3bAudioServer* m_audioServer;
  K3bMainWindow* m_mainWindow;

  bool m_needToInit;
};


/**
 * The application's core which extends K3bCore with some additional features
 * like the thememanager or an enhanced device manager.
 */
class K3bApplication::Core : public K3bCore
{
  Q_OBJECT

 public:
  Core( QObject* parent );
  ~Core();

  void init();

  // make sure the libk3b uses the same configuration
  // needed since the lib still depends on K3bCore::config
  // the goal is to make the lib independant from the config
  KConfig* config() const;

  void readSettings( KConfig* c = 0 );
  void saveSettings( KConfig* c = 0 );

  /**
   * \reimplemented from K3bCore. We use our own devicemanager here.
   */
  K3bDevice::DeviceManager* deviceManager() const;

  K3bAppDeviceManager* appDeviceManager() const { return m_appDeviceManager; }

  K3bThemeManager* themeManager() const { return m_themeManager; }

  K3bProjectManager* projectManager() const { return m_projectManager; }

  K3bMainWindow* k3bMainWindow() const { return m_mainWindow; }

  static Core* k3bAppCore() { return s_k3bAppCore; }

 public slots:
  /**
   * This will just emit the busyInfoRequested signal
   * Anyone may connect to it and show the string to the
   * user in some way.
   *
   * FIXME: this is bad design
   */
  void requestBusyInfo( const QString& );

  /**
   * FIXME: this is bad design
   */
  void requestBusyFinish();

 signals:
  /**
   * This is used for showing info in the K3b splashscreen
   */
  void initializationInfo( const QString& );

  /**
   * Any component may request busy info
   * In the K3b main app this will be displayed
   * as a moving square in the taskbar
   *
   * FIXME: this is bad design
   */
  void busyInfoRequested( const QString& );

  /**
   * FIXME: this is bad design
   */
  void busyFinishRequested();

 private:
  K3bThemeManager* m_themeManager;
  K3bMainWindow* m_mainWindow;
  K3bProjectManager* m_projectManager;
  K3bAppDeviceManager* m_appDeviceManager;

  static Core* s_k3bAppCore;

  friend class K3bApplication;
};

#endif
