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

#ifndef _K3B_ARTS_AUDIO_OUTPUT_H_
#define _K3B_ARTS_AUDIO_OUTPUT_H_

#include <k3baudiooutputplugin.h>

#include <artsc/artsc.h>


class K3bArtsOutputPlugin : public K3bAudioOutputPlugin
{
 public:
  K3bArtsOutputPlugin( QObject* parent = 0, const char* name = 0 );
  ~K3bArtsOutputPlugin();

  int pluginSystemVersion() const { return 3; }
  QCString soundSystem() const { return "arts"; }

  bool init();
  void cleanup();

  QString lastErrorMessage() const;

  int write( char* data, int len );

 private:
  bool m_initialized;
  int m_lastErrorCode;

  arts_stream_t m_stream;
};

#endif
