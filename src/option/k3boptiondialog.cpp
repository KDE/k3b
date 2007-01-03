/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3boptiondialog.h"
#include <k3bcore.h>
#include "k3bcddboptiontab.h"
#include "k3bdeviceoptiontab.h"
#include "k3bburningoptiontab.h"
#include "k3bexternalbinoptiontab.h"
#include "k3bmiscoptiontab.h"
#include "k3bthemeoptiontab.h"
#include "k3bpluginoptiontab.h"
#include <k3bsystemproblemdialog.h>


#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qtabwidget.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kdeversion.h>
#include "k3bnotifyoptiontab.h"


// TODO: handle the default-settings

K3bOptionDialog::K3bOptionDialog(QWidget *parent, const char *name, bool modal )
  : KDialogBase( IconList, i18n("Settings"), Apply|Ok|Cancel, Ok, parent,name, modal, true)
{
  setupMiscPage();
  setupDevicePage();	
  setupProgramsPage();
  setupCddbPage();
  setupNotifyPage();
  setupPluginPage();
  setupThemePage();
  setupBurningPage();

  m_externalBinOptionTab->readSettings();
  m_cddbOptionTab->readSettings();
  m_deviceOptionTab->readDevices();
  m_burningOptionTab->readSettings();
  m_miscOptionTab->readSettings();
  m_notifyOptionTab->readSettings();
  m_pluginOptionTab->readSettings();
  m_themeOptionTab->readSettings();

  // if we don't do this the dialog start really huge
  // because of the label in the device-tab
  resize( 700, 500 );

  showPage( 0 );
}


K3bOptionDialog::~K3bOptionDialog()
{
}


void K3bOptionDialog::slotOk()
{
  if( saveSettings() ) {
    accept();

    k3bcore->config()->setGroup( "General Options" );
    if( k3bcore->config()->readBoolEntry( "check system config", true ) )
      K3bSystemProblemDialog::checkSystem();
  }
}

void K3bOptionDialog::slotApply()
{
  saveSettings();
}


bool K3bOptionDialog::saveSettings()
{
  // save all the shit!
  m_cddbOptionTab->apply();
  m_deviceOptionTab->saveDevices();
  m_burningOptionTab->saveSettings();
  m_externalBinOptionTab->saveSettings();
  m_notifyOptionTab->saveSettings();

  m_themeOptionTab->saveSettings();

  if( !m_miscOptionTab->saveSettings() )
    return false;

  return true;
}


void K3bOptionDialog::slotDefault()
{
  switch( activePageIndex() )
    {
    case 0: // device page

      break;
    case 1: // programs page
      break;
    default: 
      break;
    }
}


