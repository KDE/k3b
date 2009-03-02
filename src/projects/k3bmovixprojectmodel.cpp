/*
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Arthur Renato Mello <arthur@mandriva.com>
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

#include <k3bmovixdoc.h>
#include <k3bmovixfileitem.h>

#include <KLocale>
#include <KUrl>

#include <QtCore/QMimeData>
#include <QtCore/QDataStream>

using namespace K3b;

class MovixProjectModel::Private
{
    public:
        Private( MovixProjectModel* parent )
            : project( 0 ),
            q( parent ) { }

        K3b::MovixDoc* project;

        void _k_aboutToAddRows(int pos, int count, K3b::MovixFileItem* parent)
        {
            if(!parent)
                q->beginInsertRows(QModelIndex(), pos, pos + count - 1);
        }

        void _k_addedRows()
        {
            q->endInsertRows();
        }

        void _k_aboutToRemoveRows(int pos, int count, K3b::MovixFileItem* parent)
        {
            if(!parent)
                q->beginRemoveRows(QModelIndex(), pos, pos + count - 1);
        }

        void _k_removedRows()
        {
            q->endRemoveRows();
        }

    private:
        MovixProjectModel* q;
};

MovixProjectModel::MovixProjectModel( K3b::MovixDoc* doc, QObject* parent )
    : QAbstractItemModel( parent ),
    d( new Private(this) )
{
    d->project = doc;

    connect(doc, SIGNAL(aboutToAddMovixItem(int, int, K3b::MovixFileItem*)),
            this, SLOT(_k_aboutToAddRows(int, int, K3b::MovixFileItem*)));
    connect(doc, SIGNAL(addedMovixItem()), this, SLOT(_k_addedRows()));

    connect(doc, SIGNAL(aboutToRemoveMovixItem(int, int, K3b::MovixFileItem*)),
            this, SLOT(_k_aboutToRemoveRows(int, int, K3b::MovixFileItem*)));
    connect(doc, SIGNAL(removedMovixItem()), this, SLOT(_k_removedRows()));
}

MovixProjectModel::~MovixProjectModel()
{
    delete d;
}

K3b::MovixDoc* MovixProjectModel::project() const
{
    return d->project;
}

K3b::MovixFileItem* MovixProjectModel::itemForIndex( const QModelIndex& index ) const
{
    if ( index.isValid() )
    {
        Q_ASSERT( index.internalPointer() );
        return static_cast<K3b::MovixFileItem*>( index.internalPointer() );
    }

    return 0;
}

QModelIndex MovixProjectModel::indexForItem( K3b::MovixFileItem* track ) const
{
    return createIndex( d->project->indexOf(track), 0, track );
}

QModelIndex MovixProjectModel::index( int row, int column,
    const QModelIndex& parent ) const
{
    Q_UNUSED( parent );

    // just to make sure it won't crash when the model has no items
    if (row >= d->project->movixFileItems().count())
        return QModelIndex();

    return createIndex( row, column, d->project->movixFileItems().at(row) );
}

QModelIndex MovixProjectModel::parent( const QModelIndex& index ) const
{
    Q_UNUSED( index );

    return QModelIndex();
}

int MovixProjectModel::rowCount( const QModelIndex& parent) const
{
    if ( parent.isValid() )
        return 0;
    else
        return d->project->movixFileItems().size();
}

int MovixProjectModel::columnCount( const QModelIndex& parent) const
{
    Q_UNUSED( parent );

    return NumColumns;
}

bool MovixProjectModel::setData( const QModelIndex& index,
    const QVariant& value, int role )
{
    if ( index.isValid() )
    {
        K3b::MovixFileItem* item = itemForIndex( index );
        if ( role == Qt::EditRole )
        {
            if ( index.column() == TitleColumn )
            {
                item->setK3bName( value.toString() );
                return true;
            }
        }
    }

    return false;
}

QVariant MovixProjectModel::data( const QModelIndex& index, int role ) const
{
    if ( index.isValid() ) {
        K3b::MovixFileItem* item = itemForIndex( index );

        switch( index.column() ) {
            case NoColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return d->project->indexOf(item);
                }
                break;
            case TitleColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return item->k3bName();
                }
                break;
            case TypeColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    if( item->isSymLink() )
                        return i18n("Link to %1", item->mimeType()->comment());
                    else
                        return item->mimeType()->comment();
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
                        return item->linkDest() + i18n(" (broken)");
                }
                break;
        }
    }

    return QVariant();
}

QVariant MovixProjectModel::headerData( int section,
    Qt::Orientation orientation, int role ) const
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
                          Qt::ItemIsDropEnabled |
                          Qt::ItemIsDragEnabled;

        if ( index.column() == TitleColumn )
        {
            f |= Qt::ItemIsEditable;
        }

        return f;
    }
    else
    {
        return QAbstractItemModel::flags( index )|Qt::ItemIsDropEnabled;
    }
}

Qt::DropActions MovixProjectModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QMimeData* MovixProjectModel::mimeData( const QModelIndexList& indexes ) const
{
    QMimeData* mime = new QMimeData();

    QList<K3b::MovixFileItem*> items;
    KUrl::List urls;

    foreach( const QModelIndex& index, indexes ) {
        K3b::MovixFileItem* item = itemForIndex( index );
        items << item;

        if( !urls.contains( KUrl( item->localPath() ) ) )
        {
            urls << KUrl( item->localPath() );
        }
    }
    urls.populateMimeData( mime );

    // the easy road: encode the pointers
    QByteArray trackData;
    QDataStream trackDataStream( &trackData, QIODevice::WriteOnly );

    foreach( K3b::MovixFileItem* item, items ) {
        trackDataStream << ( qint64 )item;
    }

    mime->setData( "application/x-k3bmovixfileitem", trackData );

    return mime;
}

QStringList MovixProjectModel::mimeTypes() const
{
    QStringList s = KUrl::List::mimeDataTypes();
    s += QString::fromLatin1( "application/x-k3bmovixfileitem" );

    return s;
}

bool MovixProjectModel::dropMimeData( const QMimeData* data,
    Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    Q_UNUSED( column );

    if (action == Qt::IgnoreAction)
        return true;

    QList<K3b::MovixFileItem*> items;
    if ( data->hasFormat( "application/x-k3bmovixfileitem" ) )
    {
        QByteArray itemData = data->data( "application/x-k3bmovixfileitem" );
        QDataStream itemDataStream( itemData );
        while ( !itemDataStream.atEnd() )
        {
            qint64 p;
            itemDataStream >> p;
            items << ( K3b::MovixFileItem* )p;
        }

        K3b::MovixFileItem *prev;
        if(parent.isValid())
        {
            int index = d->project->indexOf(itemForIndex(parent)) - 1;
            if(index == 0)
                prev = 0;
            else
                prev = d->project->movixFileItems().at(index - 1);
        }
        else if(row >= 0)
            prev = d->project->movixFileItems().at(row - 1);
        else
            prev = d->project->movixFileItems().last();

        foreach( K3b::MovixFileItem* item, items )
        {
            d->project->moveMovixItem(item, prev);
            prev = item;
        }

        return true;
    }

    if ( KUrl::List::canDecode( data ) )
    {
        int pos;
        if(parent.isValid())
            pos = d->project->indexOf(itemForIndex(parent)) - 1;
        else if(row >= 0)
            pos = row;
        else
            pos = d->project->movixFileItems().size();

        KUrl::List urls = KUrl::List::fromMimeData( data );

        d->project->addUrlsAt( urls, pos);

        return true;
    }

    return false;
}

bool MovixProjectModel::removeRows( int row, int count, const QModelIndex& parent )
{
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

#include "k3bmovixprojectmodel.moc"
