/*
    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_PLUGIN_OPTION_TAB_H_
#define _K3B_PLUGIN_OPTION_TAB_H_

#include "ui_base_k3bpluginoptiontab.h"


namespace K3b {
    class PluginOptionTab : public QWidget, public Ui::base_K3bPluginOptionTab
    {
        Q_OBJECT

    public:
        explicit PluginOptionTab( QWidget* parent = 0 );
        ~PluginOptionTab() override;
    };
}

#endif
