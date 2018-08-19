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

#ifndef K3B_AUDIO_VIEW_IMPL_H
#define K3B_AUDIO_VIEW_IMPL_H

#include <QAbstractItemModel>
#include <QObject>
#include <QList>
#include <QUrl>

class QAction;
class KActionCollection;
class QTreeView;

namespace K3b {
    class AudioDataSource;
    class AudioDoc;
    class AudioProjectDelegate;
    class AudioProjectModel;
    class AudioTrack;
    class AudioTrackPlayer;
    class View;
    class ViewColumnAdjuster;

    /**
     * This class was created to share code and behaviour between \ref K3b::AudioView and \ref K3b::MixedView.
     */
    class AudioViewImpl : public QObject
    {
        Q_OBJECT

    public:
        AudioViewImpl( View* view, AudioDoc* doc, KActionCollection* actionCollection );

        void addUrls( const QList<QUrl>& urls );

        AudioProjectModel* model() const { return m_model; }
        QTreeView* view() const { return m_trackView; }

        AudioTrackPlayer* player() const { return m_player; }

    private Q_SLOTS:
        void slotRemove();
        void slotAddSilence();
        void slotMergeTracks();
        void slotSplitSource();
        void slotSplitTrack();
        void slotEditSource();
        void slotTrackProperties();
        void slotPlayTrack();
        void slotQueryMusicBrainz();
        void slotQueryMusicBrainzTrack();
        void slotSelectionChanged();
        void slotAudioConversion();
        void slotAdjustColumns();
        void slotPlayingTrack( const K3b::AudioTrack& track );
        void slotPlayerStateChanged();

    private:
        void tracksForIndexes( QList<AudioTrack*>& tracks,
                               const QModelIndexList& indexes ) const;
        void sourcesForIndexes( QList<AudioDataSource*>& sources,
                                const QModelIndexList& indexes ) const;

    private:
        View* m_view;
        AudioDoc* m_doc;
        AudioProjectModel* m_model;
        QTreeView* m_trackView;
        AudioProjectDelegate* m_delegate;
        AudioTrackPlayer* m_player;
        ViewColumnAdjuster* m_columnAdjuster;
        bool m_updatingColumnWidths;

		QAction* m_actionAddSilence;
        QAction* m_actionMergeTracks;
        QAction* m_actionSplitSource;
        QAction* m_actionSplitTrack;
        QAction* m_actionEditSource;
        QAction* m_actionPlayTrack;
        QAction* m_actionQueryMusicBrainz;
        QAction* m_actionQueryMusicBrainzTrack;
        QAction* m_actionProperties;
        QAction* m_actionRemove;
        QAction* m_conversionAction;
    };

} // namespace K3b

#endif
