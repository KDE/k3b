/***************************************************************************
                          k3baudioplayer.h  -  description
                             -------------------
    begin                : Sun Feb 10 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BAUDIOPLAYER_H
#define K3BAUDIOPLAYER_H

#include <klistview.h>
#include <arts/kmedia2.h>
#include <arts/kartsdispatcher.h>


class QTimer;
class QLabel;
class QToolButton;
class QSlider;
class QPainter;
class QColorGroup;
class QDropEvent;
class QDragObject;
class KAction;
class KActionMenu;


/**
 * Special ListViewItem for the K3bAudioPlayer playlist
 * @author Sebastian Trueg
 */
class K3bPlayListViewItem : public KListViewItem
{
 public:
  K3bPlayListViewItem( const QString&, QListView* parent );
  K3bPlayListViewItem( const QString&, QListView* parent, QListViewItem* after );
  ~K3bPlayListViewItem();

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



/**
 * Playlistview just needed to accept 
 * url drags
 */ 
class K3bPlayListView : public KListView
{
Q_OBJECT

 public:
  K3bPlayListView( QWidget* parent = 0, const char* name = 0 );
  ~K3bPlayListView();

 protected:
  bool acceptDrag( QDropEvent* e ) const;
  QDragObject* dragObject();
};




/**
 * @author Sebastian Trueg
 */
class K3bAudioPlayer : public QWidget
{
Q_OBJECT

 public: 
  K3bAudioPlayer( QWidget* parent = 0, const char* name = 0 );
  ~K3bAudioPlayer();

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

 signals:
  void started( const QString& filename );
  void started();
  void stopped();
  void paused();
  void ended();

 public slots:
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

 private slots:
  void slotCheckEnd();
  void slotUpdateDisplay();
  void slotUpdateCurrentTime( int time );
  void slotUpdateLength( long time );
  void slotUpdateFilename();
  void slotPlayItem( QListViewItem* item );
  void slotDropped( QDropEvent* e, QListViewItem* after );

  /**
   * set the actual item. Will set m_currentItem and 
   * handle highlighting of the current item
   */
  void setCurrentItem( QListViewItem* item );
  void slotRemoveSelected();
  void slotShowContextMenu( KListView*, QListViewItem* item, const QPoint& p );

 private:
  Arts::PlayObject m_playObject;
  KArtsDispatcher m_dispatcher;

  QString m_filename;

  QLabel* m_labelFilename;
  QLabel* m_labelCurrentTime;
  QLabel* m_labelOverallTime;
  
  QToolButton* m_buttonPlay;
  QToolButton* m_buttonPause;
  QToolButton* m_buttonStop;
  QToolButton* m_buttonForward;
  QToolButton* m_buttonBack;
  
  K3bPlayListView* m_viewPlayList;
  
  QSlider* m_seekSlider;
  
  QTimer* m_updateTimer;

  K3bPlayListViewItem* m_currentItem;

  bool m_bLengthReady;

  KAction* m_actionRemove;
  KAction* m_actionClear;
  KActionMenu* m_contextMenu;
};


#endif
