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

#ifndef _K3B_EXTERNAL_ENCODER_CONFIG_WIDGET_H_
#define _K3B_EXTERNAL_ENCODER_CONFIG_WIDGET_H_

#include "ui_base_k3bexternalencodereditwidget.h"
#include "ui_base_k3bexternalencoderconfigwidget.h"
#include "k3bexternalencodercommand.h"

#include "k3bpluginconfigwidget.h"
#include <kdialog.h>

class base_K3bExternalEncoderConfigWidget : public QWidget, public Ui::base_K3bExternalEncoderConfigWidget
{
public:
    base_K3bExternalEncoderConfigWidget( QWidget *parent ) : QWidget( parent ) {
        setupUi( this );
    }
};

class base_K3bExternalEncoderEditWidget : public QWidget, public Ui::base_K3bExternalEncoderEditWidget
{
public:
    base_K3bExternalEncoderEditWidget( QWidget *parent ) : QWidget( parent ) {
        setupUi( this );
    }
};

class K3bExternalEncoderEditDialog : public KDialog
{
    Q_OBJECT
  
public:
    K3bExternalEncoderEditDialog( QWidget* parent );
    ~K3bExternalEncoderEditDialog();

    K3bExternalEncoderCommand currentCommand() const;
    void setCommand( const K3bExternalEncoderCommand& cmd );

private Q_SLOTS:
    void slotOk();

private:
    base_K3bExternalEncoderEditWidget* m_editW;
};


class K3bExternalEncoderSettingsWidget : public K3b::PluginConfigWidget
{
    Q_OBJECT

public:
    K3bExternalEncoderSettingsWidget( QWidget* parent, const QVariantList& args );
    ~K3bExternalEncoderSettingsWidget();

public Q_SLOTS:
    void load();
    void save();

private Q_SLOTS:
    void slotSelectionChanged();
    void slotNewCommand();
    void slotEditCommand();
    void slotRemoveCommand();

private:
    base_K3bExternalEncoderConfigWidget* w;
    K3bExternalEncoderEditDialog* m_editDlg;

    class Private;
    Private* d;
};

#endif
