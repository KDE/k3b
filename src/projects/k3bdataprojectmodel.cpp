/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

#include "k3bdataprojectmodel.h"
#include "k3bdataurladdingdialog.h"
#include "k3bmodeltypes.h"

#include "k3bdatadoc.h"
#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include "k3bspecialdataitem.h"

#include <kmimetype.h>
#include <KIcon>
#include <KLocale>
#include <KUrl>

#include <QtCore/QMimeData>
#include <QtCore/QDataStream>


class K3b::DataProjectModel::Private
{
public:
    Private( DataProjectModel* parent )
        : project( 0 ),
          q( parent ) {
    }

    K3b::DataDoc* project;

    int countDirs( K3b::DirItem* dir );
    K3b::DataItem* getChild( K3b::DirItem* dir, int offset );
    int findChildIndex( K3b::DataItem* item );
    void _k_aboutToAddItem( K3b::DirItem* dir, K3b::DataItem* item );
    void _k_aboutToRemoveItem( K3b::DataItem* item );
    void _k_itemAdded( K3b::DataItem* item );
    void _k_itemRemoved( K3b::DataItem* item );

private:
    DataProjectModel* q;

    bool m_removingItem;
};



int K3b::DataProjectModel::Private::countDirs( K3b::DirItem* dir )
{
    int cnt = 0;
    QList<K3b::DataItem*> children = dir->children();
    for ( int i = 0; i < children.count(); ++i ) {
        if ( children[i]->isDir() ) {
            ++cnt;
        }
    }
    return cnt;
}


K3b::DataItem* K3b::DataProjectModel::Private::getChild( K3b::DirItem* dir, int offset )
{
    QList<K3b::DataItem*> children = dir->children();
    if ( offset >= 0 && offset < children.count() ) {
        return children[offset];
    }

    return 0;
}


int K3b::DataProjectModel::Private::findChildIndex( K3b::DataItem* item )
{
    K3b::DirItem* dir = item->parent();
    if ( dir ) {
        QList<K3b::DataItem*> cl = dir->children();
        for ( int i = 0; i < cl.count(); ++i ) {
            if ( cl[i] == item ) {
                return i;
            }
        }
        Q_ASSERT( 0 );
    }

    return 0;
}


void K3b::DataProjectModel::Private::_k_aboutToAddItem( K3b::DirItem* dir, K3b::DataItem* )
{
    int row = dir->children().count();
    q->beginInsertRows( q->indexForItem( dir ), row, row );
}


void K3b::DataProjectModel::Private::_k_aboutToRemoveItem( K3b::DataItem* item )
{
    m_removingItem = true;
    int row = findChildIndex( item );
    kDebug() << item << q->indexForItem( item->parent() ) << row;
    q->beginRemoveRows( q->indexForItem( item->parent() ), row, row );
}


void K3b::DataProjectModel::Private::_k_itemAdded( K3b::DataItem* item )
{
    Q_UNUSED( item );
    q->endInsertRows();
}


void K3b::DataProjectModel::Private::_k_itemRemoved( K3b::DataItem* item )
{
    kDebug() << item;
    Q_UNUSED( item );
    if ( m_removingItem ) {
        q->endRemoveRows();
    }
}


K3b::DataProjectModel::DataProjectModel( K3b::DataDoc* doc, QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private(this) )
{
    d->project = doc;

    connect( doc, SIGNAL( aboutToAddItem(K3b::DirItem*, K3b::DataItem*) ), this, SLOT( _k_aboutToAddItem(K3b::DirItem*, K3b::DataItem*) ) );
    connect( doc, SIGNAL( aboutToRemoveItem(K3b::DataItem*) ), this, SLOT( _k_aboutToRemoveItem(K3b::DataItem*) ) );
    connect( doc, SIGNAL( itemAdded(K3b::DataItem*) ), this, SLOT( _k_itemAdded(K3b::DataItem*) ) );
    connect( doc, SIGNAL( itemRemoved(K3b::DataItem*) ), this, SLOT( _k_itemRemoved(K3b::DataItem*) ) );
}


K3b::DataProjectModel::~DataProjectModel()
{
    delete d;
}


