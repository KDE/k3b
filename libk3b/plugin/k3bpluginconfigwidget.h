/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_PLUGIN_CONFIG_WIDGET_H_
#define _K3B_PLUGIN_CONFIG_WIDGET_H_

#include <KCModule>
#include <KPluginFactory>
#include <KPluginLoader>

#include "k3b_export.h"

namespace K3b {
    /**
     * A config widget for a K3b plugin.
     *
     * Most implementation details for KCModules apply.
     *
     * Create a desktop file along the lines of:
     *
     * \code
     * [Desktop Entry]
     * Name=K3b Lame Mp3 Encoder Config Module
     * Type=Service
     * X-KDE-ServiceTypes=KCModule
     * X-KDE-Library=kcm_k3blameencoder
     * X-KDE-ParentComponents=k3blameencoder
     * \endcode
     *
     * X-KDE-ParentComponents is important as it is the only indicator
     * for the plugin system to match the config widget to the plugin.
     *
     * Then implement KCModule::load(), KCModule::save() and KCModule::defaults()
     * to handle the configuration. In these methods you may use k3bcore->config()
     * to store the configuration in the main K3b config object under a specific
     * group.
     */
    class LIBK3B_EXPORT PluginConfigWidget : public KCModule
    {
        Q_OBJECT

    public:
        PluginConfigWidget( QWidget* parent = 0, const QVariantList& args = QVariantList() );
        virtual ~PluginConfigWidget();

#if 0
    public Q_SLOTS:
        /**
         * reimplemented from KCModule. Do not change.
         * implement loadConfig instead.
         */
        void load();

        /**
         * reimplemented from KCModule. Do not change.
         * implement saveConfig instead.
         */
        void save();

    protected:
        virtual void loadConfig( const KConfigGroup& config ) = 0;
        virtual void saveConfig( KConfigGroup config ) = 0;
#endif
    };
}

#define K3B_EXPORT_PLUGIN_CONFIG_WIDGET( libname, classname )   \
    K_PLUGIN_FACTORY(factory, registerPlugin<classname>();)     \
        K_EXPORT_PLUGIN(factory(#libname))

#endif
