/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
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

#ifndef K3BVCDLISTVIEW_H
#define K3BVCDLISTVIEW_H


#include "k3blistview.h"

#include <qmap.h>
//Added by qt3to4:
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QList>

class QDragEnterEvent;
class Q3DragObject;
class QDropEvent;
class KMenu;
class KAction;
namespace K3b {
    class VcdDoc;
}
namespace K3b {
    class View;
}
namespace K3b {
    class VcdTrack;
}
class KActionCollection;
namespace K3b {
    class VcdListViewItem;
}


namespace K3b {
class VcdListView : public ListView
{
        Q_OBJECT

    public:
        VcdListView( View*, VcdDoc*, QWidget *parent = 0 );
        ~VcdListView();

        /**
         * reimplemented from K3ListView
         */
        void insertItem( Q3ListViewItem* );

        KActionCollection* actionCollection() const
        {
            return m_actionCollection;
        }

        QList<VcdTrack*> selectedTracks();

     Q_SIGNALS:
        void lengthReady();

    private:
        void setupColumns();
        void setupPopupMenu();
        void setupActions();

        VcdDoc* m_doc;
        View* m_view;

        KAction* m_actionProperties;
        KAction* m_actionRemove;
        KActionCollection* m_actionCollection;

        KMenu* m_popupMenu;

        QMap<VcdTrack*, VcdListViewItem*> m_itemMap;

    private Q_SLOTS:
        void slotDropped( K3ListView*, QDropEvent* e, Q3ListViewItem* after );
        void slotUpdateItems();
        void showPopupMenu( K3ListView*, Q3ListViewItem* item, const QPoint& );
        void showPropertiesDialog();
        void slotRemoveTracks();
        void slotTrackRemoved( VcdTrack* );

    protected:
        bool acceptDrag( QDropEvent* e ) const;
        Q3DragObject* dragObject();
};
}

#endif
