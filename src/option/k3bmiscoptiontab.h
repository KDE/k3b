/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BMISCOPTIONTAB_H
#define K3BMISCOPTIONTAB_H

#include "ui_base_k3bmiscoptiontab.h"


namespace K3b {
    class MiscOptionTab : public QWidget, public Ui::base_K3bMiscOptionTab
    {
        Q_OBJECT

    public:
        explicit MiscOptionTab(QWidget *parent=0);
        ~MiscOptionTab() override;

        void readSettings();
        bool saveSettings();

Q_SIGNALS:
        void changed();
    };
}

#endif