void K3bOptionDialog::setupBurningPage()
{
  QFrame* frame = addPage( i18n("Advanced"), i18n("Advanced Settings"),
			   KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", KIcon::NoGroup, KIcon::SizeMedium ) );

  QGridLayout* _frameLayout = new QGridLayout( frame );
  _frameLayout->setSpacing( 0 );
  _frameLayout->setMargin( 0 );

  m_burningOptionTab = new K3bBurningOptionTab( frame );
  _frameLayout->addWidget( m_burningOptionTab, 0, 0 );
}


void K3bOptionDialog::setupProgramsPage()
{
  QFrame* frame = addPage( i18n("Programs"), i18n("Setup External Programs"),
			   KGlobal::instance()->iconLoader()->loadIcon( "exec", KIcon::NoGroup, KIcon::SizeMedium ) );

  QGridLayout* _frameLayout = new QGridLayout( frame );
  _frameLayout->setSpacing( 0 );
  _frameLayout->setMargin( 0 );

  m_externalBinOptionTab = new K3bExternalBinOptionTab( k3bcore->externalBinManager(), frame );
  _frameLayout->addWidget( m_externalBinOptionTab, 0, 0 );
}


void K3bOptionDialog::setupCddbPage()
{
  QFrame* frame = addPage( i18n("CDDB"), i18n("Setup the CDDB Server"),
			   KGlobal::instance()->iconLoader()->loadIcon( "connect_established", KIcon::NoGroup, KIcon::SizeMedium ) );

  QGridLayout* mainGrid = new QGridLayout( frame );
  mainGrid->setSpacing(0);
  mainGrid->setMargin(0);
  //  QTabWidget *_tab = new QTabWidget( frame );
  m_cddbOptionTab = new K3bCddbOptionTab( frame, "cddbremotepage");
  //  m_cddbLocalTab = new K3bCddbLocalDBTab( frame, "cddblocalpage");
//   _tab->addTab( m_cddbOptionTab, i18n("Remote") );
//   _tab->addTab( m_cddbLocalTab, i18n("Local") );
  //mainGrid->addWidget( m_cddbOptionTab, 0, 0 );
  mainGrid->addWidget( m_cddbOptionTab, 0, 0 );
}


void K3bOptionDialog::setupDevicePage()
{
  QFrame* frame = addPage( i18n("Devices"), i18n("Setup Devices"),
			   KGlobal::instance()->iconLoader()->loadIcon( "blockdevice", KIcon::NoGroup, KIcon::SizeMedium ) );

  QHBoxLayout* box = new QHBoxLayout( frame );
  box->setSpacing(0);
  box->setMargin(0);
  m_deviceOptionTab = new K3bDeviceOptionTab( frame, "deviceOptionTab" );
  box->addWidget( m_deviceOptionTab );
}


void K3bOptionDialog::setupMiscPage()
{
  QFrame* frame = addPage( i18n("Misc"), i18n("Miscellaneous Settings"),
			   KGlobal::instance()->iconLoader()->loadIcon( "misc", KIcon::NoGroup, KIcon::SizeMedium ) );

  QVBoxLayout* box = new QVBoxLayout( frame );
  box->setSpacing( 0 );
  box->setMargin( 0 );

  m_miscOptionTab = new K3bMiscOptionTab( frame );
  box->addWidget( m_miscOptionTab );
}


void K3bOptionDialog::setupNotifyPage()
{
  QFrame* frame = addPage( i18n("Notifications"), i18n("System Notifications"),
			   KGlobal::instance()->iconLoader()->loadIcon( "knotify", 
									KIcon::NoGroup, KIcon::SizeMedium ) );
  QVBoxLayout* box = new QVBoxLayout( frame );
  box->setSpacing( 0 );
  box->setMargin( 0 );

  m_notifyOptionTab = new K3bNotifyOptionTab( frame );
  box->addWidget( m_notifyOptionTab );
}


void K3bOptionDialog::setupPluginPage()
{
  QFrame* frame = addPage( i18n("Plugins"), i18n("K3b Plugin Configuration"),
			   KGlobal::instance()->iconLoader()->loadIcon( "gear",
									KIcon::NoGroup, KIcon::SizeMedium ) );
  QVBoxLayout* box = new QVBoxLayout( frame );
  box->setSpacing( 0 );
  box->setMargin( 0 );

  m_pluginOptionTab = new K3bPluginOptionTab( frame );
  box->addWidget( m_pluginOptionTab );
}


void K3bOptionDialog::setupThemePage()
{
  QFrame* frame = addPage( i18n("Themes"), i18n("K3b GUI Themes"),
			   KGlobal::instance()->iconLoader()->loadIcon( "style",
									KIcon::NoGroup, KIcon::SizeMedium ) );
  QVBoxLayout* box = new QVBoxLayout( frame );
  box->setSpacing( 0 );
  box->setMargin( 0 );

  m_themeOptionTab = new K3bThemeOptionTab( frame );
  box->addWidget( m_themeOptionTab );
}


// void K3bOptionDialog::addOptionPage( QWidget* widget,
// 				     const QString& name,
// 				     const QString& header,
// 				     const QPixmap& icon )
// {
//   QFrame* frame = addPage( name, header, icon );

//   QVBoxLayout* box = new QVBoxLayout( frame );
//   box->setSpacing( 0 );
//   box->setMargin( 0 );

//   widget->reparent( frame );
//   box->addWidget( widget );
// }


#include "k3boptiondialog.moc"
