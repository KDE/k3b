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


#ifndef _K3B_CORE_H_
#define _K3B_CORE_H_

#include <qobject.h>


#define k3bcore K3bCore::k3bCore()


class K3bExternalBinManager;
class K3bSongManager;
class K3bVersion;
class KConfig;
class KAboutData;


namespace K3bCdDevice {
  class DeviceManager;
};


/**
 * The K3b core takes care of the managers. 
 * This has been seperated from K3bApplication to 
 * make creating a K3bPart easy.
 */
class K3bCore : public QObject
{
  Q_OBJECT

 public:
  K3bCore( const KAboutData*, QObject* parent = 0, const char* name = 0 );
  ~K3bCore();

  void init();
  void saveConfig();

  K3bCdDevice::DeviceManager* deviceManager() const;
  K3bExternalBinManager* externalBinManager() const;
  K3bSongManager* songManager() const;

  const K3bVersion& version() const;

  KConfig* config() const;

  /**
   * Checks the system and pops up a dialog if
   * any things are not configured properly.
   */
  void checkSystem() const;

  static K3bCore* k3bCore() { return s_k3bCore; }

 signals:
  void initializationInfo( const QString& );

 private:
  class Private;
  Private* d;

  static K3bCore* s_k3bCore;
};

#endif
