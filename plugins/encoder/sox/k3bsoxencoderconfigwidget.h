/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C)      2010 Michal Malek <michalm@jabster.pl>
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

#ifndef _K3B_SOX_ENCODER_CONFIG_WIDGET_H_
#define _K3B_SOX_ENCODER_CONFIG_WIDGET_H_

#include "k3bpluginconfigwidget.h"

#include "ui_base_k3bsoxencoderconfigwidget.h"

class K3bSoxEncoderConfigWidget : public K3b::PluginConfigWidget, Ui::base_K3bSoxEncoderConfigWidget
{
    Q_OBJECT

public:
    K3bSoxEncoderConfigWidget( QWidget* parent = 0, const QVariantList& args = QVariantList() );
    ~K3bSoxEncoderConfigWidget();

public Q_SLOTS:
    virtual void load();
    virtual void save();
    virtual void defaults();
    
private:
    void setChannels( int channels );
    int getChannels() const;
    void setDataSize( int size );
    int getDataSize() const;
    void setDataEncoding( const QString& encoding );
    QString getDataEncoding() const;
};

#endif // _K3B_SOX_ENCODER_CONFIG_WIDGET_H_
