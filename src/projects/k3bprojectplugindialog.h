/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_PROJECTPLUGIN_DIALOG_H_
#define _K3B_PROJECTPLUGIN_DIALOG_H_

#include "k3binteractiondialog.h"

namespace K3b {
    class ProjectPlugin;
    class ProjectPluginGUIBase;
    class Doc;

    class ProjectPluginDialog : public InteractionDialog
    {
        Q_OBJECT

    public:
        ProjectPluginDialog( ProjectPlugin*, Doc*, QWidget* );
        ~ProjectPluginDialog() override;

    protected Q_SLOTS:
        void slotStartClicked() override;
        void saveSettings( KConfigGroup config ) override;
        void loadSettings( const KConfigGroup& config ) override;

    private:
        ProjectPluginGUIBase* m_pluginGui;
    };
}

#endif
