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

#ifndef K3BMP3MODULE_H
#define K3BMP3MODULE_H


#include "../k3baudiomodule.h"

extern "C" {
#include <mad.h>
}

#include <qfile.h>



class K3bMp3Module : public K3bAudioModule
{
  Q_OBJECT

 public:
  K3bMp3Module( QObject* parent = 0, const char* name = 0 );
  ~K3bMp3Module();

  bool canDecode( const KURL& url );

  int analyseTrack( const QString& filename, unsigned long& size );

  void cleanup();

  bool seek( const K3b::Msf& );

 protected:
  bool initDecodingInternal( const QString& filename );
  int decodeInternal( char* _data, int maxLen );
 
 private:
  int countFrames( unsigned long& frames );
  inline unsigned short linearRound( mad_fixed_t fixed );
  void madStreamBuffer();
  bool madDecodeNextFrame();
  bool createPcmSamples( mad_synth* );
  unsigned int resampleBlock( mad_fixed_t const *source, 
			      unsigned int nsamples, 
			      mad_fixed_t* target,
			      mad_fixed_t& last,
			      mad_fixed_t& step );

  static const int INPUT_BUFFER_SIZE = 5*8192;

  static int MaxAllowedRecoverableErrors;

  class Private;
  Private* d;
};


#endif
