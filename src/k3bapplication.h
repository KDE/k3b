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

#define k3bapp K3bApplication::k3bApplication()

class K3bMainWindow;
class K3bInterface;
class K3bSongManager;
class K3bAudioServer;


class K3bApplication : public KUniqueApplication
{
  Q_OBJECT

 public:
  K3bApplication();
  ~K3bApplication();

  void init();

  int newInstance();

  K3bMainWindow* k3bMainWindow() const;
  K3bSongManager* songManager() const { return m_songManager; }

  static K3bApplication* k3bApplication() { return s_k3bApp; }

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
  K3bMainWindow* m_mainWindow;
  K3bSongManager* m_songManager;
  K3bAudioServer* m_audioServer;

  bool m_needToInit;

  static K3bApplication* s_k3bApp;
};


/**
 * Just to show some info in the splash screen
 */
class K3bApplication::Core : public K3bCore
{
  Q_OBJECT

 public:
  Core( QObject* parent );
  ~Core();

  void init();

 signals:
  /**
   * This is used for showing info in the K3b splashscreen
   */
  void initializationInfo( const QString& );
};

#endif
