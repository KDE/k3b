/* 
 *
 * $Id: $
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


#ifndef K3BOGGVORBISMODULE_H
#define K3BOGGVORBISMODULE_H

#include <config.h>

#ifdef OGG_VORBIS

#include "../k3baudiomodule.h"

class OggVorbis_File;
class QTimer;
class KURL;


/**
  *@author Sebastian Trueg
  */
class K3bOggVorbisModule : public K3bAudioModule
{
  Q_OBJECT

 public: 
  K3bOggVorbisModule( QObject* parent = 0, const char* name = 0 );
  ~K3bOggVorbisModule();

  bool canDecode( const KURL& url );

 public slots:
  void cancel();

 private slots:
  void startDecoding();
  void decode();
  void slotConsumerReady();

  void analyseTrack();
  void stopAnalysingTrack();
  void slotEmitTrackAnalysed();

 private:
  OggVorbis_File* m_oggVorbisFile;
  int m_currentOggVorbisSection;
  char* m_outputBuffer;

  unsigned long m_rawDataLengthToStream;
  unsigned long m_rawDataAlreadyStreamed;

  QTimer* m_decodingTimer;

  bool m_bDecodingInProgress;

  static const int OUTPUT_BUFFER_SIZE = 4096;
};

#endif

#endif
