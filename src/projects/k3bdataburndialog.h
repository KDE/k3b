/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
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

class QCheckBox;
class QGroupBox;
class QLabel;

namespace K3b {
    class DataDoc;
    class DataImageSettingsWidget;
    class DataModeWidget;
    class DataMultiSessionCombobox;

    /**
     *@author Sebastian Trueg
     */
    class DataBurnDialog : public ProjectBurnDialog
    {
        Q_OBJECT

    public:
        DataBurnDialog(DataDoc*, QWidget *parent=0 );
        ~DataBurnDialog();

    protected:
        void setupSettingsTab();

        void loadSettings( const KConfigGroup& );
        void saveSettings( KConfigGroup );
        void toggleAll();

        // --- settings tab ---------------------------
        DataImageSettingsWidget* m_imageSettingsWidget;
        // ----------------------------------------------

        QGroupBox* m_groupDataMode;
        DataModeWidget* m_dataModeWidget;
        DataMultiSessionCombobox* m_comboMultisession;

        QCheckBox* m_checkVerify;

    protected Q_SLOTS:
        void slotStartClicked();
        void saveSettingsToProject();
        void readSettingsFromProject();

        void slotMultiSessionModeChanged();
    };
}

#endif
