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

#ifndef _K3B_AUDIO_SERVER_H_
#define _K3B_AUDIO_SERVER_H_

#include <qobject.h>

class K3bAudioClient;


/**
 * Interface for the K3b Audio Servers. It's implementations (like
 * the K3bArtsAudioServer) take care of playing 4100 Hz 16bit stereo audio
 * data which is provided by a K3bAudioClient
 */
class K3bAudioServer : public QObject
{
  Q_OBJECT

 public:
  virtual ~K3bAudioServer() {
  }

 public slots:
  /**
   * Start playing the clients data. It's up to the server if older
   * clients are suspended, stopped or mixed into a single stream.
   */
  virtual void attachClient( K3bAudioClient* ) = 0;
  virtual void detachClient( K3bAudioClient* ) = 0;

 protected:
  K3bAudioServer( QObject* parent = 0, const char* name = 0 )
    : QObject( parent, name ) {
  }
};

#endif
