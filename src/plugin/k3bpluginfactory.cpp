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

#include <qwidget.h>
#include <qcstring.h>
#include <kdebug.h>



K3bPluginFactory::K3bPluginFactory( QObject* parent, const char* name )
  : KLibFactory( parent, name )
{
}


K3bPluginFactory::~K3bPluginFactory()
{
}


K3bPlugin* K3bPluginFactory::createPlugin( QObject* parent, 
					   const char* name,
					   const QStringList &args )
{
  K3bPlugin* plugin = createPluginObject( parent, name, args );
  if( plugin )
    emit objectCreated( plugin );
  return plugin;
}

  
K3bPluginConfigWidget* K3bPluginFactory::createConfigWidget( QWidget* parent, 
							     const char* name,
							     const QStringList &args )
{
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


QString K3bPluginFactory::author() const
{
  return QString::null;
}


QString K3bPluginFactory::version() const
{
  return QString::null;
}


QString K3bPluginFactory::comment() const
{
  return QString::null;
}

#include "k3bpluginfactory.moc"
