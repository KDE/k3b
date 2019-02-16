/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *           (C) 2009 Michal Malek <michalm@jabster.pl>
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

#include "k3bmetaitemmodel.h"
// this header is required to return the ItemTypeRole correctly for the
// places items
#include "k3bdataprojectmodel.h"

#include "k3bcore.h"

#include <QDebug>
#include <QMimeData>
#include <QVector>
#include <QIcon>

// IDEA: K3b::MetaItemModel::placeData( int row, int column );

#ifdef __clang__
#define CLANG_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#else
#define CLANG_ANALYZER_NORETURN
#endif

namespace {
    class Place;

    class Node {
    public:
        Node()
            : parent( 0 ),
              m_place( 0 ) {

        }

        virtual ~Node() {
            qDeleteAll(children);
        }

        virtual bool isPlace() const { return false; }
        virtual Place* place() const;
        void setPlace( Place* place );
        virtual QAbstractItemModel* model() const;
        Node* findNodeForOriginalIndex( const QModelIndex& index );
        Node* createNodeForOriginalIndex( const QModelIndex& index );
        Node* getChildNode( const QModelIndex& originalIndex );
        void updateChildren();
        void reset();
        bool supportsMimeData( const QMimeData* data ) const;

        // the parent node, 0 for Place instances
        Node* parent;

        // the model index as returned by the original model, not
        // to be used in the public API (exception: mapToSubModel())
        QPersistentModelIndex originalModelIndex;

        // the child nodes
        QVector<Node*> children;

    private:
        // the root element of this node
        Place* m_place;
    };


    class Place : public Node {
    public:
        Place( QAbstractItemModel* model )
            : m_model( model ) {
        }

        Place* place() const override;
        bool isPlace() const override { return true; }
        QAbstractItemModel* model() const override;

        // a name and icon for the place (used for display)
        // FIXME: better use something like placeData(...)
        QString name;
        QIcon icon;

        // the row index of this node
        int row;

        bool flat;

    private:
        QAbstractItemModel* m_model;
    };


    QAbstractItemModel* Node::model() const
    {
        return place()->model();
    }


    QAbstractItemModel* Place::model() const
    {
        return m_model;
    }


    Place* Node::place() const
    {
        return m_place;
    }


    void Node::setPlace( Place* place )
    {
        m_place = place;
    }


    Place* Place::place() const
    {
        return const_cast<Place*>( this );
    }


    void Node::updateChildren()
    {
        // only update children when there is no items in the list
        if ( children.isEmpty() ) {
            // TODO: this is kind of evil since indexes store pointers to the nodes
            //qDebug() << "resizing children from" << children.size() << "to" << rows;

            int rows = model()->rowCount( originalModelIndex );
            for ( int i = 0; i < rows; ++i ) {
                Node *node = new Node();
                node->setPlace( place() );
                node->parent = this;
                node->originalModelIndex = model()->index( i, 0, originalModelIndex );
                children.append(node);
            }
        }
    }


    Node* Node::getChildNode( const QModelIndex& originalIndex )
    {
        updateChildren();

        Q_ASSERT(children.size() > originalIndex.row());

        Node* node = children[originalIndex.row()];
        return node;
    }


    Node* Node::findNodeForOriginalIndex( const QModelIndex& index )
    {
        if ( originalModelIndex == index ) {
            return this;
        }

        for ( int i = 0; i < children.count(); ++i ) {
            if ( Node* node = children[i]->findNodeForOriginalIndex( index ) )
                return node;
        }

        return 0;
    }

    static void K3b_ASSERT(bool test) CLANG_ANALYZER_NORETURN { Q_ASSERT(test); }

    Node* Node::createNodeForOriginalIndex( const QModelIndex& index )
    {
        if ( !index.isValid() && isPlace() ) {
            return this;
        }

        K3b_ASSERT(index.isValid());

        // all the node mapping is done on the first col, so make sure we use
        // an index on the first col
        // 
        // A valid index belongs to a model, and has non-negative row and column numbers
        // so index.model() is NOT nullptr if index.isValid()
        QModelIndex firstColIndex = index.model()->index(index.row(), 0, index.parent());
        Node* node = findNodeForOriginalIndex( firstColIndex );
        if ( !node ) {
            Node* parentNode = createNodeForOriginalIndex( firstColIndex.parent() );
            node = parentNode->getChildNode( firstColIndex );
        }

        return node;
    }


