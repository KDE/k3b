/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BFILETREEVIEW_H
#define K3BFILETREEVIEW_H


#include <QTreeView>
#include <QPixmap>
#include <QDragEnterEvent>
#include <QDropEvent>

class QPoint;
class QDropEvent;
class QDragEnterEvent;
class KUrl;

namespace K3bDevice {
    class Device;
}




/**
 *@author Sebastian Trueg
 */
class K3bFileTreeView : public QTreeView
{
    Q_OBJECT

public: 
    K3bFileTreeView( QWidget *parent = 0 );
    ~K3bFileTreeView();

    /**
     * returns 0 if no device is selected 
     */
    K3bDevice::Device* selectedDevice() const;

    /** 
     * returnes an empty url if no url is selected
     */
    KUrl selectedUrl() const;

public Q_SLOTS:
    void setSelectedUrl( const KUrl& url );
    void setSelectedDevice( K3bDevice::Device* dev );

Q_SIGNALS:
    void activated( const KUrl& url );
    void activated( K3bDevice::Device* dev );

private Q_SLOTS:
    void slotClicked( const QModelIndex& index );
    void slotExpandUrl( const QModelIndex& index );
    void slotContextMenu( const QPoint& pos );

private:
    class Private;
    Private* const d;
};

#endif
