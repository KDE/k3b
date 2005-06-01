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


#ifndef _K3B_PLUGIN_H_
#define _K3B_PLUGIN_H_

#include <qobject.h>
#include <kgenericfactory.h>
#include "k3b_export.h"

#define K3B_PLUGIN_SYSTEM_VERSION 3


class K3bPluginConfigWidget;
class QWidget;



class K3bPluginInfo
{
  friend class K3bPluginManager;

 public:
  K3bPluginInfo() {
  }

  K3bPluginInfo( QString libraryName,
		 QString name,
		 QString author,
		 QString email,
		 QString comment,
		 QString version,
		 QString licence )
    : m_libraryName(libraryName),
    m_name(name),
    m_author(author),
    m_email(email),
    m_comment(comment),
    m_version(version),
    m_licence(licence) {
  }

  const QString& name() const { return m_name; }
  const QString& author() const { return m_author; }
  const QString& email() const { return m_email; }
  const QString& comment() const { return m_comment; }
  const QString& version() const { return m_version; }
  const QString& licence() const { return m_licence; }

  const QString& libraryName() const { return m_libraryName; }

 private:
  QString m_libraryName;

  QString m_name;
  QString m_author;
  QString m_email;
  QString m_comment;
  QString m_version;
  QString m_licence;
};


/**
 * Base class for all plugins. You may use the K3bPluginFactory to make your plugin available.
 */
class LIBK3B_EXPORT K3bPlugin : public QObject
{
  Q_OBJECT

    friend class K3bPluginManager;

 public:
  K3bPlugin( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bPlugin();

  const K3bPluginInfo& pluginInfo() const { return m_pluginInfo; }

  /**
   * Version of the plugin system this plugin was written for.
   */
  virtual int pluginSystemVersion() const = 0;

  /**
   * The plugin group.
   */
  virtual QString group() const = 0;

  /**
   * Returns a widget which configures the plugin.
   *
   * The caller has to destroy the widget
   */
  virtual K3bPluginConfigWidget* createConfigWidget( QWidget* parent = 0, const char* name = 0 ) const;

 private:
  K3bPluginInfo m_pluginInfo;
};

#endif
