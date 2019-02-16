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
