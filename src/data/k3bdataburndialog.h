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
class QComboBox;
class QGroupBox;
class QLabel;
class QToolButton;
class QRadioButton;
class QButtonGroup;
class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class K3bDataDoc;
class KLineEdit;


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
   void setupBurnTab( QFrame* frame );
   void setupAdvancedTab( QFrame* frame );
   void setupSettingsTab( QFrame* frame );
   void setupMultisessionTab( QFrame* frame );

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

   // --- settings tab ---------------------------
   KLineEdit* m_editSystemId;
   KLineEdit* m_editVolumeID;
   KLineEdit* m_editVolumeSetId;
   KLineEdit* m_editPublisher;
   KLineEdit* m_editPreparer;
   KLineEdit* m_editApplicationID;
   QCheckBox* m_checkCreateRR;
   QCheckBox* m_checkCreateJoliet;
   QButtonGroup* m_groupWhiteSpace;
   QRadioButton* m_radioSpaceLeave;
   QRadioButton* m_radioSpaceReplace;
   QRadioButton* m_radioSpaceStrip;
   QRadioButton* m_radioSpaceExtended;
   // ----------------------------------------------
	
   // --- advanced tab -------------------------
   QPushButton* m_buttonSaveAsDefault;
   QGroupBox* m_groupPreSettings;
   QComboBox* m_comboPreSettings;
   QFrame* frameSettings;
   QButtonGroup* m_groupIsoLevel;
   QRadioButton* m_radioIsoLevel1;
   QRadioButton* m_radioIsoLevel2;
   QRadioButton* m_radioIsoLevel3;
   QCheckBox* m_checkNoDeepDirRel;
   QCheckBox* m_checkPadding;
   QCheckBox* m_checkHideRR_MOVED;
   QCheckBox* m_checkCreateTRANS_TBL;
   QCheckBox* m_checkHideTRANS_TBL;
   QCheckBox* m_checkUntranslatedNames;
   QCheckBox* m_checkAllow31;
   QCheckBox* m_checkMaxNames;
   QCheckBox* m_checkBeginPeriod;
   QCheckBox* m_checkRelaxedNames;
   QCheckBox* m_checkOmitVersion;
   QCheckBox* m_checkNoISOTrans;
   QCheckBox* m_checkMultiDot;
   QCheckBox* m_checkLowercase;
   // ---------------------------------------------
	
   // --- multisession tab -------------------------
   QButtonGroup* m_groupMultiSession;
   QRadioButton* m_radioMultiSessionNone;
   QRadioButton* m_radioMultiSessionStart;
   QRadioButton* m_radioMultiSessionContinue;
   QRadioButton* m_radioMultiSessionFinish;
   // ---------------------------------------------

 protected slots:
   void slotUser1();
   void saveSettings();
   void readSettings();
   void slotLoadPreSettings( const QString& );
   void slotSaveDefaults();
   void slotSelectCustom();
   void slotWriterChanged();

   /**
    * converts all the ISO-header field strings
    * to upercase
    */
   void slotConvertAllToUpperCase();

   void slotOnlyCreateImageToggled( bool on );
};

#endif
