/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef _K3B_PLUGIN_H_
#define _K3B_PLUGIN_H_

#include "k3b_export.h"
#include <KPluginFactory>
#include <KPluginMetaData>
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

        KPluginInfo pluginInfo() const { return KPluginInfo::fromMetaData(d->metadata); }
        KPluginMetaData pluginMetaData() const { return d->metadata; }

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
        struct Private {
            KPluginMetaData metadata;
        };
        std::unique_ptr<Private> d=std::unique_ptr<Private>(new Private());
    };
}

#define K3B_EXPORT_PLUGIN( libname, classname ) K_PLUGIN_FACTORY(factory, registerPlugin<classname>();)

#endif
