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

class K3bAudioOutputPlugin;
class K3bAudioClient;


/**
 * The AudioServer manages AudioClients to play audio data through
 * some output plugin.
 */
class K3bAudioServer : public QObject
{
  Q_OBJECT

 public:
  K3bAudioServer( QObject* parent = 0, const char* name = 0 );
  ~K3bAudioServer();

  /**
   * Returns false in case the named ouput method could not be found.
   */
  bool setOutputMethod( const QCString& name );
  void setOutputPlugin( K3bAudioOutputPlugin* p );

  /**
   * Start playing the clients data. It's up to the server if older
   * clients are suspended, stopped or mixed into a single stream.
   *
   * This is called by K3bAudioClient
   */
  void attachClient( K3bAudioClient* );

  /**
   * Stop streaming data from the client.
   * This is called by K3bAudioClient
   */
  void detachClient( K3bAudioClient* );

  /**
   * We need to be able to play data from everywhere in K3b.
   */
  static K3bAudioServer* instance() { return s_instance; }

  /**
   * Find a plugin by classname.
   */
  static K3bAudioOutputPlugin* findOutputPlugin( const QCString& name );

 private:
  static K3bAudioServer* s_instance;

  K3bAudioOutputPlugin* m_usedOutputPlugin;
  bool m_pluginInitialized;
  K3bAudioClient* m_client;

  class Private;
  Private* d;
};

#endif
