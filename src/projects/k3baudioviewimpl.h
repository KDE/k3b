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
class KMenu;

namespace K3b {
    class AudioDataSource;
    class AudioDoc;
    class AudioProjectModel;
    class AudioTrack;
    class View;
    
    /**
     * This class was created to share code and behaviour between \ref K3b::AudioView and \ref K3b::MixedView.
     */
    class AudioViewImpl : public QObject
    {
        Q_OBJECT
        
    public:
        AudioViewImpl( View* view, AudioDoc* doc, AudioProjectModel* model, KActionCollection* actionCollection );
        
        void addUrls( const KUrl::List& urls );
        void remove( const QModelIndexList& indexes );
        void addSilence( const QModelIndexList& indexes );
        void mergeTracks( const QModelIndexList& indexes );
        void splitSource( const QModelIndexList& indexes );
        void splitTrack( const QModelIndexList& indexes );
        void editSource( const QModelIndexList& indexes );
        void properties( const QModelIndexList& indexes );
        void queryMusicBrainz( const QModelIndexList& indexes );
        
        KMenu* popupMenu() const { return m_popupMenu; }
        
    public Q_SLOTS:
        void slotSelectionChanged( const QModelIndexList& indexes );
        void slotItemActivated( const QModelIndex& index );
    
        private Q_SLOTS:
        void slotAudioConversion();
        
    private:
        void tracksForIndexes( QList<AudioTrack*>& tracks,
                               const QModelIndexList& indexes ) const;
        void sourcesForIndexes( QList<AudioDataSource*>& sources,
                                const QModelIndexList& indexes ) const;
        
    private:
        View* m_view;
        AudioDoc* m_doc;
        AudioProjectModel* m_model;
        
        KMenu* m_popupMenu;
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
