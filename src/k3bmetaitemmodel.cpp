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

#include "k3bmetaitemmodel.h"

#include <k3bcore.h>

#include <KIcon>
#include <KDebug>

#include <QtCore/QVector>

// IDEA: K3bMetaItemModel::placeData( int row, int column );

// FIXME: multiple columns!

namespace {
    class Place;

    class Node {
    public:
        Node()
            : parent( 0 ),
              m_place( 0 ) {
        }

        virtual ~Node() {
        }

        virtual bool isPlace() const { return false; }
        virtual Place* place();
        void setPlace( Place* place );
        virtual QAbstractItemModel* model();
        Node* findNodeForOriginalIndex( const QModelIndex& index );
        Node* createNodeForOriginalIndex( const QModelIndex& index );
        Node* getChildNode( const QModelIndex& originalIndex );
        void updateChildren();
        void reset();

        // the parent node, 0 for Place instances
        Node* parent;

        // the model index as returned by the original model, not
        // to be used in the public API (exception: mapToSubModel())
        QPersistentModelIndex originalModelIndex;

        // the child nodes
        QVector<Node> children;

    private:
        // the root element of this node
        Place* m_place;
    };


    class Place : public Node {
    public:
        Place( QAbstractItemModel* model )
            : m_model( model ) {
        }

        Place* place();
        bool isPlace() const { return true; }
        QAbstractItemModel* model();

        // a name and icon for the place (used for display)
        // FIXME: better use something like placeData(...)
        QString name;
        KIcon icon;

        // the row index of this node
        int row;

        bool flat;

    private:
        QAbstractItemModel* m_model;
    };


    QAbstractItemModel* Node::model()
    {
        return place()->model();
    }


    QAbstractItemModel* Place::model()
    {
        return m_model;
    }

    Place* Node::place()
    {
        return m_place;
    }


    void Node::setPlace( Place* place )
    {
        m_place = place;
    }


    Place* Place::place()
    {
        return this;
    }


    void Node::updateChildren()
    {
        int rows = model()->rowCount( originalModelIndex );
        if ( children.size() < rows ) {
            kDebug() << "resizing children from" << children.size() << "to" << rows;
            children.resize( rows );

            for ( int i = 0; i < children.count(); ++i ) {
                Node& node = children[i];
                node.setPlace( place() );
                node.parent = this;
                // FIXME: support multiple columns
                node.originalModelIndex = model()->index( i, 0, originalModelIndex );
            }
        }
    }


    Node* Node::getChildNode( const QModelIndex& originalIndex )
    {
        if ( children.size() <= originalIndex.row() ) {
            int rows = model()->rowCount( originalModelIndex );
            kDebug() << "resizing children from" << children.size() << "to" << rows;
            Q_ASSERT( originalIndex.row() < rows );
            children.resize( rows );
        }

        Node* node = &children[originalIndex.row()];
        node->setPlace( place() );
        node->originalModelIndex = originalIndex;
        node->parent = this;
        return node;
    }


    Node* Node::findNodeForOriginalIndex( const QModelIndex& index )
    {
        if ( originalModelIndex == index ) {
            return this;
        }

        for ( int i = 0; i < children.count(); ++i ) {
            if ( Node* node = children[i].findNodeForOriginalIndex( index ) )
                return node;
        }

        return 0;
    }


    Node* Node::createNodeForOriginalIndex( const QModelIndex& index )
    {
        if ( !index.isValid() && isPlace() ) {
            return this;
        }

        Q_ASSERT( index.isValid() );

        Node* node = findNodeForOriginalIndex( index );
        if ( !node ) {
            Node* parentNode = createNodeForOriginalIndex( index.parent() );
            node = parentNode->getChildNode( index );
        }

        return node;
    }


    void Node::reset()
    {
        children.clear();
    }
}



class K3bMetaItemModel::Private
{
public:
    QList<Place> places;

    Place* placeForModel( const QAbstractItemModel* model ) {
        for ( int i = 0; i < places.count(); ++i ) {
            if ( places[i].model() == model ) {
                return &places[i];
            }
        }
        return 0;
    }

    void updatePlaceRows() {
        int row = 0;
        QList<Place>::iterator end = places.end();
        for ( QList<Place>::iterator it = places.begin();
              it != end; ++it ) {
            it->row = row;
            if ( it->flat ) {
                row += it->model()->rowCount( QModelIndex() );
            }
            else {
                ++row;
            }
        }
    }