K3b::DataDoc* K3b::DataProjectModel::project() const
{
    return d->project;
}

K3b::DataItem* K3b::DataProjectModel::itemForIndex( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Q_ASSERT( index.internalPointer() );
        return static_cast<K3b::DataItem*>( index.internalPointer() );
    }
    else {
        return 0;
    }
}

QModelIndex K3b::DataProjectModel::indexForItem( K3b::DataItem* item ) const
{
    return createIndex( d->findChildIndex( item ), 0, item );
}


int K3b::DataProjectModel::columnCount( const QModelIndex& index ) const
{
    // the root item has only the first column
    if (!index.isValid())
        return 1;

    return NumColumns;
}


QVariant K3b::DataProjectModel::data( const QModelIndex& index, int role ) const
{
    if ( index.isValid() ) {
        K3b::DataItem* item = itemForIndex( index );

        switch( index.column() ) {
        case FilenameColumn:
            if( role == Qt::DisplayRole ||
                role == Qt::EditRole ) {
                return item->k3bName();
            }
            else if ( role == Qt::DecorationRole ) {
                if ( item->isDir() && item->parent() ) {
                    return( static_cast<K3b::DirItem*>( item )->depth() > 7 ? KIcon( "folder-root" ) : KIcon( "folder" ) );
                }
                else if ( item->isDir() ) {
                    return KIcon( "media-optical" );
                }
                else {
                    return KIcon( item->mimeType()->iconName() );
                }
            }
            else if ( role == K3b::ItemTypeRole ) {
                if (item->isDir())
                    return (int) K3b::DirItemType;
                else
                    return (int) K3b::FileItemType;
            }
            else if ( role == K3b::CustomFlagsRole ) {
                if (item->isRemoveable())
                    return (int) K3b::ItemIsRemovable;
                else
                    return 0;
            }
            break;

        case TypeColumn:
            if( role == Qt::DisplayRole ) {
                if ( item->isSpecialFile() ) {
                    return static_cast<K3b::SpecialDataItem*>( item )->specialType();
                }
                else {
                    return item->mimeType()->comment();
                }
            }
            break;

        case SizeColumn:
            if( role == Qt::DisplayRole ) {
                return KIO::convertSize( item->size() );
            }
            break;

        case LocalPathColumn:
            if( role == Qt::DisplayRole ) {
                return item->localPath();
            }
            break;

        case LinkColumn:
            if( role == Qt::DisplayRole ) {
                if( item->isSymLink() ) {
                    QString s;
                    if ( item->doc()->isoOptions().followSymbolicLinks() ) {
                        s = K3b::resolveLink( item->localPath() );
                    }
                    else {
                        s = QFileInfo( item->localPath() ).readLink();
                    }
                    if( !item->isValid() ) {
                        s += " (" + i18n("outside of project") + ")";
                    }
                    return s;
                }
            }
            break;
        }
    }

    return QVariant();
}


QVariant K3b::DataProjectModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    if ( role == Qt::DisplayRole ) {
        switch( section ) {
        case FilenameColumn:
            return i18nc( "file name", "Name" );
        case TypeColumn:
            return i18nc( "file type", "Type" );
        case SizeColumn:
            return i18nc( "file size", "Size" );
        case LocalPathColumn:
            return i18n( "Local Path" );
        case LinkColumn:
            return i18nc( "symbolic link target", "Link" );
        }
    }

    return QVariant();
}


Qt::ItemFlags K3b::DataProjectModel::flags( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Qt::ItemFlags f = Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsDropEnabled;
        if ( index.column() == FilenameColumn ) {
            f |= Qt::ItemIsEditable;
        }
        if ( itemForIndex( index ) != d->project->root() ) {
            f |= Qt::ItemIsDragEnabled;
        }

        return f;
    }
    else {
        return QAbstractItemModel::flags( index )|Qt::ItemIsDropEnabled;
    }
}


QModelIndex K3b::DataProjectModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( parent.isValid() ) {
        K3b::DirItem* dir = itemForIndex( parent )->getDirItem();
        K3b::DataItem* child = d->getChild( dir, row );
        if ( child ) {
            return createIndex( row, column, child );
        }
        else {
            return QModelIndex();
        }
    }
    else {
        // doc root item
        return createIndex( 0, 0, d->project->root() );
    }
}


