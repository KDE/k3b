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


#ifndef K3BMIXEDBURNDIALOG_H
#define K3BMIXEDBURNDIALOG_H

#include "k3bprojectburndialog.h"

class QCheckBox;
class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class K3bMixedDoc;
class K3bDataImageSettingsWidget;
class K3bDataAdvancedImageSettingsWidget;
class K3bDataVolumeDescWidget;
class QButtonGroup;
class QRadioButton;
class K3bAudioCdTextWidget;
class K3bDataModeWidget;


/**
  *@author Sebastian Trueg
  */

class K3bMixedBurnDialog : public K3bProjectBurnDialog  
{
 Q_OBJECT

 public:
   K3bMixedBurnDialog( K3bMixedDoc*, QWidget *parent=0, const char *name=0, bool modal = true );

 protected:
   void loadK3bDefaults();
   void loadUserDefaults( KConfigBase* );
   void saveUserDefaults( KConfigBase* );

   K3bDataVolumeDescWidget* m_volumeDescWidget;
   K3bDataImageSettingsWidget* m_imageSettingsWidget;
   K3bDataAdvancedImageSettingsWidget* m_advancedImageSettingsWidget;
   K3bAudioCdTextWidget* m_cdtextWidget;

 protected slots:
   /**
    * Reimplemented for internal reasons (shut down the audio player)
    */
   void slotStartClicked();
   void saveSettings();
   void readSettings();

   void toggleAllOptions();
   void slotOnTheFlyToggled( bool on );
   void slotNormalizeToggled( bool on );

 private:
   void setupSettingsPage();
   void createContextHelp();
   K3bMixedDoc* m_doc;

   QButtonGroup* m_groupMixedType;
   QRadioButton* m_radioMixedTypeFirstTrack;
   QRadioButton* m_radioMixedTypeLastTrack;
   QRadioButton* m_radioMixedTypeSessions;

   QCheckBox* m_checkNormalize;

   K3bDataModeWidget* m_dataModeWidget;
};

#endif
