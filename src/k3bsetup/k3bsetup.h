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

#include <qstring.h>
#include <qstringlist.h>


class K3bDeviceManager;
//class K3bExternalBinManager;

/**
 * K3bSetup represents the setup status.
 * This status can be applied by applyPermissions()
 */
class K3bSetup
{
 public:
  K3bSetup();
  ~K3bSetup();

  bool loadConfig( KConfig* );
  bool saveConfig( KConfig* );

  void applyDevicePermissions( K3bDeviceManager* );
  //void applyExternalProgramPermissions( K3bExternalBinManager* );

  void setCdWritingGroup( const QString& );
  void addUser( const QString& );
  void clearUsers();

  const QString& cdWritingGroup() const;
  const QStringList& users() const;

 private:
  uint createCdWritingGroup();

  QString m_cdwritingGroup;
  QStringList m_userList;
};

#endif
