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

#include "k3bpluginfactory.h"
#include "k3bplugin.h"
#include "k3bpluginconfigwidget.h"

#include <k3bcore.h>

#include <qwidget.h>
#include <qcstring.h>
#include <qptrlist.h>

#include <kdebug.h>
#include <kconfig.h>



class K3bPluginFactory::Private
{
public:
  Private()
    : fakeObject(0) {
  }

  QPtrList<K3bPlugin> pluginObjects;

  QObject* fakeObject;
};


K3bPluginFactory::K3bPluginFactory( QObject* parent, const char* name )
  : KLibFactory( parent, name )
{
  d = new Private();

  connect( this, SIGNAL(objectCreated(QObject*)),
	   this, SLOT(slotObjectCreated(QObject*)) );

  kdDebug() << "(K3bPluginFactory) creating K3bPluginFactory." << endl;
}


K3bPluginFactory::~K3bPluginFactory()
{
  kdDebug() << "(K3bPluginFactory) deleting K3bPluginFactory." << endl;
  delete d;
}


K3bPlugin* K3bPluginFactory::createPlugin( QObject* parent, 
					   const char* name,
					   const QStringList &args )
{
  // HACK: We don't want the LibLoader to unload the factory when no object
  // has been created so we just create a fake object
  if( !d->fakeObject ) {
    d->fakeObject = new QObject( this );
    emit objectCreated( d->fakeObject );
  }

  K3bPlugin* plugin = createPluginObject( parent, name, args );
  if( plugin )
    emit objectCreated( plugin );
  
  return plugin;
}

  
K3bPluginConfigWidget* K3bPluginFactory::createConfigWidget( QWidget* parent, 
							     const char* name,
							     const QStringList &args )
{
  // HACK: We don't want the LibLoader to unload the factory when no object
  // has been created so we just create a fake object
  if( !d->fakeObject ) {
    d->fakeObject = new QObject( this );
    emit objectCreated( d->fakeObject );
  }

  K3bPluginConfigWidget* w = createConfigWidgetObject( parent, name, args );
  if( w )
    emit objectCreated( w );

  return w;
}


K3bPluginConfigWidget* K3bPluginFactory::createConfigWidgetObject( QWidget*, 
								   const char*,
								   const QStringList& )
{
  return 0;
}


QObject* K3bPluginFactory::createObject( QObject* parent, 
					 const char* name, 
					 const char* classname, 
					 const QStringList &args )
{
  if( QCString(classname) == "Plugin" ) {
    return createPluginObject( parent, name, args );
  }
  else if( QCString(classname) == "ConfigWidget" ) {
    if( parent && !parent->inherits("QWidget") )
      kdError() << "(K3bPluginFactory) parent does not inherit QWidget!" << endl;
    else
      return createConfigWidgetObject( (QWidget*)parent, name, args );
  }

  return 0;
}


QString K3bPluginFactory::name() const
{
  if( m_name.isEmpty() )
    return className();
  else
    return m_name;
}


void K3bPluginFactory::slotObjectCreated( QObject* obj )
{
  K3bPlugin* plugin = dynamic_cast<K3bPlugin*>(obj);
  if( !plugin )
    return;

  if( d->pluginObjects.findRef( plugin ) )
    return;

  connect( plugin, SIGNAL(destroyed()),
	   this, SLOT(slotObjectDestroyed()) );

  d->pluginObjects.append( plugin );
}


void K3bPluginFactory::slotObjectDestroyed()
{
  const K3bPlugin* plugin = dynamic_cast<const K3bPlugin*>(sender());
  if( !plugin )
    return;

  d->pluginObjects.removeRef( plugin );
}


#include "k3bpluginfactory.moc"
