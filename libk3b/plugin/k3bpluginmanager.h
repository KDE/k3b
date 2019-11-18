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

#ifndef _K3B_PLUGIN_MANAGER_H_
#define _K3B_PLUGIN_MANAGER_H_

#include "k3b_export.h"

#include <QList>
#include <QObject>
#include <QStringList>

class QWidget;


namespace K3b {
    class Plugin;


    /**
     * Use this class to access all K3b plugins (this does not include the
     * KParts Plugins!).
     * Like the Core the single instance (which has to be created manually)
     * can be obtained with the k3bpluginmanager macro.
     */
    class LIBK3B_EXPORT PluginManager : public QObject
    {
        Q_OBJECT

    public:
        explicit PluginManager( QObject* parent = 0 );
        ~PluginManager() override;

        /**
         * if group is empty all plugins are returned
         */
        QList<Plugin*> plugins( const QString& category = QString() ) const;

        /**
         * Returns a list of the available categories.
         */
        QStringList categories() const;

        int pluginSystemVersion() const;
        
        bool hasPluginDialog( Plugin* plugin ) const;

    public Q_SLOTS:
        void loadAll();

        int execPluginDialog( Plugin* plugin, QWidget* parent = 0 );

    private:
        class Private;
        Private* d;
    };
}

#endif
