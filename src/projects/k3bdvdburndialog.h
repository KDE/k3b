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


#ifndef _K3B_DVD_BURNDIALOG_H_
#define _K3B_DVD_BURNDIALOG_H_

#include "k3bprojectburndialog.h"


class K3bDvdDoc;
class K3bDataImageSettingsWidget;
class K3bDataVolumeDescWidget;
class QGroupBox;
class QRadioButton;
class QButtonGroup;
class QCheckBox;
class K3bDataMultiSessionCombobox;


class K3bDvdBurnDialog : public K3bProjectBurnDialog
{
 Q_OBJECT

 public:
   K3bDvdBurnDialog( K3bDvdDoc*, QWidget *parent = 0, const char *name = 0, bool modal = true );
   ~K3bDvdBurnDialog();

 protected slots:
   void slotMultiSessionModeChanged();

   void saveSettings();
   void readSettings();

 protected:
   void loadK3bDefaults();
   void loadUserDefaults( KConfigBase* );
   void saveUserDefaults( KConfigBase* );
   void toggleAll();

 private:
   void setupSettingsTab();

   K3bDataVolumeDescWidget* m_volumeDescWidget;

   // --- settings tab ---------------------------
   K3bDataImageSettingsWidget* m_imageSettingsWidget;
   // ----------------------------------------------
	
   // --- multisession tab -------------------------
   K3bDataMultiSessionCombobox* m_comboMultisession;
   // ---------------------------------------------

   QCheckBox* m_checkVerify;

   K3bDvdDoc* m_doc;
};

#endif
