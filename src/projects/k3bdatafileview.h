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


#ifndef K3BDATAFILEVIEW_H
#define K3BDATAFILEVIEW_H

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
namespace K3b {
    class DataProjectModel;
}

/**
 *@author Sebastian Trueg
 */

namespace K3b {
class DataFileView : public QTreeView
{
    Q_OBJECT

public:
    DataFileView( View*, DataDoc*, QWidget* parent );
    ~DataFileView();
	
    DirItem* currentDir() const;

    /**
     * \return The item at position \p pos (local coordinates)
     * or 0 if there is no item at that position.
     */
    DataItem* itemAt( const QPoint& pos );

    QList<DataItem*> selectedItems() const;

Q_SIGNALS:
    void dirSelected( DirItem* );
	
public Q_SLOTS:
    void setCurrentDir( DirItem* );

private:
    virtual void rowsInserted( const QModelIndex& parent, int begin, int end );

private:
    View* m_view;
    DataDoc* m_doc;

    K3b::DataProjectModel* m_model;
};
}

#endif
