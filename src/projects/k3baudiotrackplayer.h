/* 
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_TRACK_PLAYER_H_
#define _K3B_AUDIO_TRACK_PLAYER_H_

#include <qobject.h>

#include <k3baudioclient.h>

#include <k3bmsf.h>

class K3bAudioDoc;
class K3bAudioTrack;
class KAction;


class K3bAudioTrackPlayer : public QObject, public K3bAudioClient
{
  Q_OBJECT

 public:
  K3bAudioTrackPlayer( K3bAudioDoc* doc, QObject* parent = 0 );
  ~K3bAudioTrackPlayer();

  K3bAudioTrack* currentPlayingTrack() const { return m_currentTrack; }
  const K3b::Msf& currentPosition() const { return m_currentPosition; }

  enum Actions {
    ACTION_PLAY,
    ACTION_PAUSE,
    ACTION_PLAY_PAUSE,
    ACTION_STOP,
    ACTION_NEXT,
    ACTION_PREV,
    ACTION_SEEK
  };

  KAction* action( int action ) const;

  /**
   * Reimplemented from K3bAudioClient
   */
  int read( char* data, int maxlen );

 public slots:
  void playTrack( K3bAudioTrack* );
  void playPause();
  void stop();
  void next();
  void prev();
  void seek( const K3b::Msf& );

 signals:
  void playingTrack( K3bAudioTrack* );
  void paused( bool paused );  
  void stopped();

 private slots:
  void slotSeek( int );
  void slotTrackChanged( K3bAudioTrack* track );
  void slotTrackRemoved( K3bAudioTrack* track );
  void slotUpdateSlider();
  void slotDocChanged();

 private:
  K3bAudioDoc* m_doc;
  K3bAudioTrack* m_currentTrack;
  K3b::Msf m_currentPosition;

  class Private;
  Private* d;
};


class K3bAudioTrackPlayerSeekAction : public K3bWidgetFactoryAction
{
 public:
    K3bAudioTrackPlayerSeekAction( K3bAudioTrackPlayer* player, QObject* parent );
    ~K3bAudioTrackPlayerSeekAction();

    void setValue( int v );
    void setMaxValue( int v );

 protected:
    QWidget* createWidget( QWidget* container);

 private:
    K3bAudioTrackPlayer* m_player;
};

#endif
