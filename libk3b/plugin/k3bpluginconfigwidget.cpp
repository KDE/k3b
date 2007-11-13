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

#include "k3bpluginconfigwidget.h"


K3bPluginConfigWidget::K3bPluginConfigWidget( QWidget* parent )
  : QWidget( parent )
{
}


K3bPluginConfigWidget::~K3bPluginConfigWidget()
{
}


void K3bPluginConfigWidget::loadConfig()
{
  // do nothing
}


void K3bPluginConfigWidget::saveConfig()
{
  // do nothing
}


#include "k3bpluginconfigwidget.moc"
