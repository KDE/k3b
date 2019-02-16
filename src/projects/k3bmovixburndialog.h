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

