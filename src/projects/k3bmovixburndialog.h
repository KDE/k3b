/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_MOVIX_BURN_DIALOG_H_
#define _K3B_MOVIX_BURN_DIALOG_H_

#include "k3bprojectburndialog.h"

class QCheckBox;

namespace K3b {
    class MovixDoc;
    class MovixOptionsWidget;
    class DataImageSettingsWidget;
    class DataModeWidget;

    class MovixBurnDialog : public ProjectBurnDialog
    {
        Q_OBJECT

    public:
        explicit MovixBurnDialog( MovixDoc* doc, QWidget* parent = 0 );
        ~MovixBurnDialog() override;

    protected Q_SLOTS:
        void slotStartClicked() override;

    protected:
        void saveSettingsToProject() override;
        void readSettingsFromProject() override;
        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;
        void toggleAll() override;

    private:
        void setupSettingsPage();

        MovixDoc* m_doc;
        MovixOptionsWidget* m_movixOptionsWidget;
        DataImageSettingsWidget* m_imageSettingsWidget;

        QCheckBox* m_checkStartMultiSesssion;
        DataModeWidget* m_dataModeWidget;

        QCheckBox* m_checkVerify;
    };
}


#endif

