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


#ifndef _K3B_PLUGIN_FACTORY_H_
#define _K3B_PLUGIN_FACTORY_H_

#include <klibloader.h>

class QWidget;
class K3bPlugin;
class K3bPluginConfigWidget;

/**
 * The base class for all k3b pluginfactories
 * Instead of reimplementing the createObject method
 * one needs to reimplement the createPlugin() and if
 * needed the createConfigWidget() methods
 * You should return a pointer to your factory. The K_EXPORT_COMPONENT_FACTORY
 * macro is provided for this purpose:
 * <pre>
 *   K_EXPORT_COMPONENT_FACTORY( libkspread, KSpreadFactory )
 * </pre>
 *
 * Every plugin needs to install a XXX.plugin file in the k3b/plugins directory.
 * The plugin group and the library have to set at least.
 */
class K3bPluginFactory : public KLibFactory
{
  Q_OBJECT

 public:
  K3bPluginFactory( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bPluginFactory();

  /**
   * just calls createPluginObject() and emits the objectCreated signal
   */
  K3bPlugin* createPlugin( QObject* parent = 0, 
			   const char* name = 0,
			   const QStringList &args = QStringList() );
  
  /**
   * just calls createConfigWidgetObject() and emits the objectCreated signal
   */
  K3bPluginConfigWidget* createConfigWidget( QWidget* parent = 0, 
					     const char* name = 0,
					     const QStringList &args = QStringList() );

  virtual QString author() const;
  virtual QString version() const;
  virtual QString comment() const;

  /**
   * Version of the plugin system this plugin was written for.
   */
  virtual int pluginSystemVersion() const = 0;

  /**
   * The plugin group. Needs to be the same as in the desktop entry.
   */
  virtual QString group() const = 0;

 protected:
  virtual K3bPlugin* createPluginObject( QObject* parent = 0, 
					 const char* name = 0,
					 const QStringList &args = QStringList() ) = 0;

  virtual K3bPluginConfigWidget* createConfigWidgetObject( QWidget* parent = 0, 
							   const char* name = 0,
							   const QStringList &args = QStringList() );

  /**
   * This normally does not need to be reimplemented or used
   * the default implementation just calls createPlugin() or createConfigWidget()
   * depending on classname
   */
  virtual QObject* createObject( QObject* parent = 0, 
				 const char* name = 0, 
				 const char* classname = "Plugin", 
				 const QStringList &args = QStringList() );
};

#endif
