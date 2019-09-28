/*
 *
 * Copyright (C) 2007-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_PLACES_MODEL_H_
#define _K3B_PLACES_MODEL_H_

#include "k3bmetaitemmodel.h"

#include <QUrl>

class KFileItem;

namespace K3b {
    namespace Device {
        class Device;
        class DeviceManager;
    }

    /**
     * Wraps multiple KDirModels and a DeviceModel
     */
    class PlacesModel : public MetaItemModel
    {
        Q_OBJECT

    public:
        explicit PlacesModel( QObject* parent = 0 );
        ~PlacesModel() override;

        /**
         * Will return an invalid item if the index is not part
         * of a KDirModel.
         */
        KFileItem itemForIndex( const QModelIndex& index ) const;

        /**
         * Will return 0 if the index does not refer to a device item.
         */
        Device::Device* deviceForIndex( const QModelIndex& index ) const;

        /**
         * Will return invalid index if model does not contain such device
         */
        QModelIndex indexForDevice( Device::Device* dev ) const;

    Q_SIGNALS:
        /**
         * Emitted for each subdirectory that is a parent of a url
         * passed to expandToUrl This allows one to asynchronously open
         * a tree view down to a given directory.
         *
         * \sa KDirModel::expand
         */
        void expand( const QModelIndex& index );

    public Q_SLOTS:
        void addPlace( const QString& name, const QIcon& icon, const QUrl& rootUrl );

        /**
         * \short Lists subdirectories using fetchMore() as needed until the given \p url exists in the model.
         *
         * When the model is used by a treeview, call KDirLister::openUrl with the base url of the tree,
         * then the treeview will take care of calling fetchMore() when the user opens directories.
         * However if you want the tree to show a given URL (i.e. open the tree recursively until that URL),
         * call expandToUrl().
         * Note that this is asynchronous; the necessary listing of subdirectories will take time so
         * the model will not immediately have this url available.
         * The model emits the signal expand() when an index has become available; this can be connected
         * to the treeview in order to let it open that index.
         * \param url the url of a subdirectory of the directory model
         *
         * \sa KDirModel::expandToUrl
         */
        void expandToUrl( const QUrl& url );

    private Q_SLOTS:
        void slotDevicesChanged( K3b::Device::DeviceManager* dm );
        void slotExpand( const QModelIndex& index );

    private:
        class Private;
        Private* const d;
    };
}

#endif
