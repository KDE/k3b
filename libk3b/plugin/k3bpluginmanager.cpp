/*
    SPDX-FileCopyrightText: 2009 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bpluginmanager.h"
#include "k3bplugin.h"
#include "k3bpluginconfigwidget.h"
#include "k3bversion.h"
#include "k3b_i18n.h"

#include <kcmutils_export.h>
#include <KCModuleLoader>
#include <KService>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <KMessageBox>

#include <QDebug>
#include <QDir>
#include <QList>
#include <QMap>
#include <QSharedPointer>
#include <QAbstractButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>



class K3b::PluginManager::Private
{
public:
    Private(){
    }

    QList<K3b::Plugin*> plugins;

    KCModule* getModule( Plugin* plugin ) const;
};




K3b::PluginManager::PluginManager( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
}


K3b::PluginManager::~PluginManager()
{
    delete d;
}



QStringList K3b::PluginManager::categories() const
{
    QStringList grps;

    QList<K3b::Plugin*> fl;
    Q_FOREACH( K3b::Plugin* plugin, d->plugins ) {
        if( !grps.contains( plugin->category() ) )
            grps.append( plugin->category() );
    }

    return grps;
}


QList<K3b::Plugin*> K3b::PluginManager::plugins( const QString& group ) const
{
    QList<K3b::Plugin*> fl;
    Q_FOREACH( K3b::Plugin* plugin, d->plugins ) {
        if( plugin->category() == group || group.isEmpty() )
            fl.append( plugin );
    }
    return fl;
}

KCModule *K3b::PluginManager::Private::getModule( Plugin* plugin ) const
{
    const QString kcm = plugin->pluginMetaData().value("X-KDE-ConfigModule");
    qDebug() << "for plugin" << plugin->pluginMetaData().pluginId() << "KCM" << kcm;
    if (!kcm.isEmpty()) {
        return KCModuleLoader::loadModule(plugin->pluginMetaData());
    }
    return 0;
}


void K3b::PluginManager::loadAll()
{
    const QVector<KPluginMetaData> metadataList = KPluginMetaData::findPlugins("k3b_plugins");
    for (const auto &metadata : metadataList) {
        KPluginFactory::Result<K3b::Plugin> result = KPluginFactory::instantiatePlugin<K3b::Plugin>(metadata);
        if (result) {
            K3b::Plugin *plugin = result.plugin;
            plugin->d->metadata = metadata;
            qDebug() << "Loaded plugin" << metadata.fileName();
            d->plugins.append(plugin);
        } else {
            qDebug() << "failed to load plugin" << metadata.fileName();
        }
    }
}

int K3b::PluginManager::pluginSystemVersion() const
{
    return K3B_PLUGIN_SYSTEM_VERSION;
}


bool K3b::PluginManager::hasPluginDialog( Plugin* plugin ) const
{
    QSharedPointer<KCModule> module( d->getModule( plugin ) );
    return !module.isNull();
}


int K3b::PluginManager::execPluginDialog( Plugin* plugin, QWidget* parent )
{
    if( auto module = d->getModule( plugin ) ) {
        QDialog dlg( parent );
        dlg.setWindowTitle( plugin->pluginMetaData().name() );
        QVBoxLayout* layout = new QVBoxLayout( &dlg );
        QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults, &dlg );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        layout->addWidget( module );
#else
        layout->addWidget( module->widget() );
#endif
        layout->addWidget( buttonBox );

        connect( buttonBox, &QDialogButtonBox::clicked, [&](QAbstractButton *button) {
            switch( buttonBox->standardButton( button ) )
            {
            case QDialogButtonBox::Ok: dlg.accept(); break;
            case QDialogButtonBox::Cancel: dlg.reject(); break;
            case QDialogButtonBox::RestoreDefaults: module->defaults(); break;
            default: break;
            }
        } );
        
        int ret = dlg.exec();
        if( ret == QDialog::Accepted )
        {
            module->save();
        }
        return ret;
    }
    else {
        KMessageBox::error( parent, i18n("No settings available for plugin %1.", plugin->pluginMetaData().name() ) );
        return 0;
    }
}

#include "moc_k3bpluginmanager.cpp"
