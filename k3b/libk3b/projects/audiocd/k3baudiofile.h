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

#ifndef _K3B_AUDIO_FILE_H_
#define _K3B_AUDIO_FILE_H_

#include "k3baudiodatasource.h"

#include <k3bmsf.h>
#include <kurl.h>
#include "k3b_export.h"

class K3bAudioDecoder;
class K3bAudioTrack;


/**
 * The K3bAudioFile is the most important audio data source. It gets its data
 * from an audio file and uses a K3bAudioDecoder to decode this data.
 *
 * Be aware that it is currently not possible to change the doc of an AudioFile.
 * The reason for this is the decoder sharing which is in place to allow gapless
 * splitting of audio files into several tracks.
 *
 * \see K3bAudioDoc::createDecoderForUrl
 */
class LIBK3B_EXPORT K3bAudioFile : public K3bAudioDataSource
{
 public:
  /**
   * The AudioFile registers itself with the doc. This is part of the
   * decoder handling facility in K3bAudioDoc which reuses the same decoder
   * for sources with the same url.
   *
   * Use K3bAudioDoc::getDecoderForUrl to create a decoder.
   */
  K3bAudioFile( K3bAudioDecoder*, K3bAudioDoc* );
  K3bAudioFile( const K3bAudioFile& );

  /**
   * The AudioFile deregisters itself from the doc. If it was the last file
   * to use the decoder the doc will take care of deleting it.
   */
  ~K3bAudioFile();

  const QString& filename() const;

  /**
   * The complete length of the file used by this source.
   */
  K3b::Msf originalLength() const;

  QString type() const;
  QString sourceComment() const;

  bool isValid() const;

  K3bAudioDecoder* decoder() const { return m_decoder; }

  bool seek( const K3b::Msf& );

  int read( char* data, unsigned int max );

  K3bAudioDataSource* copy() const;

 private:
  K3bAudioDoc* m_doc;
  K3bAudioDecoder* m_decoder;

  unsigned long long m_decodedData;
};

#endif
