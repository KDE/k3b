/* 
 *
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

#ifndef _K3B_LAME_ENCODER_CONFIG_WIDGET_H_
#define _K3B_LAME_ENCODER_CONFIG_WIDGET_H_

#include <k3bpluginconfigwidget.h>

#include "ui_base_k3blameencodersettingswidget.h"
#include "ui_base_k3bmanualbitratesettingswidget.h"

class K3bLameManualSettingsDialog;

class K3bLameEncoderSettingsWidget : public K3b::PluginConfigWidget, Ui::K3bLameEncoderSettingsWidget
{
    Q_OBJECT

public:
    K3bLameEncoderSettingsWidget( QWidget* parent, const QVariantList& args );
    ~K3bLameEncoderSettingsWidget();

public Q_SLOTS:
    void load();
    void save();
    void defaults();

private Q_SLOTS:
    void slotQualityLevelChanged( int val );
    void slotShowManualSettings();
    void updateManualSettingsLabel();

private:
    K3bLameManualSettingsDialog* m_manualSettingsDialog;
};

#endif
