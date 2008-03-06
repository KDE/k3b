/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bpluginmanager.h"
#include "k3bplugin.h"
#include "k3bpluginconfigwidget.h"
#include <k3bversion.h>

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <klibloader.h>
#include <kdialog.h>

#include <KServiceTypeTrader>
#include <KService>
#include <KPluginInfo>

#include <qlist.h>
#include <qmap.h>
#include <qdir.h>



class K3bPluginManager::Private
{
public:
    Private( K3bPluginManager* parent )
        : m_parent( parent ) {
    }

    QList<K3bPlugin*> plugins;

    void loadPlugin( const KService::Ptr service );

private:
    K3bPluginManager* m_parent;
};




K3bPluginManager::K3bPluginManager( QObject* parent )
    : QObject( parent ),
      d( new Private( this ) )
{
}


K3bPluginManager::~K3bPluginManager()
{
    delete d;
}



QStringList K3bPluginManager::categories() const
{
    QStringList grps;

    QList<K3bPlugin*> fl;
    Q_FOREACH( K3bPlugin* plugin, d->plugins ) {
        if( !grps.contains( plugin->group() ) )
            grps.append( plugin->group() );
    }

    return grps;
}


QList<K3bPlugin*> K3bPluginManager::plugins( const QString& group ) const
{
    QList<K3bPlugin*> fl;
    Q_FOREACH( K3bPlugin* plugin, d->plugins ) {
        if( plugin->group() == group || group.isEmpty() )
            fl.append( plugin );
    }
    return fl;
}


void K3bPluginManager::Private::loadPlugin( const KService::Ptr service )
{
    kDebug() << service->name() << service->library();
    K3bPlugin* plugin = service->createInstance<K3bPlugin>( m_parent );
    if ( plugin ) {
        kDebug() << "Loaded plugin" << service->name();
        // FIXME: improve this versioning stuff
        if( plugin->pluginSystemVersion() != K3B_PLUGIN_SYSTEM_VERSION ) {
            delete plugin;
            kDebug() << "plugin system does not fit";
        }
        else {
            plugin->m_pluginInfo = KPluginInfo( service );
            plugins.append( plugin );
        }
    }


// 	// make sure to only use the latest version of one plugin
// 	bool addPlugin = true;
// 	for( Q3PtrListIterator<K3bPlugin> it( d->plugins ); *it; ++it ) {
// 	  if( it.current()->pluginInfo().name() == plugin->pluginInfo().name() ) {
// 	    if( K3bVersion(it.current()->pluginInfo().version()) < K3bVersion(plugin->pluginInfo().version()) ) {
// 	      K3bPlugin* p = it.current();
// 	      d->plugins.removeRef( p );
// 	      delete p;
// 	    }
// 	    else {
// 	      addPlugin = false;
// 	    }
// 	    break;
// 	  }
}


void K3bPluginManager::loadAll()
{
    kDebug();
    KService::List services = KServiceTypeTrader::self()->query( "K3b/Plugin" );
    Q_FOREACH( KService::Ptr service, services ) {
        d->loadPlugin( service );
    }
}

int K3bPluginManager::pluginSystemVersion() const
{
    return K3B_PLUGIN_SYSTEM_VERSION;
}


int K3bPluginManager::execPluginDialog( K3bPlugin* plugin, QWidget* parent, const char* name )
{
//     KDialog dlg( parent );
//     dlg.setCaption( i18n("Configure plugin %1", plugin->pluginInfo().name() ) );

//     K3bPluginConfigWidget* configWidget = plugin->createConfigWidget( &dlg );
//     if( configWidget ) {
//         dlg.setMainWidget( configWidget );
//         connect( &dlg, SIGNAL(applyClicked()), configWidget, SLOT(saveConfig()) );
//         connect( &dlg, SIGNAL(okClicked()), configWidget, SLOT(saveConfig()) );
//         configWidget->loadConfig();
//         int r = dlg.exec();
//         delete configWidget;
//         return r;
//     }
//     else
    {
         KMessageBox::sorry( parent, i18n("No settings available for plugin %1.", plugin->pluginInfo().name() ) );
         return 0;
    }
#warning FIXME: use KService::kcmServices to get the config widget for a plugin as done in KPluginSelector
}

#include "k3bpluginmanager.moc"
