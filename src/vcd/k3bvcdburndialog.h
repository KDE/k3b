/***************************************************************************
                          k3bvcdburndialog.h  -  description
                             -------------------
    begin                : Son Nov 10 2002
    copyright            : (C) 2002 by Sebastian Trueg & Christian Kvasny
    email                : trueg@informatik.uni-freiburg.de
                         : chris@ckvsoft.at
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BVCDBURNDIALOG_H
#define K3BVCDBURNDIALOG_H

#include "../k3bprojectburndialog.h"

class QCheckBox;
class QGroupBox;
class QButtonGroup;
class QSpinBox;
class QRadioButton;
class QLabel;
class QLineEdit;
class QToolButton;
class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class K3bVcdDoc;
class K3bVcdOptions;

class K3bVcdBurnDialog : public K3bProjectBurnDialog
{
   Q_OBJECT

 public:
   K3bVcdBurnDialog(K3bVcdDoc* doc, QWidget *parent=0, const char *name=0, bool modal = true );
   ~K3bVcdBurnDialog();

 K3bVcdDoc* vcdDoc() const {return m_vcdDoc;};
 
 protected:
   void setupVideoCdTab();
   void setupLabelTab();
   void saveSettings();
   void readSettings();

   // -----------------------------------------------------------
   // the video-cd-tab
   // -----------------------------------------------------------

   QButtonGroup* m_groupVcdFormat;
   QRadioButton* m_radioVcd11;
   QRadioButton* m_radioVcd20;
   QRadioButton* m_radioSvcd10;

   QGroupBox* m_groupOptions;
   QCheckBox* m_checkNonCompliant;
   QCheckBox* m_check2336;
      
   // -----------------------------------------------------------
   // the video-label-tab
   // -----------------------------------------------------------

   QCheckBox* m_checkApplicationId;
   QLineEdit* m_editVolumeId;
   QLineEdit* m_editAlbumId;

   QSpinBox* m_spinVolumeCount;
   QSpinBox* m_spinVolumeNumber;
   // -----------------------------------------------------------

 private:
  K3bVcdDoc* m_vcdDoc;
  
 protected slots:
   void slotOk();

   void loadDefaults();
   void loadUserDefaults();
   void saveUserDefaults();

};

#endif
