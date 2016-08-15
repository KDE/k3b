/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bpluginconfigwidget.h"
#include "k3bcore.h"

#include <KConfigCore/KConfigGroup>
#include <KConfigCore/KSharedConfig>


// we only use the plugins when loaded into the main application. Thus they do not need
// their own KComponentData
K3b::PluginConfigWidget::PluginConfigWidget( QWidget* parent, const QVariantList& args )
    : KCModule( parent, args )
{
}


K3b::PluginConfigWidget::~PluginConfigWidget()
{
}

#if 0
void K3b::PluginConfigWidget::load()
{
//    loadConfig( KSharedConfig::openConfig()->group() );
}


void K3b::PluginConfigWidget::defaults()
{
    KConfigGroup defaultGroup;
    loadConfig( defaultGroup );
}


void K3b::PluginConfigWidget::save()
{
//    saveConfig( KSharedConfig::openConfig()->group() );
}


void K3b::PluginConfigWidget::loadConfig( const KConfigGroup& )
{
}


void K3b::PluginConfigWidget::saveConfig( KConfigGroup )
{
}
#endif


