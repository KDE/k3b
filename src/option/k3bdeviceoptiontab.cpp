/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
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

#include "k3bdeviceoptiontab.h"
#include "k3bdevicemanager.h"
#include "k3bdevicewidget.h"
#include "k3bglobals.h"
#include "k3bcore.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>

#include <QCursor>
#include <QApplication>
#include <QVBoxLayout>


K3b::DeviceOptionTab::DeviceOptionTab( QWidget* parent )
    : QWidget( parent )
{
    m_deviceWidget = new K3b::DeviceWidget( k3bcore->deviceManager(), this );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( m_deviceWidget );

    connect( m_deviceWidget, SIGNAL(refreshButtonClicked()), this, SLOT(slotRefreshButtonClicked()) );
}


K3b::DeviceOptionTab::~DeviceOptionTab()
{
}


void K3b::DeviceOptionTab::readDevices()
{
    m_deviceWidget->init();
}



void K3b::DeviceOptionTab::saveDevices()
{
    // save the config
    k3bcore->deviceManager()->saveConfig( KSharedConfig::openConfig()->group( "Devices" ) );
}


void K3b::DeviceOptionTab::slotRefreshButtonClicked()
{
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    k3bcore->deviceManager()->clear();
    k3bcore->deviceManager()->scanBus();
    m_deviceWidget->init();
    QApplication::restoreOverrideCursor();
}


