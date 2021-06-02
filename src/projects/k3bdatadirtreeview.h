/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef K3BDATADIRTREEVIEW_H
#define K3BDATADIRTREEVIEW_H

#include <QUrl>
#include <QTreeView>


class QItemSelection;

/**
 *@author Sebastian Trueg
 */

namespace K3b {
    class DataDoc;
    class DataItem;
    class DirItem;
    class View;

    class DataDirTreeView : public QTreeView
    {
        Q_OBJECT

    public:
        DataDirTreeView( View*, DataDoc*, QWidget* parent );
        ~DataDirTreeView() override;

        /**
         * \return The item at position \p pos (local coordinates)
         * or 0 if there is no item at that position.
         */
        DataItem* itemAt( const QPoint& pos );

        DirItem* selectedDir() const;

    public Q_SLOTS:
        void setCurrentDir( K3b::DirItem* );

    Q_SIGNALS:
        void dirSelected( K3b::DirItem* );

    private Q_SLOTS:
        void slotSelectionChanged( const QItemSelection& selected, const QItemSelection& );
        void slotAddUrlsRequested( QList<QUrl> urls, K3b::DirItem* targetDir );
        void slotMoveItemsRequested( QList<K3b::DataItem*> items, K3b::DirItem* targetDir );

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
