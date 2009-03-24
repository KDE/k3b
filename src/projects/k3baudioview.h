/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Mello <arthur@mandriva.com>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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


#ifndef K3BAUDIOVIEW_H
#define K3BAUDIOVIEW_H

#include <k3bstandardview.h>
#include "k3baudiotrackview.h"

#include <qstringlist.h>


namespace K3b {

    class AudioDoc;
    class AudioTrack;
    class AudioProjectModel;
    class ViewColumnAdjuster;

    class AudioView : public StandardView
    {
        Q_OBJECT

    public:
        AudioView( AudioDoc* pDoc, QWidget* parent );
        ~AudioView();

        //AudioTrackPlayer* player() const { return m_songlist->player(); }
        AudioTrackPlayer* player() const { return 0; }

        void getSelectedItems( QList<AudioTrack*>& tracks,
                               QList<AudioDataSource*>& sources );
    public Q_SLOTS:
        void addUrls( const KUrl::List& );

    protected:
        ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

        /**
         * reimplemented from \ref K3b::StandardView
         */
        void contextMenuForSelection(const QModelIndexList &selectedIndexes, const QPoint &pos);

        void init();

    private Q_SLOTS:
        void slotAudioConversion();

        // action slots
        void slotAddSilence();
        void slotRemove();
        void slotMergeTracks();
        void slotSplitSource();
        void slotSplitTrack();
        void slotTrackProperties();
        void slotQueryMusicBrainz();
        void slotEditSource();

        void slotAdjustColumns();

    private:
        void setupActions();

        AudioDoc* m_doc;
        K3b::AudioProjectModel* m_model;

        KAction* m_actionProperties;
        KAction* m_actionRemove;
        KAction* m_actionAddSilence;
        KAction* m_actionMergeTracks;
        KAction* m_actionSplitSource;
        KAction* m_actionSplitTrack;
        KAction* m_actionEditSource;
        KAction* m_actionPlayTrack;

        K3b::ViewColumnAdjuster* m_columnAdjuster;
        bool m_updatingColumnWidths;
    };
}

#endif
