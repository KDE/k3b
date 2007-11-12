/* 
 *
 * $Id$
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

#ifndef _K3B_AUDIO_OUTPUTPLUGIN_H_
#define _K3B_AUDIO_OUTPUTPLUGIN_H_

#include <k3bplugin.h>
#include "k3b_export.h"
//Added by qt3to4:
#include <Q3CString>

/**
 *
 */
class LIBK3B_EXPORT K3bAudioOutputPlugin : public K3bPlugin
{
  Q_OBJECT

 public:
  virtual ~K3bAudioOutputPlugin() {
  }

  QString group() const { return "AudioOutput"; }

  /**
   * This is the short name of the sound system which can be used
   * to specify the sound system on the command line (like "arts", "alsa", or "oss")
   */
  virtual Q3CString soundSystem() const = 0;

  /**
   * Initialize the plugin.
   *
   * Return true on success.
   * In case of a failure report the error through lastErrorMessage
   */
  virtual bool init() { return true; }

  /**
   * Cleanup the plugin. This is the counterpart to init()
   */
  virtual void cleanup() {}

  virtual QString lastErrorMessage() const { return QString::null; }

  /**
   * Let there be sound...
   *
   * @returns number of written bytes or -1 on error.
   */
  virtual int write( char* data, int len ) = 0;

 protected:
  K3bAudioOutputPlugin( QObject* parent = 0, const char* name = 0 )
    : K3bPlugin( parent, name ) {
  }
};

#endif
