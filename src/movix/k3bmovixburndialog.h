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



#ifndef _K3B_MOVIX_BURN_DIALOG_H_
#define _K3B_MOVIX_BURN_DIALOG_H_

#include <k3bprojectburndialog.h>

class K3bMovixDoc;
class K3bMovixOptionsWidget;
class K3bMovixInstallation;
class K3bDataImageSettingsWidget;
class K3bDataAdvancedImageSettingsWidget;
class K3bDataVolumeDescWidget;
class QCheckBox;
class K3bDataModeWidget;


class K3bMovixBurnDialog : public K3bProjectBurnDialog
{
  Q_OBJECT

 public:
  K3bMovixBurnDialog( K3bMovixDoc* doc, QWidget* parent = 0, const char* name = 0, bool modal = true );
  ~K3bMovixBurnDialog();

 protected slots:
  void loadDefaults();
  void loadUserDefaults();
  void saveUserDefaults();
  void slotOk();

 protected:
  void saveSettings();
  void readSettings();

 private:
  void setupSettingsPage();

  K3bMovixDoc* m_doc;
  K3bMovixOptionsWidget* m_movixOptionsWidget;
  K3bDataVolumeDescWidget* m_volumeDescWidget;
  K3bDataImageSettingsWidget* m_imageSettingsWidget;
  K3bDataAdvancedImageSettingsWidget* m_advancedImageSettingsWidget;
  K3bMovixInstallation* m_installation;

  QCheckBox* m_checkStartMultiSesssion;
  K3bDataModeWidget* m_dataModeWidget;
};


#endif

