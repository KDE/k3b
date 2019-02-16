/*
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Arthur Renato Mello <arthur@mandriva.com>
 * Copyright (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bmovixprojectmodel.h"
#include "k3bmovixdoc.h"
#include "k3bmovixfileitem.h"

#include <KUrlMimeData>
#include <KLocalizedString>

#include <QUrl>
#include <QMimeData>
#include <QDataStream>
#include <QIcon>

namespace K3b {

class MovixProjectModel::Private
{
    public:
        Private( MovixProjectModel* parent, MovixDoc* doc )
            : project( doc ),
            q( parent ) { }

        MovixDoc* project;

        void _k_itemsAboutToBeInserted( int pos, int count )
        {
            q->beginInsertRows( QModelIndex(), pos, pos + count - 1 );
        }

        void _k_itemsInserted()
        {
            q->endInsertRows();
        }

        void _k_itemsAboutToBeRemoved( int pos, int count )
        {
            q->beginRemoveRows( QModelIndex(), pos, pos + count - 1 );
        }

        void _k_itemsRemoved()
        {
            q->endRemoveRows();
        }

        void _k_subTitleAboutToBeInserted( K3b::MovixFileItem* item )
        {
            q->beginInsertRows( q->indexForItem( item ), 0, 0 );
        }

        void _k_subTitleInserted()
        {
            q->endInsertRows();
        }

        void _k_subTitleAboutToBeRemoved( K3b::MovixFileItem* item )
        {
            q->beginRemoveRows( q->indexForItem( item ), 0, 0 );
        }

        void _k_subTitleRemoved()
        {
            q->endRemoveRows();
        }

    private:
        MovixProjectModel* q;
};


MovixProjectModel::MovixProjectModel( MovixDoc* doc, QObject* parent )
:
    QAbstractItemModel( parent ),
    d( new Private( this, doc ) )
{
    // item handling
    connect( doc, SIGNAL(itemsAboutToBeInserted(int,int)),
             this, SLOT(_k_itemsAboutToBeInserted(int,int)), Qt::DirectConnection );
    connect( doc, SIGNAL(itemsInserted()),
             this, SLOT(_k_itemsInserted()), Qt::DirectConnection );
    connect( doc, SIGNAL(itemsAboutToBeRemoved(int,int)),
             this, SLOT(_k_itemsAboutToBeRemoved(int,int)), Qt::DirectConnection );
    connect( doc, SIGNAL(itemsRemoved()),
             this, SLOT(_k_itemsRemoved()), Qt::DirectConnection );

    // subtitle handling
    connect( doc, SIGNAL(subTitleAboutToBeInserted(K3b::MovixFileItem*)),
             this, SLOT(_k_subTitleAboutToBeInserted(K3b::MovixFileItem*)), Qt::DirectConnection );
    connect( doc, SIGNAL(subTitleInserted()),
             this, SLOT(_k_subTitleInserted()), Qt::DirectConnection );
    connect( doc, SIGNAL(subTitleAboutToBeRemoved(K3b::MovixFileItem*)),
             this, SLOT(_k_subTitleAboutToBeRemoved(K3b::MovixFileItem*)), Qt::DirectConnection );
    connect( doc, SIGNAL(subTitleRemoved()),
             this, SLOT(_k_subTitleRemoved()), Qt::DirectConnection );
}


MovixProjectModel::~MovixProjectModel()
{
    delete d;
}


MovixDoc* MovixProjectModel::project() const
{
    return d->project;
}


MovixFileItem* MovixProjectModel::itemForIndex( const QModelIndex& index ) const
{
    if( index.isValid() && index.internalPointer() != 0 ) {
        MovixFileItem* item = static_cast<MovixFileItem*>( index.internalPointer() );
        if( !item->isSubtitle() )
            return item;
    }
    return 0;
}


QModelIndex MovixProjectModel::indexForItem( MovixFileItem* item ) const
{
    if( item && !item->isSubtitle() ) {
        int row = d->project->indexOf( item );
        if( row >= 0 && item != 0 )
            return createIndex( row, NoColumn, item );
    }
    return QModelIndex();
}


MovixSubtitleItem* MovixProjectModel::subtitleForIndex( const QModelIndex& index ) const
{
    if( index.isValid() && index.internalPointer() != 0 ) {
        MovixFileItem* item = static_cast<MovixFileItem*>( index.internalPointer() );
        if( item->isSubtitle() ) {
            return dynamic_cast<MovixSubtitleItem*>( item );
        }
    }
    return 0;
}


QModelIndex MovixProjectModel::indexForSubtitle( MovixSubtitleItem* sub ) const
{
    if( sub )
        return createIndex( 0, NoColumn, sub );
    else
        return QModelIndex();
}


QModelIndex MovixProjectModel::index( int row, int column, const QModelIndex& parent ) const
{
    if( !hasIndex( row, column, parent ) ) {
        return QModelIndex();
    }
    else if( !parent.isValid() ) {
        // just to make sure it won't crash when the model has no items
        if( row >= 0 && row < d->project->movixFileItems().count() )
            return createIndex( row, column, d->project->movixFileItems().at(row) );
        else
            return QModelIndex();
    }
    else {
        // if the parent is valid, we are returning a subtitle item
        MovixFileItem* item = itemForIndex( parent );
        if( row == 0 && parent.column() == NoColumn && item != 0 && item->subTitleItem() != 0 )
            return createIndex( row, column, item->subTitleItem() );
        else
            return QModelIndex();
    }
}


QModelIndex MovixProjectModel::parent( const QModelIndex& index ) const
{
    if( MovixSubtitleItem* sub = subtitleForIndex( index ) ) {
        // if it really is a subtitle, the parent is the movix item
        MovixFileItem* item = sub->parent();
        const int row = d->project->indexOf( item );
        if( item != 0 && row >= 0 )
            return createIndex( row, NoColumn, item );
    }
    return QModelIndex();
}


int MovixProjectModel::rowCount( const QModelIndex& parent ) const
{
    if( parent.isValid() ) {
        MovixFileItem* item = itemForIndex(parent);
        if( item && item->subTitleItem() != 0 && parent.column() == NoColumn )
            return 1;
        else
            return 0;
    }
    else
        return d->project->movixFileItems().count();
}


int MovixProjectModel::columnCount( const QModelIndex& /*parent*/ ) const
{
    return NumColumns;
}


