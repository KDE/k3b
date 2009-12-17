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

#ifndef _K3B_OGG_VORBIS_ENCODER_CONFIG_WIDGET_H_
#define _K3B_OGG_VORBIS_ENCODER_CONFIG_WIDGET_H_

#include "k3bpluginconfigwidget.h"
#include "ui_base_k3boggvorbisencodersettingswidget.h"

class base_K3bOggVorbisEncoderSettingsWidget : public QWidget, public Ui::base_K3bOggVorbisEncoderSettingsWidget
{
public:
    base_K3bOggVorbisEncoderSettingsWidget( QWidget *parent ) : QWidget( parent ) {
        setupUi( this );
    }
};

class K3bOggVorbisEncoderSettingsWidget : public K3b::PluginConfigWidget
{
    Q_OBJECT

public:
    K3bOggVorbisEncoderSettingsWidget( QWidget* parent = 0, const QVariantList& = QVariantList() );
    ~K3bOggVorbisEncoderSettingsWidget();

public Q_SLOTS:
    virtual void load();
    virtual void save();
    virtual void defaults();

private Q_SLOTS:
    void slotQualityLevelChanged( int val );

private:
    base_K3bOggVorbisEncoderSettingsWidget* w;
};

#endif
