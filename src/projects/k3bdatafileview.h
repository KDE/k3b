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

class K3bDataDoc;
class K3bDirItem;
class K3bDataItem;
class K3bView;
namespace K3b {
    class DataProjectModel;
}

/**
 *@author Sebastian Trueg
 */

class K3bDataFileView : public QTreeView
{
    Q_OBJECT

public:
    K3bDataFileView( K3bView*, K3bDataDoc*, QWidget* parent );
    ~K3bDataFileView();
	
    K3bDirItem* currentDir() const;

    /**
     * \return The item at position \p pos (local coordinates)
     * or 0 if there is no item at that position.
     */
    K3bDataItem* itemAt( const QPoint& pos );

    QList<K3bDataItem*> selectedItems() const;

 Q_SIGNALS:
    void dirSelected( K3bDirItem* );
	
public Q_SLOTS:
    void setCurrentDir( K3bDirItem* );

private:
    K3bView* m_view;
    K3bDataDoc* m_doc;

    K3b::DataProjectModel* m_model;
};

#endif
