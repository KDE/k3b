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

#include "k3bstandardview.h"

class QLineEdit;
class KMenu;
class KAction;
class QModelIndex;

namespace K3b {
    class DataDoc;
    class DirItem;
    class DataItem;
    class DataProjectModel;

    namespace Device {
        class Device;
    }

    class DataView : public StandardView
    {
        Q_OBJECT

    public:
        DataView(DataDoc* doc, QWidget *parent=0);
        virtual ~DataView();

    public Q_SLOTS:
        void slotBurn();
        void importSession();
        void clearImportedSession();
        void editBootImages();

        void slotDocChanged();

        void addUrls( const KUrl::List& );

    private Q_SLOTS:
        void slotNewDir();
        void slotItemProperties();
        void slotOpen();

    protected:
        QLineEdit* m_volumeIDEdit;

        virtual ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

        /**
         * reimplemented from @ref StandardView
         */
        virtual void contextMenuForSelection(const QModelIndexList &selectedIndexes, const QPoint &pos);

    private:
        DataDoc* m_doc;
        K3b::DataProjectModel* m_model;

        void setupContextMenu();

        KMenu* m_popupMenu;
        KAction* m_actionParentDir;
        KAction* m_actionRemove;
        KAction* m_actionRename;
        KAction* m_actionNewDir;
        KAction* m_actionProperties;
        KAction* m_actionOpen;
        QModelIndexList m_currentSelection;

        // used for mounting when importing old session
        Device::Device* m_device;
    };
}

#endif
