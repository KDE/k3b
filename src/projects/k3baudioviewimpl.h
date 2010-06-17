/*
 *
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
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

#include <QObject>
#include <QAbstractItemModel>
#include <QList>
#include <KUrl>

class KAction;
class KActionCollection;
class QTreeView;

namespace K3b {
    class AudioDataSource;
    class AudioDoc;
    class AudioProjectModel;
    class AudioTrack;
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

        void addUrls( const KUrl::List& urls );

        AudioProjectModel* model() const { return m_model; }
        QTreeView* view() const { return m_trackView; }

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
        void slotSelectionChanged();
        void slotAudioConversion();
        void slotAdjustColumns();

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
        ViewColumnAdjuster* m_columnAdjuster;
        bool m_updatingColumnWidths;

        KAction* m_actionAddSilence;
        KAction* m_actionMergeTracks;
        KAction* m_actionSplitSource;
        KAction* m_actionSplitTrack;
        KAction* m_actionEditSource;
        KAction* m_actionPlayTrack;
        KAction* m_actionMusicBrainz;
        KAction* m_actionProperties;
        KAction* m_actionRemove;
        KAction* m_conversionAction;
    };

} // namespace K3b

#endif
