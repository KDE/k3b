/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bpluginconfigwidget.h"
#include "k3bcore.h"

#include <KConfigGroup>
#include <KSharedConfig>


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


