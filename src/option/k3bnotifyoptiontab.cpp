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


