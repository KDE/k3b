/*
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_EXTERNAL_ENCODER_CONFIG_WIDGET_H_
#define _K3B_EXTERNAL_ENCODER_CONFIG_WIDGET_H_

#include "ui_base_k3bexternalencodereditdialog.h"
#include "ui_base_k3bexternalencoderconfigwidget.h"
#include "k3bexternalencodercommand.h"

#include "k3bpluginconfigwidget.h"

#include <QtCore/QList>
#include <QtWidgets/QDialog>


class QTreeWidgetItem;

class K3bExternalEncoderEditDialog : public QDialog, public Ui::base_K3bExternalEncoderEditDialog
{
    Q_OBJECT

public:
    K3bExternalEncoderEditDialog( QWidget* parent );
    ~K3bExternalEncoderEditDialog();

    K3bExternalEncoderCommand currentCommand() const;
    void setCommand( const K3bExternalEncoderCommand& cmd );

private Q_SLOTS:
    void accept() override;
};


class K3bExternalEncoderSettingsWidget : public K3b::PluginConfigWidget, public Ui::base_K3bExternalEncoderConfigWidget
{
    Q_OBJECT

public:
    K3bExternalEncoderSettingsWidget( QWidget* parent, const QVariantList& args );
    ~K3bExternalEncoderSettingsWidget();

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

private Q_SLOTS:
    void slotSelectionChanged( QTreeWidgetItem* current );
    void slotNewCommand();
    void slotEditCommand();
    void slotRemoveCommand();

private:
    QTreeWidgetItem* createItem( const K3bExternalEncoderCommand& cmd );
    void fillItem( QTreeWidgetItem* item, const K3bExternalEncoderCommand& cmd );
    void fillEncoderView( const QList<K3bExternalEncoderCommand>& commands );

    K3bExternalEncoderEditDialog* m_editDlg;
    QMap<QTreeWidgetItem*, K3bExternalEncoderCommand> m_commands;
};

#endif
