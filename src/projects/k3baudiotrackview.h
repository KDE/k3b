/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIO_TRACK_VIEW_H_
#define _K3B_AUDIO_TRACK_VIEW_H_

#include <k3blistview.h>

#include <qmap.h>
#include <qptrlist.h>

class K3bAudioTrack;
class K3bAudioTrackViewItem;
class K3bAudioDataSource;
class K3bAudioDoc;
class KActionCollection;
class KAction;
class QDropEvent;
class QKeyEvent;
class QFocusEvent;
class QMouseEvent;
class QDragMoveEvent;
class QTimer;
class KPopupMenu;
class QPainter;
class K3bListViewItemAnimator;
class K3bAudioTrackPlayer;


class K3bAudioTrackView : public K3bListView
{
  Q_OBJECT

 public:
  K3bAudioTrackView( K3bAudioDoc*, QWidget* parent, const char* name = 0 );
  ~K3bAudioTrackView();

  KActionCollection* actionCollection() const { return m_actionCollection; }

  K3bAudioTrackPlayer* player() const { return m_player; }

  void getSelectedItems( QPtrList<K3bAudioTrack>& tracks, 
			 QPtrList<K3bAudioDataSource>& sources );

 public slots:
  void showPlayerIndicator( K3bAudioTrack* );
  void togglePauseIndicator( bool b );
  void removePlayerIndicator();

 private:
  void setupColumns();
  void setupActions();
  void showAllSources();
  K3bAudioTrackViewItem* findTrackItem( const QPoint& pos ) const;
  K3bAudioTrackViewItem* getTrackViewItem( K3bAudioTrack* track, bool* isNew = 0 );

  K3bAudioDoc* m_doc;

  KAction* m_actionProperties;
  KAction* m_actionRemove;
  KAction* m_actionAddSilence;
  KAction* m_actionMergeTracks;
  KAction* m_actionSplitSource;
  KAction* m_actionSplitTrack;
  KAction* m_actionPlayTrack;
  KActionCollection* m_actionCollection;

  bool m_updatingColumnWidths;

  QMap<K3bAudioTrack*, K3bAudioTrackViewItem*> m_trackItemMap;

  K3bAudioTrackViewItem* m_currentMouseOverItem;
  QTimer* m_autoOpenTrackTimer;
  QTimer* m_animationTimer;

  KPopupMenu* m_popupMenu;

  K3bAudioTrackPlayer* m_player;

  // used for the audiotrackplayer indicator
  K3bAudioTrack* m_currentlyPlayingTrack;

  // to animate the player icon
  K3bListViewItemAnimator* m_playerItemAnimator;

 private slots:
  void slotAnimation();
  void slotDropped( QDropEvent* e, QListViewItem* parent, QListViewItem* after );
  void slotChanged();
  void slotTrackChanged( K3bAudioTrack* );
  void slotTrackRemoved( K3bAudioTrack* );
  void slotDragTimeout();

  // action slots
  void slotAddSilence();
  void slotRemove();
  void slotMergeTracks();
  void slotSplitSource();
  void slotSplitTrack();
  void showPopupMenu( KListView*, QListViewItem* item, const QPoint& pos );
  void slotProperties();
  void slotPlayTrack();

 protected:
  void keyPressEvent( QKeyEvent* e );
  void keyReleaseEvent( QKeyEvent* e );
  void focusOutEvent( QFocusEvent* e );
  void contentsMouseMoveEvent( QMouseEvent* e );
  void contentsDragMoveEvent( QDragMoveEvent* e );
  void contentsDragLeaveEvent( QDragLeaveEvent* e );
  void resizeEvent( QResizeEvent* e );
  void resizeColumns();
  bool acceptDrag(QDropEvent* e) const;
  QDragObject* dragObject();
};

#endif
