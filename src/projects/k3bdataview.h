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


#ifndef K3BDATAVIEW_H
#define K3BDATAVIEW_H

#include <k3bview.h>

class K3bDataDoc;
class K3bDirItem;
class K3bDataItem;
class K3bDataDirTreeView;
class K3bDataFileView;
class QLineEdit;
class KMenu;
class KAction;
class QModelIndex;

namespace K3bDevice {
    class Device;
}



/**
 *@author Sebastian Trueg
 */
class K3bDataView : public K3bView
{
    Q_OBJECT

public:
    K3bDataView(K3bDataDoc* doc, QWidget *parent=0);
    virtual ~K3bDataView();
	
    K3bDirItem* currentDir() const;

public Q_SLOTS:
    void setCurrentDir( K3bDirItem* );
    void slotBurn();
    void importSession();
    void clearImportedSession();
    void editBootImages();

    void slotDocChanged();

    void addUrls( const KUrl::List& );

private Q_SLOTS:
    void showPopupMenu( const QPoint& );
    void slotRenameItem();
    void slotRemoveItem();
    void slotNewDir();
    void slotParentDir();
    void slotItemProperties();
    void slotOpen();
    void slotDoubleClicked( const QModelIndex& );

protected:
    K3bDataDirTreeView* m_dataDirTree;
    K3bDataFileView* m_dataFileView;
    QLineEdit* m_volumeIDEdit;

    virtual K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );
		
private:
    K3bDataDoc* m_doc;

    void setupContextMenu();

    /**
     * \return a list of the selected items in the currently
     * active view.
     */
    QList<K3bDataItem*> selectedItems() const;

    KMenu* m_popupMenu;
    KAction* m_actionParentDir;
    KAction* m_actionRemove;
    KAction* m_actionRename;
    KAction* m_actionNewDir;
    KAction* m_actionProperties;
    KAction* m_actionOpen;

    // used for mounting when importing old session
    K3bDevice::Device* m_device;

    // used for the context menu (to distinguish between the dir and file view)
    bool m_contextMenuOnTreeView;
};


#endif
