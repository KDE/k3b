/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "k3bnotifyoptiontab.h"

#include <KNotifyConfigWidget>

#include <QDebug>
#include <QHBoxLayout>



K3b::NotifyOptionTab::NotifyOptionTab( QWidget* parent )
    : QWidget( parent )
{
    m_notifyWidget = new KNotifyConfigWidget(this);
    m_notifyWidget->setApplication();
    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( m_notifyWidget );
}


K3b::NotifyOptionTab::~NotifyOptionTab()
{
}


void K3b::NotifyOptionTab::readSettings()
{
}


bool K3b::NotifyOptionTab::saveSettings()
{
    m_notifyWidget->save();
    return true;
}


