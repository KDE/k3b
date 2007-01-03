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

#ifndef _K3B_AUDIO_ZERO_DATA_H_
#define _K3B_AUDIO_ZERO_DATA_H_

#include "k3baudiodatasource.h"
#include "k3b_export.h"

class LIBK3B_EXPORT K3bAudioZeroData : public K3bAudioDataSource
{
 public:
  K3bAudioZeroData( const K3b::Msf& msf = 150 );
  K3bAudioZeroData( const K3bAudioZeroData& );
  ~K3bAudioZeroData();

  K3b::Msf originalLength() const { return m_length; }
  void setLength( const K3b::Msf& msf );

  QString type() const;
  QString sourceComment() const;

  bool seek( const K3b::Msf& );
  int read( char* data, unsigned int max );

  K3bAudioDataSource* copy() const;

  /**
   * Only changes the length
   */
  void setStartOffset( const K3b::Msf& );

  /**
   * Only changes the length
   */
  void setEndOffset( const K3b::Msf& );

 private:
  K3b::Msf m_length;
  unsigned long long m_writtenData;
};

#endif