bool MovixProjectModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( MovixFileItem* item = itemForIndex( index ) ) {
        if( role == Qt::EditRole && index.column() == TitleColumn ) {
            if( item->k3bName() != value.toString() ) {
                item->setK3bName( value.toString() );
                Q_EMIT dataChanged( index, index );
                return true;
            }
        }
    }

    return false;
}


QVariant MovixProjectModel::data( const QModelIndex& index, int role ) const
{
    if ( index.isValid() ) {
        FileItem* item = static_cast<FileItem*>(index.internalPointer());

        switch( index.column() ) {
            case NoColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    if( MovixFileItem* movixItem = itemForIndex(index) )
                        return d->project->indexOf( movixItem ) + 1;
                    else
                        return QVariant();
                }
                break;
            case TitleColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return item->k3bName();
                }
                else if ( role == Qt::DecorationRole )
                {
                    return QIcon::fromTheme( item->mimeType().iconName() );
                }
                break;
            case TypeColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    if( item->isSymLink() )
                        return i18n("Link to %1", item->mimeType().comment());
                    else
                        return item->mimeType().comment();
                }
                break;
            case SizeColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return KIO::convertSize( item->size() );
                }
                break;
            case LocalPathColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return item->localPath();
                }
                break;
            case LinkColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    if(item->isValid())
                        return item->linkDest();
                    else
                        return i18n( "%1 (broken)", item->linkDest() );
                }
                break;
        }
    }

    return QVariant();
}


QVariant MovixProjectModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    if ( role == Qt::DisplayRole ) {
        switch( section ) {
            case NoColumn:
                return i18nc( "Movix File Position", "No." );
            case TitleColumn:
                return i18nc( "Movix File Title", "Title" );
            case TypeColumn:
                return i18nc( "Movix File Type(ie. MPEG)", "Type" );
            case SizeColumn:
                return i18nc( "Movix File Size", "Size" );
            case LocalPathColumn:
                return i18nc( "Movix File Path", "Local Path" );
            case LinkColumn:
                return i18nc( "Movix File Link", "Link" );
        }
    }

    return QVariant();
}


