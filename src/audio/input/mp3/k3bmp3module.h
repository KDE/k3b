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
#include "libmad/mad.h"
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
  bool metaInfo( const QString& filename, K3bAudioTitleMetaInfo& info );

  void cleanup();

 protected:
  bool initDecodingInternal( const QString& filename );
  int decodeInternal( const char** _data );
 
 private:
  int countFrames( unsigned long& frames );
  inline unsigned short linearRound( mad_fixed_t fixed );
  void fillInputBuffer();
  bool createPcmSamples( mad_synth* );
  unsigned int resampleBlock( mad_fixed_t const *source, 
			      unsigned int nsamples, 
			      mad_fixed_t* target,
			      mad_fixed_t& last,
			      mad_fixed_t& step );

  bool m_bEndOfInput;
  bool m_bInputError;
  bool m_bOutputFinished;

  mad_stream*   m_madStream;
  mad_frame*    m_madFrame;
  mad_header*   m_madHeader;
  mad_synth*    m_madSynth;
  mad_timer_t*  m_madTimer;

  // needed for resampling
  // ----------------------------------
  bool m_bFrameNeedsResampling;
  mad_fixed_t m_madResampledRatio;

  // left channel
  mad_fixed_t m_madResampledStepLeft;
  mad_fixed_t m_madResampledLastLeft;
  mad_fixed_t* m_madResampledLeftChannel;

  // right channel
  mad_fixed_t m_madResampledStepRight;
  mad_fixed_t m_madResampledLastRight;
  mad_fixed_t* m_madResampledRightChannel;
  // ----------------------------------

  unsigned long m_frameCount;

  unsigned char* m_inputBuffer;
  unsigned char* m_outputBuffer;
  unsigned char* m_outputPointer;
  unsigned char* m_outputBufferEnd;

  QFile m_inputFile;

  static const int INPUT_BUFFER_SIZE = 5*8192;
  static const int OUTPUT_BUFFER_SIZE = 5*8192;

  static int MaxAllowedRecoverableErrors;
};


#endif
