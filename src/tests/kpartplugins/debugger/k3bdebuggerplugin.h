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

#ifndef _K3B_DEBUGGER_PLUGIN_H_
#define _K3B_DEBUGGER_PLUGIN_H_


#include <kparts/plugin.h>
#include <klibloader.h>

#include <qstringlist.h>


class K3bDebuggerPlugin : public KParts::Plugin
{
  Q_OBJECT

 public:
  K3bDebuggerPlugin( QObject* parent, const char* name, const QStringList& );
  virtual ~K3bDebuggerPlugin();

 public slots:
  void slotDoDebuggerStuff();
};


#endif
