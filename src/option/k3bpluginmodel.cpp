/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bpluginmodel.h"

#include <k3bpluginmanager.h>
#include <k3bplugin.h>
#include <k3bcore.h>

#include <kcategorizedsortfilterproxymodel.h>


K3bPluginModel::K3bPluginModel( QObject* parent )
    : QAbstractItemModel( parent )
{
}


K3bPluginModel::~K3bPluginModel()
{
}


K3bPlugin* K3bPluginModel::pluginForIndex( const QModelIndex& index ) const
{
    return static_cast<K3bPlugin*>( index.internalPointer() );
}


QModelIndex K3bPluginModel::index( int row, int column, const QModelIndex& parent ) const
{
    QList<K3bPlugin*> plugins = k3bcore->pluginManager()->plugins();
    if ( row < plugins.count() ) {
        return createIndex( row, column, plugins[row] );
    }
    else {
        return QModelIndex();
    }
}


QModelIndex K3bPluginModel::parent( const QModelIndex& ) const
{
    return QModelIndex();
}


int K3bPluginModel::columnCount( const QModelIndex& ) const
{
    return 1;
}


int K3bPluginModel::rowCount( const QModelIndex& ) const
{
    return k3bcore->pluginManager()->plugins().count();
}


QVariant K3bPluginModel::data( const QModelIndex& index, int role ) const
{
    if ( K3bPlugin* plugin = pluginForIndex( index ) ) {
        KPluginInfo pluginInfo = plugin->pluginInfo();
        switch( role ) {
        case Qt::DisplayRole: {
            switch( index.column() ) {
            case 0:
                return "<p><b>" + pluginInfo.name() + "</b><br>" + pluginInfo.comment();
            case 1:
                return i18n( "<p>Version %1 released under the %1<br>by %2 <%3>", pluginInfo.license(), pluginInfo.author(), pluginInfo.email() );
            }
        }

        case KCategorizedSortFilterProxyModel::CategoryDisplayRole:
            return plugin->group();
        }
    }

    return QVariant();
}

#include "k3bpluginmodel.moc"
