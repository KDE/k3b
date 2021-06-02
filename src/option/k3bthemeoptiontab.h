/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef _K3BTHEMEOPTIONTAB_H_
#define _K3BTHEMEOPTIONTAB_H_

#include "ui_base_k3bthemeoptiontab.h"

/**
 *@author Sebastian Trueg
 */
namespace K3b {
    class ThemeModel;
    
    class ThemeOptionTab : public QWidget, public Ui::base_K3bThemeOptionTab
    {
        Q_OBJECT

    public:
        explicit ThemeOptionTab( QWidget* parent = 0 );
        ~ThemeOptionTab() override;

        void readSettings();
        bool saveSettings();

    protected:
        bool event( QEvent* event ) override;

    private Q_SLOTS:
        void selectionChanged();
        void slotInstallTheme();
        void slotRemoveTheme();
        void slotGetNewThemes();
        
    private:
        ThemeModel* m_themeModel;
    };
}

#endif
