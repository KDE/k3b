/* 
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BDATABURNDIALOG_H
#define K3BDATABURNDIALOG_H

#include "k3bprojectburndialog.h"
//Added by qt3to4:
#include <QLabel>

class QCheckBox;
class Q3GroupBox;
class QLabel;
namespace K3b {
    class DataDoc;
}
namespace K3b {
    class DataImageSettingsWidget;
}
namespace K3b {
    class DataModeWidget;
}
namespace K3b {
    class DataMultiSessionCombobox;
}


/**
 *@author Sebastian Trueg
 */

namespace K3b {
class DataBurnDialog : public ProjectBurnDialog
{
    Q_OBJECT

public:
    DataBurnDialog(DataDoc*, QWidget *parent=0 );
    ~DataBurnDialog();

protected:
    void setupSettingsTab();
    void loadK3bDefaults();
    void loadUserDefaults( const KConfigGroup& );
    void saveUserDefaults( KConfigGroup );
    void toggleAll();

    // --- settings tab ---------------------------
    DataImageSettingsWidget* m_imageSettingsWidget;
    // ----------------------------------------------
	
    Q3GroupBox* m_groupDataMode;
    DataModeWidget* m_dataModeWidget;
    DataMultiSessionCombobox* m_comboMultisession;

    QCheckBox* m_checkVerify;

    protected Q_SLOTS:
    void slotStartClicked();
    void saveSettings();
    void readSettings();

    void slotMultiSessionModeChanged();
};
}

#endif
