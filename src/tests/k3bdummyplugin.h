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

#ifndef _K3B_DUMMY_PLUGIN_H_
#define _K3B_DUMMY_PLUGIN_H_


#include <kparts/plugin.h>
#include <klibloader.h>


class K3bDummyPlugin : public KParts::Plugin
{
  Q_OBJECT

 public:
  K3bDummyPlugin( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bDummyPlugin();

 public slots:
  void slotDoDummyStuff();
};


class KPluginFactory : public KLibFactory
{
  Q_OBJECT

 public:
  KPluginFactory( QObject *parent = 0, const char *name = 0 );
  ~KPluginFactory() { delete s_instance; };


  virtual QObject* createObject( QObject* parent = 0, const char* pname = 0,
				 const char* name = "QObject",
				 const QStringList &args = QStringList() );

 private:
  static KInstance* s_instance;
};


#endif
