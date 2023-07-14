/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bpluginoptiontab.h"

#include "k3bpluginmanager.h"
#include "k3bplugin.h"
#include "k3bcore.h"

#include <KLocalizedString>
#include <KPluginWidget>

#include <QDebug>
#include <QHash>
#include <QList>
#include <QLabel>
#include <QVBoxLayout>




K3b::PluginOptionTab::PluginOptionTab( QWidget* parent )
    : QWidget( parent )
{
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );

    QLabel* label = new QLabel( i18n( "<p>Here all <em>K3b Plugins</em> may be configured. Be aware that this does not include the "
                                      "<em>KPart Plugins</em> which embed themselves in the K3b menu structure.</p>" ), this );
    label->setWordWrap( true );

    KPluginWidget * pluginSelector = new KPluginWidget( this );

    layout->addWidget( label );
    layout->addWidget( pluginSelector );

    // find all categories
    QHash<QString, QString> categoryNames;

    foreach( Plugin* plugin, k3bcore->pluginManager()->plugins() ) {
        categoryNames[ plugin->category() ] = plugin->categoryName();
    }

    // add all plugins in each category
    foreach( const QString &category, categoryNames.keys() ) {
        QVector<KPluginMetaData> plugins;

        foreach( Plugin* plugin, k3bcore->pluginManager()->plugins( category ) ) {
            plugins << plugin->pluginMetaData();
            qDebug() << "Adding plugin" << plugin->pluginMetaData().pluginId();
        }
        pluginSelector->addPlugins( plugins, categoryNames[ category ] );
    }
}


K3b::PluginOptionTab::~PluginOptionTab()
{
}

#include "moc_k3bpluginoptiontab.cpp"
