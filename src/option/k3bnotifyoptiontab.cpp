/*
 *
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


#include "k3bnotifyoptiontab.h"

#include <kdebug.h>
#include <KNotifyConfigWidget>

#include <qlayout.h>



K3bNotifyOptionTab::K3bNotifyOptionTab( QWidget* parent )
  : QWidget( parent )
{
  m_notifyWidget = new KNotifyConfigWidget(this);
  m_notifyWidget->setApplication();
}


K3bNotifyOptionTab::~K3bNotifyOptionTab()
{
}


void K3bNotifyOptionTab::readSettings()
{
}


bool K3bNotifyOptionTab::saveSettings()
{
  m_notifyWidget->save();
  return true;
}

#include "k3bnotifyoptiontab.moc"
