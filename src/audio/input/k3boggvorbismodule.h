/***************************************************************************
                          k3boggvorbismodule.h  -  description
                             -------------------
    begin                : Mon Apr 1 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BOGGVORBISMODULE_H
#define K3BOGGVORBISMODULE_H

#include "../../../../config.h"

#ifdef OGG_VORBIS

#include "k3baudiomodule.h"

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
  K3bOggVorbisModule( K3bAudioTrack* );
  ~K3bOggVorbisModule();

  static bool canDecode( const KURL& url );

 public slots:
  void cancel();

 private slots:
  void startDecoding();
  void decode();
  void slotConsumerReady();

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
