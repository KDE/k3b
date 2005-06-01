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


#ifndef _K3B_PLUGIN_CONFIG_WIDGET_H_
#define _K3B_PLUGIN_CONFIG_WIDGET_H_

#include <qwidget.h>
#include "k3b_export.h"

class LIBK3B_EXPORT K3bPluginConfigWidget : public QWidget
{
  Q_OBJECT

 public:
  K3bPluginConfigWidget( QWidget* parent = 0, const char* name = 0 );
  virtual ~K3bPluginConfigWidget();

 public slots:
  /**
   * Use k3bcore->config() to store the settings
   * FIXME: add a KConfig parameter here
   */
  virtual void loadConfig();
  virtual void saveConfig();
};

#endif
