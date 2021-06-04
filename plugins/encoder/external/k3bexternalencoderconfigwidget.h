/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef _K3B_EXTERNAL_ENCODER_CONFIG_WIDGET_H_
#define _K3B_EXTERNAL_ENCODER_CONFIG_WIDGET_H_

#include "ui_base_k3bexternalencodereditdialog.h"
#include "ui_base_k3bexternalencoderconfigwidget.h"
#include "k3bexternalencodercommand.h"

#include "k3bpluginconfigwidget.h"

#include <QList>
#include <QDialog>


class QTreeWidgetItem;

class K3bExternalEncoderEditDialog : public QDialog, public Ui::base_K3bExternalEncoderEditDialog
{
    Q_OBJECT

public:
    explicit K3bExternalEncoderEditDialog( QWidget* parent );
    ~K3bExternalEncoderEditDialog() override;

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
    ~K3bExternalEncoderSettingsWidget() override;

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