    /**
     * root nodes are all non-flat places +
     * all root items from flat places.
     */
    Node* getRootNode( int row ) {
        int i = 0;
        QList<Place>::iterator end = places.end();
        for ( QList<Place>::iterator it = places.begin();
              it != end; ++it ) {
            if ( it->flat ) {
                it->updateChildren();
                if ( i + it->children.count() > row ) {
                    return &it->children[row-i];
                }
                else {
                    i += it->children.count();
                }
            }
            else if ( row == i ) {
                return &( *it );
            }
            else {
                ++i;
            }
        }

        return 0;
    }
};



K3bMetaItemModel::K3bMetaItemModel( QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private() )
{
}


K3bMetaItemModel::~K3bMetaItemModel()
{
    delete d;
}


QAbstractItemModel* K3bMetaItemModel::subModelForIndex( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Q_ASSERT( index.model() == this );
        Node* node = static_cast<Node*>( index.internalPointer() );
        return node->model();
    }
    else {
        return 0;
    }
}


QModelIndex K3bMetaItemModel::mapToSubModel( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Q_ASSERT( index.model() == this );
        Node* node = static_cast<Node*>( index.internalPointer() );
        return node->originalModelIndex;
    }
    else {
        return QModelIndex();
    }
}


QModelIndex K3bMetaItemModel::mapFromSubModel( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Node* node = d->placeForModel( index.model() )->createNodeForOriginalIndex( index );
        Q_ASSERT( node );
        return createIndex( index.row(), index.column(), node );
    }
    else {
        return QModelIndex();
    }
}


int K3bMetaItemModel::columnCount( const QModelIndex& parent ) const
{
    // FIXME: support multiple column in Node
    return 1;

//     if ( parent.isValid() ) {
//         Node* parentNode = static_cast<Node*>( parent.internalPointer() );

//         // for places the originalModelIndex is invalid
//         return parentNode->model()->columnCount( parentNode->originalModelIndex );
//     }
//     else {
//         // we define only one column for root items
//         return 1;
//     }
}


QVariant K3bMetaItemModel::data( const QModelIndex& index, int role ) const
{
    Node* node = static_cast<Node*>( index.internalPointer() );

    Q_ASSERT( node );
    Q_ASSERT( node->model() );
    Q_ASSERT( node->isPlace() || node->originalModelIndex.isValid() );

    if ( node->isPlace() ) {
        // provide the root elements of the places
        switch( role ) {
        case Qt::DisplayRole:
            return node->place()->name;

        case Qt::DecorationRole:
            return node->place()->icon;

        default:
            return QVariant();
        }
    }
    else {
        return node->model()->data( node->originalModelIndex, role );
    }
}


QModelIndex K3bMetaItemModel::index( int row, int column, const QModelIndex& parent ) const
{
//    kDebug() << row << column << parent;

    // FIXME: add multiple column support to Node
    if ( row < 0 || column != 0 ) {
        return QModelIndex();
    }

    if ( parent.isValid() ) {
        Node* parentNode = static_cast<Node*>( parent.internalPointer() );

        Q_ASSERT( parentNode->parent || parentNode->isPlace() );
        Q_ASSERT( parentNode->place() );
        Q_ASSERT( parentNode->model() );

        // for places the originalModelIndex is invalid
        QModelIndex originalIndex = parentNode->model()->index( row, column, parentNode->originalModelIndex );
        Node* node = parentNode->getChildNode( originalIndex );
        return createIndex( row, column, node );
    }
    else {
        if ( Node* node = d->getRootNode( row ) ) {
            return createIndex( row, column, node );
        }
        else {
            return QModelIndex();
        }
    }
}


QModelIndex K3bMetaItemModel::parent( const QModelIndex& index ) const
{
//    kDebug() << index;

    Node* node = static_cast<Node*>( index.internalPointer() );

    if ( !index.isValid() || node->isPlace() ) {
        return QModelIndex();
    }
    else {
        Q_ASSERT( index.internalPointer() );
        Q_ASSERT( node->parent );
        Q_ASSERT( node->place() );
        Q_ASSERT( node->model() );

        QModelIndex origIndex = node->model()->parent( node->originalModelIndex );
        if ( origIndex.isValid() ) {
            // update node (unsure if this is necessary...)
            node->parent->originalModelIndex = origIndex;

            return createIndex( origIndex.row(), origIndex.column(), node->parent );
        }
        else if ( !node->place()->flat ) {
            return createIndex( node->place()->row, index.column(), node->place() );
        }
        else {
            return QModelIndex();
        }
    }
}


Qt::ItemFlags K3bMetaItemModel::flags( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Node* node = static_cast<Node*>( index.internalPointer() );
        if ( node->isPlace() ) {
            return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
        }
        else {
            return node->model()->flags( node->originalModelIndex );
        }
    }

    return QAbstractItemModel::flags( index );
}


