/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bpluginmanager.h"
#include "k3bpluginfactory.h"

#include <kdebug.h>
#include <ksimpleconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#include <qptrlist.h>
#include <qmap.h>
#include <qdir.h>



class K3bPluginManager::Private
{
public:
  QMap<K3bPluginFactory*, QString> factories;
};


K3bPluginManager::K3bPluginManager( QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private();
}


K3bPluginManager::~K3bPluginManager()
{
  delete d;
}



QPtrList<K3bPluginFactory> K3bPluginManager::factories( const QString& group ) const
{
  QPtrList<K3bPluginFactory> fl;
  for( QMapConstIterator<K3bPluginFactory*, QString> it = d->factories.begin();
       it != d->factories.end(); ++it ) {
    if( it.key()->group() == group || group.isEmpty() )
      fl.append( it.key() );
  }
  return fl;
}


void K3bPluginManager::loadPlugin( const QString& fileName )
{
  KSimpleConfig c( fileName, true );
  c.setGroup( "K3b Plugin" );

  // read the lib
  KLibFactory* factory = KLibLoader::self()->factory( c.readEntry( "lib" ) );
  if( factory ) {
    K3bPluginFactory* k3bFactory = dynamic_cast<K3bPluginFactory*>( factory );
    if( k3bFactory ) {
      // TODO: check if it provides info and if not try to load it from the config
      d->factories.insert( k3bFactory, c.readEntry( "lib" ) );
    }
    else
      kdDebug() << "(K3bPluginManager) lib " << c.readEntry( "lib" ) << " not a K3b plugin" << endl;
  }
  else
    kdDebug() << "(K3bPluginManager) lib " << c.readEntry( "lib" ) << " no found" << endl;
}


void K3bPluginManager::loadAll()
{
  // we simply search the K3b plugin dir for now
  QStringList dirs = KGlobal::instance()->dirs()->findDirs( "data", "k3b/plugins" );

  for( QStringList::const_iterator it = dirs.begin();
       it != dirs.end(); ++it ) {
    QStringList entries = QDir(*it).entryList( "*.plugin", QDir::Files );
    for( QStringList::const_iterator it2 = entries.begin();
	 it2 != entries.end(); ++it2 ) {
      loadPlugin( *it2 );
    }
  }
}



void K3bPluginManager::unloadPlugin( K3bPluginFactory* factory )
{
  // TODO: is it enough to just unload the lib or do we need to delete all objects?
  QString lib = d->factories[factory];
  d->factories.erase( factory );

  KLibLoader::self()->unloadLibrary( lib );
}

#include "k3bpluginmanager.moc"
