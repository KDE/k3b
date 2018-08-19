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

#ifndef _K3B_META_ITEM_MODEL_H_
#define _K3B_META_ITEM_MODEL_H_

#include <QAbstractItemModel>
#include <QUrl>

class QIcon;

// TODO: * Have a MetaItemView which allows to set delegates for submodel header painting
//       * implement something like modelHeaderData() to get data for the root elements

namespace K3b {
    /**
     * Meta item model which combines multiple submodels into
     * one big model.
     *
     * Usage is very simple: just call addSubModel for each
     * model that should be added to the meta model.
     */
    class MetaItemModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        MetaItemModel( QObject* parent = 0 );
        ~MetaItemModel();

        QModelIndex indexForSubModel( QAbstractItemModel* model ) const;
        QAbstractItemModel* subModelForIndex( const QModelIndex& index ) const;

        /**
         * Map index to an index used in the submodel. The returned index
         * should be used carefully.
         */
        QModelIndex mapToSubModel( const QModelIndex& index ) const;
        QModelIndex mapFromSubModel( const QModelIndex& index ) const;

        /**
         * Returns the column count for the given index
         * For the root index it always return 1
         */
        virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;

        virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
        virtual QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
        virtual QModelIndex parent( const QModelIndex& index ) const;
        virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
        virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
        virtual bool hasChildren( const QModelIndex& parent = QModelIndex() ) const;
        virtual bool canFetchMore( const QModelIndex& parent ) const;
        virtual void fetchMore( const QModelIndex& parent );
        virtual bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );
        virtual QStringList mimeTypes() const;
        virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );
        virtual bool removeRows( int row, int count, const QModelIndex& parent = QModelIndex() );

        /**
         * Can handle lists of indexes from a single submodel. Mixing indexes
         * from different submodels is not supported yet and results in the method
         * returning 0.
         */
        virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;

        /**
         * The default implementation just returns the list of all drop actions
         * supported by any of the submodels.
         */
        virtual Qt::DropActions supportedDragActions() const;

        /**
         * The default implementation just returns the list of all drop actions
         * supported by any of the submodels.
         */
        virtual Qt::DropActions supportedDropActions() const;

    public Q_SLOTS:
        /**
         * PlacesModel takes over ownership of model.
         * FIXME: name and icon are weird parameters here
         *
         * \param model The submodel to be added.
         * \param flat If flat is set true the root items of the submodel will
         * be merged into the root item list of this model. Otherwise the submodel
         * will be added under a new root item.
         */
        void addSubModel( const QString& name, const QIcon& icon, QAbstractItemModel* model, bool flat = false );

        /**
         * FIXME: better use an id or something?
         */
        void removeSubModel( QAbstractItemModel* model );

    private Q_SLOTS:
        void slotRowsAboutToBeInserted( const QModelIndex&, int, int );
        void slotRowsInserted( const QModelIndex&, int, int );
        void slotRowsAboutToBeRemoved( const QModelIndex&, int, int );
        void slotRowsRemoved( const QModelIndex&, int, int );
        void slotDataChanged( const QModelIndex&, const QModelIndex& );
        void slotAboutToBeReset();
        void slotReset();

    private:
        class Private;
        Private* const d;
    };
}

#endif
