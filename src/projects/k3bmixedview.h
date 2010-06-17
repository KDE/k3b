/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

#ifndef K3B_MIXED_VIEW_H
#define K3B_MIXED_VIEW_H

#include "k3bview.h"
#include <QList>

class QAbstractItemModel;
class QAction;
class QModelIndex;
class QStackedWidget;
class QTreeView;

namespace K3b {
    class AudioTrack;
    class AudioTrackPlayer;
    class AudioDataSource;
    class AudioViewImpl;
    class DataViewImpl;
    class DirProxyModel;
    class MixedDoc;
    class MetaItemModel;

    class MixedView : public View
    {
        Q_OBJECT

    public:
        MixedView( MixedDoc* doc, QWidget* parent = 0 );
        ~MixedView();

    public Q_SLOTS:
        virtual void slotBurn();
        virtual void addUrls( const KUrl::List& urls );

    private Q_SLOTS:
        void slotParentDir();
        void slotCurrentDirChanged();
        void slotSetCurrentRoot( const QModelIndex& index );

    protected:
        /**
         * reimplemented from @ref View
         */
        virtual ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

    private:
        MixedDoc* m_doc;
        AudioViewImpl* m_audioViewImpl;
        DataViewImpl* m_dataViewImpl;
        MetaItemModel* m_model;
        DirProxyModel* m_dirProxy;
        QTreeView* m_dirView;
        QStackedWidget* m_fileViewWidget;
        QList<QAction*> m_audioActions;
        QList<QAction*> m_dataActions;
    };
}

#endif
