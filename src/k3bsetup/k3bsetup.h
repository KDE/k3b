/* 
 *
 * $Id: $
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

/***************************************************************************
                          k3bsetup.h  -  description
                             -------------------
    begin                : Sat Dec  1 16:18:59 CET 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BSETUP_H
#define K3BSETUP_H


class KConfig;

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>

#include "../device/k3bdevice.h"
#include "../device/k3bdevicemanager.h"

class K3bExternalBinManager;
class KSimpleConfig;


/**
 * K3bSetup represents the setup status.
 */
class K3bSetup : public QObject
{
Q_OBJECT

 public:
  K3bSetup( QObject* = 0 );
  ~K3bSetup();

  bool saveConfig();

  void setApplyDevicePermissions( bool b ) { m_applyDevicePermissions = b; }
  void setApplyExternalBinPermissions( bool b ) { m_applyExternalBinPermission = b; }
  void setCreateFstabEntries( bool b ) { m_createFstabEntries = b; }

  bool applyExternalProgramPermissions() const { return m_applyExternalBinPermission; }
  bool applyDevicePermissions() const { return m_applyDevicePermissions; }
  bool createFstabEntries() const { return m_createFstabEntries; }

  void setCdWritingGroup( const QString& );
  void addUser( const QString& );
  void clearUsers();

  const QString& cdWritingGroup() const;
  const QStringList& users() const;

  K3bDeviceManager* deviceManager() const { return m_deviceManager; }
  K3bExternalBinManager* externalBinManager() const { return m_externalBinManager; }

 signals:
  void writingSettings();
  void writingSetting( const QString& s );
  void settingWritten( bool success, const QString& comment );
  void error( const QString& error );

 private:
  uint createCdWritingGroup();
  void doApplyExternalProgramPermissions( uint );
  void doApplyDevicePermissions( uint );
  void doCreateFstabEntries();

  QString m_cdwritingGroup;
  QStringList m_userList;

  K3bDeviceManager* m_deviceManager;
  K3bExternalBinManager* m_externalBinManager;

  bool m_applyDevicePermissions;
  bool m_applyExternalBinPermission;
  bool m_createFstabEntries;

  QString m_configPath;
  KSimpleConfig* m_config;
};

#endif
