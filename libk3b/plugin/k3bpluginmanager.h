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

#ifndef _K3B_PLUGIN_MANAGER_H_
#define _K3B_PLUGIN_MANAGER_H_

#include <qobject.h>
#include <qptrlist.h>
#include <qstringlist.h>



class K3bPlugin;
class QWidget;


/**
 * Use this class to access all K3b plugins (this does not include the
 * KParts Plugins!).
 * Like the K3bCore the single instance (which has to be created manually)
 * can be obtained with the k3bpluginmanager macro.
 */
class K3bPluginManager : public QObject
{
  Q_OBJECT

 public:
  K3bPluginManager( QObject* parent = 0, const char* name = 0 );
  ~K3bPluginManager();

  /**
   * if group is empty all plugins are returned
   */
  QPtrList<K3bPlugin> plugins( const QString& group = QString::null ) const;

  /**
   * Returnes a list of the available groups.
   */
  QStringList groups() const;

  int pluginSystemVersion() const;

 public slots:
  /**
   * Loads all plugins from the ressource directories.
   */
  void loadAll();

  void loadPlugin( const QString& fileName );

  int execPluginDialog( K3bPlugin*, QWidget* parent = 0, const char* name = 0 );

 private:
  class Private;
  Private* d;
};

#endif
