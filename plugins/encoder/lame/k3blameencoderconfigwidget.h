/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_LAME_ENCODER_CONFIG_WIDGET_H_
#define _K3B_LAME_ENCODER_CONFIG_WIDGET_H_

#include "k3bpluginconfigwidget.h"

#include "ui_base_k3blameencodersettingswidget.h"
#include "ui_base_k3bmanualbitratesettingsdialog.h"

class K3bLameManualSettingsDialog;

class K3bLameEncoderSettingsWidget : public K3b::PluginConfigWidget, Ui::K3bLameEncoderSettingsWidget
{
    Q_OBJECT

public:
    K3bLameEncoderSettingsWidget( QObject* parent, const KPluginMetaData& metaData, const QVariantList& args );
    ~K3bLameEncoderSettingsWidget() override;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

private Q_SLOTS:
    void slotQualityLevelChanged( int val );
    void slotShowManualSettings();
    void updateManualSettingsLabel();

private:
    K3bLameManualSettingsDialog* m_manualSettingsDialog;
};

#endif