bool K3bMetaItemModel::hasChildren( const QModelIndex& parent ) const
{
//    kDebug() << parent;

    if ( parent.isValid() ) {
        Q_ASSERT( parent.internalPointer() );

        Node* parentNode = static_cast<Node*>( parent.internalPointer() );

        Q_ASSERT( parentNode->place() );
        Q_ASSERT( parentNode->place()->model() );
        Q_ASSERT( parentNode->model() );

        // the originalModelIndex is invalid for place nodes
        return parentNode->model()->hasChildren( parentNode->originalModelIndex );
    }
    else {
        return !d->places.isEmpty();
    }
}


bool K3bMetaItemModel::canFetchMore( const QModelIndex& parent ) const
{
//    kDebug() << parent;

    if ( parent.isValid() ) {
        Node* parentNode = static_cast<Node*>( parent.internalPointer() );
        if ( parentNode->isPlace() ) {
            return parentNode->place()->model()->canFetchMore( QModelIndex() );
        }
        else {
            return parentNode->model()->canFetchMore( parentNode->originalModelIndex );
        }
    }
    else {
        return false;
    }
}


void K3bMetaItemModel::fetchMore( const QModelIndex& parent )
{
//    kDebug() << parent;

    if ( parent.isValid() ) {
        Node* parentNode = static_cast<Node*>( parent.internalPointer() );
        if ( parentNode->isPlace() ) {
            parentNode->place()->model()->fetchMore( QModelIndex() );
        }
        else {
            parentNode->model()->fetchMore( parentNode->originalModelIndex );
        }
    }
}


int K3bMetaItemModel::rowCount( const QModelIndex& parent ) const
{
//    kDebug() << parent;

    if ( parent.isValid() ) {
        Node* parentNode = static_cast<Node*>( parent.internalPointer() );
        if ( parentNode->isPlace() ) {
            return parentNode->place()->model()->rowCount( QModelIndex() );
        }
        else {
            return parentNode->model()->rowCount( parentNode->originalModelIndex );
        }
    }
    else {
        int cnt = 0;
        QList<Place>::iterator end = d->places.end();
        for ( QList<Place>::iterator it = d->places.begin();
              it != end; ++it ) {
            if( it->flat ) {
                cnt += it->model()->rowCount( QModelIndex() );
            }
            else {
                ++cnt;
            }
        }
        return cnt;
    }
}


bool K3bMetaItemModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
//    kDebug() << index;

    if ( index.isValid() ) {
        Node* node = static_cast<Node*>( index.internalPointer() );
        if ( node->isPlace() ) {
            // cannot edit the place, should not happen anyway, see flags()
            return false;
        }
        else {
            return node->model()->setData( node->originalModelIndex, value, role );
        }
    }
    else {
        return false;
    }
}


bool K3bMetaItemModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
//    kDebug();

    if ( parent.isValid() ) {
        Node* parentNode = static_cast<Node*>( parent.internalPointer() );
        // for places the originalModelIndex will be invalid
        return parentNode->model()->dropMimeData( data, action, row, column, parentNode->originalModelIndex );
    }
    else if ( row != -1 ) {
        Node *node = d->getRootNode(row);

        if (node->isPlace())
        {
            // if the node is place, threat it like if it was being dropped on an empty space of the 
            // original model
            return node->model()->dropMimeData(data, action, -1, -1, QModelIndex());
        }
        else
        {
            // if it is not a place (which means the original model is a flat model)
            // drop like if it was being dropped in the row/col of the original index
            node->model()->dropMimeData(data, action, node->originalModelIndex.row(), node->originalModelIndex.column(), QModelIndex());
        }
    }


    return false;
}


QMimeData* K3bMetaItemModel::mimeData( const QModelIndexList& indexes ) const
{
    if ( !indexes.isEmpty() ) {
        QModelIndexList origIndexes;
        for ( QModelIndexList::const_iterator it = indexes.constBegin();
              it != indexes.constEnd(); ++it ) {
            Node* node = static_cast<Node*>( ( *it ).internalPointer() );
            if ( !origIndexes.isEmpty() && node->model() != origIndexes.first().model() ) {
                kDebug() << "cannot handle indexes from different submodels yet.";
                return 0;
            }
            origIndexes.append( node->originalModelIndex );
        }
        return origIndexes.first().model()->mimeData( origIndexes );
    }

    return 0;
}


Qt::DropActions K3bMetaItemModel::supportedDropActions() const
{
    Qt::DropActions a = Qt::IgnoreAction;

    for ( int i = 0; i < d->places.count(); ++i ) {
        a |= d->places[i].model()->supportedDropActions();
    }
    return a;
}


