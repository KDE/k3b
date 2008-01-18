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


#ifndef _K3B_PLUGIN_H_
#define _K3B_PLUGIN_H_

#include <qobject.h>
#include <kgenericfactory.h>
#include "k3b_export.h"

#include <KPluginInfo>

#define K3B_PLUGIN_SYSTEM_VERSION 4


class K3bPluginConfigWidget;
class QWidget;


/**
 * Base class for all plugins.
 */
class LIBK3B_EXPORT K3bPlugin : public QObject
{
    Q_OBJECT

    friend class K3bPluginManager;

public:
    K3bPlugin( QObject* parent = 0 );
    virtual ~K3bPlugin();

    KPluginInfo pluginInfo() const { return m_pluginInfo; }

    /**
     * Version of the plugin system this plugin was written for.
     */
    virtual int pluginSystemVersion() const = 0;

    /**
     * The plugin group.
     */
// FIXME: rename to category and return m_pluginInfo.category(). Or do not use categories but different ServiceTypes
    virtual QString group() const = 0;

    /**
     * Returns a widget which configures the plugin.
     *
     * The caller has to destroy the widget
     */
    virtual K3bPluginConfigWidget* createConfigWidget( QWidget* parent = 0 ) const;

private:
    KPluginInfo m_pluginInfo;
};

#define K3B_EXPORT_PLUGIN( libname, classname )     \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory(#libname))

#endif
