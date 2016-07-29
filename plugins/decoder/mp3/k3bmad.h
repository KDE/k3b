/*
 *
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

#ifndef _K3B_MAD_H_
#define _K3B_MAD_H_

extern "C" {
#include <mad.h>
}

#include <qfile.h>


class K3bMad
{
public:
  K3bMad();
  ~K3bMad();

  bool open( const QString& filename );

  /**
   * @return true if the mad stream contains data
   *         false if there is no data left or an error occurred.
   *         In the latter case inputError() returns true.
   */
  bool fillStreamBuffer();

  /**
   * Skip id3 tags.
   *
   * This will reset the input file.
   */
  bool skipTag();

  /**
   * Find first frame and seek to the beginning of that frame.
   * This is used to skip the junk that many mp3 files start with.
   */
  bool seekFirstHeader();

  bool eof() const;
  bool inputError() const;

  /**
   * Current position in theinput file. This does NOT
   * care about the status of the mad stream. Use streamPos()
   * in that case.
   */
  qint64 inputPos() const;

  /**
   * Current absolut position of the decoder stream.
   */
  qint64 streamPos() const;
  bool inputSeek( qint64 pos );

  void initMad();
  void cleanup();

  bool decodeNextFrame();
  bool findNextHeader();
  bool checkFrameHeader( mad_header* header ) const;

  mad_stream*   madStream;
  mad_frame*    madFrame;
  mad_synth*    madSynth;
  mad_timer_t*  madTimer;

private:
  QFile m_inputFile;
  bool m_madStructuresInitialized;
  unsigned char* m_inputBuffer;
  bool m_bInputError;

  int m_channels;
  int m_sampleRate;
};

#endif