Qt::ItemFlags MovixProjectModel::flags( const QModelIndex& index ) const
{
    if ( index.isValid() )
    {
        Qt::ItemFlags f = Qt::ItemIsSelectable |
                          Qt::ItemIsEnabled |
                          Qt::ItemIsDropEnabled;

        MovixFileItem* item = itemForIndex( index );

        if ( item ) {
            f |= Qt::ItemIsDragEnabled;

            if ( index.column() == TitleColumn ) {
                f |= Qt::ItemIsEditable;
            }
        }

        return f;
    }
    else {
        return QAbstractItemModel::flags( index )|Qt::ItemIsDropEnabled;
    }
}


Qt::DropActions MovixProjectModel::supportedDropActions() const
{
    return Qt::CopyAction;
}


QMimeData* MovixProjectModel::mimeData( const QModelIndexList& indexes ) const
{
    QMimeData* mime = new QMimeData();

    QList<MovixFileItem*> items;
    QList<QUrl> urls;

    Q_FOREACH( const QModelIndex& index, indexes ) {
        MovixFileItem* item = itemForIndex( index );
        if (item) {
            items << item;

            if( !urls.contains( QUrl::fromLocalFile( item->localPath() ) ) ) {
                urls << QUrl::fromLocalFile( item->localPath() );
            }
        }
    }
    mime->setUrls(urls);

    // the easy road: encode the pointers
    QByteArray trackData;
    QDataStream trackDataStream( &trackData, QIODevice::WriteOnly );

    Q_FOREACH( MovixFileItem* item, items ) {
        trackDataStream << ( qint64 )item;
    }

    mime->setData( "application/x-k3bmovixfileitem", trackData );

    return mime;
}

QStringList MovixProjectModel::mimeTypes() const
{
    QStringList s = KUrlMimeData::mimeDataTypes();
    s += QString::fromLatin1( "application/x-k3bmovixfileitem" );

    return s;
}


bool MovixProjectModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    Q_UNUSED( column );

    if (action == Qt::IgnoreAction)
        return true;

    QList<MovixFileItem*> items;
    if ( data->hasFormat( "application/x-k3bmovixfileitem" ) )
    {
        QByteArray itemData = data->data( "application/x-k3bmovixfileitem" );
        QDataStream itemDataStream( itemData );
        while ( !itemDataStream.atEnd() )
        {
            qint64 p;
            itemDataStream >> p;
            items << ( MovixFileItem* )p;
        }

        MovixFileItem *prev;
        if(parent.isValid())
        {
            MovixFileItem *item = itemForIndex(parent);

            // TODO: handle drop in subtitles
            if (!item)
                return false;

            int index = d->project->indexOf(itemForIndex(parent));
            if(index == 0)
                prev = 0;
            else
                prev = d->project->movixFileItems().at(index - 1);
        }
        else if(row >= 0)
            prev = d->project->movixFileItems().at(row - 1);
        else
            prev = d->project->movixFileItems().last();

        Q_FOREACH( MovixFileItem* item, items )
        {
            d->project->moveMovixItem(item, prev);
            prev = item;
        }

        return true;
    }

    if ( data->hasUrls() )
    {
        int pos;
        if(parent.isValid())
            pos = d->project->indexOf(itemForIndex(parent));
        else if(row >= 0)
            pos = row;
        else
            pos = d->project->movixFileItems().size();

        QList<QUrl> urls = KUrlMimeData::urlsFromMimeData( data );

        QMetaObject::invokeMethod( d->project, "addUrlsAt", Qt::QueuedConnection, Q_ARG( QList<QUrl>, urls ), Q_ARG( int, pos ) );

        return true;
    }

    return false;
}


bool MovixProjectModel::removeRows( int row, int count, const QModelIndex& parent )
{
    // if the parent item is valid, we are removing a subtitle
    if (parent.isValid()) {
        MovixFileItem *item = itemForIndex(parent);
        d->project->removeSubTitleItem( item );
        return true;
    }

    // remove the indexes from the project
    while (count > 0)
    {
        QModelIndex i = index( row, 0, parent );
        d->project->removeMovixItem( itemForIndex(i) );

        row++;
        count--;
    }

    return true;
}

} // namespace K3b

#include "moc_k3bmovixprojectmodel.cpp"
