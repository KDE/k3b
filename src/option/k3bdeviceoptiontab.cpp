/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
    k3bcore->deviceManager()->saveConfig( KSharedConfig::openConfig()->group( QStringLiteral("Devices") ) );
}


void K3b::DeviceOptionTab::slotRefreshButtonClicked()
{
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    k3bcore->deviceManager()->clear();
    k3bcore->deviceManager()->scanBus();
    m_deviceWidget->init();
    QApplication::restoreOverrideCursor();
}

#include "moc_k3bdeviceoptiontab.cpp"
