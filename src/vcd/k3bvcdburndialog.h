/*
 *
 * $Id$
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

#ifndef K3BVCDBURNDIALOG_H
#define K3BVCDBURNDIALOG_H

#include <k3bprojectburndialog.h>
#include <qmultilineedit.h>

class QCheckBox;
class QGroupBox;
class QButtonGroup;
class QSpinBox;
class QRadioButton;
class QLabel;
class QLineEdit;
class QMultiLineEdit;
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

 K3bVcdDoc* vcdDoc() const {return m_vcdDoc;}
 
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
   QCheckBox* m_checkAutoDetect;
   QCheckBox* m_checkNonCompliant;
   QCheckBox* m_check2336;

   // CD-i
   QGroupBox* m_groupCdi;
   QCheckBox* m_checkCdiSupport;
   QMultiLineEdit* m_editCdiCfg;

            
   // -----------------------------------------------------------
   // the video-label-tab
   // -----------------------------------------------------------

   QLineEdit* m_editVolumeId;
   QLineEdit* m_editAlbumId;

   QSpinBox* m_spinVolumeCount;
   QSpinBox* m_spinVolumeNumber;

   // -----------------------------------------------------------

 private:
  K3bVcdDoc* m_vcdDoc;
  void saveCdiConfig();
  void loadCdiConfig();
  void loadDefaultCdiConfig();
  
 protected slots:
   void slotOk();

   void loadDefaults();
   void loadUserDefaults();
   void saveUserDefaults();

   void slotSpinVolumeCount();
   void slotSetImagePath();
   void slotVcdTypeClicked(int);
   void slotCdiSupportChecked(bool);
   void slotAutoDetect(bool);
};

#endif
