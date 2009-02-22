/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIO_TRACK_VIEW_H_
#define _K3B_AUDIO_TRACK_VIEW_H_

#include <QtGui/QTreeView>

#include <qmap.h>
#include <QList>
//Added by qt3to4:
#include <QResizeEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QDragLeaveEvent>
#include <kurl.h>

namespace K3b {
    class AudioTrack;
}
namespace K3b {
    class AudioTrackViewItem;
}
namespace K3b {
    class AudioDataSource;
}
namespace K3b {
    class AudioDoc;
}
class KActionCollection;
class KAction;
class QDropEvent;
class QKeyEvent;
class QFocusEvent;
class QMouseEvent;
class QDragMoveEvent;
class QTimer;
namespace K3b {
    class ListViewItemAnimator;
}
namespace K3b {
    class AudioTrackPlayer;
}
namespace K3b {
    class AudioProjectModel;
    class ViewColumnAdjuster;
}


namespace K3b {
    class AudioTrackView : public QTreeView
    {
        Q_OBJECT

    public:
        AudioTrackView( AudioDoc*, QWidget* parent );
        ~AudioTrackView();

        KActionCollection* actionCollection() const { return m_actionCollection; }

        AudioTrackPlayer* player() const { return m_player; }

        void getSelectedItems( QList<AudioTrack*>& tracks,
                               QList<AudioDataSource*>& sources );

    public Q_SLOTS:
        void showPlayerIndicator( K3b::AudioTrack* );
        void togglePauseIndicator( bool b );
        void removePlayerIndicator();

    private:
        void setupColumns();
        void setupActions();
        void showAllSources();
/*     AudioTrackViewItem* findTrackItem( const QPoint& pos ) const; */
/*     AudioTrackViewItem* getTrackViewItem( AudioTrack* track, bool* isNew = 0 ); */

        AudioDoc* m_doc;

        KAction* m_actionProperties;
        KAction* m_actionRemove;
        KAction* m_actionAddSilence;
        KAction* m_actionMergeTracks;
        KAction* m_actionSplitSource;
        KAction* m_actionSplitTrack;
        KAction* m_actionEditSource;
        KAction* m_actionPlayTrack;
        KActionCollection* m_actionCollection;

        bool m_updatingColumnWidths;

/*     QMap<AudioTrack*, AudioTrackViewItem*> m_trackItemMap; */

/*     AudioTrackViewItem* m_currentMouseOverItem; */
        QTimer* m_autoOpenTrackTimer;
        QTimer* m_animationTimer;

        AudioTrackPlayer* m_player;

        // used for the audiotrackplayer indicator
        AudioTrack* m_currentlyPlayingTrack;

        // to animate the player icon
/*     ListViewItemAnimator* m_playerItemAnimator; */

        // used for the drop-event hack
        KUrl::List m_dropUrls;
        AudioTrack* m_dropTrackAfter;
        AudioTrack* m_dropTrackParent;
        AudioDataSource* m_dropSourceAfter;

        K3b::AudioProjectModel* m_model;
        K3b::ViewColumnAdjuster* m_columnAdjuster;

    private Q_SLOTS:
/*     void slotAnimation(); */
/*     void slotDropped( QDropEvent* e, Q3ListViewItem* parent, Q3ListViewItem* after ); */
        void slotTrackChanged( AudioTrack* );
/*     void slotTrackRemoved( AudioTrack* ); */
/*     void slotDragTimeout(); */
        void slotAdjustColumns();

        // action slots
        void slotAddSilence();
        void slotRemove();
        void slotMergeTracks();
        void slotSplitSource();
        void slotSplitTrack();
        void showPopupMenu( const QPoint& pos );
        void slotProperties();
        void slotPlayTrack();
        void slotQueryMusicBrainz();
        void slotEditSource();

    protected:
        void dragEnterEvent( QDragEnterEvent* event );
        void dragLeaveEvent( QDragLeaveEvent* event );
        void dragMoveEvent( QDragMoveEvent* event );

/*     void keyPressEvent( QKeyEvent* e ); */
/*     void keyReleaseEvent( QKeyEvent* e ); */
/*     void focusOutEvent( QFocusEvent* e ); */
/*     void contentsMouseMoveEvent( QMouseEvent* e ); */
/*     void contentsDragMoveEvent( QDragMoveEvent* e ); */
/*     void contentsDragLeaveEvent( QDragLeaveEvent* e ); */
/*     void resizeEvent( QResizeEvent* e ); */
/*     bool acceptDrag(QDropEvent* e) const; */
/*     Q3DragObject* dragObject(); */
    };
}

#endif
