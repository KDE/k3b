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

#ifndef K3B_AUDIO_STREAMER_H
#define K3B_AUDIO_STREAMER_H

#include <k3bjob.h>

class K3bAudioDoc;


/**
 * Decodes all tracks of an audio project into one raw audio data stream.
 * In the future it will also take care of the filtering.
 * Every track will contain the pregap of it's successor.
 * The first track's pregap will not be contained in the data at all since
 * the first pregap is the only one cdrecord creates for itself. cdrdao on the
 * other hand is able to do both: create the pregap or get the data. So we 
 * just let cdrdao also create the first pregap (exception: hideFirstTrack mode)
 */
class K3bAudioStreamer : public K3bJob
{
  Q_OBJECT

 public:
  K3bAudioStreamer( K3bAudioDoc*, QObject* parent = 0, const char* name = 0 );
  ~K3bAudioStreamer();

  /**
   * If fd is != -1 the decoder will write the data directly to the file 
   * descriptor.
   * Setting fd to -1 will cause the decoder to emit data signals (default)
   */
  void writeToFd( int fd );

  /**
   * The default output are big endian samples since
   * these are needed when writing cds. This method
   * may be used to change this.
   */
  void setLittleEndian( bool b );

 public slots:
  void start();
  void cancel();
  void resume();

 private slots:
  void startModule();
  void decode();
  void writePregap();
 
 private:
  void cancelAll();
  bool writeData( long len );

  class Private;
  Private* d;
};

#endif
