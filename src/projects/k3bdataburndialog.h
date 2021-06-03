/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
        explicit DataBurnDialog(DataDoc*, QWidget *parent=0 );
        ~DataBurnDialog() override;

    protected:
        void setupSettingsTab();

        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;
        void toggleAll() override;

        // --- settings tab ---------------------------
        DataImageSettingsWidget* m_imageSettingsWidget;
        // ----------------------------------------------

        QGroupBox* m_groupDataMode;
        DataModeWidget* m_dataModeWidget;
        DataMultiSessionCombobox* m_comboMultisession;

        QCheckBox* m_checkVerify;

    protected Q_SLOTS:
        void slotStartClicked() override;
        void saveSettingsToProject() override;
        void readSettingsFromProject() override;

        void slotMultiSessionModeChanged();
    };
}

#endif
