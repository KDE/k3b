/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_VIDEODVD_BURNDIALOG_H_
#define _K3B_VIDEODVD_BURNDIALOG_H_

#include "k3bprojectburndialog.h"

class QCheckBox;

namespace K3b {
    class VideoDvdDoc;
    class DataImageSettingsWidget;

    class VideoDvdBurnDialog : public ProjectBurnDialog
    {
        Q_OBJECT

    public:
        explicit VideoDvdBurnDialog( VideoDvdDoc*, QWidget *parent = 0 );
        ~VideoDvdBurnDialog() override;

    protected Q_SLOTS:
        void slotStartClicked() override;
        void saveSettingsToProject() override;
        void readSettingsFromProject() override;

    protected:
        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;
        void toggleAll() override;

    private:
        DataImageSettingsWidget* m_imageSettingsWidget;

        QCheckBox* m_checkVerify;

        VideoDvdDoc* m_doc;
    };
}

#endif
