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
namespace K3b {
    class MixedDoc;
}
namespace K3b {
    class DataImageSettingsWidget;
}
class QRadioButton;
namespace K3b {
    class AudioCdTextWidget;
}
namespace K3b {
    class DataModeWidget;
}
namespace K3b {
    class IntMapComboBox;
}


/**
  *@author Sebastian Trueg
  */
namespace K3b {
class MixedBurnDialog : public ProjectBurnDialog  
{
 Q_OBJECT

 public:
   MixedBurnDialog( MixedDoc*, QWidget *parent=0 );

 protected:
   void loadK3bDefaults();
   void loadUserDefaults( const KConfigGroup& );
   void saveUserDefaults( KConfigGroup );
   void toggleAll();

   DataImageSettingsWidget* m_imageSettingsWidget;
   AudioCdTextWidget* m_cdtextWidget;

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
   MixedDoc* m_doc;

   IntMapComboBox* m_comboMixedModeType;
   QRadioButton* m_radioMixedTypeFirstTrack;
   QRadioButton* m_radioMixedTypeLastTrack;
   QRadioButton* m_radioMixedTypeSessions;

   QCheckBox* m_checkNormalize;

   DataModeWidget* m_dataModeWidget;
};
}

#endif
