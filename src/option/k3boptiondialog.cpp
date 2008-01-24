/*
 *
 * $Id$
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


#include "k3boptiondialog.h"
#include <k3bcore.h>
#include "k3bdeviceoptiontab.h"
#include "k3badvancedoptiontab.h"
#include "k3bexternalbinoptiontab.h"
#include "k3bmiscoptiontab.h"
#include "k3bthemeoptiontab.h"
#include "k3bpluginoptiontab.h"
#include "k3bcddboptiontab.h"

#include <k3bsystemproblemdialog.h>


#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <qtabwidget.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QFrame>

#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include "k3bnotifyoptiontab.h"


// TODO: handle the default-settings

K3bOptionDialog::K3bOptionDialog(QWidget *parent )
    : KPageDialog( parent )
{
    setFaceType( List );
    setCaption( i18n("Settings") );
    showButtonSeparator( true );

    setupMiscPage();
    setupDevicePage();
    setupProgramsPage();
    setupNotifyPage();
    setupPluginPage();
    setupThemePage();
    setupCddbPage();
    setupAdvancedPage();

    m_externalBinOptionTab->readSettings();
    m_deviceOptionTab->readDevices();
    m_advancedOptionTab->readSettings();
    m_miscOptionTab->readSettings();
    m_notifyOptionTab->readSettings();
    m_pluginOptionTab->readSettings();
    m_themeOptionTab->readSettings();

    // if we don't do this the dialog start really huge
    // because of the label in the device-tab
    resize( 700, 500 );

    connect( this, SIGNAL( okClicked() ), SLOT( slotOk() ) );
    connect( this, SIGNAL( defaultClicked() ), SLOT( slotDefault() ) );
    connect( this, SIGNAL( applyClicked() ), SLOT( slotApply() ) );
}


K3bOptionDialog::~K3bOptionDialog()
{
}


void K3bOptionDialog::slotOk()
{
    if( saveSettings() ) {
        accept();

        KConfigGroup grp( k3bcore->config(), "General Options" );
        if( grp.readEntry( "check system config", true ) )
            K3bSystemProblemDialog::checkSystem();
    }
}

void K3bOptionDialog::slotApply()
{
    saveSettings();
}


bool K3bOptionDialog::saveSettings()
{
    m_deviceOptionTab->saveDevices();
    m_advancedOptionTab->saveSettings();
    m_externalBinOptionTab->saveSettings();
    m_notifyOptionTab->saveSettings();

    m_themeOptionTab->saveSettings();

    if( !m_miscOptionTab->saveSettings() )
        return false;

    return true;
}


void K3bOptionDialog::slotDefault()
{
}


void K3bOptionDialog::setupAdvancedPage()
{
    m_advancedOptionTab = new K3bAdvancedOptionTab;
    KPageWidgetItem* item = addPage( m_advancedOptionTab, i18n("Advanced") );
    item->setHeader( i18n("Advanced Settings") );
    item->setIcon( KIcon( "media-optical-recordable" ) );
}


void K3bOptionDialog::setupProgramsPage()
{
    m_externalBinOptionTab = new K3bExternalBinOptionTab( k3bcore->externalBinManager() );
    KPageWidgetItem* item = addPage( m_externalBinOptionTab, i18n("Programs") );
    item->setHeader( i18n("Setup External Programs") );
    item->setIcon( KIcon( "system-run" ) );
}


void K3bOptionDialog::setupDevicePage()
{
    m_deviceOptionTab = new K3bDeviceOptionTab;
    KPageWidgetItem* item = addPage( m_deviceOptionTab, i18n("Devices") );
    item->setHeader( i18n("Setup Devices") );
    item->setIcon( KIcon( "drive-optical" ) );
}


void K3bOptionDialog::setupMiscPage()
{
    m_miscOptionTab = new K3bMiscOptionTab;
    KPageWidgetItem* item = addPage( m_miscOptionTab, i18n("Misc") );
    item->setHeader( i18n("Miscellaneous Settings") );
    item->setIcon( KIcon( "preferences-other" ) );
}


void K3bOptionDialog::setupNotifyPage()
{
    m_notifyOptionTab = new K3bNotifyOptionTab;
    KPageWidgetItem* item = addPage( m_notifyOptionTab, i18n("Notifications") );
    item->setHeader( i18n("System Notifications") );
    item->setIcon( KIcon( "preferences-desktop-notification" ) );
}


void K3bOptionDialog::setupPluginPage()
{
    m_pluginOptionTab = new K3bPluginOptionTab;
    KPageWidgetItem* item = addPage( m_pluginOptionTab, i18n("Plugins") );
    item->setHeader( i18n("K3b Plugin Configuration") );
    item->setIcon( KIcon( "preferences-plugin" ) );
}


void K3bOptionDialog::setupThemePage()
{
    m_themeOptionTab = new K3bThemeOptionTab;
    KPageWidgetItem* item = addPage( m_themeOptionTab, i18n("Themes") );
    item->setHeader( i18n("K3b GUI Themes") );
    item->setIcon( KIcon( "preferences-desktop-theme" ) );
}


void K3bOptionDialog::setupCddbPage()
{
    m_cddbOptionTab = new K3bCddbOptionTab;
    KPageWidgetItem* item = addPage( m_cddbOptionTab, i18n("CDDB") );
    item->setHeader( i18n("CDDB Audio CD Info Retrieval") );
    item->setIcon( KIcon( "media-optical-audio" ) );
}

#include "k3boptiondialog.moc"
