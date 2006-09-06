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

#ifndef _K3B_PLUGIN_OPTION_TAB_H_
#define _K3B_PLUGIN_OPTION_TAB_H_

#include "base_k3bpluginoptiontab.h"



class K3bPluginOptionTab : public base_K3bPluginOptionTab
{
  Q_OBJECT

 public:
  K3bPluginOptionTab( QWidget* parent = 0, const char* name = 0 );
  ~K3bPluginOptionTab();

 public slots:
  void readSettings();
  bool saveSettings();

 private slots:
  void slotConfigureButtonClicked();
  void slotSelectionChanged();

 private:
  class PluginViewItem;
};

#endif
