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

#include "k3b_export.h"

#include <KCModule>
#include <KPluginFactory>

class KConfigGroup;

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
        explicit PluginConfigWidget( QWidget* parent = 0, const QVariantList& args = QVariantList() );
        ~PluginConfigWidget() override;

        // TODO: find a nice way to get the plugin name for the config groups
#if 0
    public Q_SLOTS:
        /**
         * reimplemented from KCModule. Do not change.
         * implement loadConfig instead.
         */
        void defaults();

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
        /**
         * Load the config. Implement this instead of KCModule::load and
         * KCModule::defaults (the latter case will be handled by loading
         * from an invalid KConfigGroup object.
         *
         * \param config The config group to load the config From
         */
        virtual void loadConfig( const KConfigGroup& config );

        /**
         * Save the config
         */
        virtual void saveConfig( KConfigGroup config );
#endif
    };
}

#define K3B_EXPORT_PLUGIN_CONFIG_WIDGET( libname, classname ) K_PLUGIN_FACTORY(factory, registerPlugin<classname>();)

#endif
