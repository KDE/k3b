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

#include <qlist.h>
#include <qmap.h>
#include <qdir.h>



class K3bPluginManager::Private
{
public:
  QList<K3bPlugin*> plugins;
};




K3bPluginManager::K3bPluginManager( QObject* parent )
  : QObject( parent )
{
  d = new Private();
}


K3bPluginManager::~K3bPluginManager()
{
  delete d;
}



QStringList K3bPluginManager::groups() const
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


void K3bPluginManager::loadPlugin( const QString& fileName )
{
#ifdef __GNUC__
#warning Port plugin loading to KDE 4
#endif
//   KSimpleConfig c( fileName, true );
//   c.setGroup( "K3b Plugin" );

//   QString libName = c.readEntry( "Lib" );
//   if( libName.isEmpty() ) {
//     kDebug() << "(K3bPluginManager) no Lib specified in " << fileName;
//     return;
//   }

//   // read the lib
//   KLibFactory* factory = KLibLoader::self()->factory( libName.latin1() );
//   if( factory ) {
//     K3bPlugin* plugin = dynamic_cast<K3bPlugin*>( factory->create( this ) );
//     if( plugin ) {
//       // FIXME: improve this versioning stuff
//       if( plugin->pluginSystemVersion() != K3B_PLUGIN_SYSTEM_VERSION ) {
// 	delete plugin;
// 	kDebug() << "(K3bPluginManager) plugin system does not fit lib " << libName;
//       }
//       else {
// 	plugin->m_pluginInfo = K3bPluginInfo( libName,
// 					      c.readEntry( "Name" ),
// 					      c.readEntry( "Author" ),
// 					      c.readEntry( "Email" ),
// 					      c.readEntry( "Comment" ),
// 					      c.readEntry( "Version" ),
// 					      c.readEntry( "License" ) );

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
// 	}
// 	if( addPlugin )
// 	  d->plugins.append( plugin );
// 	else
// 	  delete plugin;
//       }
//     }
//     else
//       kDebug() << "(K3bPluginManager) lib " << libName << " not a K3b plugin";
//   }
//   else
//     kDebug() << "(K3bPluginManager) lib " << libName << " not found";
}


void K3bPluginManager::loadAll()
{
//   // we simply search the K3b plugin dir for now
//   QStringList dirs = KGlobal::dirs()->findDirs( "data", "k3b/plugins/" );

//   for( QStringList::const_iterator it = dirs.begin();
//        it != dirs.end(); ++it ) {
//     QStringList entries = QDir(*it).entryList( "*.plugin", QDir::Files );
//     for( QStringList::const_iterator it2 = entries.begin();
// 	 it2 != entries.end(); ++it2 ) {
//       loadPlugin( *it + *it2 );
//     }
//   }
}

int K3bPluginManager::pluginSystemVersion() const
{
  return K3B_PLUGIN_SYSTEM_VERSION;
}


int K3bPluginManager::execPluginDialog( K3bPlugin* plugin, QWidget* parent, const char* name )
{
    KDialog dlg( parent );
    dlg.setCaption( i18n("Configure plugin %1", plugin->pluginInfo().name() ) );

    K3bPluginConfigWidget* configWidget = plugin->createConfigWidget( &dlg );
    if( configWidget ) {
        dlg.setMainWidget( configWidget );
        connect( &dlg, SIGNAL(applyClicked()), configWidget, SLOT(saveConfig()) );
        connect( &dlg, SIGNAL(okClicked()), configWidget, SLOT(saveConfig()) );
        configWidget->loadConfig();
        int r = dlg.exec();
        delete configWidget;
        return r;
    }
    else {
        KMessageBox::sorry( parent, i18n("No settings available for plugin %1.", plugin->pluginInfo().name() ) );
        return 0;
    }
}

#include "k3bpluginmanager.moc"