    void Node::reset()
    {
        qDeleteAll(children);
        children.clear();
    }

    bool Node::supportsMimeData( const QMimeData* data ) const
    {
        QStringList supportedFormats = model()->mimeTypes();
        QStringList formats = data->formats();
        Q_FOREACH( const QString& format, formats )
        {
            if( supportedFormats.indexOf( format ) >= 0 )
                return true;
        }
        return false;
    }
} // namespace



class K3b::MetaItemModel::Private
{
public:
    Private( MetaItemModel* model ) : q( model ) {}

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
                    return it->children[row-i];
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

    int getRootNodeRow(Node *node)
    {
        int row = 0;
        QList<Place>::iterator end = places.end();
        for ( QList<Place>::iterator it = places.begin();
              it != end; ++it ) {
            if (node->isPlace() && node->model() == it->model())
                return row;

            if ( it->flat ) {
                it->updateChildren();
                for (int i = 0; i < it->children.count(); ++i)
                {
                    if (!node->isPlace() && it->children[i]->originalModelIndex == node->originalModelIndex)
                        return row;

                    ++row;
                }
            }
            else
                ++row;
        }

        return -1;
    }
    /**
     * returns the node pointer for the given index.
     * This makes it easier to handle multiple column
     */
    Node* nodeForIndex( const QModelIndex &index )
    {
        // all indexes store the node in their internal pointers
        if( index.isValid() && index.model() == q )
            return static_cast<Node*>(index.internalPointer());
        else
            return 0;
    }

    /**
     * returns an index from the source model for the given index
     */
    QModelIndex sourceIndex( const QModelIndex &index )
    {
        Node *node = nodeForIndex(index);
        if (!node || node->isPlace())
            return QModelIndex();

        return node->model()->index( node->originalModelIndex.row(), index.column(), node->originalModelIndex.parent() );
    }

    QList<Place> places;
    MetaItemModel* q;
};



K3b::MetaItemModel::MetaItemModel( QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private( this ) )
{
}


K3b::MetaItemModel::~MetaItemModel()
{
    delete d;
}


QModelIndex K3b::MetaItemModel::indexForSubModel( QAbstractItemModel* model ) const
{
    if( !d->places.empty() ) {
        if( Place* place = d->placeForModel( model ) )
            return createIndex( place->row, 0, place );
    }
    return QModelIndex();
}


QAbstractItemModel* K3b::MetaItemModel::subModelForIndex( const QModelIndex& index ) const
{
    if( Node* node = d->nodeForIndex( index ) )
        return node->model();
    else
        return 0;
}


QModelIndex K3b::MetaItemModel::mapToSubModel( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Q_ASSERT( index.model() == this );
        return d->sourceIndex( index );
    }
    else {
        return QModelIndex();
    }
}


QModelIndex K3b::MetaItemModel::mapFromSubModel( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Place *place = d->placeForModel( index.model() );
        Node* node = place->createNodeForOriginalIndex( index );
        Q_ASSERT( node );

        // if the place is not flat, or the parent index is valid
        // we just have to return the index for the row and column
        if ( !place->flat || index.parent().isValid() )
            return createIndex( index.row(), index.column(), node );
        else {
            // now if the place is flat and the parent is not valid,
            // we have to adjust the row according to its position in the
            // tree
            int row = d->getRootNodeRow( node );
            if ( row < 0 )
                return QModelIndex();
            else
                return createIndex( row, index.column(), node );
        }
    }
    else {
        return QModelIndex();
    }
}


int K3b::MetaItemModel::columnCount( const QModelIndex& parent ) const
{
    QAbstractItemModel *model = subModelForIndex( parent );
    if (!model)
        return 1;

    return model->columnCount( mapToSubModel( parent ) );
}


QVariant K3b::MetaItemModel::data( const QModelIndex& index, int role ) const
{
    if( Node* node = d->nodeForIndex( index ) ) {
        Q_ASSERT( node->model() );
        Q_ASSERT( node->isPlace() || node->originalModelIndex.isValid() );

        if ( node->isPlace() ) {
            // provide the root elements of the places
            switch( role ) {
            case Qt::DisplayRole:
                return node->place()->name;

            case Qt::DecorationRole:
                return node->place()->icon;

            case DataProjectModel::ItemTypeRole:
                return (int) DataProjectModel::DirItemType;

            default:
                return QVariant();
            }
        }
        else {
            return node->model()->data( mapToSubModel( index ), role );
        }
    }
    else {
        return QVariant();
    }
}


