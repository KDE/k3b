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

class K3bAudioDecoder;
class K3bAudioTrack;


class K3bAudioFile : public K3bAudioDataSource
{
 public:
  K3bAudioFile( K3bAudioDecoder*, K3bAudioDoc* doc );
  ~K3bAudioFile();

  const QString& filename() const;
  K3b::Msf length() const;

  /**
   * The complete length of the file used by this source.
   */
  K3b::Msf fileLength() const;

  QString type() const;
  QString sourceComment() const;

  bool isValid() const;

  K3bAudioDecoder* decoder() const { return m_decoder; }

  /**
   * Set the start offset from the beginning of the file.
   */
  void setStartOffset( const K3b::Msf& );

  /**
   * Set the end offset from the beginning of the file. The endOffset sector
   * is not included in the data.
   * The maximum value is fileLength() which means to use all data.
   * 0 means the same as fileLength().
   * This has to be bigger than the start offset.
   */
  void setEndOffset( const K3b::Msf& );

  const K3b::Msf& startOffset() const { return m_startOffset; }
  const K3b::Msf& endOffset() const { return m_endOffset; }

  /**
   * Get the last used sector in the file.
   */
  K3b::Msf lastSector() const;

  bool seek( const K3b::Msf& );

  int read( char* data, int max );

  K3bAudioDataSource* copy() const;

 private:
  void fixupOffsets();
  K3bAudioDecoder* m_decoder;
  K3b::Msf m_startOffset;
  K3b::Msf m_endOffset;

  long long m_decodedData;
};

#endif
