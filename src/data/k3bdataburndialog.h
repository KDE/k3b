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


#ifndef K3BDATABURNDIALOG_H
#define K3BDATABURNDIALOG_H

#include "../k3bprojectburndialog.h"

class QCheckBox;
class KComboBox;
class QGroupBox;
class QLabel;
class QToolButton;
class QRadioButton;
class QButtonGroup;
class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class K3bDataDoc;
class KLineEdit;
class K3bDataImageSettingsWidget;
class K3bDataAdvancedImageSettingsWidget;
class K3bDataVolumeDescWidget;
class K3bDataModeWidget;

/**
  *@author Sebastian Trueg
  */

class K3bDataBurnDialog : public K3bProjectBurnDialog  
{
 Q_OBJECT

 public:
   K3bDataBurnDialog(K3bDataDoc*, QWidget *parent=0, const char *name=0, bool modal = true );
   ~K3bDataBurnDialog();

 protected:
   void setupSettingsTab();

   K3bDataVolumeDescWidget* m_volumeDescWidget;

   // --- settings tab ---------------------------
   K3bDataImageSettingsWidget* m_imageSettingsWidget;
   // ----------------------------------------------
	
   // --- advanced tab -------------------------
   K3bDataAdvancedImageSettingsWidget* m_advancedImageSettingsWidget;
   // ---------------------------------------------
	
   // --- multisession tab -------------------------
   QButtonGroup* m_groupMultiSession;
   QRadioButton* m_radioMultiSessionNone;
   QRadioButton* m_radioMultiSessionStart;
   QRadioButton* m_radioMultiSessionContinue;
   QRadioButton* m_radioMultiSessionFinish;
   // ---------------------------------------------

   K3bDataModeWidget* m_dataModeWidget;

 protected slots:
   void slotOk();
   void saveSettings();
   void readSettings();

   void loadDefaults();
   void loadUserDefaults();
   void saveUserDefaults();
};

#endif
