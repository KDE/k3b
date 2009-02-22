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

namespace K3b {
    class AudioDoc;
}
namespace K3b {
    class AudioTrack;
}
class KAction;


namespace K3b {
class AudioTrackPlayer : public QObject, public AudioClient
{
  Q_OBJECT

 public:
  AudioTrackPlayer( AudioDoc* doc, QObject* parent = 0 );
  ~AudioTrackPlayer();

  AudioTrack* currentPlayingTrack() const { return m_currentTrack; }
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
   * Reimplemented from AudioClient
   */
  int read( char* data, int maxlen );

 public Q_SLOTS:
  void playTrack( AudioTrack* );
  void playPause();
  void stop();
  void next();
  void prev();
  void seek( const K3b::Msf& );

  Q_SIGNALS:
  void playingTrack( AudioTrack* );
  void paused( bool paused );  
  void stopped();

 private Q_SLOTS:
  void slotSeek( int );
  void slotTrackChanged( AudioTrack* track );
  void slotTrackRemoved( AudioTrack* track );
  void slotUpdateSlider();
  void slotDocChanged();

 private:
  AudioDoc* m_doc;
  AudioTrack* m_currentTrack;
  K3b::Msf m_currentPosition;

  class Private;
  Private* d;
};
}

#if 0
namespace K3b {
class AudioTrackPlayerSeekAction : public WidgetFactoryAction
{
 public:
    AudioTrackPlayerSeekAction( AudioTrackPlayer* player, QObject* parent );
    ~AudioTrackPlayerSeekAction();

    void setValue( int v );
    void setMaxValue( int v );

 protected:
    QWidget* createWidget( QWidget* container);

 private:
    AudioTrackPlayer* m_player;
};
}
#endif

#endif
