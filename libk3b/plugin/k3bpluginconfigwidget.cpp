/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "k3bpluginconfigwidget.h"
#include "k3bcore.h"

#include <KConfigGroup>
#include <KSharedConfig>


// we only use the plugins when loaded into the main application. Thus they do not need
// their own KComponentData
K3b::PluginConfigWidget::PluginConfigWidget(QObject *parent, const KPluginMetaData& metaData, const QVariantList& args )
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    : KCModule( qobject_cast<QWidget *>(parent), args )
{
    Q_UNUSED(metaData);
}
#else
    : KCModule( parent, metaData )
{
    Q_UNUSED(args);
}
#endif

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

#include "moc_k3bpluginconfigwidget.cpp"
