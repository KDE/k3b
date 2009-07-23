/* 
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2009      Michal Malek <michalm@jabster.pl>
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

#include "k3bstandardview.h"

class QAbstractItemModel;
class QModelIndex;

namespace K3b {
    class AudioTrack;
    class AudioTrackPlayer;
    class AudioDataSource;
    class AudioViewImpl;
    class DataViewImpl;
    class MixedDoc;
    class MixedProjectModel;
    
    class MixedView : public StandardView
    {
        Q_OBJECT

    public:
        MixedView( MixedDoc* doc, QWidget* parent = 0 );
        ~MixedView();

        AudioTrackPlayer* player() const;

    public Q_SLOTS:
        void slotBurn();
        void addUrls( const KUrl::List& );

    private Q_SLOTS:
        void slotAddSilence();
        void slotRemove();
        void slotMergeTracks();
        void slotSplitSource();
        void slotSplitTrack();
        void slotEditSource();
        void slotQueryMusicBrainz();
        void slotNewDir();
        void slotItemProperties();
        void slotOpen();
        void slotCurrentRootChanged( const QModelIndex& newRoot );
        void slotItemActivated( const QModelIndex& index );
        void slotSetCurrentRoot( const QModelIndex& index );

    protected:
        QAbstractItemModel* currentSubModel() const;
        
        /**
         * reimplemented from @ref View
         */
        virtual ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

        /**
         * reimplemented from @ref StandardView
         */
        virtual void selectionChanged( const QModelIndexList& indexes );
        virtual void contextMenu( const QPoint& pos );

    private:
        void mapToSubModel( QModelIndexList& subIndexes, const QModelIndexList& indexes ) const;
        
        MixedDoc* m_doc;
        MixedProjectModel* m_model;
        AudioViewImpl* m_audioViewImpl;
        DataViewImpl* m_dataViewImpl;
    };
}

#endif
