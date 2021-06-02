/*


    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include "k3bexternalencoderconfigwidget.h"
#include "k3bcore.h"
#include "k3bplugin_i18n.h"

#include <KConfig>
#include <KMessageBox>

#include <QDebug>
#include <QList>
#include <QCheckBox>
#include <QTreeWidget>

K3B_EXPORT_PLUGIN_CONFIG_WIDGET( kcm_k3bexternalencoder, K3bExternalEncoderSettingsWidget )


K3bExternalEncoderEditDialog::K3bExternalEncoderEditDialog( QWidget* parent )
    : QDialog( parent )
{
    setModal( true );
    setWindowTitle( i18n("Editing external audio encoder") );
    setupUi( this );
}


K3bExternalEncoderEditDialog::~K3bExternalEncoderEditDialog()
{
}


K3bExternalEncoderCommand K3bExternalEncoderEditDialog::currentCommand() const
{
    K3bExternalEncoderCommand cmd;
    cmd.name = m_editName->text();
    cmd.extension = m_editExtension->text();
    cmd.command = m_editCommand->text();
    cmd.swapByteOrder = m_checkSwapByteOrder->isChecked();
    cmd.writeWaveHeader = m_checkWriteWaveHeader->isChecked();
    return cmd;
}


void K3bExternalEncoderEditDialog::setCommand( const K3bExternalEncoderCommand& cmd )
{
    m_editName->setText( cmd.name );
    m_editExtension->setText( cmd.extension );
    m_editCommand->setText( cmd.command );
    m_checkSwapByteOrder->setChecked( cmd.swapByteOrder );
    m_checkWriteWaveHeader->setChecked( cmd.writeWaveHeader );
}


void K3bExternalEncoderEditDialog::accept()
{
    if( m_editName->text().isEmpty() ) {
        KMessageBox::error( this,
                            i18n("Please specify a name for the command."),
                            i18n("No name specified") );
    }
    else if( m_editExtension->text().isEmpty() ) {
        KMessageBox::error( this,
                            i18n("Please specify an extension for the command."),
                            i18n("No extension specified") );
    }
    else if( m_editCommand->text().isEmpty() ) {
        KMessageBox::error( this,
                            i18n("Please specify the command line."),
                            i18n("No command line specified") );
    }
    else if( !m_editCommand->text().contains( "%f" ) ) {
        KMessageBox::error( this,
                            // xgettext: no-c-format
                            i18n("Please add the output filename (%f) to the command line."),
                            i18n("No filename specified") );
    }
    // FIXME: check for name and extension uniqueness
    else {
        QDialog::accept();
    }
}




K3bExternalEncoderSettingsWidget::K3bExternalEncoderSettingsWidget( QWidget* parent, const QVariantList& args )
    : K3b::PluginConfigWidget( parent, args )
{
    setupUi( this );

    connect( m_viewEncoders, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
             this, SLOT(slotSelectionChanged(QTreeWidgetItem*)) );
    connect( m_viewEncoders, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
             this, SLOT(slotEditCommand()) );
    connect( m_buttonAdd, SIGNAL(clicked()),
             this, SLOT(slotNewCommand()) );
    connect( m_buttonEdit, SIGNAL(clicked()),
             this, SLOT(slotEditCommand()) );
    connect( m_buttonRemove, SIGNAL(clicked()),
             this, SLOT(slotRemoveCommand()) );

    m_editDlg = new K3bExternalEncoderEditDialog( this );
    slotSelectionChanged( 0 );
}


K3bExternalEncoderSettingsWidget::~K3bExternalEncoderSettingsWidget()
{
}


void K3bExternalEncoderSettingsWidget::slotNewCommand()
{
    // clear the dialog
    m_editDlg->setCommand( K3bExternalEncoderCommand() );

    if( m_editDlg->exec() == QDialog::Accepted ) {
        K3bExternalEncoderCommand cmd = m_editDlg->currentCommand();
        createItem( cmd );
        emit changed( true );
    }
}


void K3bExternalEncoderSettingsWidget::slotSelectionChanged( QTreeWidgetItem* current )
{
    m_buttonRemove->setEnabled( current != 0 );
    m_buttonEdit->setEnabled( current != 0 );
}


void K3bExternalEncoderSettingsWidget::slotEditCommand()
{
    if( QTreeWidgetItem* item = m_viewEncoders->currentItem() ) {
        m_editDlg->setCommand( m_commands[item] );
        if( m_editDlg->exec() == QDialog::Accepted ) {
            m_commands[item] = m_editDlg->currentCommand();
            fillItem( item, m_editDlg->currentCommand() );
            emit changed( true );
        }
    }
}


void K3bExternalEncoderSettingsWidget::slotRemoveCommand()
{
    if( QTreeWidgetItem* item = m_viewEncoders->currentItem() ) {
        m_commands.remove( item );
        delete item;
        emit changed( true );
    }
}


void K3bExternalEncoderSettingsWidget::load()
{
    qDebug();
    fillEncoderView( K3bExternalEncoderCommand::readCommands() );
}


void K3bExternalEncoderSettingsWidget::save()
{
    qDebug();
    K3bExternalEncoderCommand::saveCommands( m_commands.values() );
    emit changed( false );
}


void K3bExternalEncoderSettingsWidget::defaults()
{
    qDebug();
    fillEncoderView( K3bExternalEncoderCommand::defaultCommands() );
    emit changed( true );
}


QTreeWidgetItem* K3bExternalEncoderSettingsWidget::createItem( const K3bExternalEncoderCommand& cmd )
{
    QTreeWidgetItem* item = new QTreeWidgetItem( m_viewEncoders );
    fillItem( item, cmd );
    m_commands.insert( item, cmd );
    return item;
}


void K3bExternalEncoderSettingsWidget::fillItem( QTreeWidgetItem* item, const K3bExternalEncoderCommand& cmd )
{
    item->setText( 0, cmd.name );
    item->setText( 1, cmd.extension );
    item->setText( 2, cmd.command );
}


void K3bExternalEncoderSettingsWidget::fillEncoderView( const QList<K3bExternalEncoderCommand>& commands )
{
    m_commands.clear();
    m_viewEncoders->clear();

    QList<K3bExternalEncoderCommand> cmds( commands );
    for( QList<K3bExternalEncoderCommand>::iterator it = cmds.begin();
         it != cmds.end(); ++it ) {
        K3bExternalEncoderCommand& cmd = *it;
        createItem( cmd );
    }
}

#include "k3bexternalencoderconfigwidget.moc"
