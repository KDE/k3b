/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BAUDIOPLAYER_H
#define K3BAUDIOPLAYER_H

#include <k3listview.h>

#include <config-k3b.h>
//Added by qt3to4:
#include <QDropEvent>
#include <QLabel>

#ifdef WITH_ARTS
#include <arts/kmedia2.h>
#include <arts/kartsdispatcher.h>
#endif

class QTimer;
class QLabel;
class QToolButton;
class QSlider;
class QPainter;
class QColorGroup;
class QDropEvent;
class Q3DragObject;
class KAction;
class KActionMenu;


/**
 * Special ListViewItem for the AudioPlayer playlist
 * @author Sebastian Trueg
 */
namespace K3b {
class PlayListViewItem : public K3ListViewItem
{
 public:
  PlayListViewItem( const QString&, Q3ListView* parent );
  PlayListViewItem( const QString&, Q3ListView* parent, Q3ListViewItem* after );
  ~PlayListViewItem();

  /** @returns the filename for the first column and the 
   *           length in format 00:00.00 for the second column
   */
  virtual QString text( int c ) const;

  void setLength( unsigned long l ) { m_length = l; }
  unsigned long length() const { return m_length; }
  const QString& filename() const { return m_filename; }

  /**
   * reimplemented from QListViewItem
   * takes the m_bActive flag into account.
   */
  virtual void paintCell( QPainter*, const QColorGroup&, int, int, int );

  void setActive( bool a ) { m_bActive = a; }

 protected:
  /** path to the associated file */
  QString m_filename;

  /** length in frames (1/75 second) */
  unsigned long m_length;

  bool m_bActive;
};
}



/**
 * Playlistview just needed to accept 
 * url drags
 */ 
namespace K3b {
class PlayListView : public K3ListView
{
Q_OBJECT

 public:
  PlayListView( QWidget* parent = 0 );
  ~PlayListView();

 protected:
  bool acceptDrag( QDropEvent* e ) const;
  Q3DragObject* dragObject();
};
}




/**
 * @author Sebastian Trueg
 */
namespace K3b {
class AudioPlayer : public QWidget
{
Q_OBJECT

 public: 
  AudioPlayer( QWidget* parent = 0 );
  ~AudioPlayer();

  bool supportsMimetype( const QString& mimetype );

  /**
   * length of current playing in seconds
   */
  long length();

  /**
   * current position in seconds
   */
  long position();

  /**
   * EMPTY - no file loaded
   */
  enum player_state { PLAYING, PAUSED, STOPPED, EMPTY };

  int state();

  Q_SIGNALS:
  void started( const QString& filename );
  void started();
  void stopped();
  void paused();
  void ended();

 public Q_SLOTS:
  void playFile( const QString& filename );
  void playFiles( const QStringList& files );
  void enqueueFile( const QString& filename );
  void enqueueFiles( const QStringList& files );

  /** clears the playlist */
  void clear();
  void play();
  void forward();
  void back();
  void stop();
  void pause();
  void seek( long pos );
  void seek( int pos );

/*  protected: */
/*   void dragEnterEvent( QDragEnterEvent* e ); */
/*   void dropEvent( QDropEvent* e ); */

 private Q_SLOTS:
  void slotCheckEnd();
  void slotUpdateDisplay();
  void slotUpdateCurrentTime( int time );
  void slotUpdateLength( long time );
  void slotUpdateFilename();
  void slotPlayItem( Q3ListViewItem* item );
  void slotDropped( QDropEvent* e, Q3ListViewItem* after );

  /**
   * set the actual item. Will set m_currentItem and 
   * handle highlighting of the current item
   */
  void setCurrentItem( Q3ListViewItem* item );
  void slotRemoveSelected();
  void slotShowContextMenu( K3ListView*, Q3ListViewItem* item, const QPoint& p );

 private:
#ifdef WITH_ARTS
  Arts::PlayObject m_playObject;
  KArtsDispatcher m_dispatcher;
#endif
  QString m_filename;

  QLabel* m_labelFilename;
  QLabel* m_labelCurrentTime;
  QLabel* m_labelOverallTime;
  
  QToolButton* m_buttonPlay;
  QToolButton* m_buttonPause;
  QToolButton* m_buttonStop;
  QToolButton* m_buttonForward;
  QToolButton* m_buttonBack;
  
  PlayListView* m_viewPlayList;
  
  QSlider* m_seekSlider;
  
  QTimer* m_updateTimer;

  PlayListViewItem* m_currentItem;

  bool m_bLengthReady;

  KAction* m_actionRemove;
  KAction* m_actionClear;
  KActionMenu* m_contextMenu;
};
}


#endif
