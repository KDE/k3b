/*
 *
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bexternalbinmodel.h"
#include "k3bexternalbinmanager.h"

#include <KLocalizedString>

#include <QHash>
#include <QList>
#include <QFont>
#include <QPalette>
#include <QApplication>

namespace K3b {

class ExternalBinModel::Private
{
public:
    Private( ExternalBinManager* m ) : manager( m ) {}

    ExternalBinManager* manager;
    QList<ExternalProgram*> programs;
    QHash<ExternalProgram*,const ExternalBin*> defaults;
};

ExternalBinModel::ExternalBinModel( ExternalBinManager* manager, QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private( manager ) )
{
}


ExternalBinModel::~ExternalBinModel()
{
    delete d;
}


void ExternalBinModel::reload()
{
    beginResetModel();

    d->programs.clear();
    d->defaults.clear();

    // load programs
    const QMap<QString, K3b::ExternalProgram*>& map = d->manager->programs();
    for( QMap<QString, K3b::ExternalProgram*>::const_iterator it = map.begin(); it != map.end(); ++it ) {

        K3b::ExternalProgram* p = *it;
        d->programs.append( p );
        d->defaults.insert( p, p->defaultBin() );
    }

    endResetModel();
}


void ExternalBinModel::save()
{
    Q_FOREACH( ExternalProgram* program, d->programs ) {
        program->setDefault( d->defaults[ program ] );
    }
}


ExternalProgram* ExternalBinModel::programForIndex( const QModelIndex& index ) const
{
    if( index.isValid() && index.internalPointer() == 0 &&
        index.row() >= 0 && index.row() < d->programs.size() )
        return d->programs.at( index.row() );
    else
        return 0;
}


QModelIndex ExternalBinModel::indexForProgram( ExternalProgram* program, int column ) const
{
    int i = d->programs.indexOf( program );
    if( i >= 0 && i < d->programs.size() )
        return createIndex( i, column, nullptr );
    else
        return QModelIndex();
}


const ExternalBin* ExternalBinModel::binForIndex( const QModelIndex& index ) const
{
    if( index.isValid() && index.internalId() != 0 )
        return static_cast<const ExternalBin*>( index.internalPointer() );
    else
        return 0;
}


QModelIndex ExternalBinModel::indexForBin( const ExternalBin* bin, int column ) const
{
    if( bin ) {
        int i = bin->program().bins().indexOf( bin );
        if( i >= 0 && i < bin->program().bins().size() )
            return createIndex( i, column, const_cast<void*>( reinterpret_cast<const void*>( bin ) ) );
    }
    return QModelIndex();
}


QModelIndex ExternalBinModel::index( int row, int column, const QModelIndex& parent ) const
{
    if( !hasIndex( row, column, parent ) )
        return QModelIndex();
    else if( ExternalProgram* program = programForIndex( parent ) )
        return indexForBin( program->bins().at( row ), column );
    else
        return indexForProgram( d->programs.at( row ), column );
}


QModelIndex ExternalBinModel::parent( const QModelIndex& index ) const
{
    if( const ExternalBin* bin = binForIndex( index ) )
        return indexForProgram( &bin->program() );
    else
        return QModelIndex();
}


Qt::ItemFlags ExternalBinModel::flags( const QModelIndex& index ) const
{
    if( programForIndex( index ) != 0 )
    {
        return Qt::ItemIsEnabled;
    }
    else if( const ExternalBin* bin = binForIndex( index ) )
    {
        if( bin->program().bins().size() > 1 )
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
        else
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    else
    {
        return 0;
    }
}


QVariant ExternalBinModel::data( const QModelIndex& index, int role ) const
{
    if( ExternalProgram* program = programForIndex( index ) ) {
        if( Qt::DisplayRole == role && index.column() == PathColumn ) {
            if( program->bins().isEmpty() )
                return i18n( "%1 (not found)", program->name() );
            else
                return program->name();
        }
        else if( Qt::FontRole == role && index.column() == PathColumn ) {
            QFont font;
            font.setBold( true );
            return font;
        }
        else if( Qt::BackgroundRole == role ) {
            return QApplication::palette().alternateBase();
        }
    }
    else if( const ExternalBin* bin = binForIndex( index ) ) {
        if( Qt::DisplayRole == role ) {
            switch( index.column() ) {
                case PathColumn: return bin->path();
                case VersionColumn: return bin->version().toString();
                case FeaturesColumn: return bin->features().join( ", " );
                default: return QVariant();
            }
        }
        else if( Qt::CheckStateRole == role && index.column() == PathColumn ) {
            if( bin == d->defaults[ &bin->program() ] )
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
        else if( Qt::ToolTipRole == role && index.column() == FeaturesColumn ) {
            return bin->features().join( ", " );
        }
    }
    return QVariant();
}


bool ExternalBinModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( Qt::CheckStateRole == role && value.toBool() ) {
        if( const ExternalBin* bin = binForIndex( index ) ) {
            if( d->defaults[ &bin->program() ] != bin ) {
                d->defaults[ &bin->program() ] = bin;
                Q_EMIT dataChanged( indexForBin( bin->program().bins().first() ),
                                    indexForBin( bin->program().bins().last() ) );
                return true;
            }
        }
    }
    return false;
}


int ExternalBinModel::columnCount( const QModelIndex& /*parent*/ ) const
{
    return NumColumns;
}


int ExternalBinModel::rowCount( const QModelIndex& parent ) const
{
    if( ExternalProgram* program = programForIndex( parent ) )
        return program->bins().size();
    else if( !parent.isValid() )
        return d->programs.size();
    else
        return 0;
}


QVariant ExternalBinModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        switch( section ) {
            case PathColumn: return i18n( "Path" );
            case VersionColumn: return i18n( "Version" );
            case FeaturesColumn: return i18n( "Features" );
            default: return QVariant();
        }
    }
    return QVariant();
}


QModelIndex ExternalBinModel::buddy( const QModelIndex& index ) const
{
    if( binForIndex( index ) != 0 )
        return ExternalBinModel::index( index.row(), PathColumn, index.parent() );
    else
        return index;
}

} // namespace K3b


