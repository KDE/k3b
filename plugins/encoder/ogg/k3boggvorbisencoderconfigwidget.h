/*
    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_OGG_VORBIS_ENCODER_CONFIG_WIDGET_H_
#define _K3B_OGG_VORBIS_ENCODER_CONFIG_WIDGET_H_

#include "k3bpluginconfigwidget.h"
#include "ui_base_k3boggvorbisencodersettingswidget.h"

class base_K3bOggVorbisEncoderSettingsWidget : public QWidget, public Ui::base_K3bOggVorbisEncoderSettingsWidget
{
public:
    explicit base_K3bOggVorbisEncoderSettingsWidget( QWidget *parent ) : QWidget( parent ) {
        setupUi( this );
    }
};

class K3bOggVorbisEncoderSettingsWidget : public K3b::PluginConfigWidget
{
    Q_OBJECT

public:
    explicit K3bOggVorbisEncoderSettingsWidget( QWidget* parent = 0, const QVariantList& = QVariantList() );
    ~K3bOggVorbisEncoderSettingsWidget() override;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

private Q_SLOTS:
    void slotQualityLevelChanged( int val );

private:
    base_K3bOggVorbisEncoderSettingsWidget* w;
};

#endif
