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

#include <qobject.h>
#include <arts/kmedia2.h>


class QTimer;


/**
  *@author Sebastian Trueg
  */
class K3bAudioPlayer : public QObject
{
Q_OBJECT

 public: 
  K3bAudioPlayer( QObject* parent = 0, const char* name = 0 );
  ~K3bAudioPlayer();

  bool supportsMimetype( const QString& mimetype );
  bool playFile( const QString& );

  const QString& filename() const { return m_filename; }

  /**
   * length in seconds
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
  void play();
  void stop();
  void pause();
  void seek( long pos );
  void seek( int pos );

 private slots:
  void slotCheckEnd();

 private:
  Arts::PlayObject m_playObject;
  Arts::Dispatcher m_dispatcher;

  QString m_filename;

  QTimer* m_endTimer;
};


#endif
