/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_PLUGIN_H_
#define _K3B_PLUGIN_H_

#include "k3b_export.h"
#include <KPluginFactory>
#include <KPluginInfo>
#include <QObject>

#define K3B_PLUGIN_SYSTEM_VERSION 5



namespace K3b {


    /**
     * Base class for all plugins.
     */
    class LIBK3B_EXPORT Plugin : public QObject
    {
        Q_OBJECT

        friend class PluginManager;

    public:
        explicit Plugin( QObject* parent = 0 );
        ~Plugin() override;

        KPluginInfo pluginInfo() const { return m_pluginInfo; }

        /**
         * Version of the plugin system this plugin was written for.
         */
        virtual int pluginSystemVersion() const = 0;

        /**
         * The plugin category.
         */
        virtual QString category() const = 0;

        virtual QString categoryName() const = 0;

    private:
        KPluginInfo m_pluginInfo;
    };
}

#define K3B_EXPORT_PLUGIN( libname, classname ) K_PLUGIN_FACTORY(factory, registerPlugin<classname>();)

#endif
