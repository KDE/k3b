/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bpluginmanager.h"
#include "k3bpluginfactory.h"
#include "k3bpluginconfigwidget.h"

#include <kdebug.h>
#include <ksimpleconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kdialogbase.h>
#include <kmessagebox.h>

#include <qptrlist.h>
#include <qmap.h>
#include <qdir.h>



class K3bPluginManager::Private
{
public:
  QMap<K3bPluginFactory*, QString> factories;
};


K3bPluginManager* K3bPluginManager::s_k3bPluginManager = 0;


K3bPluginManager::K3bPluginManager( QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private();

  if( s_k3bPluginManager ) {
    qFatal("ONLY ONE INSTANCE OF K3BPLUGINMANAGER ALLOWED!");
  }

  s_k3bPluginManager = this;
}


K3bPluginManager::~K3bPluginManager()
{
  delete d;
}



QStringList K3bPluginManager::groups() const
{
  QStringList grps;

  QPtrList<K3bPluginFactory> fl;
  for( QMapConstIterator<K3bPluginFactory*, QString> it = d->factories.begin();
       it != d->factories.end(); ++it ) {
    if( !grps.contains( it.key()->group() ) )
	grps.append( it.key()->group() );
  }

  return grps;
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

  QString libName = c.readEntry( "Lib" );
  if( libName.isEmpty() ) {
    kdDebug() << "(K3bPluginManager) no Lib specified in " << fileName << endl;
    return;
  }

  // read the lib
  KLibFactory* factory = KLibLoader::self()->factory( libName.latin1() );
  if( factory ) {
    K3bPluginFactory* k3bFactory = dynamic_cast<K3bPluginFactory*>( factory );
    if( k3bFactory ) {
      k3bFactory->setName( c.readEntry( "Name" ) );
      k3bFactory->setAuthor( c.readEntry( "Author" ) );
      k3bFactory->setEmail( c.readEntry( "Email" ) );
      k3bFactory->setVersion( c.readEntry( "Version" ) );
      k3bFactory->setComment( c.readEntry( "Comment" ) );
      k3bFactory->setLicense( c.readEntry( "License" ) );

      d->factories.insert( k3bFactory, libName );
    }
    else
      kdDebug() << "(K3bPluginManager) lib " << libName << " not a K3b plugin" << endl;
  }
  else
    kdDebug() << "(K3bPluginManager) lib " << libName << " not found" << endl;
}


void K3bPluginManager::loadAll()
{
  // we simply search the K3b plugin dir for now
  QStringList dirs = KGlobal::dirs()->findDirs( "data", "k3b/plugins/" );

  for( QStringList::const_iterator it = dirs.begin();
       it != dirs.end(); ++it ) {
    QStringList entries = QDir(*it).entryList( "*.plugin", QDir::Files );
    for( QStringList::const_iterator it2 = entries.begin();
	 it2 != entries.end(); ++it2 ) {
      loadPlugin( *it + *it2 );
    }
  }
}



void K3bPluginManager::unloadPlugin( K3bPluginFactory* factory )
{
  // TODO: is it enough to just unload the lib or do we need to delete all objects?
  QString lib = d->factories[factory];
  d->factories.erase( factory );

  KLibLoader::self()->unloadLibrary( lib.latin1() );
}


int K3bPluginManager::pluginSystemVersion() const
{
  return 1;
}


int K3bPluginManager::execPluginDialog( K3bPluginFactory* f, QWidget* parent, const char* name )
{
  KDialogBase dlg( parent, 
		   name, 
		   true,
		   i18n("Configure plugin %1").arg( f->name() ) );
  
  K3bPluginConfigWidget* configWidget = f->createConfigWidget( &dlg );
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
    KMessageBox::sorry( parent, i18n("No settings available for plugin %1.").arg( f->name() ) );
    return 0;
  }
}

#include "k3bpluginmanager.moc"
