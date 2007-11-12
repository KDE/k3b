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

#include "k3bdeviceoptiontab.h"
#include <k3bdevicemanager.h>
#include "k3bdevicewidget.h"
#include <k3bglobals.h>
#include <k3bcore.h>

#include <qlabel.h>
#include <qstring.h>
#include <qlayout.h>
#include <qcursor.h>
#include <qapplication.h>
//Added by qt3to4:
#include <Q3GridLayout>

#include <kapplication.h>
#include <kdialog.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>


K3bDeviceOptionTab::K3bDeviceOptionTab( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  Q3GridLayout* frameLayout = new Q3GridLayout( this );
  frameLayout->setSpacing( KDialog::spacingHint() );
  frameLayout->setMargin( 0 );


  // Info Label
  // ------------------------------------------------
  m_labelDevicesInfo = new QLabel( this, "m_labelDevicesInfo" );
  m_labelDevicesInfo->setAlignment( int( QLabel::WordBreak | QLabel::Qt::AlignVCenter | QLabel::Qt::AlignLeft ) );
  m_labelDevicesInfo->setText( i18n( "K3b tries to detect all your devices properly. "
				     "You can add devices that have not been detected and change "
				     "the black values by clicking in the list. If K3b is unable "
				     "to detect your drive, you need to modify their permissions "
				     "to give K3b write access to all devices." ) );
  // ------------------------------------------------

  m_deviceWidget = new K3bDeviceWidget( k3bcore->deviceManager(), this );

  frameLayout->addWidget( m_labelDevicesInfo, 0, 0 );
  frameLayout->addWidget( m_deviceWidget, 1, 0 );

  connect( m_deviceWidget, SIGNAL(refreshButtonClicked()), this, SLOT(slotRefreshButtonClicked()) );
}


K3bDeviceOptionTab::~K3bDeviceOptionTab()
{
}


void K3bDeviceOptionTab::readDevices()
{
  m_deviceWidget->init();
}



void K3bDeviceOptionTab::saveDevices()
{
  // save changes to deviceManager
  m_deviceWidget->apply();

  // save the config
  k3bcore->deviceManager()->saveConfig( kapp->config() );
}


void K3bDeviceOptionTab::slotRefreshButtonClicked()
{
  QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
  k3bcore->deviceManager()->clear();
  k3bcore->deviceManager()->scanBus();
  m_deviceWidget->init();
  QApplication::restoreOverrideCursor();
}

#include "k3bdeviceoptiontab.moc"
