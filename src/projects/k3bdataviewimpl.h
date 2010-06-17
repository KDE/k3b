/*
 *
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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

#ifndef K3B_DATA_VIEW_IMPL_H
#define K3B_DATA_VIEW_IMPL_H

#include <QObject>
#include <QAbstractItemModel>
#include <KUrl>

class QTreeView;

class KAction;
class KActionCollection;
class KMenu;

namespace K3b {
    class DataDoc;
    class DataProjectModel;
    class View;

    /**
     * This class was created to share code and behaviour between \ref K3b::DataView and \ref K3b::MixedView.
     */
    class DataViewImpl : public QObject
    {
        Q_OBJECT

    public:
        DataViewImpl( View* view, DataDoc* doc, KActionCollection* actionCollection );

        void addUrls( const QModelIndex& parent, const KUrl::List& urls );

        DataProjectModel* model() const { return m_model; }
        QTreeView* view() const { return m_fileView; }

    signals:
        void setCurrentRoot( const QModelIndex& index );

    public Q_SLOTS:
        void slotCurrentRootChanged( const QModelIndex& newRoot );

    private Q_SLOTS:
        void slotNewDir();
        void slotRemove();
        void slotRename();
        void slotProperties();
        void slotOpen();
        void slotSelectionChanged();
        void slotItemActivated( const QModelIndex& index );
        void slotImportSession();
        void slotClearImportedSession();
        void slotEditBootImages();
        void slotImportedSessionChanged( int importedSession );

    private:
        View* m_view;
        DataDoc* m_doc;
        DataProjectModel* m_model;
        QTreeView* m_fileView;

        KMenu* m_popupMenu;
        KAction* m_actionParentDir;
        KAction* m_actionRemove;
        KAction* m_actionRename;
        KAction* m_actionNewDir;
        KAction* m_actionProperties;
        KAction* m_actionOpen;
        KAction* m_actionImportSession;
        KAction* m_actionClearSession;
        KAction* m_actionEditBootImages;
    };

} // namespace K3b

#endif
