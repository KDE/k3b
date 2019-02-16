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

#include "k3bthememodel.h"
#include "k3bthememanager.h"

#include <KLocalizedString>
#include <KIO/DeleteJob>

#include <QFile>
#include <QUrl>

namespace K3b {

ThemeModel::ThemeModel( ThemeManager* themeManager, QObject* parent )
    : QAbstractTableModel( parent ),
      m_themeManager( themeManager )
{
}


ThemeModel::~ThemeModel()
{
}


void ThemeModel::reload()
{
    beginResetModel();
    m_themeManager->loadThemes();
    endResetModel();
}


Theme* ThemeModel::themeForIndex( const QModelIndex& index ) const
{
    if( index.isValid() && index.row() >= 0 && index.row() < m_themeManager->themes().size() )
        return m_themeManager->themes().at( index.row() );
    else
        return 0;
}


QModelIndex ThemeModel::indexForTheme( Theme* theme, int column ) const
{
    int i = m_themeManager->themes().indexOf( theme );
    if( i >= 0 && i < m_themeManager->themes().size() )
        return index( i, column );
    else
        return QModelIndex();
}


QVariant ThemeModel::data( const QModelIndex& index, int role ) const
{
    if( Theme* theme = themeForIndex( index ) ) {
        if( Qt::DisplayRole == role ) {
            switch( index.column() ) {
                case ThemeColumn: return theme->name();
                case AuthorColumn: return theme->author();
                case VersionColumn: return theme->version();
                case CommentColumn: return theme->comment();
                default: break;
            }
        }
    }
    return QVariant();
}


int ThemeModel::columnCount( const QModelIndex& /*parent*/ ) const
{
    return NumColumns;
}


int ThemeModel::rowCount( const QModelIndex& parent ) const
{
    if( parent.isValid() )
        return 0;
    else
        return m_themeManager->themes().size();
}


QVariant ThemeModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        switch( section ) {
            case ThemeColumn: return i18n( "Theme" );
            case AuthorColumn: return i18n( "Author" );
            case VersionColumn: return i18n( "Version" );
            case CommentColumn: return i18n( "Comment" );
            default: return QVariant();
        }
    }
    return QVariant();
}


bool ThemeModel::removeRows( int row, int count, const QModelIndex& parent )
{
    if( !parent.isValid() ) {
        beginRemoveRows( parent, row, row+count-1 );
        for( int i = 0; i < count; ++i ) {
            if( row >= 0 && row < m_themeManager->themes().size() ) {
                QScopedPointer<Theme> theme(m_themeManager->themes().takeAt(row));
                QString path = theme->path();

                // delete k3b.theme file to avoid it to get loaded
                QFile::remove( path + "/k3b.theme" );

                // delete the theme data itself
                KIO::del( QUrl::fromLocalFile( path ), KIO::HideProgressInfo );
            }
        }
        endRemoveRows();
        return true;
    }
    else {
        return false;
    }
}

} // namespace K3b


