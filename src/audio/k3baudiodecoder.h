/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_AUDIO_DECODER_H
#define K3B_AUDIO_DECODER_H

#include <k3bthreadjob.h>

class K3bAudioDoc;


/**
 * Decodes all tracks of an audio project into one raw audio data stream.
 * In the future it will also take care of the filtering.
 */
class K3bAudioDecoder : public K3bThreadJob
{
  Q_OBJECT

 public:
  K3bAudioDecoder( K3bAudioDoc*, QObject* parent = 0, const char* name = 0 );
  ~K3bAudioDecoder();

  /**
   * If fd is != -1 the decoder will write the data directly to the file 
   * descriptor.
   * Setting fd to -1 will cause the decoder to emit data signals (default)
   * This does only make sense before starting the job.
   */
  void writeToFd( int fd );

 public slots:
  void resume();

 private:
  class DecoderThread;
  DecoderThread* m_thread;
};

#endif
