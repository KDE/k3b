/*
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


#include "k3bmiscoptiontab.h"

#include "k3bpluginmanager.h"
#include "k3bcore.h"
#include "k3bglobalsettings.h"
#include "k3binteractiondialog.h"
#include "k3bintmapcombobox.h"

#include <KComboBox>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>
#include <KUrlRequester>
#include <KMessageBox>

#include <QDir>
#include <QFileInfo>
#include <QCheckBox>
#include <QRadioButton>


K3b::MiscOptionTab::MiscOptionTab(QWidget *parent )
    : QWidget(parent)
{
    setupUi( this );

    m_editTempDir->setMode( KFile::Directory );

    m_comboActionDialogSettings->insertItem( K3b::InteractionDialog::LOAD_K3B_DEFAULTS,
                                             i18n("Default Settings"),
                                             i18n("Load the K3b Defaults at dialog startup.") );
    m_comboActionDialogSettings->insertItem( K3b::InteractionDialog::LOAD_SAVED_SETTINGS,
                                             i18n("Saved Settings"),
                                             i18n("Load the settings saved by the user at dialog startup.") );
    m_comboActionDialogSettings->insertItem( K3b::InteractionDialog::LOAD_LAST_SETTINGS,
                                             i18n("Last Used Settings"),
                                             i18n("Load the last used settings at dialog startup.") );
    m_comboActionDialogSettings->addGlobalWhatsThisText( i18n("K3b handles three sets of settings in action dialogs "
                                                              "(action dialogs include the CD Copy dialog or the Audio CD "
                                                              "project dialog):"),
                                                         i18n("One of these sets is loaded once an action dialog is opened. "
                                                              "This setting defines which set it will be.") );

    connect(m_checkSaveOnExit, &QCheckBox::stateChanged, [this]{ Q_EMIT changed(); });
}


K3b::MiscOptionTab::~MiscOptionTab()
{
}


void K3b::MiscOptionTab::readSettings()
{
    KConfigGroup c = KSharedConfig::openConfig()->group( "General Options" );

    m_checkSaveOnExit->setChecked( c.readEntry( "ask_for_saving_changes_on_exit", true ) );
    m_checkShowSplash->setChecked( c.readEntry("Show splash", true) );
    m_checkShowProgressOSD->setChecked(c.readEntry("Show progress OSD", false));
    m_checkHideMainWindowWhileWriting->setChecked( c.readEntry( "hide main window while writing", false ) );
    m_checkKeepDialogsOpen->setChecked( c.readEntry( "keep action dialogs open", false ) );
    m_comboActionDialogSettings->setSelectedValue( c.readEntry( "action dialog startup settings",
                                                                ( int )K3b::InteractionDialog::LOAD_SAVED_SETTINGS ) );
    m_checkSystemConfig->setChecked( c.readEntry( "check system config", true ) );

    m_editTempDir->setUrl( QUrl::fromLocalFile( k3bcore->globalSettings()->defaultTempPath() ) );

//   if( c.readEntry( "Multiple Instances", "smart" ) == "smart" )
//     m_radioMultipleInstancesSmart->setChecked(true);
//   else
//     m_radioMultipleInstancesNew->setChecked(true);
}


bool K3b::MiscOptionTab::saveSettings()
{
    KConfigGroup c = KSharedConfig::openConfig()->group( "General Options" );

    c.writeEntry( "ask_for_saving_changes_on_exit", m_checkSaveOnExit->isChecked() );
    c.writeEntry( "Show splash", m_checkShowSplash->isChecked() );
    c.writeEntry( "Show progress OSD", m_checkShowProgressOSD->isChecked() );
    c.writeEntry( "hide main window while writing", m_checkHideMainWindowWhileWriting->isChecked() );
    c.writeEntry( "keep action dialogs open", m_checkKeepDialogsOpen->isChecked() );
    c.writeEntry( "check system config", m_checkSystemConfig->isChecked() );
    c.writeEntry( "action dialog startup settings", m_comboActionDialogSettings->selectedValue() );

    QString tempDir = m_editTempDir->url().toLocalFile();
    QFileInfo fi( tempDir );
    
    if( fi.isRelative() ) {
        fi.setFile( fi.absoluteFilePath() );
    }

    if( !fi.exists() ) {
        if( KMessageBox::questionYesNo( this,
                                        i18n("Folder (%1) does not exist. Create?",tempDir),
                                        i18n("Create Folder"),
                                        KGuiItem( i18n("Create") ),
                                        KStandardGuiItem::cancel() ) == KMessageBox::Yes ) {
            if( !QDir().mkpath( fi.absoluteFilePath() ) ) {
                KMessageBox::error( this, i18n("Unable to create folder %1",tempDir) );
                return false;
            }
        }
        else {
            // the dir does not exist and the user doesn't want to create it
            return false;
        }
    }

    if( fi.isFile() ) {
        KMessageBox::information( this, i18n("You specified a file for the temporary folder. "
                                             "K3b will use its base path as the temporary folder."),
                                  i18n("Warning"),
                                  "temp file only using base path" );
        fi.setFile( fi.path() );
    }

    // check for writing permission
    if( !fi.isWritable() ) {
        KMessageBox::error( this, i18n("You do not have permission to write to %1.",fi.absoluteFilePath()) );
        return false;
    }
    

    m_editTempDir->setUrl( QUrl::fromLocalFile( fi.absoluteFilePath() ) );

    k3bcore->globalSettings()->setDefaultTempPath( m_editTempDir->url().toLocalFile() );

//   if( m_radioMultipleInstancesSmart->isChecked() )
//     c.writeEntry( "Multiple Instances", "smart" );
//   else
//     c.writeEntry( "Multiple Instances", "always_new" );

    return true;
}