QModelIndex K3b::MetaItemModel::index( int row, int column, const QModelIndex& parent ) const
{
    //qDebug() << row << column << parent;

    if ( row < 0 || column < 0 ) {
        return QModelIndex();
    }

    if ( parent.isValid() ) {
        Node* parentNode = d->nodeForIndex( parent );

        Q_ASSERT( parentNode->parent || parentNode->isPlace() );
        Q_ASSERT( parentNode->place() );
        Q_ASSERT( parentNode->model() );

        // for places the originalModelIndex is invalid
        QModelIndex originalIndex = parentNode->model()->index( row, 0, parentNode->originalModelIndex );
        Node* node = parentNode->place()->createNodeForOriginalIndex( originalIndex );
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


QModelIndex K3b::MetaItemModel::parent( const QModelIndex& index ) const
{
    //qDebug() << "Parent of" << index;
    Node* node = d->nodeForIndex( index );

    if ( !index.isValid() || !node || node->isPlace() )
        return QModelIndex();

    Q_ASSERT( node->parent );
    Q_ASSERT( node->place() );
    Q_ASSERT( node->model() );


    QModelIndex origIndex = mapToSubModel( index ).parent();

    if ( origIndex.isValid() ) {
        return mapFromSubModel( origIndex );
    }
    else if ( !node->place()->flat ) {
        return createIndex( node->place()->row, 0, node->place() );
    }
    else {
        return QModelIndex();
    }
}


Qt::ItemFlags K3b::MetaItemModel::flags( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Node* node = d->nodeForIndex( index );
        if ( node->isPlace() ) {
            // flags from invalid index can be helpful when model is drop-enabled
            return node->model()->flags( QModelIndex() )|Qt::ItemIsSelectable|Qt::ItemIsEnabled;
        }
        else {
            return mapToSubModel( index ).flags();
        }
    }

    return QAbstractItemModel::flags( index );
}


bool K3b::MetaItemModel::hasChildren( const QModelIndex& parent ) const
{
//    qDebug() << parent;

    if ( parent.isValid() ) {
        Node* parentNode = d->nodeForIndex( parent );

        Q_ASSERT( parentNode->place() );
        Q_ASSERT( parentNode->place()->model() );
        Q_ASSERT( parentNode->model() );

        // the originalModelIndex is invalid for place nodes
        return parentNode->model()->hasChildren( mapToSubModel( parent ) );
    }
    else {
        return !d->places.isEmpty();
    }
}


bool K3b::MetaItemModel::canFetchMore( const QModelIndex& parent ) const
{
//    qDebug() << parent;

    if ( parent.isValid() ) {
        Node* parentNode = d->nodeForIndex( parent );
        return parentNode->model()->canFetchMore( mapToSubModel( parent ) );
    }
    else {
        return false;
    }
}


void K3b::MetaItemModel::fetchMore( const QModelIndex& parent )
{
//    qDebug() << parent;

    if ( parent.isValid() ) {
        Node* parentNode = d->nodeForIndex( parent );
        parentNode->model()->fetchMore( mapToSubModel( parent ) );
    }
}


int K3b::MetaItemModel::rowCount( const QModelIndex& parent ) const
{
//    qDebug() << parent;
    if ( parent.column() > 0 )
        return 0;

    if ( parent.isValid() ) {
        Node* parentNode = d->nodeForIndex( parent );
        return parentNode->model()->rowCount( mapToSubModel( parent ) );
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


bool K3b::MetaItemModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
//    qDebug() << index;

    if ( index.isValid() ) {
        Node* node = d->nodeForIndex( index );
        if ( node->isPlace() ) {
            // cannot edit the place, should not happen anyway, see flags()
            return false;
        }
        else {
            return node->model()->setData( mapToSubModel( index ), value, role );
        }
    }
    else {
        return false;
    }
}


QStringList K3b::MetaItemModel::mimeTypes() const
{
    QSet<QString> types;
    for( QList<Place>::const_iterator it = d->places.constBegin();
        it != d->places.constEnd(); ++it )
    {
        types += it->model()->mimeTypes().toSet();
    }
    return types.toList();
}


bool K3b::MetaItemModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
//    qDebug();

    if ( parent.isValid() ) {
        Node* parentNode = d->nodeForIndex( parent );

        if( !parentNode->supportsMimeData( data ) ) {
            return false;
        }
        else {
            // for places the originalModelIndex will be invalid
            return parentNode->model()->dropMimeData( data, action, row, column, mapToSubModel( parent ) );
        }
    }
    else if ( row >= 0 ) {
        Node *node = d->getRootNode(row);

        if( !node->supportsMimeData( data ) ) {
            return false;
        }
        else if (node->isPlace()) {
            // if the node is place, threat it like if it was being dropped on an empty space of the
            // original model
            return node->model()->dropMimeData(data, action, -1, column, QModelIndex());
        }
        else {
            // if it is not a place (which means the original model is a flat model)
            // drop like if it was being dropped in the row/col of the original index
            return node->model()->dropMimeData(data, action, node->originalModelIndex.row(), column, QModelIndex());
        }
    }
    else {
        return false;
    }
}


bool K3b::MetaItemModel::removeRows( int row, int count, const QModelIndex& parent )
{
    if( parent.isValid() ) {
        Node* parentNode = d->nodeForIndex( parent );
        return parentNode->model()->removeRows( row, count, mapToSubModel( parent ) );
    }
    else if( row >= 0 ) {
        for( int i = 0; i < count; ++i ) {
            d->places.removeAt( row );
        }
        return true;
    }
    else {
        return false;
    }
}


QMimeData* K3b::MetaItemModel::mimeData( const QModelIndexList& indexes ) const
{
    if ( !indexes.isEmpty() ) {
        QModelIndexList origIndexes;
        for ( QModelIndexList::const_iterator it = indexes.constBegin();
              it != indexes.constEnd(); ++it ) {
            QModelIndex sourceIndex = mapToSubModel( *it );
            if ( !origIndexes.isEmpty() && sourceIndex.model() != origIndexes.first().model() ) {
                qDebug() << "cannot handle indexes from different submodels yet.";
                return 0;
            }
            origIndexes.append( sourceIndex );
        }
        return origIndexes.first().model()->mimeData( origIndexes );
    }

    return 0;
}


Qt::DropActions K3b::MetaItemModel::supportedDragActions() const
{
    Qt::DropActions a = Qt::IgnoreAction;

    for ( int i = 0; i < d->places.count(); ++i ) {
        a |= d->places[i].model()->supportedDragActions();
    }
    return a;
}


Qt::DropActions K3b::MetaItemModel::supportedDropActions() const
{
    Qt::DropActions a = Qt::IgnoreAction;

    for ( int i = 0; i < d->places.count(); ++i ) {
        a |= d->places[i].model()->supportedDropActions();
    }
    return a;
}


void K3b::MetaItemModel::addSubModel( const QString& name, const QIcon& icon, QAbstractItemModel* model, bool flat )
{
    const int first = rowCount(QModelIndex());
    const int last = first + (flat ? model->rowCount() - 1 : 0);

    if ( first <= last )
        beginInsertRows( QModelIndex(), first, last );

    model->setParent( this );

    d->places.append( Place( model ) );

    Place& place = d->places.last();
    place.name = name;
    place.icon = icon;
    place.flat = flat;
    place.updateChildren();

    d->updatePlaceRows();

    connect( place.model(), SIGNAL(modelAboutToBeReset()),
             this, SLOT(slotAboutToBeReset()) );

    connect( place.model(), SIGNAL(modelReset()),
             this, SLOT(slotReset()) );

    connect( place.model(), SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
             this, SLOT(slotRowsAboutToBeInserted(QModelIndex,int,int)) );

    connect( place.model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
             this, SLOT(slotRowsInserted(QModelIndex,int,int)) );

    connect( place.model(), SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
             this, SLOT(slotRowsAboutToBeRemoved(QModelIndex,int,int)) );

    connect( place.model(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
             this, SLOT(slotRowsRemoved(QModelIndex,int,int)) );

    connect( place.model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
             this, SLOT(slotDataChanged(QModelIndex,QModelIndex)) );

    if ( first <= last )
        endInsertRows();
}


void K3b::MetaItemModel::removeSubModel( QAbstractItemModel* model )
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
    int end = row;

    if ( it->flat ) {
        end += it->children.count() - 1;
    }


    // if the model is flat and has no root rows, there is nothing not notify
    if ( row <= end )
        beginRemoveRows( QModelIndex(), row, end );

    d->places.erase( it );
    d->updatePlaceRows();

    if ( row <= end )
        endRemoveRows();

    // finally delete the model
    delete model;
}


void K3b::MetaItemModel::slotRowsAboutToBeInserted( const QModelIndex& parent, int start, int end )
{
    Place* place = d->placeForModel( qobject_cast<QAbstractItemModel*>( sender() ) );
    Q_ASSERT( place != 0 );

    QModelIndex newParent;
    int targetStart = start, targetEnd = end;

    // ---------- Preparing to insert ----------
    if( parent.isValid() ) {
        // search node corresponding to 'index'
        newParent = mapFromSubModel( parent );
    }
    else {
        if ( place->flat ) {
            targetStart += place->row;
            targetEnd += place->row;
        }
        else {
            newParent = createIndex( place->row, 0, place );
        }
    }

    beginInsertRows( newParent, targetStart, targetEnd );

    // ---------- Inserting ----------
    Node* parentNode;
    if( parent.isValid() )
        parentNode = place->createNodeForOriginalIndex( parent );
    else
        parentNode = place;

    // if the node doesn't have children yet (maybe not yet acessed)
    // or if it has less items than the start point of this insertion
    // simply load the child nodes
    if (start > parentNode->children.count()) {
        parentNode->updateChildren();
    }
    else {
        // insert the newly created items in the children list
        for( int i = start; i <= end; ++i) {
            Node *newChild = new Node();
            newChild->parent = parentNode;
            newChild->setPlace( parentNode->place() );
            newChild->originalModelIndex = QModelIndex();
            parentNode->children.insert(i, 1, newChild);
        }
    }
    if( place->flat ) {
        d->updatePlaceRows();
    }
}


void K3b::MetaItemModel::slotRowsInserted( const QModelIndex& parent, int start, int end )
{
    Place* place = d->placeForModel( qobject_cast<QAbstractItemModel*>( sender() ) );
    Q_ASSERT( place != 0 );

    Node* parentNode;
    if( parent.isValid() )
        parentNode = place->createNodeForOriginalIndex( parent );
    else
        parentNode = place;

    // updating original indexes in newly created nodes
    for( int i = start; i <= end; ++i) {
        Node* child = parentNode->children.at( i );
        Q_ASSERT( child != 0 );
        child->originalModelIndex = parentNode->model()->index( i, 0, parent );
    }

    endInsertRows();
}


void K3b::MetaItemModel::slotRowsAboutToBeRemoved( const QModelIndex& parent, int start, int end )
{
    //qDebug();

    Place* place = d->placeForModel( qobject_cast<QAbstractItemModel*>( sender() ) );
    Q_ASSERT( place != 0 );

    QModelIndex newParent;
    int targetStart = start, targetEnd = end;

    // --------------- Preparing for removing ----------------
    if ( parent.isValid() ) {
        // search node corresponding to 'index'
        newParent = mapFromSubModel( parent );
    }
    else {
        if ( place->flat ) {
            targetStart += place->row;
            targetEnd += place->row;
        }
        else
            newParent = createIndex( place->row, 0, place );
    }

    // --------------- Removing -------------------------------
    beginRemoveRows( newParent, targetStart, targetEnd );

    Node* parentNode;

    if ( parent.isValid() )
        parentNode = place->createNodeForOriginalIndex( parent );
    else
        parentNode = place;

    // parentNode->children may be empty if one of
    // the previous remove ended up causing a reset
    if (!parentNode->children.isEmpty()) {
        // remove the contents of pointers
        for (int i = start; i <= end; ++i)
            delete parentNode->children[i];

        // and remove the pointers themselves
        parentNode->children.remove( start, (end - start + 1) );
    }
}


void K3b::MetaItemModel::slotRowsRemoved( const QModelIndex&, int, int )
{
    Place* place = d->placeForModel( qobject_cast<QAbstractItemModel*>( sender() ) );
    Q_ASSERT( place != 0 );

    if ( place->flat ) {
        d->updatePlaceRows();
    }

    endRemoveRows();
}


void K3b::MetaItemModel::slotDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
    //qDebug();

    Q_ASSERT( topLeft.isValid() );
    Q_ASSERT( bottomRight.isValid() );

    emit dataChanged( mapFromSubModel( topLeft ), mapFromSubModel( bottomRight ) );
}


void K3b::MetaItemModel::slotAboutToBeReset()
{
    beginResetModel();
}


void K3b::MetaItemModel::slotReset()
{
    // clean out any cached nodes
    for ( int i = 0; i < d->places.count(); ++i ) {
        d->places[i].reset();
    }
    d->updatePlaceRows();

    endResetModel();
}


