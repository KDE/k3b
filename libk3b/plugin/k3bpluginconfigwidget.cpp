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

#include "k3bpluginconfigwidget.h"

#include <KGlobal>

#include <k3bcore.h>


// we only use the plugins when loaded into the main application. Thus they do not need
// their own KComponentData
K3b::PluginConfigWidget::PluginConfigWidget( QWidget* parent, const QVariantList& args )
    : KCModule( KGlobal::mainComponent(), parent, args )
{
}


K3b::PluginConfigWidget::~PluginConfigWidget()
{
}


// void K3b::PluginConfigWidget::load()
// {

//     loadConfig();
// }


// void K3b::PluginConfigWidget::saveConfig()
// {

// }


#include "k3bpluginconfigwidget.moc"