void K3bMetaItemModel::addSubModel( const QString& name, const KIcon& icon, QAbstractItemModel* model, bool flat )
{
    beginInsertRows( QModelIndex(), d->places.count(), d->places.count() );

    model->setParent( this );

    d->places.append( Place( model ) );

    Place& place = d->places.last();
    place.name = name;
    place.icon = icon;
    place.flat = flat;

    d->updatePlaceRows();

    connect( place.model(), SIGNAL( modelReset() ),
             this, SLOT( slotReset() ) );

    connect( place.model(), SIGNAL( rowsAboutToBeInserted( const QModelIndex&, int, int ) ),
             this, SLOT( slotRowsAboutToBeInserted( const QModelIndex&, int, int ) ) );
    connect( place.model(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
             this, SLOT( slotRowsInserted( const QModelIndex&, int, int ) ) );

    connect( place.model(), SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int )  ),
             this, SLOT( slotRowsAboutToBeRemoved( const QModelIndex&, int, int ) ) );
    connect( place.model(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ),
             this, SLOT( slotRowsRemoved( const QModelIndex&, int, int ) ) );

    connect( place.model(), SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ),
             this, SLOT( slotDataChanged( const QModelIndex&, const QModelIndex& ) ) );

    endInsertRows();
}


void K3bMetaItemModel::removeSubModel( QAbstractItemModel* model )
{
    // find the place index
    int row = 0;
    QList<Place>::iterator it = d->places.begin();
    while ( it != d->places.end() && ( *it ).model() != model ) {
        ++it;
        ++row;
    }

    Q_ASSERT( it != d->places.end() );

    // and simply remove the place from the list
    beginRemoveRows( QModelIndex(), row, row );
    d->places.erase( it );
    d->updatePlaceRows();
    endRemoveRows();

    // finally delete the model
    delete model;
}


void K3bMetaItemModel::slotRowsAboutToBeInserted( const QModelIndex& index, int start, int end )
{
//    kDebug() << index << start << end;
    Place* place = d->placeForModel( qobject_cast<QAbstractItemModel*>( sender() ) );

    QModelIndex newIndex;

    if ( index.isValid() ) {
        // search node corresponding to 'index'
        Node* node = place->createNodeForOriginalIndex( index );
        Q_ASSERT( node );
        Q_ASSERT( node->originalModelIndex == index );
        newIndex = createIndex( index.row(), index.column(), node );
    }
    else {
        newIndex = createIndex( place->row, 0, place );
    }

    beginInsertRows( newIndex, start, end );
}


void K3bMetaItemModel::slotRowsInserted( const QModelIndex& index, int start, int end )
{
//    kDebug() << index << start << end;

    if ( d->placeForModel( qobject_cast<QAbstractItemModel*>( sender() ) )->flat ) {
        d->updatePlaceRows();
    }

    Q_UNUSED( index );
    Q_UNUSED( start );
    Q_UNUSED( end );
    endInsertRows();
}


void K3bMetaItemModel::slotRowsAboutToBeRemoved( const QModelIndex& index, int start, int end )
{
    kDebug();

    Place* place = d->placeForModel( qobject_cast<QAbstractItemModel*>( sender() ) );

    QModelIndex newIndex;

    if ( index.isValid() ) {
        // search node corresponding to 'index'
        Node* node = place->findNodeForOriginalIndex( index );
        Q_ASSERT( node );
        newIndex = createIndex( index.row(), index.column(), node );
    }
    else {
        newIndex = createIndex( place->row, 0, place );
    }

    beginRemoveRows( newIndex, start, end );
}


void K3bMetaItemModel::slotRowsRemoved( const QModelIndex& index, int start, int end )
{
    kDebug();

    if ( d->placeForModel( qobject_cast<QAbstractItemModel*>( sender() ) )->flat ) {
        d->updatePlaceRows();
    }

    Q_UNUSED( index );
    Q_UNUSED( start );
    Q_UNUSED( end );
    endRemoveRows();
}


void K3bMetaItemModel::slotDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
    kDebug();

    Q_ASSERT( topLeft.isValid() );
    Q_ASSERT( bottomRight.isValid() );

    Place* place = d->placeForModel( qobject_cast<QAbstractItemModel*>( sender() ) );

    emit dataChanged( createIndex( topLeft.row(), topLeft.column(), place->findNodeForOriginalIndex( topLeft ) ),
                      createIndex( bottomRight.row(), bottomRight.column(), place->findNodeForOriginalIndex( bottomRight ) ) );
}


void K3bMetaItemModel::slotReset()
{
    // clean out any cached nodes
    for ( int i = 0; i < d->places.count(); ++i ) {
        d->places[i].reset();
    }

    reset();
}

#include "k3bmetaitemmodel.moc"
