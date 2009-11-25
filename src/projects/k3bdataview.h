/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Michal Malek <michalm@jabster.pl>
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

class QModelIndex;
class KAction;
class KMenu;

namespace K3b {
    class DataDoc;
    class DirItem;
    class DataItem;
    class DataProjectModel;
    class DataViewImpl;

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

        void addUrls( const KUrl::List& );

    private Q_SLOTS:
        void slotNewDir();
        void slotItemProperties();
        void slotOpen();

    protected:
        virtual ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

        /**
         * reimplemented from @ref StandardView
         */
        virtual void selectionChanged( const QModelIndexList& indexes );
        virtual void contextMenu( const QPoint& pos );

    private:
        void setupActions();
        
        DataDoc* m_doc;
        K3b::DataProjectModel* m_model;

        DataViewImpl* m_dataViewImpl;

        // used for mounting when importing old session
        Device::Device* m_device;
    };
}

#endif
