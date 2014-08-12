/*
 *
 *
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_LAME_MANUAL_SETTINGS_DIALOG_H_
#define _K3B_LAME_MANUAL_SETTINGS_DIALOG_H_

#include <KDELibs4Support/KDE/KDialog>

#include "ui_base_k3bmanualbitratesettingswidget.h"

class K3bLameManualSettingsDialog : public KDialog, public Ui::K3bManualBitrateSettingsWidget
{
public:
    K3bLameManualSettingsDialog( QWidget* parent = 0 );
    ~K3bLameManualSettingsDialog();
};

#endif