QModelIndex K3b::DataProjectModel::parent( const QModelIndex& index ) const
{
    //kDebug() << index;
    if ( index.isValid() ) {
        K3b::DataItem* item = itemForIndex( index );
        K3b::DirItem* dir = item->parent();
        if( dir ) {
            return createIndex( d->findChildIndex( dir ), 0, dir );
        }
    }

    return QModelIndex();
}


int K3b::DataProjectModel::rowCount( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        K3b::DataItem* item = itemForIndex( index );
        if ( K3b::DirItem* dir = dynamic_cast<K3b::DirItem*>( item ) ) {
            return( dir->children().count() );
        }
        else {
            return 0;
        }
    }
    else {
        return 1;
    }
}


bool K3b::DataProjectModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if ( index.isValid() ) {
        K3b::DataItem* item = itemForIndex( index );
        if ( role == Qt::EditRole ) {
            if ( index.column() == 0 ) {
                item->setK3bName( value.toString() );
                emit dataChanged( index, index );
                return true;
            }
        }
    }

    return false;
}


QMimeData* K3b::DataProjectModel::mimeData( const QModelIndexList& indexes ) const
{
    QMimeData* mime = new QMimeData();

    QList<K3b::DataItem*> items;
    KUrl::List urls;
    foreach( const QModelIndex& index, indexes ) {
        K3b::DataItem* item = itemForIndex( index );
        items << item;
        if ( item->isFile() ) {
            urls << KUrl( item->localPath() );
        }
    }
    urls.populateMimeData( mime );

    // the easy road: encode the pointers
    QByteArray itemData;
    QDataStream itemDataStream( &itemData, QIODevice::WriteOnly );
    foreach( K3b::DataItem* item, items ) {
        itemDataStream << ( qint64 )item;
    }
    mime->setData( "application/x-k3bdataitem", itemData );

    return mime;
}


Qt::DropActions K3b::DataProjectModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


QStringList K3b::DataProjectModel::mimeTypes() const
{
    QStringList s = KUrl::List::mimeDataTypes();
    s += QString::fromLatin1( "application/x-k3bdataitem" );
    return s;
}


bool K3b::DataProjectModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    kDebug();

    // no need to handle the row as item order is not important (the model just forces us to use them above)
    Q_UNUSED( row );
    Q_UNUSED( column );

    if (action == Qt::IgnoreAction)
        return true;

    // determine the target dir:
    // - drop ontp an item -> get the item's dir (i.e. itself or its parent)
    // - drop onto the viewport -> the project's root
    // --------------------------------------------------------------
    K3b::DirItem* dir = d->project->root();
    if ( parent.isValid() ) {
        dir = itemForIndex( parent )->getDirItem();
    }

    if ( data->hasFormat( "application/x-k3bdataitem" ) ) {
        kDebug() << "data item drop";

        QByteArray itemData = data->data( "application/x-k3bdataitem" );
        QDataStream itemDataStream( itemData );
        QList<K3b::DataItem*> items;
        while ( !itemDataStream.atEnd() ) {
            qint64 p;
            itemDataStream >> p;
            items << ( K3b::DataItem* )p;
        }
        // always move the items, no copy from within the views
        K3b::DataUrlAddingDialog::copyMoveItems( items, dir, 0, false );
        return true;
    }
    else if ( KUrl::List::canDecode( data ) ) {
        kDebug() << "url list drop";
        KUrl::List urls = KUrl::List::fromMimeData( data );
        K3b::DataUrlAddingDialog::addUrls( urls, dir, 0 );
        return true;
    }
    else {
        return false;
    }
}

bool K3b::DataProjectModel::removeRows( int row, int count, const QModelIndex& parent)
{
    // remove the indexes from the project
    while (count > 0)
    {
        QModelIndex i = index( row, 0, parent );
        d->project->removeItem( itemForIndex(i) );

        row++;
        count--;
    }
    return true;
}

#include "k3bdataprojectmodel.moc"
