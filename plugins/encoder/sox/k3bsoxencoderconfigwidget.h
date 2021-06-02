/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_SOX_ENCODER_CONFIG_WIDGET_H_
#define _K3B_SOX_ENCODER_CONFIG_WIDGET_H_

#include "k3bpluginconfigwidget.h"

#include "ui_base_k3bsoxencoderconfigwidget.h"

class K3bSoxEncoderConfigWidget : public K3b::PluginConfigWidget, Ui::base_K3bSoxEncoderConfigWidget
{
    Q_OBJECT

public:
    explicit K3bSoxEncoderConfigWidget( QWidget* parent = 0, const QVariantList& args = QVariantList() );
    ~K3bSoxEncoderConfigWidget() override;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;
    
private:
    void setChannels( int channels );
    int getChannels() const;
    void setDataSize( int size );
    int getDataSize() const;
    void setDataEncoding( const QString& encoding );
    QString getDataEncoding() const;
};

#endif // _K3B_SOX_ENCODER_CONFIG_WIDGET_H_
