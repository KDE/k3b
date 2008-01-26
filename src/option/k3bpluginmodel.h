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

#ifndef _K3B_PLUGIN_MODEL_H_
#define _K3B_PLUGIN_MODEL_H_

#include <QtCore/QAbstractItemModel>

class K3bPlugin;

class K3bPluginModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    K3bPluginModel( QObject* parent );
    ~K3bPluginModel();

    QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent( const QModelIndex& index = QModelIndex() ) const;
    int columnCount( const QModelIndex& parent = QModelIndex() ) const;
    int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;

    K3bPlugin* pluginForIndex( const QModelIndex& index ) const;

private:

};

#endif
