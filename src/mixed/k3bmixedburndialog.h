/***************************************************************************
                          k3bmixedburndialog.h  -  description
                             -------------------
    begin                : Fri Aug 2 2002
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

#ifndef K3BMIXEDBURNDIALOG_H
#define K3BMIXEDBURNDIALOG_H

#include "../k3bprojectburndialog.h"

class QCheckBox;
class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class K3bMixedDoc;
class K3bDataImageSettingsWidget;
class K3bDataAdvancedImageSettingsWidget;
class K3bDataVolumeDescWidget;


/**
  *@author Sebastian Trueg
  */

class K3bMixedBurnDialog : public K3bProjectBurnDialog  
{
 Q_OBJECT

 public:
   K3bMixedBurnDialog( K3bMixedDoc*, QWidget *parent=0, const char *name=0, bool modal = true );

 protected:
   QCheckBox* m_checkDummy;
   QCheckBox* m_checkOnTheFly;
   QCheckBox* m_checkOnlyCreateImage;
   QCheckBox* m_checkDeleteImage;
   QCheckBox* m_checkDao;
   QCheckBox* m_checkBurnProof;
   K3bTempDirSelectionWidget* m_tempDirSelectionWidget;
   K3bWriterSelectionWidget* m_writerSelectionWidget;

   K3bDataVolumeDescWidget* m_volumeDescWidget;
   K3bDataImageSettingsWidget* m_imageSettingsWidget;
   K3bDataAdvancedImageSettingsWidget* m_advancedImageSettingsWidget;

 protected slots:
   void slotOk();
   void saveSettings();
   void readSettings();
   void slotWriterChanged();

   void loadDefaults();
   void loadUserDefaults();
   void saveUserDefaults();

   void slotOnlyCreateImageToggled( bool on );

 private:
   K3bMixedDoc* m_doc;
};

#endif
