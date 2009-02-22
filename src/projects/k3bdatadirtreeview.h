/* 
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BDATADIRTREEVIEW_H
#define K3BDATADIRTREEVIEW_H

#include <QtGui/QTreeView>


namespace K3b {
    class DataDoc;
}
namespace K3b {
    class DirItem;
}
namespace K3b {
    class DataItem;
}
namespace K3b {
    class View;
}
class QItemSelection;

/**
 *@author Sebastian Trueg
 */

namespace K3b {
class DataDirTreeView : public QTreeView
{
    Q_OBJECT

public:
    DataDirTreeView( View*, DataDoc*, QWidget* parent );
    virtual ~DataDirTreeView();

    /**
     * \return The item at position \p pos (local coordinates)
     * or 0 if there is no item at that position.
     */
    DataItem* itemAt( const QPoint& pos );

    DirItem* selectedDir() const;

public Q_SLOTS:
    void setCurrentDir( DirItem* );

Q_SIGNALS:
    void dirSelected( DirItem* );

private Q_SLOTS:
    void slotSelectionChanged( const QItemSelection& selected, const QItemSelection& );

private:
    void startDropAnimation( DirItem* );
    void stopDropAnimation();

    View* m_view;

    DataDoc* m_doc;

    class Private;
    Private* d;
};
}

#endif
