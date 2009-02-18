/*
 *
 *
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

#include "k3bexternalencoderconfigwidget.h"

#include <k3bcore.h>

#include <qcheckbox.h>
#include <qlayout.h>
#include <q3listview.h>
#include <qpushbutton.h>
//Added by qt3to4:
#include <QHBoxLayout>
#include <QList>

#include <klineedit.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>


K3B_EXPORT_PLUGIN_CONFIG_WIDGET( kcm_k3bexternalencoder, K3bExternalEncoderSettingsWidget )

K3bExternalEncoderEditDialog::K3bExternalEncoderEditDialog( QWidget* parent )
    : KDialog( parent )
{
    setModal( true );
    setCaption( i18n("Editing external audio encoder") );
    setButtons( Ok | Cancel );

    m_editW = new base_K3bExternalEncoderEditWidget( this );
    setMainWidget( m_editW );
    connect(this, SIGNAL(okClicked()),this,SLOT(slotOk()));
}


K3bExternalEncoderEditDialog::~K3bExternalEncoderEditDialog()
{
}


K3bExternalEncoderCommand K3bExternalEncoderEditDialog::currentCommand() const
{
    K3bExternalEncoderCommand cmd;
    cmd.name = m_editW->m_editName->text();
    cmd.extension = m_editW->m_editExtension->text();
    cmd.command = m_editW->m_editCommand->text();
    cmd.swapByteOrder = m_editW->m_checkSwapByteOrder->isChecked();
    cmd.writeWaveHeader = m_editW->m_checkWriteWaveHeader->isChecked();
    return cmd;
}


void K3bExternalEncoderEditDialog::setCommand( const K3bExternalEncoderCommand& cmd )
{
    m_editW->m_editName->setText( cmd.name );
    m_editW->m_editExtension->setText( cmd.extension );
    m_editW->m_editCommand->setText( cmd.command );
    m_editW->m_checkSwapByteOrder->setChecked( cmd.swapByteOrder );
    m_editW->m_checkWriteWaveHeader->setChecked( cmd.writeWaveHeader );
}


void K3bExternalEncoderEditDialog::slotOk()
{
    if( m_editW->m_editName->text().isEmpty() ) {
        KMessageBox::error( this,
                            i18n("Please specify a name for the command."),
                            i18n("No name specified") );
    }
    else if( m_editW->m_editExtension->text().isEmpty() ) {
        KMessageBox::error( this,
                            i18n("Please specify an extension for the command."),
                            i18n("No extension specified") );
    }
    else if( m_editW->m_editCommand->text().isEmpty() ) {
        KMessageBox::error( this,
                            i18n("Please specify the command line."),
                            i18n("No command line specified") );
    }
    else if( !m_editW->m_editCommand->text().contains( "%f" ) ) {
        KMessageBox::error( this,
                            // xgettext: no-c-format
                            i18n("Please add the output filename (%f) to the command line."),
                            i18n("No filename specified") );
    }
    // FIXME: check for name and extension uniqueness
    else {
        accept();
    }
}






class K3bExternalEncoderSettingsWidget::Private
{
public:
    QMap<Q3ListViewItem*, K3bExternalEncoderCommand> commands;
};


K3bExternalEncoderSettingsWidget::K3bExternalEncoderSettingsWidget( QWidget* parent, const QVariantList& args )
    : K3bPluginConfigWidget( parent, args )
{
    d = new Private();

    w = new base_K3bExternalEncoderConfigWidget( this );
    QHBoxLayout* lay = new QHBoxLayout( this );
    lay->setMargin( 0 );
    lay->addWidget( w );

    connect( w->m_viewEncoders, SIGNAL(selectionChanged()),
             this, SLOT(slotSelectionChanged()) );
    connect( w->m_buttonAdd, SIGNAL(clicked()),
             this, SLOT(slotNewCommand()) );
    connect( w->m_buttonEdit, SIGNAL(clicked()),
             this, SLOT(slotEditCommand()) );
    connect( w->m_buttonRemove, SIGNAL(clicked()),
             this, SLOT(slotRemoveCommand()) );

    m_editDlg = new K3bExternalEncoderEditDialog( this );
    slotSelectionChanged();
}


K3bExternalEncoderSettingsWidget::~K3bExternalEncoderSettingsWidget()
{
    delete d;
}


void K3bExternalEncoderSettingsWidget::slotNewCommand()
{
    // clear the dialog
    m_editDlg->setCommand( K3bExternalEncoderCommand() );

    if( m_editDlg->exec() == QDialog::Accepted ) {
        K3bExternalEncoderCommand cmd = m_editDlg->currentCommand();
        d->commands.insert( new Q3ListViewItem( w->m_viewEncoders,
                                                w->m_viewEncoders->lastItem(),
                                                cmd.name,
                                                cmd.extension,
                                                cmd.command ),
                            cmd );
    }
}


void K3bExternalEncoderSettingsWidget::slotSelectionChanged()
{
    w->m_buttonRemove->setEnabled( w->m_viewEncoders->selectedItem() != 0 );
    w->m_buttonEdit->setEnabled( w->m_viewEncoders->selectedItem() != 0 );
}


void K3bExternalEncoderSettingsWidget::slotEditCommand()
{
    if( Q3ListViewItem* item = w->m_viewEncoders->selectedItem() ) {
        m_editDlg->setCommand( d->commands[item] );
        if( m_editDlg->exec() == QDialog::Accepted ) {
            d->commands[item] = m_editDlg->currentCommand();
        }
    }
}


void K3bExternalEncoderSettingsWidget::slotRemoveCommand()
{
    if( Q3ListViewItem* item = w->m_viewEncoders->selectedItem() ) {
        d->commands.remove( item );
        delete item;
    }
}


void K3bExternalEncoderSettingsWidget::load()
{
    d->commands.clear();
    w->m_viewEncoders->clear();

    QList<K3bExternalEncoderCommand> cmds( K3bExternalEncoderCommand::readCommands() );
    for( QList<K3bExternalEncoderCommand>::iterator it = cmds.begin();
         it != cmds.end(); ++it ) {
        K3bExternalEncoderCommand& cmd = *it;
        d->commands.insert( new Q3ListViewItem( w->m_viewEncoders,
                                                w->m_viewEncoders->lastItem(),
                                                cmd.name,
                                                cmd.extension,
                                                cmd.command ),
                            cmd );
    }
}


void K3bExternalEncoderSettingsWidget::save()
{
    KSharedConfig::Ptr c = KGlobal::config();
    c->deleteGroup( "K3bExternalEncoderPlugin" );

    KConfigGroup grp( c, "K3bExternalEncoderPlugin" );

    QStringList cmdNames;

    for( QMap<Q3ListViewItem*, K3bExternalEncoderCommand>::const_iterator it = d->commands.constBegin();
         it != d->commands.constEnd(); ++it ) {
        QStringList cmd;
        cmd << it.value().name << it.value().extension << it.value().command;
        if( it.value().swapByteOrder )
            cmd << "swap";
        if( it.value().writeWaveHeader )
            cmd << "wave";
        grp.writeEntry( "command_" + it.value().name, cmd );
        cmdNames << it.value().name;
    }
    grp.writeEntry( "commands", cmdNames );

    grp.sync();
}

#include "k3bexternalencoderconfigwidget.moc"
