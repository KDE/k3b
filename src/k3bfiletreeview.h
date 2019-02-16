/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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


#ifndef K3BFILETREEVIEW_H
#define K3BFILETREEVIEW_H

#include <QUrl>
#include <QTreeView>

class QPoint;

namespace K3b {
    namespace Device {
        class Device;
    }

    class FileTreeView : public QTreeView
    {
        Q_OBJECT

    public:
        explicit FileTreeView( QWidget *parent = 0 );
        ~FileTreeView() override;

        /**
         * returns 0 if no device is selected
         */
        Device::Device* selectedDevice() const;

        /**
         * returnes an empty url if no url is selected
         */
        QUrl selectedUrl() const;

    public Q_SLOTS:
        void setSelectedUrl( const QUrl& url );
        void setSelectedDevice( K3b::Device::Device* dev );

    Q_SIGNALS:
        void activated( const QUrl& url );
        void activated( K3b::Device::Device* dev );

    private Q_SLOTS:
        void slotClicked( const QModelIndex& index );
        void slotExpandUrl( const QModelIndex& index );
        void slotContextMenu( const QPoint& pos );
        void slotAddFilesToProject();

    private:
        void initActions();
        class Private;
        Private* const d;
    };
}

#endif
