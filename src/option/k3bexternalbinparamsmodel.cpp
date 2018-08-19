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

#include "k3bexternalbinparamsmodel.h"
#include "k3bexternalbinmanager.h"

#include <KLocalizedString>

#include <QFont>
#include <QHash>
#include <QList>
#include <QRegExp>

namespace K3b {
    
class ExternalBinParamsModel::Private
{
public:
    Private( ExternalBinManager* m ) : manager( m ) {}
    
    ExternalBinManager* manager;
    QList<ExternalProgram*> programs;
    QHash<ExternalProgram*,QString> parameters;
};

ExternalBinParamsModel::ExternalBinParamsModel( ExternalBinManager* manager, QObject* parent )
    : QAbstractTableModel( parent ),
      d( new Private( manager ) )
{
}


ExternalBinParamsModel::~ExternalBinParamsModel()
{
    delete d;
}


void ExternalBinParamsModel::reload()
{
    beginResetModel();
    
    d->programs.clear();
    d->parameters.clear();
    
    // load programs
    const QMap<QString, K3b::ExternalProgram*>& map = d->manager->programs();
    for( QMap<QString, K3b::ExternalProgram*>::const_iterator it = map.begin(); it != map.end(); ++it ) {
        
        K3b::ExternalProgram* p = *it;
        // load user parameters
        if( p->supportsUserParameters() ) {
            d->programs.append( p );
            d->parameters.insert( p, p->userParameters().join( " " ) );
        }
    }
    
    endResetModel();
}


void ExternalBinParamsModel::save()
{
    QRegExp reSpace( "\\s" );
    Q_FOREACH( ExternalProgram* program, d->programs ) {
        QStringList params = d->parameters[ program ].split( reSpace, QString::SkipEmptyParts );
        program->setUserParameters( params );
    }
}


ExternalProgram* ExternalBinParamsModel::programForIndex( const QModelIndex& index ) const
{
    if( index.isValid() && index.row() >= 0 && index.row() < d->programs.size() )
        return d->programs.at( index.row() );
    else
        return 0;
}


QModelIndex ExternalBinParamsModel::indexForProgram( ExternalProgram* program, int column ) const
{
    int i = d->programs.indexOf( program );
    if( i >= 0 && i < d->programs.size() )
        return index( i, column );
    else
        return QModelIndex();
}


Qt::ItemFlags ExternalBinParamsModel::flags( const QModelIndex& index ) const
{
    if( index.isValid() && index.column() == ParametersColumn )
        return QAbstractTableModel::flags( index ) | Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags( index );
}


QVariant ExternalBinParamsModel::data( const QModelIndex& index, int role ) const
{
    if( ExternalProgram* program = programForIndex( index ) ) {
        if( Qt::DisplayRole == role || Qt::EditRole == role ) {
            switch( index.column() ) {
                case ProgramColumn: return program->name();
                case ParametersColumn: return d->parameters[ program ];
                default: break;
            }
        }
        else if( Qt::FontRole == role && index.column() == ProgramColumn ) {
            QFont font;
            font.setBold( true );
            return font;
        }
    }
    return QVariant();
}


bool ExternalBinParamsModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( ExternalProgram* program = programForIndex( index ) ) {
        if( Qt::EditRole == role && index.column() == ParametersColumn ) {
            QString params = value.toString();
            if( params != d->parameters[ program ] ) {
                d->parameters[ program ] = params;
                Q_EMIT dataChanged( index, index );
                return true;
            }
        }
    }
    return false;
}


int ExternalBinParamsModel::columnCount( const QModelIndex& /*parent*/ ) const
{
    return NumColumns;
}


int ExternalBinParamsModel::rowCount( const QModelIndex& parent ) const
{
    if( parent.isValid() )
        return 0;
    else
        return d->programs.size();
}


QVariant ExternalBinParamsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        switch( section ) {
            case ProgramColumn: return i18n( "Program" );
            case ParametersColumn: return i18n( "Parameters" );
            default: return QVariant();
        }
    }
    return QVariant();
}

QModelIndex ExternalBinParamsModel::buddy( const QModelIndex& index ) const
{
    if( index.isValid() && index.column() == ProgramColumn )
        return QAbstractTableModel::index( index.row(), ParametersColumn );
    else
        return index;
}

} // namespace K3b


