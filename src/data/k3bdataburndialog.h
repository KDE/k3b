/***************************************************************************
                          k3bdataburndialog.h  -  description
                             -------------------
    begin                : Wed May 16 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
   void prepareJob( K3bBurnJob* );

   void setupBurnTab( QFrame* frame );
   void setupAdvancedTab( QFrame* frame );
   void setupSettingsTab( QFrame* frame );
   void setupVolumeInfoTab( QFrame* frame );

   // --- general tab -------------------------	
   QLabel* TextLabel1;
   QLabel* TextLabel1_2;
   QGroupBox* m_groupOptions;
   QCheckBox* m_checkDummy;
   QCheckBox* m_checkOnTheFly;
   QCheckBox* m_checkOnlyCreateImage;
   QCheckBox* m_checkDeleteImage;
   QCheckBox* m_checkDao;
   QCheckBox* m_checkBurnProof;
   K3bTempDirSelectionWidget* m_tempDirSelectionWidget;
   K3bWriterSelectionWidget* m_writerSelectionWidget;
   // ----------------------------------------------

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

 protected slots:
   void slotOk();
   void saveSettings();
   void readSettings();
   void slotWriterChanged();

   void loadDefaults();
   void loadUserDefaults();
   void saveUserDefaults();

   void slotOnlyCreateImageToggled( bool on );
};

#endif
