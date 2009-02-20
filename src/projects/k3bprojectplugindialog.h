/* 
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_PROJECTPLUGIN_DIALOG_H_
#define _K3B_PROJECTPLUGIN_DIALOG_H_

#include <k3binteractiondialog.h>

class K3bProjectPlugin;
class K3bProjectPluginGUIBase;
class K3bDoc;


class K3bProjectPluginDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public:
  K3bProjectPluginDialog( K3bProjectPlugin*, K3bDoc*, QWidget* );
  ~K3bProjectPluginDialog();
  
 protected Q_SLOTS:
  void slotStartClicked();
  void saveUserDefaults( KConfigGroup config );
  void loadUserDefaults( const KConfigGroup& config );
  void loadK3bDefaults();

 private:
  K3bProjectPlugin* m_plugin;
  K3bProjectPluginGUIBase* m_pluginGui;
};

#endif
