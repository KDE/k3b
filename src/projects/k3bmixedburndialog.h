/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BMIXEDBURNDIALOG_H
#define K3BMIXEDBURNDIALOG_H

#include "k3bprojectburndialog.h"

class QCheckBox;
class K3bMixedDoc;
class K3bDataImageSettingsWidget;
class QRadioButton;
class K3bAudioCdTextWidget;
class K3bDataModeWidget;
class K3bIntMapComboBox;


/**
  *@author Sebastian Trueg
  */
class K3bMixedBurnDialog : public K3bProjectBurnDialog  
{
 Q_OBJECT

 public:
   K3bMixedBurnDialog( K3bMixedDoc*, QWidget *parent=0 );

 protected:
   void loadK3bDefaults();
   void loadUserDefaults( const KConfigGroup& );
   void saveUserDefaults( KConfigGroup );
   void toggleAll();

   K3bDataImageSettingsWidget* m_imageSettingsWidget;
   K3bAudioCdTextWidget* m_cdtextWidget;

 protected Q_SLOTS:
   /**
    * Reimplemented for internal reasons (shut down the audio player)
    */
   void slotStartClicked();
   void saveSettings();
   void readSettings();

   void slotCacheImageToggled( bool on );
   void slotNormalizeToggled( bool on );

 private:
   void setupSettingsPage();
   K3bMixedDoc* m_doc;

   K3bIntMapComboBox* m_comboMixedModeType;
   QRadioButton* m_radioMixedTypeFirstTrack;
   QRadioButton* m_radioMixedTypeLastTrack;
   QRadioButton* m_radioMixedTypeSessions;

   QCheckBox* m_checkNormalize;

   K3bDataModeWidget* m_dataModeWidget;
};

#endif
